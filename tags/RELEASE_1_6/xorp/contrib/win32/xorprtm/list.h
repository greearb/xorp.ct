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
 * $XORP: xorp/contrib/win32/xorprtm/list.h,v 1.7 2008/10/02 21:56:40 bms Exp $
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



