// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

/* Copyright (c) 2001-2003 International Computer Science Institute
 *
 *
 * This code is derived from the FreeBSD popen, which carried the following 
 * copyright:
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software written by Ken Arnold and
 * published in UNIX Review, Vol. 6, No. 8.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

#ident "$XORP: xorp/rtrmgr/popen.cc,v 1.4 2004/01/13 00:18:02 pavlin Exp $"

#include <sys/param.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "popen.hh"


extern char **environ;

static struct pid_s {
    struct pid_s *next;
    FILE *fp_out;
    FILE *fp_err;
    pid_t pid;
} *pidlist;


void
popen2(const string& command, FILE *& outstream, FILE *&errstream)
{
    struct pid_s *cur;
    FILE *iop_out, *iop_err;
    int pdes_out[2], pdes_err[2], pid;
    const char *argv[4];
    struct pid_s *p;
    outstream = NULL;
    errstream = NULL;

    if (pipe(pdes_out) < 0)
	return;
    if (pipe(pdes_err) < 0) {
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	return;
    }

    if ((cur = (struct pid_s*)malloc(sizeof(struct pid_s))) == NULL) {
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	return;
    }

    /* Disable blocking on read */
    int fl;
    fl = fcntl(pdes_out[0], F_GETFL);
    if (fcntl(pdes_out[0], F_SETFL, fl | O_NONBLOCK) == -1) {
	XLOG_FATAL("Cannot set O_NONBLOCK on file descriptor %d",
		   pdes_out[0]);
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	return;
    }
    fl = fcntl(pdes_err[0], F_GETFL);
    if (fcntl(pdes_err[0], F_SETFL, fl | O_NONBLOCK) == -1) {
	XLOG_FATAL("Cannot set O_NONBLOCK on file descriptor %d",
		   pdes_err[0]);
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	return;
    }

    string a1 = "sh";
    string a2 = "-c";
    argv[0] = a1.c_str();
    argv[1] = a2.c_str();
    argv[2] = command.c_str();
    argv[3] = NULL;

    switch (pid = vfork()) {
    case -1:				/* Error. */
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	free(cur);
	return;
	/* NOTREACHED */
    case 0:				/* Child. */
	/*
	 * The dup2() to STDIN_FILENO is repeated to avoid
	 * writing to pdes[1], which might corrupt the
	 * parent's copy.  This isn't good enough in
	 * general, since the _exit() is no return, so
	 * the compiler is free to corrupt all the local
	 * variables.
	 */
	(void)close(pdes_out[0]);
	(void)close(pdes_err[0]);
	if (pdes_out[1] != STDOUT_FILENO) {
	    (void)dup2(pdes_out[1], STDOUT_FILENO);
	    (void)close(pdes_out[1]);
	} 
	if (pdes_err[1] != STDERR_FILENO) {
	    (void)dup2(pdes_err[1], STDERR_FILENO);
	    (void)close(pdes_err[1]);
	} 
	for (p = pidlist; p; p = p->next) {
	    (void)close(fileno(p->fp_out));
	    (void)close(fileno(p->fp_err));
	}
	execve(_PATH_BSHELL, const_cast<char**>(argv), environ);
	exit(127);
	/* NOTREACHED */
    }

    /* Parent; assume fdopen can't fail. */
    iop_out = fdopen(pdes_out[0], "r");
    iop_err = fdopen(pdes_err[0], "r");
    (void)close(pdes_out[1]);
    (void)close(pdes_err[1]);

    /* Link into list of file descriptors. */
    cur->fp_out = iop_out;
    cur->fp_err = iop_err;
    cur->pid =  pid;
    cur->next = pidlist;
    pidlist = cur;

    outstream = iop_out;
    errstream = iop_err;
}

/*
 * pclose --
 *	Pclose returns -1 if stream is not associated with a `popened' command,
 *	if already `pclosed', or waitpid returns an error.
 */
int
pclose2(FILE *iop_out)
{
    register struct pid_s *cur, *last;
    int pstat;
    pid_t pid;

    /* Find the appropriate file pointer. */
    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
	if (cur->fp_out == iop_out)
	    break;
    if (cur == NULL)
	return (-1);

    (void)fclose(cur->fp_out);
    (void)fclose(cur->fp_err);

    do {
	pid = wait4(cur->pid, &pstat, 0, (struct rusage *)0);
    } while (pid == -1 && errno == EINTR);

    /* Remove the entry from the linked list. */
    if (last == NULL)
	pidlist = cur->next;
    else
	last->next = cur->next;
    free(cur);

    return (pid == -1 ? -1 : pstat);
}

