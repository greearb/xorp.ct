/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
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

/*
 * $XORP: xorp/libxorp/daemon.h,v 1.2 2008/07/23 05:10:51 pavlin Exp $
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
