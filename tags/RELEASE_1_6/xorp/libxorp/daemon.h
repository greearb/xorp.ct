/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

/*
 * $XORP: xorp/libxorp/daemon.h,v 1.4 2008/10/02 21:57:30 bms Exp $
 */

#ifndef __LIBXORP_DAEMON_H__
#define __LIBXORP_DAEMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

/**
 * @short Options for xorp_daemonize().
 */
enum {
	DAEMON_CHDIR = 0,
	DAEMON_NOCHDIR = 1,
	DAEMON_CLOSE = 0,
	DAEMON_NOCLOSE = 1
} xorp_daemon_t;

/**
 * A local implementation of daemon(3).
 *
 * Fork a new process and detach from parent in a controlled way, allowing
 * XORP processes to run as UNIX daemons.
 * Uses the POSIX setsid() to detach from the controlling terminal.
 *
 * The parent SHOULD use the platform _exit() function to exit.
 *
 * This function is a no-op under Microsoft Windows.
 *
 * @param nochdir set to 0 if the process should chdir to / once detached.
 * @param noclose set to 0 if the process should close 
 *
 * @return -1 if any error occurred, 0 if we are in the child process,
 * otherwise the return value is the child process ID.
 */
int xorp_daemonize(int nochdir, int noclose);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXORP_DAEMON_H__ */
