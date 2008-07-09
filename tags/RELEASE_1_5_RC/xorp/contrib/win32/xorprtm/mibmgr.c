/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
 * vim:set sts=4 ts=8:
 *
 * Copyright (c) 2001-2008 International Computer Science Institute
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
 *
 * $XORP: xorp/contrib/win32/xorprtm/mibmgr.c,v 1.3 2007/02/16 22:45:32 pavlin Exp $
 */

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
