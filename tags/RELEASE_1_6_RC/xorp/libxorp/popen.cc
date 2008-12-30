// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/popen.cc,v 1.23 2008/07/23 05:10:53 pavlin Exp $"

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
    HANDLE h_out_child;	// child ends of pipes visible in parent
    HANDLE h_err_child;	// need to be closed after termination. 
#else
    pid_t pid;
#endif
    bool is_closed;
    int pstat;		// The process wait status if is_closed is true
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
#if 0
    if (redirect_stderr_to_stdout) {
	if (0 == DuplicateHandle(GetCurrentProcess(), hout[0],
				 GetCurrentProcess(), &herr[0],
				 0, TRUE, DUPLICATE_SAME_ACCESS)) {
	    CloseHandle(hout[0]);
	    CloseHandle(hout[1]);
	    return (0);
	}
	if (0 == DuplicateHandle(GetCurrentProcess(), hout[1],
				 GetCurrentProcess(), &herr[1],
				 0, TRUE, DUPLICATE_SAME_ACCESS)) {
	    CloseHandle(hout[0]);
	    CloseHandle(hout[1]);
	    return (0);
	}
    } else
#endif // 0
    {
	if (0 == CreatePipe(&herr[0], &herr[1], &pipesa, 0)) {
	    CloseHandle(hout[0]);
	    CloseHandle(hout[1]);
	    return (0);
	}
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
    si.hStdError = herr[1];

    if (redirect_stderr_to_stdout)
	si.hStdError = hout[1];

#if 0
    // We need to close the child ends of the pipe when the child terminates.
    // We need not do this for the parent ends; fclose() does this for us.
    cur->h_out_child = hout[1];
    cur->h_err_child = herr[1];
#endif // 0

    // XXX: We currently force the program name to be escaped with quotes
    // before munging the command line for CreateProcess().
    string escaped_args = "\"" + command + "\"";
    win_quote_args(arguments, escaped_args);

    debug_msg("Trying to execute: '%s'\n", escaped_args.c_str());

    if (CreateProcessA(NULL,
		       const_cast<char *>(escaped_args.c_str()),
		       NULL, NULL, TRUE,
		       CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED,
		       NULL, NULL, &si, &pi) == 0) {
	DWORD err = GetLastError();
	string error_msg = c_format("Execution of %s failed: %u",
				    command.c_str(),
				    XORP_UINT_CAST(err));
	UNUSED(error_msg);
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
    cur->is_closed = false;
    cur->pstat = 0;
    cur->next = pidlist;
    pidlist = cur;

    outstream = iop_out;
    errstream = iop_err;

    /* Kick off the child process's main thread. */
    ResumeThread(pi.hThread);
    return (cur->pid);

#else // ! HOST_OS_WINDOWS

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
#endif // ! HOST_OS_WINDOWS
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

#ifdef HOST_OS_WINDOWS
    if (! (dont_wait || cur->is_closed)) {
	DWORD dwStat = 0;
	BOOL result = GetExitCodeProcess(cur->ph, (LPDWORD)&dwStat);
	while (dwStat == STILL_ACTIVE) {
	    WaitForSingleObject(cur->ph, INFINITE);
	    result = GetExitCodeProcess(cur->ph, (LPDWORD)&dwStat);
	}
	XLOG_ASSERT(result != 0);
	pstat = (int)dwStat;
    }

    (void)fclose(cur->fp_out);
    (void)fclose(cur->fp_err);

    if (! (dont_wait || cur->is_closed)) {
	// CloseHandle(cur->h_out_child);
	// CloseHandle(cur->h_err_child);
	CloseHandle(cur->ph);
    }
#else // ! HOST_OS_WINDOWS

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
#endif // ! HOST_OS_WINDOWS

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

#ifdef HOST_OS_WINDOWS
/*
 * Return the process handle given the process ID.
 * The handle we get from CreateProcess() has privileges which
 * the OpenProcess() handle doesn't get.
 */
HANDLE
pgethandle(pid_t pid)
{
    struct pid_s *cur;

    for (cur = pidlist; cur != NULL; cur = cur->next) {
	if (static_cast<pid_t>(cur->pid) == pid)
	    break;
    }
    if (cur == NULL)
	return (INVALID_HANDLE_VALUE);

    return (cur->ph);
}
#endif // HOST_OS_WINDOWS
