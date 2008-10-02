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

#ident "$XORP$"

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#include "pchsample.h"
#pragma hdrstop

BOOL
EnterSubsystemAPI()
{
    BOOL    bEntered    = FALSE;

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    if (g_ce.iscStatus == XORPRTM_STATUS_RUNNING) {
        /* subsystem is running, so continue */
        g_ce.ulActivityCount++;
        bEntered = TRUE;
    }
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return bEntered;
}

BOOL
EnterSubsystemWorker()
{
    BOOL    bEntered    = FALSE;

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do {
        /* subsystem is running, so the function may continue */
        if (g_ce.iscStatus == XORPRTM_STATUS_RUNNING) {
            bEntered = TRUE;
            break;
        }

        /* subsystem is not running, but it was, so the function must stop */
        if (g_ce.iscStatus == XORPRTM_STATUS_STOPPING) {
            g_ce.ulActivityCount--;
            ReleaseSemaphore(g_ce.hActivitySemaphore, 1, NULL);
            break;
        }

        /* subsystem probably never started. quit. */
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return bEntered;
}

VOID
LeaveSubsystemWorker()
{
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    g_ce.ulActivityCount--;
    if (g_ce.iscStatus == XORPRTM_STATUS_STOPPING) {
        ReleaseSemaphore(g_ce.hActivitySemaphore, 1, NULL);
    }

    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));
}
