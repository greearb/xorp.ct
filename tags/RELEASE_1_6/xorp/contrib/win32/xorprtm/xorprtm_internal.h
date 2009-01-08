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

/*
 * $XORP: xorp/contrib/win32/xorprtm/xorprtm_internal.h,v 1.7 2008/10/02 21:56:41 bms Exp $
 */

#ifndef _CONFIGURATIONENTRY_H_
#define _CONFIGURATIONENTRY_H_

typedef struct _EVENT_ENTRY {
    QUEUE_ENTRY             qeEventQueueLink;
    ROUTING_PROTOCOL_EVENTS rpeEvent;
    MESSAGE                 mMessage;
} EVENT_ENTRY, *PEVENT_ENTRY;

DWORD
EE_Create (
    ROUTING_PROTOCOL_EVENTS rpeEvent,
    MESSAGE                 mMessage,
    PEVENT_ENTRY            *ppeeEventEntry);

DWORD
EE_Destroy (PEVENT_ENTRY            peeEventEntry);

#ifdef DEBUG
DWORD
EE_Display (
    PEVENT_ENTRY            peeEventEntry);
#else
#define EE_Display(peeEventEntry)
#endif /* DEBUG */

DWORD
EnqueueEvent(
    ROUTING_PROTOCOL_EVENTS rpeEvent,
    MESSAGE                 mMessage);

DWORD
DequeueEvent(
    ROUTING_PROTOCOL_EVENTS  *prpeEvent,
    MESSAGE                  *pmMessage);

/* various codes describing states of XORPRTM. */
typedef enum _XORPRTM_STATUS_CODE
{
    XORPRTM_STATUS_RUNNING     = 101,
    XORPRTM_STATUS_STOPPING    = 102,
    XORPRTM_STATUS_STOPPED     = 103
} XORPRTM_STATUS_CODE, *PXORPRTM_STATUS_CODE;

#define PIPE_INSTANCES 12
#define PIPE_READBUF_SIZE 2048

typedef enum _pipe_state_t {
    PIPE_STATE_INIT,
    PIPE_STATE_LISTEN,
    PIPE_STATE_CONNECTED
} pipe_state_t;

typedef struct pipe_instance {
    HANDLE	    pipe;
    pipe_state_t    state;

    HANDLE	    cevent;
    HANDLE	    cwait;
    OVERLAPPED	    cov;

    HANDLE	    revent;
    HANDLE	    rwait;
    OVERLAPPED	    rov;

    CRITICAL_SECTION	rcs;

    size_t	    rsize;
    char	    rbuf[PIPE_READBUF_SIZE];	    /* XXX */

} pipe_instance_t;

typedef struct _CONFIGURATION_ENTRY {
    /* Following are PERSISTENT, across Start and Stop Protocol */
    
    READ_WRITE_LOCK         rwlLock;
    HANDLE                  hGlobalHeap;
    ULONG                   ulActivityCount;
    HANDLE                  hActivitySemaphore;
    DWORD		    dwTraceID;
    LOCKED_QUEUE            lqEventQueue;
    XORPRTM_STATUS_CODE    iscStatus;
    
    /* Router Manager Information  */
    HANDLE		    hMgrNotificationEvent;
    SUPPORT_FUNCTIONS       sfSupportFunctions;

    /* RTMv2 Information */
    RTM_ENTITY_INFO	reiRtmEntity;
    RTM_REGN_PROFILE	rrpRtmProfile;
    HANDLE		hRtmHandle;
    HANDLE		hRtmNotificationHandle;

    HANDLE		hMprConfig;

    pipe_instance_t *pipes[PIPE_INSTANCES];

} CONFIGURATION_ENTRY, *PCONFIGURATION_ENTRY;



/* create all fields on DLL_PROCESS_ATTACH */
DWORD
CE_Create (PCONFIGURATION_ENTRY    pce);

/* destroy all fields on DLL_PROCESS_DEATTACH */
DWORD
CE_Destroy (PCONFIGURATION_ENTRY    pce);

/* initialize non persistent fields on StartProtocol */
DWORD
CE_Initialize (
    PCONFIGURATION_ENTRY    pce,
    HANDLE                  hMgrNotificationEvent,
    PSUPPORT_FUNCTIONS      psfSupportFunctions,
    PXORPRTM_GLOBAL_CONFIG pigc);

/* cleanup non persistent fields on StopProtocol */
DWORD
CE_Cleanup (PCONFIGURATION_ENTRY    pce);

extern CONFIGURATION_ENTRY      g_ce;

DWORD
CM_StartProtocol (
    HANDLE                  hMgrNotificationEvent,
    PSUPPORT_FUNCTIONS      psfSupportFunctions,
    PVOID                   pvGlobalInfo);

DWORD
CM_StopProtocol ();

DWORD
CM_GetGlobalInfo (
    PVOID 	            pvGlobalInfo,
    PULONG              pulBufferSize,
    PULONG	            pulStructureVersion,
    PULONG              pulStructureSize,
    PULONG              pulStructureCount);

DWORD
CM_SetGlobalInfo (PVOID pvGlobalInfo);

DWORD
CM_GetEventMessage (
    ROUTING_PROTOCOL_EVENTS *prpeEvent,
    MESSAGE                 *pmMessage);

#endif /* _CONFIGURATIONENTRY_H_ */
