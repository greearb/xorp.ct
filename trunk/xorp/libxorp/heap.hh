// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

// Portions of this code originally derived from:
// 	FreeBSD dummynet code, (C) 2001 Luigi Rizzo.

// $XORP: xorp/libxorp/heap.hh,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $

#ifndef __LIBXORP_HEAP_HH__
#define __LIBXORP_HEAP_HH__

#define LRD	0

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory>

#include "timeval.hh"

/**
 * @short Heap class
 *
 * Heap implements a priority queue, mostly used by @ref Timer
 * objects. This implementation supports removal of arbitrary
 * objects from the heap, even if they are not located at the top.
 * To support this, the objects must reserve an "int" field,
 * whose offset is passed to the heap constructor, and which is
 * is used to store the position of the object within the heap.
 * This offset can be computed using the @ref OFFSET_OF macro.
 *
 * The OFFSET_OF macro is used to return the offset of a field within
 * a structure. It is used by the heap management routines.
 */
#define OFFSET_OF(obj, field) ((int)(&((obj).field)) - (int)&(obj) )

const int NOT_IN_HEAP =	-1 ;

class Heap {
private:
typedef struct timeval Heap_Key ;
    struct heap_entry {
	Heap_Key key ;    /* sorting key. Topmost element is smallest one */
	void *object ;      /* object pointer */
    } ;
public:
    /**
     * Default constructor used to build a standard heap with no support for
     * removal from the middle.
     */
    Heap() { Heap(0); }
    
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
    explicit Heap(int); // heap supporting removal from the middle
    
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
    void push(Heap_Key k, void *p) { push(k, p, 0); }

    /**
     * Bubble-up an object in the heap.
     * 
     * Note: this probably should not be exposed.
     * 
     * @param i the offset of the object to bubble-up.
     */
    void push(int i) { push( (Heap_Key){0,0} /* anything */, NULL, i); }

    /**
     * Move an object in the heap according to the new key.
     * Note: can only be used if the heap supports removal from the middle.
     * 
     * @param new_key the new key.
     * @param object the object to move.
     */
    void move(Heap_Key new_key, void *object);

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
    void pop_obj(void *p);

    /**
     * Rebuild the heap structure.
     */
    void heapify();

#ifdef LRD
    void print();
    void print_all(int);
#endif
    
private:
    void push(Heap_Key key, void *p, int son);
    int resize(int new_size);
    void verify();
    
    int _size;
    int _elements ;
    int _offset ; // XXX if > 0 this is the offset of direct ptr to obj
    struct heap_entry *_p ;   // really an array of "size" entries
};

#endif // __LIBXORP_HEAP_HH__
