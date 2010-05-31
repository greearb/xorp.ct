// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

//
// Part of this program is derived from the following software:
// - FreeBSD's popen(3) implementation
//
// The above software is covered by the following license(s):
//

/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software written by Ken Arnold and
 * published in UNIX Review, Vol. 6, No. 8.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libc/gen/popen.c,v 1.14 2000/01/27 23:06:19 jasone Exp $
 */



#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"
#include "libxorp/utils.hh"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "popen.hh"

extern char **environ;

// XXX: static instance
static struct pid_s {
    struct pid_s *next;
    FILE *fp_out;
    FILE *fp_err;
    pid_t pid;
    bool is_closed;
    int pstat;		// The process wait status if is_closed is true
} *pidlist;


pid_t
popen2(const string& command, const list<string>& arguments,
       FILE *& outstream, FILE *&errstream, bool redirect_stderr_to_stdout)
{
    struct pid_s *cur;
    FILE *iop_out, *iop_err;
    int pdes_out[2], pdes_err[2], pid;
    size_t argv_size = 1 + arguments.size() + 1;
    const char **argv = reinterpret_cast<const char**>(malloc(argv_size * sizeof(char *)));
    struct pid_s *p;
    outstream = NULL;
    errstream = NULL;

    if (pipe(pdes_out) < 0) {
	free(argv);
	return 0;
    }
    if (pipe(pdes_err) < 0) {
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	free(argv);
	return 0;
    }

    if ((cur = (struct pid_s*)malloc(sizeof(struct pid_s))) == NULL) {
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	free(argv);
	return 0;
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
	free(argv);
	return 0;
    }
    fl = fcntl(pdes_err[0], F_GETFL);
    if (fcntl(pdes_err[0], F_SETFL, fl | O_NONBLOCK) == -1) {
	XLOG_FATAL("Cannot set O_NONBLOCK on file descriptor %d",
		   pdes_err[0]);
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	free(argv);
	return 0;
    }

    //
    // Create the array with the command and the arguments
    //
    argv[0] = xorp_basename(command.c_str());
    size_t i;
    list<string>::const_iterator iter;
    for (i = 0, iter = arguments.begin();
	 iter != arguments.end();
	 ++i, ++iter) {
	argv[i + 1] = iter->c_str();
    }
    argv[argv_size - 1] = NULL;

    switch (pid = vfork()) {
    case -1:				/* Error. */
	(void)close(pdes_out[0]);
	(void)close(pdes_out[1]);
	(void)close(pdes_err[0]);
	(void)close(pdes_err[1]);
	free(cur);
	free(argv);
	return 0;
	/* NOTREACHED */
    case 0:				/* Child. */
	//
	// Unblock all signals that may have been blocked by the parent
	//
	sigset_t sigset;
	sigfillset(&sigset);
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);

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

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	if (redirect_stderr_to_stdout) {
	    //
	    // Redirect stderr to stdout
	    //
	    bool do_close_pdes_out = false;
	    bool do_close_pdes_err = false;
	    if ((pdes_out[1] != STDOUT_FILENO)
		&& (pdes_out[1] != STDERR_FILENO)) {
		do_close_pdes_out = true;
	    }
	    if ((pdes_err[1] != STDOUT_FILENO)
		&& (pdes_err[1] != STDERR_FILENO)) {
		do_close_pdes_err = true;
	    }
	    if (pdes_out[1] != STDOUT_FILENO) {
		(void)dup2(pdes_out[1], STDOUT_FILENO);
	    }
	    if (pdes_out[1] != STDERR_FILENO) {
		(void)dup2(pdes_out[1], STDERR_FILENO);
	    }
	    if (do_close_pdes_out)
		(void)close(pdes_out[1]);
	    if (do_close_pdes_err)
		(void)close(pdes_err[1]);
	} else {
	    if (pdes_out[1] != STDOUT_FILENO) {
		(void)dup2(pdes_out[1], STDOUT_FILENO);
		(void)close(pdes_out[1]);
	    }
	    if (pdes_err[1] != STDERR_FILENO) {
		(void)dup2(pdes_err[1], STDERR_FILENO);
		(void)close(pdes_err[1]);
	    }
	}
	for (p = pidlist; p; p = p->next) {
	    (void)close(fileno(p->fp_out));
	    (void)close(fileno(p->fp_err));
	}
	// Set the process as a group leader
	setpgid(0, 0);
	execve(const_cast<char*>(command.c_str()), const_cast<char**>(argv),
	       environ);
	//
	// XXX: don't call any function that may corrupt the state of the
	// parent process, otherwise the result is unpredictable.
	//
	_exit(127);
	/* NOTREACHED */
    }

    /* Parent; assume fdopen can't fail. */
    iop_out = fdopen(pdes_out[0], "r");
    iop_err = fdopen(pdes_err[0], "r");
    setvbuf(iop_out, NULL, _IONBF, 0);
    setvbuf(iop_err, NULL, _IONBF, 0);
    (void)close(pdes_out[1]);
    (void)close(pdes_err[1]);
    free(argv);

    /* Link into list of file descriptors. */
    cur->fp_out = iop_out;
    cur->fp_err = iop_err;
    cur->pid = pid;
    cur->is_closed = false;
    cur->pstat = 0;
    cur->next = pidlist;
    pidlist = cur;

    outstream = iop_out;
    errstream = iop_err;

    return pid;
}

/*
 * pclose --
 *	Pclose returns -1 if stream is not associated with a `popened' command,
 *	if already `pclosed', or waitpid returns an error.
 */
int
pclose2(FILE *iop_out, bool dont_wait)
{
    register struct pid_s *cur, *last;
    int pstat = 0;
    pid_t pid = 0;

    /* Find the appropriate file pointer. */
    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
	if (cur->fp_out == iop_out)
	    break;
    if (cur == NULL)
	return (-1);

    pid = cur->pid;
    if (dont_wait || cur->is_closed) {
	if (cur->is_closed)
	    pstat = cur->pstat;
	else
	    pstat = 0;	// XXX: imitating the result of wait4(WNOHANG)
    }

    (void)fclose(cur->fp_out);
    (void)fclose(cur->fp_err);

    if (dont_wait || cur->is_closed) {
	if (cur->is_closed)
	    pstat = cur->pstat;
	else
	    pstat = 0;	// XXX: imitating the result of wait4(WNOHANG)
    } else {
	do {
	    pid = wait4(cur->pid, &pstat, 0, (struct rusage *)0);
	} while (pid == -1 && errno == EINTR);
    }

    /* Remove the entry from the linked list. */
    if (last == NULL)
	pidlist = cur->next;
    else
	last->next = cur->next;
    free(cur);

    return (pid == -1 ? -1 : pstat);
}

int
popen2_mark_as_closed(pid_t pid, int wait_status)
{
    struct pid_s *cur;

    for (cur = pidlist; cur != NULL; cur = cur->next) {
	if (static_cast<pid_t>(cur->pid) == pid)
	    break;
    }
    if (cur == NULL)
	return (-1);

    cur->is_closed = true;
    cur->pstat = wait_status;
    return (0);
}
