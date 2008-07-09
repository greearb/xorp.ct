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
 * $XORP: xorp/contrib/win32/xorprtm/defs.h,v 1.3 2007/02/16 22:45:32 pavlin Exp $
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
