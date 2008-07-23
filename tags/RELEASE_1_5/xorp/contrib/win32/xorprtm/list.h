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
 * $XORP: xorp/contrib/win32/xorprtm/list.h,v 1.4 2008/01/04 03:15:39 pavlin Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#ifndef _LIST_H_
#define _LIST_H_

#define InitializeListHead(ListHead)                            \
    ((ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead)                                   \
    ((ListHead)->Flink == (ListHead))

#define RemoveHeadList(ListHead)                                \
    (ListHead)->Flink;                                          \
    {RemoveEntryList((ListHead)->Flink)}

#define RemoveTailList(ListHead)                                \
    (ListHead)->Blink;                                          \
    {RemoveEntryList((ListHead)->Blink)}

#define RemoveEntryList(Entry)                                  \
{                                                               \
    PLIST_ENTRY _EX_Blink;                                      \
    PLIST_ENTRY _EX_Flink;                                      \
    _EX_Flink = (Entry)->Flink;                                 \
    _EX_Blink = (Entry)->Blink;                                 \
    _EX_Blink->Flink = _EX_Flink;                               \
    _EX_Flink->Blink = _EX_Blink;                               \
}

#define InsertTailList(ListHead,Entry)                          \
{                                                               \
    PLIST_ENTRY _EX_Blink;                                      \
    PLIST_ENTRY _EX_ListHead;                                   \
    _EX_ListHead = (ListHead);                                  \
    _EX_Blink = _EX_ListHead->Blink;                            \
    (Entry)->Flink = _EX_ListHead;                              \
    (Entry)->Blink = _EX_Blink;                                 \
    _EX_Blink->Flink = (Entry);                                 \
    _EX_ListHead->Blink = (Entry);                              \
}

#define InsertHeadList(ListHead,Entry)                          \
{                                                               \
    PLIST_ENTRY _EX_Flink;                                      \
    PLIST_ENTRY _EX_ListHead;                                   \
    _EX_ListHead = (ListHead);                                  \
    _EX_Flink = _EX_ListHead->Flink;                            \
    (Entry)->Flink = _EX_Flink;                                 \
    (Entry)->Blink = _EX_ListHead;                              \
    _EX_Flink->Blink = (Entry);                                 \
    _EX_ListHead->Flink = (Entry);                              \
}

#define InsertSortedList(ListHead, Entry, CompareFunction)      \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    PLIST_ENTRY _EX_Blink;                                      \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        if ((*(CompareFunction))((Entry), _EX_Entry) <= 0)      \
            break;                                              \
    _EX_Blink = _EX_Entry->Blink;                               \
    _EX_Blink->Flink = (Entry);                                 \
    _EX_Entry->Blink = (Entry);                                 \
    (Entry)->Flink     = _EX_Entry;                             \
    (Entry)->Blink     = _EX_Blink;                             \
}

#define FindList(ListHead, Key, Entry, CompareFunction)         \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    *(Entry) = NULL;                                            \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        if ((*(CompareFunction))((Key), _EX_Entry) == 0)        \
        {                                                       \
            *(Entry) = _EX_Entry;                               \
            break;                                              \
        }                                                       \
}

#define FindSortedList(ListHead, Key, Entry, CompareFunction)   \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    *(Entry) = NULL;                                            \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        if ((*(CompareFunction))((Key), _EX_Entry) <= 0)        \
        {                                                       \
            *(Entry) = _EX_Entry;                               \
            break;                                              \
        }                                                       \
}

#define MapCarList(ListHead, VoidFunction)                      \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        (*(VoidFunction))(_EX_Entry);                           \
}

#define FreeList(ListHead, FreeFunction)                        \
{                                                               \
    PLIST_ENTRY _EX_Head;                                       \
    while (!IsListEmpty(ListHead))                              \
    {                                                           \
        _EX_Head = RemoveHeadList(ListHead);                    \
        (*(FreeFunction))(_EX_Head);                            \
    }                                                           \
}

#define QUEUE_ENTRY                     LIST_ENTRY
#define PQUEUE_ENTRY                    PLIST_ENTRY

#define InitializeQueueHead(QueueHead)  InitializeListHead(QueueHead)
#define IsQueueEmpty(QueueHead)         IsListEmpty(QueueHead)
#define Enqueue(QueueHead, Entry)       InsertTailList(QueueHead, Entry)
#define Dequeue(QueueHead)              RemoveHeadList(QueueHead)
#define FreeQueue(QueueHead, FreeFunction)                      \
    FreeList(QueueHead, FreeFunction)
#define MapCarQueue(QueueHead, VoidFunction)                    \
    MapCarList(QueueHead, VoidFunction)

#define STACK_ENTRY                     LIST_ENTRY
#define PSTACK_ENTRY                    PLIST_ENTRY

#define InitializeStackHead(StackHead)  InitializeListHead(StackHead)
#define IsStackEmpty(StackHead)         IsListEmpty(StackHead)
#define Push(StackHead, Entry)          InsertHeadList(StackHead, Entry)
#define Pop(StackHead)                  RemoveHeadList(StackHead)
#define FreeStack(StackHead, FreeFunction)                      \
    FreeList(StackHead, FreeFunction)
#define MapCarStack(StackHead, VoidFunction)                    \
    MapCarList(StackHead, VoidFunction)

#endif /* _LIST_H_ */



