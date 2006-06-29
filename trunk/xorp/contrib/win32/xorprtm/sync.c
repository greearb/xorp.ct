/* vim:set sts=4 ts=8: */

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

