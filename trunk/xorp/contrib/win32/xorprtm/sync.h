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
 * $XORP: xorp/contrib/win32/xorprtm/sync.h,v 1.4 2008/01/04 03:15:39 pavlin Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#ifndef _SYNC_H_
#define _SYNC_H_

typedef struct _READ_WRITE_LOCK {
    CRITICAL_SECTION    RWL_ReadWriteBlock;
    LONG                RWL_ReaderCount;
    HANDLE              RWL_ReaderDoneEvent;
} READ_WRITE_LOCK, *PREAD_WRITE_LOCK;

DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL);

VOID
DeleteReadWriteLock(
    PREAD_WRITE_LOCK pRWL);

VOID
AcquireReadLock(
    PREAD_WRITE_LOCK pRWL);

VOID
ReleaseReadLock(
    PREAD_WRITE_LOCK pRWL);

VOID
AcquireWriteLock(
    PREAD_WRITE_LOCK pRWL);

VOID
ReleaseWriteLock(
    PREAD_WRITE_LOCK pRWL);

#define CREATE_READ_WRITE_LOCK(pRWL)                                \
    CreateReadWriteLock(pRWL)
#define DELETE_READ_WRITE_LOCK(pRWL)                                \
    DeleteReadWriteLock(pRWL)
#define READ_WRITE_LOCK_CREATED(pRWL)                               \
    ((pRWL)->RWL_ReaderDoneEvent != NULL)

#define ACQUIRE_READ_LOCK(pRWL)                                     \
    AcquireReadLock(pRWL)
#define RELEASE_READ_LOCK(pRWL)                                     \
    ReleaseReadLock(pRWL)
#define ACQUIRE_WRITE_LOCK(pRWL)                                    \
    AcquireWriteLock(pRWL)
#define RELEASE_WRITE_LOCK(pRWL)                                    \
    ReleaseWriteLock(pRWL)

#define WRITE_LOCK_TO_READ_LOCK(pRWL)                               \
{                                                                   \
    ACQUIRE_READ_LOCK(pRWL);                                        \
    RELEASE_WRITE_LOCK(pRWL);                                       \
}
                
                
typedef struct _LOCKED_LIST {
    CRITICAL_SECTION    lock;
    LIST_ENTRY          head;
    DWORD               created;
} LOCKED_LIST, *PLOCKED_LIST;

#define INITIALIZE_LOCKED_LIST(pLL)                                 \
{                                                                   \
    do                                                              \
    {                                                               \
        __try {                                                     \
            InitializeCriticalSection(&((pLL)->lock));              \
        }                                                           \
        __except (EXCEPTION_EXECUTE_HANDLER) {                      \
            break;                                                  \
        }                                                           \
        InitializeListHead(&((pLL)->head));                         \
        (pLL)->created = 0x12345678;                                \
    } while (FALSE);                                                \
}

#define LOCKED_LIST_INITIALIZED(pLL)                                \
     ((pLL)->created == 0x12345678)

#define DELETE_LOCKED_LIST(pLL, FreeFunction)                       \
{                                                                   \
     (pLL)->created = 0;                                            \
     FreeList(&((pLL)->head), FreeFunction);                        \
     DeleteCriticalSection(&(pLL)->lock);                           \
}

#define ACQUIRE_LIST_LOCK(pLL)                                      \
     EnterCriticalSection(&(pLL)->lock)

#define RELEASE_LIST_LOCK(pLL)                                      \
     LeaveCriticalSection(&(pLL)->lock)


         
#define LOCKED_QUEUE    LOCKED_LIST
#define PLOCKED_QUEUE   PLOCKED_LIST

         
#define INITIALIZE_LOCKED_QUEUE(pLQ)                                \
     INITIALIZE_LOCKED_LIST(pLQ)
#define LOCKED_QUEUE_INITIALIZED(pLQ)                               \
     LOCKED_LIST_INITIALIZED(pLQ)
#define DELETE_LOCKED_QUEUE(pLQ, FreeFunction)                      \
     DELETE_LOCKED_LIST(pLQ, FreeFunction)
#define ACQUIRE_QUEUE_LOCK(pLQ)                                     \
     ACQUIRE_LIST_LOCK(pLQ)
#define RELEASE_QUEUE_LOCK(pLQ)                                     \
     RELEASE_LIST_LOCK(pLQ)


#endif
