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
 * $XORP: xorp/contrib/win32/xorprtm/defs.h,v 1.7 2008/10/02 21:56:40 bms Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#ifndef _DEFS_H_
#define _DEFS_H_

#define GLOBAL_HEAP                 g_ce.hGlobalHeap
#define TRACEID                     g_ce.dwTraceID

#define ENTER_XORPRTM_API()          EnterSubsystemAPI()
#define ENTER_XORPRTM_WORKER()       EnterSubsystemWorker()
#define LEAVE_XORPRTM_API()          LeaveSubsystemWorker()
#define LEAVE_XORPRTM_WORKER()       LeaveSubsystemWorker()

#define MALLOC(ppPointer, ulSize, pdwErr)                                   \
{                                                                           \
    if (*(ppPointer) = HeapAlloc(GLOBAL_HEAP, HEAP_ZERO_MEMORY, (ulSize)))  \
    {                                                                       \
        *(pdwErr) = NO_ERROR;                                               \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        *(pdwErr) = ERROR_NOT_ENOUGH_MEMORY;                                \
        TRACE1(ANY, "Error allocating %u bytes", (ulSize));                 \
    }                                                                       \
}
#define REALLOC(ptr, size)          HeapReAlloc(GLOBAL_HEAP, 0, ptr, size)
#define FREE(ptr)                                           \
{                                                           \
     HeapFree(GLOBAL_HEAP, 0, (ptr));                       \
     (ptr) = NULL;                                          \
}

#define XORPRTM_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_ENTER           ((DWORD)0x00010000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_LEAVE           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_DEBUG           ((DWORD)0x00040000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_CONFIGURATION   ((DWORD)0x00100000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_NETWORK         ((DWORD)0x00200000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_PACKET          ((DWORD)0x00400000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_TIMER           ((DWORD)0x00800000 | TRACE_USE_MASK)
#define XORPRTM_TRACE_MIB             ((DWORD)0x01000000 | TRACE_USE_MASK)

#define TRACE0(l,a)                                                     \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfEx(TRACEID, XORPRTM_TRACE_ ## l, a)
#define TRACE1(l,a,b)                                                   \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfEx(TRACEID, XORPRTM_TRACE_ ## l, a, b)
#define TRACE2(l,a,b,c)                                                 \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfEx(TRACEID, XORPRTM_TRACE_ ## l, a, b, c)
#define TRACE3(l,a,b,c,d)                                               \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfEx(TRACEID, XORPRTM_TRACE_ ## l, a, b, c, d)
#define TRACE4(l,a,b,c,d,e)                                             \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfEx(TRACEID, XORPRTM_TRACE_ ## l, a, b, c, d, e)
#define TRACE5(l,a,b,c,d,e,f)                                           \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfEx(TRACEID, XORPRTM_TRACE_ ## l, a, b, c, d, e, f)

#endif /* _DEFS_H_ */
