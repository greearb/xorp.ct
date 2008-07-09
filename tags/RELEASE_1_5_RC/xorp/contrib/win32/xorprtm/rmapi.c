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
 * $XORP: xorp/contrib/win32/xorprtm/rmapi.c,v 1.3 2007/02/16 22:45:32 pavlin Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#include "pchsample.h"
#pragma hdrstop

extern int rtm_ifannounce(LPWSTR ifname, DWORD ifindex, int what);
extern int rtm_ifinfo(DWORD ifindex, int up);
#ifdef IPV6_DLL
extern int rtm_newaddr(DWORD ifindex, PIPV6_ADAPTER_BINDING_INFO pbind);
#else
extern int rtm_newaddr(DWORD ifindex, PIP_ADAPTER_BINDING_INFO pbind);
#endif

DWORD
WINAPI
StartProtocol (
    HANDLE 	            NotificationEvent,
    PSUPPORT_FUNCTIONS   SupportFunctions,
    LPVOID               GlobalInfo,
    ULONG                StructureVersion,
    ULONG                StructureSize,
    ULONG                StructureCount
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE3(ENTER, "Entering StartProtocol 0x%08x 0x%08x 0x%08x",
           NotificationEvent, SupportFunctions, GlobalInfo);

    do                          /* breakout loop */
    {
        /* validate parameters */
        if (!NotificationEvent || !SupportFunctions  || !GlobalInfo)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = CM_StartProtocol(NotificationEvent,
                                 SupportFunctions,
                                 GlobalInfo);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  StartProtocol: %u", dwErr);

    return dwErr;
}

DWORD
WINAPI
StartComplete (
    VOID
    )
{
    TRACE0(ENTER, "Entering StartComplete");
    TRACE0(LEAVE, "Leaving  StartComplete");

    return NO_ERROR;
}


DWORD
WINAPI
StopProtocol (
    VOID
    )
{
    DWORD dwErr = NO_ERROR;
    
    TRACE0(ENTER, "Entering StopProtocol");

    dwErr = CM_StopProtocol();
    
    TRACE1(LEAVE, "Leaving  StopProtocol: %u", dwErr);

    return dwErr;
}

DWORD
WINAPI
GetGlobalInfo (
        PVOID 	GlobalInfo,
    PULONG   BufferSize,
       PULONG	StructureVersion,
       PULONG   StructureSize,
       PULONG   StructureCount
    )
{
    DWORD dwErr = NO_ERROR;
    
    TRACE2(ENTER, "Entering GetGlobalInfo: 0x%08x 0x%08x",
           GlobalInfo, BufferSize);
    do
    {
        if (!BufferSize) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        dwErr = CM_GetGlobalInfo(GlobalInfo,
                                 BufferSize,
                                 StructureVersion,
                                 StructureSize,
                                 StructureCount);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  GetGlobalInfo: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
SetGlobalInfo (
     PVOID 	GlobalInfo,
     ULONG	StructureVersion,
     ULONG   StructureSize,
     ULONG   StructureCount
    )
{
    TRACE1(ENTER, "Entering SetGlobalInfo: 0x%08x", GlobalInfo);
    TRACE0(LEAVE, "Leaving  SetGlobalInfo");
    return NO_ERROR;
}

DWORD
WINAPI
AddInterface (
    LPWSTR               InterfaceName,
    ULONG	            InterfaceIndex,
    NET_INTERFACE_TYPE   InterfaceType,
    DWORD                MediaType,
    WORD                 AccessType,
    WORD                 ConnectionType,
    PVOID	            InterfaceInfo,
    ULONG                StructureVersion,
    ULONG                StructureSize,
    ULONG                StructureCount
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering AddInterface: %S %u %u 0x%08x",
           InterfaceName, InterfaceIndex, AccessType, InterfaceInfo);

    rtm_ifannounce(InterfaceName, InterfaceIndex, IFAN_ARRIVAL);

    TRACE1(LEAVE, "Leaving  AddInterface: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
DeleteInterface (
    ULONG	InterfaceIndex
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering DeleteInterface: %u", InterfaceIndex);

    rtm_ifannounce(NULL, InterfaceIndex, IFAN_DEPARTURE);

    TRACE1(LEAVE, "Leaving  DeleteInterface: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
InterfaceStatus (
    ULONG	InterfaceIndex,
    BOOL     InterfaceActive,
    DWORD    StatusType,
    PVOID	StatusInfo
    )
{
#ifdef IPV6_DLL
    PIPV6_ADAPTER_BINDING_INFO pbind = NULL;
#else
    PIP_ADAPTER_BINDING_INFO pbind = NULL;
#endif
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering InterfaceStatus: %u %u %u %p",
           InterfaceIndex, InterfaceActive, StatusType, StatusInfo);
    TRACE1(ANY, "interface is %sactive", InterfaceActive ? "" : "in");

    switch (StatusType) {
    case RIS_INTERFACE_ADDRESS_CHANGE:
	TRACE0(ANY, "interface address has changed");
#ifdef IPV6_DLL
	pbind6 = (PIPV6_ADAPTER_BINDING_INFO) StatusInfo;
	TRACE1(ANY, "%d addresses associated with this adapter",
	       pbind6->AddressCount);
#else
	pbind = (PIP_ADAPTER_BINDING_INFO) StatusInfo;
	TRACE1(ANY, "%d addresses associated with this adapter",
	       pbind->AddressCount);
#endif
#if 0
	rtm_newaddr(InterfaceIndex, pbind);
#endif
	break;
    case RIS_INTERFACE_ENABLED:
	TRACE0(ANY, "interface is enabled");
	break;
    case RIS_INTERFACE_DISABLED:
	TRACE0(ANY, "interface is disabled");
	break;
    case RIS_INTERFACE_MEDIA_PRESENT:
    case RIS_INTERFACE_MEDIA_ABSENT:
	TRACE1(ANY, "interface link is %s",
	       StatusType == RIS_INTERFACE_MEDIA_PRESENT ? "up" : "down");
	rtm_ifinfo(InterfaceIndex,
		   StatusType == RIS_INTERFACE_MEDIA_PRESENT ? 1 : 0);
	break;
    default:
	TRACE1(ANY, "unknown StatusType", StatusType);
	break;
    }

    TRACE1(LEAVE, "Leaving  InterfaceStatus: %u", dwErr);

    return dwErr;
}

DWORD
WINAPI
GetInterfaceConfigInfo (
         ULONG	InterfaceIndex,
         PVOID   InterfaceInfo,
     PULONG  BufferSize,
        PULONG	StructureVersion,
        PULONG	StructureSize,
        PULONG	StructureCount
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE3(ENTER, "Entering GetInterfaceConfigInfo: %u 0x%08x 0x%08x",
           InterfaceIndex, InterfaceInfo, BufferSize);
    TRACE1(LEAVE, "Leaving  GetInterfaceConfigInfo: %u",
           dwErr);

    return dwErr;
}



DWORD
WINAPI
SetInterfaceConfigInfo (
    ULONG	InterfaceIndex,
    PVOID	InterfaceInfo,
    ULONG    StructureVersion,
    ULONG    StructureSize,
    ULONG    StructureCount
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE2(ENTER, "Entering SetInterfaceConfigInfo: %u 0x%08x",
           InterfaceIndex, InterfaceInfo);
    TRACE1(LEAVE, "Leaving  SetInterfaceConfigInfo: %u", dwErr);

    return dwErr;
}

/*
 * This is totally required. The Router Manager calls it to
 * know when we've stopped.
 */
DWORD
WINAPI
GetEventMessage (
    ROUTING_PROTOCOL_EVENTS  *Event,
    MESSAGE                  *Result
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE2(ENTER, "Entering GetEventMessage: 0x%08x 0x%08x",
           Event, Result);
    do
    {
        if (!Event || !Result) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = CM_GetEventMessage(Event, Result);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  GetEventMessage: %u", dwErr);

    return dwErr;
}

/* Only used for demand dial interfaces; stub. */
DWORD
WINAPI
DoUpdateRoutes (
    ULONG	InterfaceIndex
    )
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering DoUpdateRoutes: %u", InterfaceIndex);
    TRACE1(LEAVE, "Leaving  DoUpdateRoutes: %u", dwErr);

    return dwErr;
}

DWORD
WINAPI
MibCreate (
    ULONG 	InputDataSize,
    PVOID 	InputData
    )
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE2(ENTER, "Entering MibCreate: %u 0x%08x",
           InputDataSize, InputData);
    TRACE1(LEAVE, "Leaving  MibCreate: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibDelete (
    ULONG 	InputDataSize,
    PVOID 	InputData
    )
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE2(ENTER, "Entering MibDelete: %u 0x%08x",
           InputDataSize, InputData);
    TRACE1(LEAVE, "Leaving  MibDelete: %u", dwErr);

    return dwErr;
}


/* XXX: Not sure if this is really needed */
DWORD
WINAPI
MibSet (
    ULONG 	InputDataSize,
    PVOID	InputData
    )
/*++

Routine Description
    This function sets XORPRTM's global or interface configuration.

Arguments
    InputData           Relevant input, struct XORPRTM_MIB_SET_INPUT_DATA
    InputDataSize       Size of the input
    
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE2(ENTER, "Entering MibSet: %u 0x%08x",
           InputDataSize, InputData);

    do                          /* breakout loop */
    {
        /* validate parameters */
        if ((!InputData) ||
            (InputDataSize < sizeof(XORPRTM_MIB_SET_INPUT_DATA)))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibSet((PXORPRTM_MIB_SET_INPUT_DATA) InputData);

    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  MibSet: %u", dwErr);

    return dwErr;
}


/* XXX: Not sure if this is really needed */

DWORD
WINAPI
MibGet (
         ULONG	InputDataSize,
         PVOID	InputData,
     PULONG	OutputDataSize,
        PVOID	OutputData
    )
/*++

Routine Description
    This function retrieves one of...
    . global configuration
    . interface configuration
    . global stats
    . interface stats
    . interface binding

    Called by an admin (SNMP) utility.  It actually passes through the IP
    Router Manager, but all that does is demux the call to the desired
    routing protocol.

Arguments
    InputData           Relevant input, struct XORPRTM_MIB_GET_INPUT_DATA
    InputDataSize       Size of the input
    OutputData          Buffer for struct XORPRTM_MIB_GET_OUTPUT_DATA
    OutputDataSize       size of output buffer received
                        size of output buffer required
                        
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering MibGet: %u 0x%08x 0x%08x 0x%08x",
           InputDataSize, InputData, OutputDataSize, OutputData);

    do                          /* breakout loop */
    {
        /* validate parameters */
        if ((!InputData) ||
            (InputDataSize < sizeof(XORPRTM_MIB_GET_INPUT_DATA)) ||
            (!OutputDataSize))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibGet((PXORPRTM_MIB_GET_INPUT_DATA) InputData,
                          (PXORPRTM_MIB_GET_OUTPUT_DATA) OutputData,
                          OutputDataSize,
                          GET_EXACT);
        
    } while(FALSE);

    TRACE1(LEAVE, "Leaving  MibGet: %u", dwErr);

    return dwErr;
}


/* XXX: Not sure if this is really needed */

DWORD
WINAPI
MibGetFirst (
         ULONG	InputDataSize,
         PVOID	InputData,
     PULONG  OutputDataSize,
        PVOID   OutputData
    )
/*++

Routine Description
    This function retrieves one of...
    . global configuration
    . interface configuration
    . global stats
    . interface stats
    . interface binding

    It differs from MibGet() in that it always returns the FIRST entry in
    whichever table is being queried.  There is only one entry in the
    global configuration and global stats tables, but the interface
    configuration, interface stats, and interface binding tables are sorted
    by IP address; this function returns the first entry from these.

Arguments
    InputData           Relevant input, struct XORPRTM_MIB_GET_INPUT_DATA
    InputDataSize       Size of the input
    OutputData          Buffer for struct XORPRTM_MIB_GET_OUTPUT_DATA
    OutputDataSize       size of output buffer received
                        size of output buffer required
                        
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering MibGetFirst: %u 0x%08x 0x%08x 0x%08x",
           InputDataSize, InputData, OutputDataSize, OutputData);

    do                          /* breakout loop */
    {
        /* validate parameters */
        if ((!InputData) ||
            (InputDataSize < sizeof(XORPRTM_MIB_GET_INPUT_DATA)) ||
            (!OutputDataSize))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibGet((PXORPRTM_MIB_GET_INPUT_DATA) InputData,
                          (PXORPRTM_MIB_GET_OUTPUT_DATA) OutputData,
                          OutputDataSize,
                          GET_FIRST);
        
    } while(FALSE);

    TRACE1(LEAVE, "Leaving  MibGetFirst: %u", dwErr);

    return dwErr;
}


/* XXX: Not sure if this is really needed */

DWORD
WINAPI
MibGetNext (
         ULONG   InputDataSize,
         PVOID	InputData,
     PULONG  OutputDataSize,
        PVOID	OutputData
    )
/*++

Routine Description
    This function retrieves one of...
    . global configuration
    . interface configuration
    . global stats
    . interface stats
    . interface binding

    It differs from both MibGet() and MibGetFirst() in that it returns the
    entry AFTER the one specified in the indicated table.  Thus, in the
    interface configuration, interface stats, and interface binding tables,
    this function supplies the entry after the one with the input address.

    If there are no more entries in the table being queried we return
    ERROR_NO_MORE_ITEMS.  Unlike SNMP we don't walk to the next table.
    This does not take away any functionality since the NT SNMP agent
    will try the next variable (having ID one greater than the ID passed
    in) automatically on getting this error.

Arguments
    InputData           Relevant input, struct XORPRTM_MIB_GET_INPUT_DATA
    InputDataSize       Size of the input
    OutputData          Buffer for struct XORPRTM_MIB_GET_OUTPUT_DATA
    OutputDataSize       size of output buffer received
                        size of output buffer required
                        
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD                           dwErr   = NO_ERROR;

    TRACE4(ENTER, "Entering MibGetFirst: %u 0x%08x 0x%08x 0x%08x",
           InputDataSize, InputData, OutputDataSize, OutputData);

    do                          /* breakout loop */
    {
        /* validate parameters */
        if ((!InputData) ||
            (InputDataSize < sizeof(XORPRTM_MIB_GET_INPUT_DATA)) ||
            (!OutputDataSize))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibGet((PXORPRTM_MIB_GET_INPUT_DATA) InputData,
                          (PXORPRTM_MIB_GET_OUTPUT_DATA) OutputData,
                          OutputDataSize,
                          GET_NEXT);

    } while(FALSE);

    TRACE1(LEAVE, "Leaving  MibGetNext: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibSetTrapInfo (
     HANDLE  Event,
     ULONG   InputDataSize,
     PVOID	InputData,
    PULONG	OutputDataSize,
    PVOID	OutputData
    )
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE0(ENTER, "Entering MibSetTrapInfo");
    TRACE1(LEAVE, "Leaving  MibSetTrapInfo: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibGetTrapInfo (
     ULONG	InputDataSize,
     PVOID	InputData,
    PULONG  OutputDataSize,
    PVOID	OutputData
    )
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE0(ENTER, "Entering MibGetTrapInfo");
    TRACE1(LEAVE, "Leaving  MibGetTrapInfo: %u", dwErr);

    return dwErr;
}



/*------------------------------------------------------------------------ */

/* This is where the action is. First function called after dll load. */

#define RF_FUNC_FLAGS (RF_ROUTING | RF_ADD_ALL_INTERFACES)

DWORD
APIENTRY
RegisterProtocol(
    PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    DWORD   dwErr = NO_ERROR;
    
    TRACE0(ENTER, "Entering RegisterProtocol");

    do {
        if (pRoutingChar->dwProtocolId != XORPRTM_PROTOCOL_ID) {
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }


	TRACE1(CONFIGURATION, "fSupportedFunctionality is: %08lx", 
		pRoutingChar->fSupportedFunctionality);

        if  ((pRoutingChar->fSupportedFunctionality & RF_FUNC_FLAGS) !=
	     RF_FUNC_FLAGS) {
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }
    
        pRoutingChar->fSupportedFunctionality = RF_FUNC_FLAGS;
        pServiceChar->fSupportedFunctionality = 0;

        pRoutingChar->pfnStartProtocol      = StartProtocol;
        pRoutingChar->pfnStartComplete      = StartComplete;
        pRoutingChar->pfnStopProtocol       = StopProtocol;
        pRoutingChar->pfnGetGlobalInfo      = GetGlobalInfo;
        pRoutingChar->pfnSetGlobalInfo      = SetGlobalInfo;
        pRoutingChar->pfnQueryPower         = NULL;
        pRoutingChar->pfnSetPower           = NULL;

        pRoutingChar->pfnAddInterface       = AddInterface;
        pRoutingChar->pfnDeleteInterface    = DeleteInterface;
        pRoutingChar->pfnInterfaceStatus    = InterfaceStatus;
        pRoutingChar->pfnGetInterfaceInfo   = GetInterfaceConfigInfo;
        pRoutingChar->pfnSetInterfaceInfo   = SetInterfaceConfigInfo;

        pRoutingChar->pfnGetEventMessage    = GetEventMessage;

        pRoutingChar->pfnUpdateRoutes       = DoUpdateRoutes;

        pRoutingChar->pfnConnectClient      = NULL;
        pRoutingChar->pfnDisconnectClient   = NULL;

        pRoutingChar->pfnGetNeighbors       = NULL;
        pRoutingChar->pfnGetMfeStatus       = NULL; /* XXX multicast */

        pRoutingChar->pfnMibCreateEntry     = MibCreate;
        pRoutingChar->pfnMibDeleteEntry     = MibDelete;
        pRoutingChar->pfnMibGetEntry        = MibGet;
        pRoutingChar->pfnMibSetEntry        = MibSet;
        pRoutingChar->pfnMibGetFirstEntry   = MibGetFirst;
        pRoutingChar->pfnMibGetNextEntry    = MibGetNext;
        pRoutingChar->pfnMibSetTrapInfo     = MibSetTrapInfo;
        pRoutingChar->pfnMibGetTrapInfo     = MibGetTrapInfo;
    } while (FALSE);

    TRACE1(LEAVE, "Leaving RegisterProtocol: %u", dwErr);

    return dwErr;
}
