/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ident "$XORP: xorp/contrib/win32/xorprtm/utils.c,v 1.7 2008/10/02 21:56:41 bms Exp $"

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
