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
