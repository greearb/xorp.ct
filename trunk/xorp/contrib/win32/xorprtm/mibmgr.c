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

#ident "$XORP: xorp/contrib/win32/xorprtm/mibmgr.c,v 1.7 2008/10/02 21:56:40 bms Exp $"

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#include "pchsample.h"
#pragma hdrstop

/* XXX: This needs to use the router manager event queue interface. */
DWORD
WINAPI
MM_MibSet (
    PXORPRTM_MIB_SET_INPUT_DATA    pimsid)
{
    DWORD                   dwErr = NO_ERROR;
    ROUTING_PROTOCOL_EVENTS rpeEvent;
    MESSAGE                 mMessage = {0, 0, 0};

    TRACE0(ENTER, "Entering MM_MibSet");

    if (!ENTER_XORPRTM_API()) { return ERROR_CAN_NOT_COMPLETE; }

    do {
        if (pimsid->IMSID_TypeID == XORPRTM_GLOBAL_CONFIG_ID) {
            if (pimsid->IMSID_BufferSize < sizeof(XORPRTM_GLOBAL_CONFIG)) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
           rpeEvent = SAVE_GLOBAL_CONFIG_INFO;
        } else {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        /* notify router manager */
        if (EnqueueEvent(rpeEvent, mMessage) == NO_ERROR)
            SetEvent(g_ce.hMgrNotificationEvent);

    } while(FALSE);

    LEAVE_XORPRTM_API();

    TRACE0(ENTER, "Leaving MM_MibSet");
    return dwErr;
}

/* XXX: We must have a 'mib get' function to retrieve the global config. */
DWORD
WINAPI
MM_MibGet (
    PXORPRTM_MIB_GET_INPUT_DATA    pimgid,
    PXORPRTM_MIB_GET_OUTPUT_DATA   pimgod,
    PULONG	                        pulOutputSize,
    MODE                            mMode)
{
    DWORD               dwErr           = NO_ERROR;
    ULONG               ulSizeGiven     = 0;
    ULONG               ulSizeNeeded    = 0;

    TRACE0(ENTER, "Entering MM_MibGet");
    
    if (!ENTER_XORPRTM_API()) { return ERROR_CAN_NOT_COMPLETE; }

    if (*pulOutputSize < sizeof(XORPRTM_MIB_GET_OUTPUT_DATA))
        ulSizeGiven = 0;
    else
        ulSizeGiven = *pulOutputSize - sizeof(XORPRTM_MIB_GET_OUTPUT_DATA);

    switch (pimgid->IMGID_TypeID) {
        case XORPRTM_GLOBAL_CONFIG_ID: {
            if (mMode == GET_NEXT) {
                dwErr = ERROR_NO_MORE_ITEMS;
                break;
            }
            dwErr = CM_GetGlobalInfo ((PVOID) pimgod->IMGOD_Buffer,
                                      &ulSizeGiven,
                                      NULL,
                                      NULL,
                                      NULL);
            ulSizeNeeded = ulSizeGiven;
            if (dwErr != NO_ERROR)
                break;
            pimgod->IMGOD_TypeID = XORPRTM_GLOBAL_CONFIG_ID;
            break;
        }

        default: {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    *pulOutputSize = sizeof(XORPRTM_MIB_GET_OUTPUT_DATA) + ulSizeNeeded;

    LEAVE_XORPRTM_API();

    TRACE0(ENTER, "Leaving MM_MibGet");
    return dwErr;
}
