/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2003 International Computer Science Institute
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
 * $XORP: xorp/libxorp/status_codes.h,v 1.6 2003/12/10 22:01:01 pavlin Exp $
 */

#ifndef __LIBXORP_STATUS_CODES_H__
#define __LIBXORP_STATUS_CODES_H__

/**
 * <pre>
 * Explanation of Process States
 * -----------------------------
 *    +-------------> PROC_NULL
 *    |                   |
 *    |                   | (1)
 *    |                   V
 *    |             PROC_STARTUP
 *    |                   |
 *    |                   | (2)
 *    |                   V
 *    |             PROC_NOT_READY
 *    |                |     ^
 *    |(9)         (3) |     | (4)
 *    |                V     |
 *    |               PROC_READY
 *    |               /        \
 *    |           (5)/          \(6)
 *    |             V            V
 *    |      PROC_SHUTDOWN  PROC_FAILED
 *    |             \           /
 *    |           (7)\         /(8)
 *    |               \       /
 *    |                V     V
 *    |               PROC_DONE
 *    |                   |
 *    |                   |
 *    |                   |
 *    |                   V
 *    +-------------------+
 * Events/Actions
 * --------------
 * (1) Register with finder.
 * (2) External dependencies satisfied, ready to be configured.
 * (3) Finished processing any config changes, ready for other
 *     processes that depend on this process to be configured.
 * (4) Received a config change that needs to be processed before other
 *     processes that depend on this process are configured.
 * (5) Received a request for a clean shutdown.
 * (6) Something failed, this process is no longer functioning correctly.
 * (7) The shutdown has completed.
 * (8) The process has completed the cleanup after the failure.
 * (9) Deregister with finder.
 *
 * States
 * ------
 * PROC_NULL        Process is not registered with finder.  It may or may
 *                  not be running.
 *
 * PROC_STARTUP     Process is registered with finder, but is waiting
 *                  on some other processes before it is ready to be
 *                  configured.
 *
 * PROC_NOT_READY   For any reason, the process is not ready for processes
 *                  that depend on this process to be configured or
 *                  reconfigured.  A common reason is that this
 *                  process just received a config change, and is
 *                  still in the process of making the config change
 *                  active.
 *
 * PROC_READY       Process is running normally.  Processes that depend
 *                  on the state of this process can be configured or
 *                  reconfigured.
 *
 * PROC_SHUTDOWN    Process has received a shutdown request is shutting
 *                  down cleanly.  Normally the process will terminate by
 *                  itself after being in this state.
 *
 * PROC_FAILED      Process has suffered a fatal error, and is in the
 *                  process of cleaning up the mess.  Normally the process
 *                  will terminate by itself after being in this state.
 *
 * PROC_DONE        The process has completed operation, but is still
 *                  capable of responding to XRLs.
 *
 * Notes
 * -----
 *
 * A process may spend zero time in PROC_STARTUP, PROC_NOT_READY,
 * PROC_READY, PROC_SHUTDOWN, PROC_FAILED, or PROC_DONE states.  For
 * example, a process may effectively go directly from PROC_NULL to
 * PROC_READY state on startup if there are no dependencies that need
 * to be taken into account.  A process may go from PROC_STARTUP or
 * PROC_NOT_READY states to PROC_SHUTDOWN or PROC_FAILED states
 * without spending any time in PROC_READY state if required.
 *
 * On reconfiguration, a process does not need to go to NOT_READY
 * state unless it needs to delay the reconfiguration of processes
 * that depend on the completion of this reconfiguration.
 *
 * After shutdown or a failure, the process may remain indefinitely in
 * PROC_DONE state (e.g., if the process itself shoudn't really exit
 * but rather await further instructions).
 * </pre>
 */

typedef enum {
    PROC_NULL	   = 0,
    PROC_STARTUP   = 1,
    PROC_NOT_READY = 2,
    PROC_READY	   = 3,
    PROC_SHUTDOWN  = 4,
    PROC_FAILED	   = 5,
    PROC_DONE	   = 6
} ProcessStatus;

#endif /* __LIBXORP_STATUS_CODES_H__ */
