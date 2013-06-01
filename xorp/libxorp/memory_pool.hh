// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2012 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ifndef _LIBXORP_MEMORY_POOL_HH_
#define _LIBXORP_MEMORY_POOL_HH_

#include "xorp.h"

template <class T, size_t EXPANSION_SIZE = 100>
class MemoryPool : public NONCOPYABLE {
public:
    MemoryPool();
    ~MemoryPool();

    //Allocate element of type T from free list
    void* alloc();

    // Return element to the free list
    void free(void* doomed);
private:
    // next element on the free list
    MemoryPool<T, EXPANSION_SIZE>* _next;

    // Add free elements to the list
    void expand_free_list();

    size_t _size;
};

template <class T, size_t EXPANSION_SIZE>
MemoryPool<T, EXPANSION_SIZE>::MemoryPool() :
    _size((sizeof(T) > sizeof(MemoryPool<T, EXPANSION_SIZE>)) ? sizeof(T) :sizeof(MemoryPool<T, EXPANSION_SIZE>))
{
    expand_free_list();
}

template <class T, size_t EXPANSION_SIZE>
MemoryPool<T, EXPANSION_SIZE>::~MemoryPool()
{
    for (MemoryPool<T, EXPANSION_SIZE> *nextPtr = _next; nextPtr != NULL; nextPtr = _next) {
	_next = _next->_next;
	delete [] reinterpret_cast<char* >(nextPtr);
    }
}

template <class T, size_t EXPANSION_SIZE>
inline void*
MemoryPool<T, EXPANSION_SIZE>::alloc()
{
    if (!_next)
	expand_free_list();

    MemoryPool<T, EXPANSION_SIZE>* head = _next;
    _next = head->_next;
    return head;
}

template <class T, size_t EXPANSION_SIZE>
inline void
MemoryPool<T, EXPANSION_SIZE>::free(void* doomed)
{
    MemoryPool<T, EXPANSION_SIZE>* head = reinterpret_cast<MemoryPool<T, EXPANSION_SIZE>* >(doomed);

    head->_next = _next;
    _next = head;
}

template <class T, size_t EXPANSION_SIZE>
inline void
MemoryPool<T, EXPANSION_SIZE>::expand_free_list()
{
    // We must allocate object large enough to contain the next pointer
    MemoryPool<T, EXPANSION_SIZE>* runner = reinterpret_cast<MemoryPool<T, EXPANSION_SIZE>* >(new char[_size]);

    _next = runner;
    for (size_t i = 0; i < EXPANSION_SIZE; ++i) {
	runner->_next = reinterpret_cast<MemoryPool<T, EXPANSION_SIZE>* >(new char[_size]);
	runner = runner->_next;
    }
    runner->_next = NULL;
}

#endif /* MEMORY_POOL_HH_ */
