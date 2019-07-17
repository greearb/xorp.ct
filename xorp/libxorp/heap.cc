// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
//
// Portions of this code originally derived from:
// 	FreeBSD dummynet code, (C) 2001 Luigi Rizzo.



#include "libxorp_module.h"
#include "libxorp/xorp.h"

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "heap.hh"

#include <strings.h>

#ifdef _TEST_HEAP_CODE
#define DBG(x)	x
#else
#define DBG(x)
#endif

/*
 * A heap entry is made of a key and a pointer to the actual
 * object stored in the heap.
 * The heap is an array of heap_entry entries, dynamically allocated.
 * Current size is "size", with "elements" actually in use.
 * The heap normally supports only ordered insert and extract from the top.
 * If we want to extract an object from the middle of the heap, we
 * have to know where the object itself is located in the heap (or we
 * need to scan the whole array). To this purpose, an object has a
 * field (int) which contains the index of the object itself into the
 * heap. When the object is moved, the field must also be updated.
 * The offset of the index in the object is stored in the 'offset'
 * field in the heap descriptor. The assumption is that this offset
 * is non-zero if we want to support extract from the middle.
 */

/*
 * Heap management functions.
 *
 * In the heap, first node is element 0. Children of i are 2i+1 and 2i+2.
 * Some macros help finding parent/children so we can optimize them.
 */
#define HEAP_FATHER(x)		( ( (x) - 1 ) / 2 )
#define HEAP_LEFT(x)		( 2*(x) + 1 )
#define HEAP_SWAP(a, b, buffer)	{ buffer = a ; a = b ; b = buffer ; }
#define HEAP_INCREMENT		15
            
int
Heap::resize(int new_size)
{
    struct heap_entry *p;

    if (_size >= new_size) {
        XLOG_ERROR("Bogus call inside heap::resize: have %d want %d",
		   _size, new_size);
        return 0;
    }
    new_size = (new_size + HEAP_INCREMENT ) & ~HEAP_INCREMENT ;
    p = new struct heap_entry[new_size];
    if (p == NULL) {
        XLOG_ERROR("Heap resize %d failed", new_size); 
        return 1;	// Error
    } 
    if (_size > 0) {
	for (int i = 0; i<_size; i++) {
	    p[i] = _p[i];
	}
	// Initialize pointers just in case.
	for (int i = _size; i<new_size; i++) {
	    p[i].object = NULL;
	}
        delete[] _p;
    }
    _p = p;
    _size = new_size;
    return 0;
}

/*
 * Insert an element in the heap.
 * Normally (p != NULL) we insert p in a new slot and reorder the heap.
 * If p == NULL, then the element is already in place, and 'son' is the
 * position where to start the bubble-up.
 *
 * If offset > 0 the position (index, int) of the element in the heap is
 * also stored in the element itself at the given offset in bytes.
 */
#define SET_OFFSET(node)				\
do {							\
    if (_intrude)					\
	_p[node].object->_pos_in_heap = node ;		\
} while (0)
/*
 * RESET_OFFSET is used for sanity checks. It sets offset to an invalid value.
 */
#define RESET_OFFSET(node)					\
do {								\
    if (_intrude)						\
	_p[node].object->_pos_in_heap = NOT_IN_HEAP ;		\
} while (0)

// inner implementation of push -- if p == NULL, the element is already
// there, so start bubbling up from 'son'. Otherwise, push in new element p
// with key k

void
Heap::push(Heap_Key k, HeapBase *p, int son)
{
    if (p != 0) {
	// Insert new element at the end, possibly resize
	debug_msg("insert key %u.%06u ptr %p\n",
		  XORP_UINT_CAST(k.sec()), XORP_UINT_CAST(k.usec()), p);
        son = _elements;
        if (son == _size) {
	    // need resize...
            if (resize(_elements + 1))
                return;		// Failure...
	}
        _p[son].object = p ;
        _p[son].key = k ;
        _elements++ ;
    }
    while (son > 0) {
	// Bubble up
        int father = HEAP_FATHER(son);
        struct heap_entry tmp;

        if (_p[father].key <= _p[son].key)
            break;	// Found right position
        // Son smaller than father, swap and repeat
        HEAP_SWAP(_p[son], _p[father], tmp);
        SET_OFFSET(son);
        son = father;
    }
    SET_OFFSET(son);
}

