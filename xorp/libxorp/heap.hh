// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// Portions of this code originally derived from:
// 	FreeBSD dummynet code, (C) 2001 Luigi Rizzo.

// $XORP: xorp/libxorp/heap.hh,v 1.18 2008/10/02 21:57:30 bms Exp $

#ifndef __LIBXORP_HEAP_HH__
#define __LIBXORP_HEAP_HH__



#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <assert.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "timeval.hh"

/**
 * @short Heap class
 *
 * Heap implements a priority queue, mostly used by @ref Timer
 * objects. This implementation supports removal of arbitrary objects
 * from the heap, even if they are not located at the top. To support
 * this the objects must inherit from HeapBase which contains an index
 * to its own positon within the heap.
 *
 */
const int NOT_IN_HEAP =	-1 ;

/**
 * Objects stored on the heap should inherit from this class. If
 * removal from arbitary positions (not just head) is required.
 */
class HeapBase {
 public:
    HeapBase() : _pos_in_heap(NOT_IN_HEAP) {}
    virtual ~HeapBase() {}

    int		_pos_in_heap;	// position of this object in the heap
};

class Heap {
    friend class TimerList;
protected:
typedef TimeVal Heap_Key ;
    struct heap_entry {
	Heap_Key key;	/* sorting key. Topmost element is smallest one */
	HeapBase *object;      /* object pointer */
    } ;
public:
    /**
     * Default constructor used to build a standard heap with no support for
     * removal from the middle.
     */
    Heap() : _intrude(false)
    {}
    
    /**
     * Constructor used to build a standard heap with support for
     * removal from the middle. Should be used with something
     * like:
     * <PRE>
     * struct _foo { ... ; int my_index ; ... } x;
     * ...
     * Heap *h = new Heap (OFFSET_OF(x, my_index));
     * </PRE>
     */
    explicit Heap(bool); // heap supporting removal from the middle
    
    /**
     * Destructor
     */
    virtual ~Heap();

    /**
     * Push an object into the heap by using a sorting key.
     * 
     * @param k the sorting key.
     * @param p the object to push into the heap.
     */
    void push(Heap_Key k, HeapBase *p) { push(k, p, 0); }

    /**
     * Bubble-up an object in the heap.
     * 
     * Note: this probably should not be exposed.
     * 
     * @param i the offset of the object to bubble-up.
     */
    void push(int i) { push( Heap_Key(0, 0) /* anything */, NULL, i); }

    /**
     * Move an object in the heap according to the new key.
     * Note: can only be used if the heap supports removal from the middle.
     * 
     * @param new_key the new key.
     * @param object the object to move.
     */
    void move(Heap_Key new_key, HeapBase *object);

    /**
     * Get a pointer to the entry at the top of the heap.
     * 
     * Both the key and the value can be derived from the return value.
     * 
     * @return the pointer to the entry at the top of the heap.
     */
    struct heap_entry *top() const {
	return  (_p == 0 || _elements == 0) ? 0 :  &(_p[0]) ;
    }

    /**
     * Get the number of elements in the heap.
     *
     * @return the number of elements in the heap.
     */
    size_t size() const { return _elements; }

    /**
     * Remove the object top of the heap.
     */
    void pop() { pop_obj(0); }

    /**
     * Remove an object from an arbitrary position in the heap.
     * 
     * Note: only valid if the heap supports this kind of operation.
     * 
     * @param p the object to remove if not NULL, otherwise the top element
     * from the heap.
     */
    void pop_obj(HeapBase *p);

    /**
     * Rebuild the heap structure.
     */
    void heapify();

#ifdef _TEST_HEAP_CODE
    void print();
    void print_all(int);
#endif
    
private:
    void push(Heap_Key key, HeapBase *p, int son);
    int resize(int new_size);
    void verify();
    
    int _size;
    int _elements ;
    bool _intrude ; // True if the object holds the heap position
    struct heap_entry *_p ;   // really an array of "size" entries
};

#endif // __LIBXORP_HEAP_HH__
