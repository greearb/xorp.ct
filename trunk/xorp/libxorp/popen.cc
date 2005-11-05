// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

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

#ident "$XORP: xorp/libxorp/popen.cc,v 1.9 2005/11/05 18:08:17 bms Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
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

#ifdef HOST_OS_WINDOWS
extern char **_environ;
#else
extern char **environ;
#endif

// XXX: static instance
static struct pid_s {
    struct pid_s *next;
    FILE *fp_out;
    FILE *fp_err;
#ifdef HOST_OS_WINDOWS
    DWORD pid;
    HANDLE ph;
#else
    pid_t pid;
#endif
} *pidlist;


pid_t
popen2(const string& command, const list<string>& arguments,
       FILE *& outstream, FILE *&errstream, bool redirect_stderr_to_stdout)
{
#ifdef HOST_OS_WINDOWS
    struct pid_s *cur;
    FILE *iop_out, *iop_err;
    HANDLE hout[2], herr[2];
    SECURITY_ATTRIBUTES pipesa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    outstream = NULL;
    errstream = NULL;

    if (CreatePipe(&hout[0], &hout[1], &pipesa, 0) == 0)
	return (0);
    if (CreatePipe(&herr[0], &herr[1], &pipesa, 0) == 0) {
	CloseHandle(hout[0]);
	CloseHandle(hout[1]);
	return (0);
    }
    if ((cur = (struct pid_s*)malloc(sizeof(struct pid_s))) == NULL) {
	CloseHandle(hout[0]);
	CloseHandle(hout[1]);
	CloseHandle(herr[0]);
	CloseHandle(herr[1]);
	return (0);
    }

    GetStartupInfoA(&si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hout[1];

    if (redirect_stderr_to_stdout)
    	si.hStdError = hout[1];
    else
    	si.hStdError = herr[1];

    // Because CreateProcess() accepts a single
    // string for the command line (vs UNIX's exec*() accepting an argv
    // array), we have to look for whitespace characters and escape them.
    //
    // XXX: We currently force the program name to be escaped with quotes.
    //
    string escaped_args = "\"" + command + "\"";

    list<string>::const_iterator iter;
    for (iter = arguments.begin(); iter != arguments.end(); ++iter) {
	escaped_args += " ";
	if ((iter->length() == 0 ||
	     string::npos != iter->find_first_of("\n\t \"") ||
	     string::npos != iter->find("\\\\"))) {
	    string thisarg(*iter);
	    string::size_type len = thisarg.length();
	    string::size_type pos = 0;
	    while (pos < len && string::npos != pos) {
		pos = thisarg.find_first_of("\\\"", pos);
	        if (thisarg[pos] == '\\') {
		    if (thisarg[pos+1] == '\\') {
			pos++;
		    } else if (pos+1 == len || thisarg[pos+1] == '\"') {
		        thisarg.insert(pos, "\\");
			pos += 2;
		    }
		} else if (thisarg[pos] == '\"') {
		    thisarg.insert(pos, "\\");
		    pos += 2;
		}
	    }
	    escaped_args += "\"" + thisarg + "\"";
	} else {
	    escaped_args += *iter;
	}
    }

    debug_msg("Trying to execute: '%s'\n", escaped_args.c_str());

    if (CreateProcessA(NULL,
		       const_cast<char *>(escaped_args.c_str()),
		       NULL, NULL, TRUE,
		       CREATE_NO_WINDOW|CREATE_SUSPENDED, NULL, NULL,
		       &si, &pi) == 0) {
	DWORD err = GetLastError();
	XLOG_WARNING("CreateProcessA failed: %u", XORP_UINT_CAST(err));
	CloseHandle(hout[0]);
	CloseHandle(hout[1]);
	CloseHandle(herr[0]);
	CloseHandle(herr[1]);
	return (0);
    }

    /* Parent; assume _fdopen can't fail. */
    iop_out = _fdopen(_open_osfhandle((long)hout[0], _O_RDONLY|_O_TEXT), "r");
    iop_err = _fdopen(_open_osfhandle((long)herr[0], _O_RDONLY|_O_TEXT), "r");
    setvbuf(iop_out, NULL, _IONBF, 0);
    setvbuf(iop_err, NULL, _IONBF, 0);

    /* Link into list of file descriptors. */
    cur->fp_out = iop_out;
    cur->fp_err = iop_err;
    cur->pid = pi.dwProcessId;
    cur->ph = pi.hProcess;
    cur->next = pidlist;
    pidlist = cur;

    outstream = iop_out;
    errstream = iop_err;

    /* Kick off the child process's main thread. */
    ResumeThread(pi.hThread);
    return (cur->pid);

#else // !HOST_OS_WINDOWS

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
    cur->pid =  pid;
    cur->next = pidlist;
    pidlist = cur;

    outstream = iop_out;
    errstream = iop_err;

    return pid;
#endif // !HOST_OS_WINDOWS
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

#ifdef HOST_OS_WINDOWS
    DWORD dwStat = 0;
    BOOL result = GetExitCodeProcess(cur->ph, (LPDWORD)&dwStat);

    while (dwStat == STILL_ACTIVE) {
    	WaitForSingleObject(cur->ph, INFINITE);
    	result = GetExitCodeProcess(cur->ph, (LPDWORD)&dwStat);
    }

    XLOG_ASSERT(result != 0);

    CloseHandle(cur->ph);
    pid = cur->pid;
    pstat = (int)dwStat;

#else /* !HOST_OS_WINDOWS */

    do {
	pid = wait4(cur->pid, &pstat, 0, (struct rusage *)0);
    } while (pid == -1 && errno == EINTR);

#endif /* HOST_OS_WINDOWS */

    /* Remove the entry from the linked list. */
    if (last == NULL)
	pidlist = cur->next;
    else
	last->next = cur->next;
    free(cur);

    return (pid == -1 ? -1 : pstat);
}

#ifdef HOST_OS_WINDOWS
/*
 * Return the process handle given the process ID.
 * The handle we get from CreateProcess() has privileges which
 * the OpenProcess() handle doesn't get.
 */
HANDLE
pgethandle(pid_t pid)
{
    register struct pid_s *cur, *last;

    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
	if ((pid_t)cur->pid == pid)
	    break;
    if (cur == NULL)
	return (INVALID_HANDLE_VALUE);

    return (cur->ph);
}
#endif
