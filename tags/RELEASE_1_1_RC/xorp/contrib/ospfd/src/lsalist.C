/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Routines implementing the LSA list classes
 */

#include <new>
#include "ospfinc.h"

LsaListElement *LsaListElement::freelist;// Free list of elements
int LsaListElement::n_allocated; // # list elements allocated
int LsaListElement::n_free; 	// # list elements free

/* New operator for list elements.
 * Allocate "Blksize" list elements at a time, keeping the
 * unused ones on a linked list.
 * This saves calls to the memory allocator for each list element,
 * and a certain amount of overhead.
 */

void *LsaListElement::operator new(size_t)

{
    LsaListElement *ep;
    int	i;

    if ((ep = freelist)) {
	freelist = freelist->next;
	n_free--;
	return(ep);
    }

    ep = (LsaListElement *) ::new char[BlkSize * sizeof(LsaListElement)];

    if (!ep) 
	throw std::bad_alloc();
    
    n_free = BlkSize - 1;
    n_allocated += BlkSize;
    for (freelist = ep, i = 0; i < BlkSize - 2; i++, ep++)
	ep->next = ep + 1;
    ep->next = 0;
    return(ep + 1);
}

/* Delete operator for list elements.
 * Return to linked list of free entries
 * DeREFERENCING the associated LSA is done in the destructor.
 */

void LsaListElement::operator delete(void *ptr, size_t)

{
    LsaListElement *ep;

    ep = (LsaListElement *) ptr;
    ep->next = freelist;
    freelist = ep;
    n_free++;
}

/* Clear an LSA list, removing all its elements and returning
 * them to the free list.
 */

void LsaList::clear()

{
    LsaListElement *ep;
    LsaListElement *next;

    for (ep = head; ep; ep = next) {
	next = ep->next;
	delete ep;
    }

    head = 0;
    tail = 0;
    size = 0;
}

/* Move the contents of the second list onto the end
 * of the first.
 */

void LsaList::append(LsaList *olst)

{
    // Second list empty?
    if (!olst->head)
	return;
    // First list empty?
    else if (!head)
	head = olst->head;
    else
	tail->next = olst->head;

    tail = olst->tail;
    size += olst->size;
    olst->head = 0;
    olst->tail = 0;
    olst->size = 0;
}

/* Remove all invalid LSAs from a given list.
 * return number of entries garbage collected
 */

int LsaList::garbage_collect()

{
    LsaListElement *ep;
    LsaListElement *prev;
    LsaListElement *next;
    int oldsize;

    oldsize = size;

    for (prev = 0, ep = head; ep; ep = next) {
	next = ep->next;
        if (ep->lsap->valid()) {
	    prev = ep;
	    continue;
	}
	// Delete current element
	// Fix forward pointer
	if (!prev)
	    head = next;
	else
	    prev->next = next;
	// Fix tail
	if (tail == ep)
	    tail = prev;
	delete ep;
	size--;
    }

    return(oldsize - size);
}

/* Remove a given LSA from an LSA list. Here the LSA is specified
 * by a pointer. Returns whether the LSA has been found.
 */

int LsaList::remove(LSA *lsap)

{
    LsaListElement *ep;
    LsaListElement *prev;
    LsaListElement *next;

    for (prev = 0, ep = head; ep; ep = next) {
	next = ep->next;
	if (ep->lsap != lsap) {
	    prev = ep;
	    continue;
	}
	// Delete current element
	// Fix forward pointer
	if (!prev)
	    head = next;
	else
	    prev->next = next;
	// Fix tail
	if (tail == ep)
	    tail = prev;
	delete ep;
	size--;
	return(true);
    }

    return(false);
}
