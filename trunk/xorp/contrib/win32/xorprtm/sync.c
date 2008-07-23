/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
 * vim:set sts=4 ts=8:
 *
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
 *
 * $XORP: xorp/contrib/win32/xorprtm/sync.c,v 1.4 2008/01/04 03:15:39 pavlin Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#include "pchsample.h"
#pragma hdrstop

DWORD
CreateReadWriteLock(PREAD_WRITE_LOCK pRWL)
{

    pRWL->RWL_ReaderCount = 0;

    __try {
        InitializeCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetLastError();
    }

    pRWL->RWL_ReaderDoneEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (pRWL->RWL_ReaderDoneEvent != NULL) {
        return GetLastError();
    }

    return NO_ERROR;
}

VOID
DeleteReadWriteLock(PREAD_WRITE_LOCK pRWL)
{

    CloseHandle(pRWL->RWL_ReaderDoneEvent);
    pRWL->RWL_ReaderDoneEvent = NULL;
    DeleteCriticalSection(&pRWL->RWL_ReadWriteBlock);
    pRWL->RWL_ReaderCount = 0;
}

VOID
AcquireReadLock(PREAD_WRITE_LOCK pRWL)
{

    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock); 
    InterlockedIncrement(&pRWL->RWL_ReaderCount);
    LeaveCriticalSection(&pRWL->RWL_ReadWriteBlock);
}

VOID
ReleaseReadLock(PREAD_WRITE_LOCK pRWL)
{

    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) < 0) {
        SetEvent(pRWL->RWL_ReaderDoneEvent); 
    }
}

VOID
AcquireWriteLock(PREAD_WRITE_LOCK pRWL)
{

    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock);
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) >= 0) { 
        WaitForSingleObject(pRWL->RWL_ReaderDoneEvent, INFINITE);
    }
}

VOID
ReleaseWriteLock(PREAD_WRITE_LOCK pRWL)
{

    InterlockedIncrement(&pRWL->RWL_ReaderCount);
    LeaveCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
}

