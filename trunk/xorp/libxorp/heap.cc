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
//
// Portions of this code originally derived from:
// 	FreeBSD dummynet code, (C) 2001 Luigi Rizzo.

#ident "$XORP: xorp/libxorp/heap.cc,v 1.10 2002/12/09 18:29:11 hodson Exp $"

#include <strings.h>

#include "heap.hh"

#define DBG(x)	//	x

static int
panic(const char *s)
{
    fprintf(stderr, "--panic: %s\n",s);
    exit(0);
}


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
 *
 */
#define HEAP_FATHER(x) ( ( (x) - 1 ) / 2 )
#define HEAP_LEFT(x) ( 2*(x) + 1 )
#define HEAP_SWAP(a, b, buffer) { buffer = a ; a = b ; b = buffer ; }
#define HEAP_INCREMENT  15
            
int
Heap::resize(int new_size)
{
    struct heap_entry *p;

    if (_size >= new_size ) {
        printf("heap::resize, Bogus call, have %d want %d\n",
                _size, new_size);
        return 0 ;
    }
    new_size = (new_size + HEAP_INCREMENT ) & ~HEAP_INCREMENT ;
    p = new struct heap_entry[new_size];
    if (p == NULL) {
        printf(" heap_init, resize %d failed\n", new_size ); 
        return 1 ; /* error */
    } 
    if (_size > 0) {
        bcopy(_p, p, _size * sizeof(*p) );
        delete[] _p;
    }
    _p = p ;
    _size = new_size ;
    return 0 ;
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
#define SET_OFFSET(node) \
    if (_offset > 0) \
            *((int *)((char *)(_p[node].object) + _offset)) = node ;
/*
 * RESET_OFFSET is used for sanity checks. It sets offset to an invalid value.
 */
#define RESET_OFFSET(node) \
    if (_offset > 0) \
            *((int *)((char *)(_p[node].object) + _offset)) = NOT_IN_HEAP ;

// inner implementation of push -- if p == NULL, the element is already
// there, so start bubbling up from 'son'. Otherwise, push in new element p
// with key k

void
Heap::push(Heap_Key k, void *p, int son)
{
    if (p != 0) { /* insert new element at the end, possibly resize */
	DBG(fprintf(stderr, "-- insert key %ld.%06ld ptr %p\n",
		 k.tv_sec, k.tv_usec, p););
        son = _elements ;
        if (son == _size) /* need resize... */
            if (resize(_elements+1) )
                return ; /* failure... */
        _p[son].object = p ;
        _p[son].key = k ;
        _elements++ ;
    }
    while (son > 0) {                           /* bubble up */
        int father = HEAP_FATHER(son) ;
        struct heap_entry tmp  ;

        if ( _p[father].key < _p[son].key )
            break ; /* found right position */
        /* son smaller than father, swap and repeat */
        HEAP_SWAP(_p[son], _p[father], tmp) ;
        SET_OFFSET(son);
        son = father ;
    }
    SET_OFFSET(son);
}

// remove top element from heap, or obj if obj != NULL
void
Heap::pop_obj(void *obj)
{
    int child, father, max_entry = _elements - 1 ;

    if (max_entry < 0) {
        printf("warning, extract from empty heap 0x%p\n", this);
        return ;
    }
    father = 0 ; /* default: move up smallest child */
    if (obj != NULL) { /* extract specific element, index is at offset */
        if (_offset <= 0)
            panic("*** heap_extract from middle not supported on this heap!!!\n"
);
        father = *((int *)((char *)obj + _offset)) ;
        if (father < 0 || father >= _elements) {
            fprintf(stderr, "-- heap_extract, father %d out of bound 0..%d\n",
                father, _elements);
            panic("heap_extract");
        }
	if (_p[father].object != obj) {
	    fprintf(stderr, "-- bad obj 0x%p instead of 0x%p at %d\n",
		_p[father].object, obj, father);
	    panic("");
	}
	DBG(fprintf(stderr, "-- delete key %ld\n", _p[father].key.tv_sec););
    }
    RESET_OFFSET(father);
    child = HEAP_LEFT(father) ;         /* left child */
    while (child <= max_entry) {        /* valid entry */
        if (child != max_entry && _p[child+1].key < _p[child].key )
            child = child+1 ;           /* take right child, otherwise left */
        _p[father] = _p[child] ;
        SET_OFFSET(father);
        father = child ;
        child = HEAP_LEFT(child) ;   /* left child for next loop */
    }
    _elements-- ;
    if (father != max_entry) {
        /*
         * Fill hole with last entry and bubble up, reusing the insert code
         */
        _p[father] = _p[max_entry] ;
        push(father); /* this one cannot fail */
    }
    verify();
}

#if 1
/*
 * change object position and update references
 * XXX this one is never used!
 */
void
Heap::move(Heap_Key new_key, void *object)
{
    int temp;
    int i ;
    int max_entry = _elements-1 ;
    struct heap_entry buf ;

    if (_offset <= 0)
        panic("cannot move items on this heap");

    i = *((int *)((char *)object + _offset));
    if ( new_key < _p[i].key ) { /* must move up */
        _p[i].key = new_key ;
        for (; i>0 && new_key < _p[(temp = HEAP_FATHER(i))].key ;
                 i = temp ) { /* bubble up */
            HEAP_SWAP(_p[i], _p[temp], buf) ;
            SET_OFFSET(i);
        }
    } else {            /* must move down */
        _p[i].key = new_key ;
        while ( (temp = HEAP_LEFT(i)) <= max_entry) { /* found left child */
            if ((temp != max_entry) && _p[temp+1].key < _p[temp].key )
                temp++ ; /* select child with min key */
            if ( _p[temp].key < new_key ) { /* go down */
                HEAP_SWAP(_p[i], _p[temp], buf) ;
                SET_OFFSET(i);
            } else
                break ;
            i = temp ;
        }
    }
    SET_OFFSET(i);
}
#endif /* heap_move, unused */

// heapify() will reorganize data inside an array to maintain the
// heap property. It is needed when we delete a bunch of entries.

void
Heap::heapify()
{
    int i ;

    for (i = 0 ; i < _elements ; i++ )
        push(i) ;
}

Heap::Heap(int ofs)
{
    bzero(this, sizeof(*this) );
    _offset = ofs ;
    DBG(fprintf(stderr, "++ constructor for 0x%p\n", this););
}

// cleanup the heap and free data structure
Heap::~Heap()
{
    if (_size >0 )
        delete[] _p ;
    bzero(this, sizeof(*this) );
}

void
Heap::verify()
{
    int i ;
    for (i = 1 ; i < _elements ; i++ )
	if ( _p[i].key < _p[(i-1)/2 ].key ) {
	    fprintf(stderr, "+++ heap violated at %d\n", i-1/2);
#if LRD
	    print_all(i-1/2);
#endif
	    return ;
	}
}

#if LRD
void
Heap::print()
{
    DBG(fprintf(stderr, "-- heap at 0x%p\n", this);
    fprintf(stderr, "\tsize is %d, offset %d\n", _size, _offset););
}

void
Heap::print_all(int base)
{
    int depth = 1;
    int l = 1 ; // nodes to print on this level
    char *blanks="" ;

    for (int i=base ; i < _elements ; i = i*2 + 1)
	depth *= 2 ;
    depth = (depth /4) ;
    for (int i=base ; i < _elements ; i = i*2 + 1) {
	for (int j = i ; j < _elements && j < i+l ; j++)
	    fprintf(stderr, "%*s[%2d]%3ld%*s",
		(depth/2)*7, blanks, j, _p[j].key.t.tv_sec, depth*7, blanks);
	fprintf(stderr, "\n");
	l = l*2 ;
	depth /= 2 ;
    }
}

struct foo {
    Heap_Key t;
    int index ;
    int the_pos;
} ;

int
main(int argc, char *argv[])
{
    struct foo a[1000] ;

    fprintf(stderr, "-- Hello World of heaps...\n");
    
    Heap *h = new Heap (OFFSET_OF(a[0], the_pos));

    h->print();
    for (int i=0; i < 100 ; i++) {
	a[i].t.tv_sec = 1000 - i ;
	a[i].t.tv_usec = 1000 + i ;
	a[i].index = i ;
	h->push( a[i].t , &a[i] );
    }
    for (int i = 0 ; i < 100 ; i+= 2) {
	h->print_all(0);
	h->pop_obj(&a[i]);
    }

    for (;;) {
	struct heap_entry *e = h->top();
	if (e == 0)
	    break ;
	h->print_all(0);
	fprintf(stderr, "++ got %ld.%06ld 0x%p\n",
		e->key.t.tv_sec, e->key.t.tv_usec, e->object);
	h->pop();
    }
}
#endif