//
// Remove top element from heap, or obj if obj != NULL
//
void
Heap::pop_obj(HeapBase *obj)
{
    int child, father, max_entry = _elements - 1 ;

    if (max_entry < 0) {
        XLOG_ERROR("Extract from empty heap 0x%p", this);
        return;
    }
    father = 0 ;	// Default: move up smallest child
    if (obj != NULL) {
	// Extract specific element, index is at offset
        if (!_intrude) {
            XLOG_FATAL("*** heap_extract from middle "
		       "not supported on this heap!!!");
	}

        father = obj->_pos_in_heap;
        if (father < 0 || father >= _elements) {
            XLOG_FATAL("-- heap_extract, father %d out of bound 0..%d",
		       father, _elements);
        }
	if (_p[father].object != obj) {
	    XLOG_FATAL("-- bad obj 0x%p instead of 0x%p at %d",
		       _p[father].object, obj, father);
	}
	debug_msg("-- delete key %u\n", XORP_UINT_CAST(_p[father].key.sec()));
    }
    RESET_OFFSET(father);
    child = HEAP_LEFT(father);		// left child
    while (child <= max_entry) {	// valid entry
        if (child != max_entry && _p[child+1].key < _p[child].key )
            child = child + 1;		// take right child, otherwise left
        _p[father] = _p[child];
        SET_OFFSET(father);
        father = child;
        child = HEAP_LEFT(child);	// left child for next loop
    }
    _elements--;
    if (father != max_entry) {
        //
	// Fill hole with last entry and bubble up, reusing the insert code
	//
        _p[father] = _p[max_entry];
        push(father);		// this one cannot fail
    }
    DBG(verify());
}

#if 1
/*
 * change object position and update references
 * XXX this one is never used!
 */
void
Heap::move(Heap_Key new_key, HeapBase *object)
{
    int temp;
    int i;
    int max_entry = _elements - 1;
    struct heap_entry buf;

    if (!_intrude)
        XLOG_FATAL("cannot move items on this heap");

    i = object->_pos_in_heap;
    if (new_key < _p[i].key) {		// must move up
        _p[i].key = new_key;
        for (; i > 0 && new_key < _p[(temp = HEAP_FATHER(i))].key;
	     i = temp) {		// bubble up
            HEAP_SWAP(_p[i], _p[temp], buf);
            SET_OFFSET(i);
        }
    } else {				// must move down
        _p[i].key = new_key;
        while ( (temp = HEAP_LEFT(i)) <= max_entry) {	// found left child
            if ((temp != max_entry) && _p[temp+1].key < _p[temp].key)
                temp++;			// select child with min key
            if (_p[temp].key < new_key) {		// go down
                HEAP_SWAP(_p[i], _p[temp], buf);
                SET_OFFSET(i);
            } else {
                break;
	    }
            i = temp;
        }
    }
    SET_OFFSET(i);
}
#endif // heap_move, unused

// heapify() will reorganize data inside an array to maintain the
// heap property. It is needed when we delete a bunch of entries.

void
Heap::heapify()
{
    int i;

    for (i = 0; i < _elements; i++)
        push(i);
}

Heap::Heap()
	: _size(0), _elements(0), _intrude(false), _p(NULL) {
    //printf("created heap (dflt), this: %p\n", this);
}

Heap::Heap(bool intrude)
	: _size(0), _elements(0), _intrude(intrude), _p(NULL) {
    //printf("created heap, this: %p\n", this);
}

Heap::~Heap()
{
    //printf("deleting heap, this: %p\n", this);
    if (_p != NULL)
        delete[] _p;
}

void
Heap::verify()
{
    int i;
    for (i = 1; i < _elements; i++) {
	if ( _p[i].key < _p[(i - 1) / 2 ].key) {
	    XLOG_WARNING("+++ heap violated at %d", (i - 1) / 2);
#ifdef _TEST_HEAP_CODE
	    print_all((i - 1) / 2);
#endif
	    return;
	}
    }
}

#ifdef _TEST_HEAP_CODE
void
Heap::print()
{
    fprintf(stderr, "-- heap at 0x%p\n", this);
    fprintf(stderr, "\tsize is %d, offset %d\n", _size, _offset);
}

void
Heap::print_all(int base)
{
    int depth = 1;
    int l = 1;			// nodes to print on this level
    const char *blanks = "";

    for (int i = base; i < _elements; i = i * 2 + 1)
	depth *= 2;
    depth = (depth /4);
    for (int i = base; i < _elements; i = i * 2 + 1) {
	for (int j = i; j < _elements && j < i + l; j++)
	    fprintf(stderr, "%*s[%2d]%3ld%*s",
		    (depth / 2) * 7, blanks, j, _p[j].key.sec(), depth * 7,
		    blanks);
	fprintf(stderr, "\n");
	l = l * 2;
	depth /= 2;
    }
}

#ifdef _TEST_HEAP_CODE_HARNESS

struct foo {
    Heap_Key	t;
    int		index;
    int		the_pos;
};

int
main(int argc, char *argv[])
{
    struct foo a[1000];

    fprintf(stderr, "-- Hello World of heaps...\n");

    Heap *h = new Heap(OFFSET_OF(a[0], the_pos));

    h->print();
    for (int i = 0; i < 100; i++) {
	a[i].t.tv_sec = 1000 - i;
	a[i].t.tv_usec = 1000 + i;
	a[i].index = i;
	h->push(a[i].t, &a[i]);
    }
    for (int i = 0; i < 100; i += 2) {
	h->print_all(0);
	h->pop_obj(&a[i]);
    }

    for (;;) {
	struct heap_entry *e = h->top();
	if (e == 0)
	    break;
	h->print_all(0);
	fprintf(stderr, "++ got %ld.%06ld 0x%p\n",
		e->key.sec(), e->key.usec(), e->object);
	h->pop();
    }
}

#endif // _TEST_HEAP_CODE_HARNESS

#endif // _TEST_HEAP_CODE
