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

/* Routines implemneting the generic Patricia tree.
 */

#include "machdep.h"
#include "pat.h"

/* Initialize the Patricia tree. Install a dummy entry
 * with a NULL key, testing the largest bit and with pointers
 * to itself.
 */

PatTree::PatTree()

{
    init();
}

/* Initialization performed in a function other
 * than the constructor, so that the object can be 
 * reconstructed later.
 */

void PatTree::init()

{
    root = new PatEntry;
    root->zeroptr = root;
    root->oneptr = root;
    root->chkbit = MaxBit;
    root->key = 0;
    root->keylen = 0;
    size = 0;
}

/* Find an element within the Patricia tree, based on its key
 * string.
 */

PatEntry *PatTree::find(const byte *key, int keylen)

{
    PatEntry *entry;
    uns32 chkbit;

    entry = root;
    do {
	chkbit = entry->chkbit;
	if (bit_check(key, keylen, chkbit))
	    entry = entry->oneptr;
	else
	    entry = entry->zeroptr;
    } while (entry->chkbit > chkbit);

    if (keylen == entry->keylen &&
	memcmp(key, entry->key, keylen) == 0)
	return(entry);

    return(0);
}

/* Find a particular character string. We assume that it is
 * NULL terminated, and so just find the length and then
 * call the above routine.
 */

PatEntry *PatTree::find(const char *key)

{
    int keylen;

    keylen = strlen(key);
    return(find((const byte *) key, keylen));
}

/* Add an element to a Patricia tree. We assume that the caller
 * has verified that the key is not already in the tree.
 * Find the next bit to test,
 * and then insert the node in the proper place.
 */

void PatTree::add(PatEntry *entry)

{
    PatEntry *ptr;
    uns32 chkbit;
    uns32 newbit;
    PatEntry **parent;

    ptr = root;
    do {
	chkbit = ptr->chkbit;
	if (entry->bit_check(chkbit))
	    ptr = ptr->oneptr;
	else
	    ptr = ptr->zeroptr;
    } while (ptr->chkbit > chkbit);

    // Find new bit to check
    for (newbit = 0; ; newbit++) {
	if (ptr->bit_check(newbit) != entry->bit_check(newbit))
	    break;
    }

    // Find place to insert new element
    parent = &root;
    for (ptr = *parent; ptr->chkbit < newbit;) {
	chkbit = ptr->chkbit;
	if (entry->bit_check(chkbit))
	    parent = &ptr->oneptr;
	else
	    parent = &ptr->zeroptr;

	ptr = *parent;
	if (ptr->chkbit <= chkbit)
	    break;
    }

    // Insert into Patricia tree
    *parent = entry;
    // Set Patricia fields
    entry->chkbit = newbit;
    if (entry->bit_check(newbit)) {
	entry->oneptr = entry;
	entry->zeroptr = ptr;
    }
    else {
	entry->zeroptr = entry;
	entry->oneptr = ptr;
    }
    size++;
}

/* Delete an element from the Patricia tree. After locating the
 * element, handle the special cases:
 * 1) does the element point to itself?
 * 2) is the element the root?
 */

void PatTree::remove(PatEntry *entry)

{
    PatEntry *ptr;
    uns32 chkbit;
    PatEntry *upptr;
    PatEntry **parent;
    PatEntry **upparent;
    bool upzero;
    PatEntry **fillptr;

    ptr = root;
    upptr = 0;
    parent = &root;
    upparent = &root;
    fillptr = 0;
    do {
        if (ptr == entry && !fillptr)
	    fillptr = parent;
	upparent = parent;
	upptr = ptr;
	chkbit = ptr->chkbit;
	if (entry->bit_check(chkbit))
	    parent = &ptr->oneptr;
	else
	    parent = &ptr->zeroptr;
	ptr = *parent;
    } while (ptr->chkbit > chkbit);

    // Entry not found?
    if (ptr != entry)
	return;

    upzero = (upptr->zeroptr == entry);
    // Unlink upptr from downward tree
    *upparent = (upzero ? upptr->oneptr : upptr->zeroptr);
    // Entry points to self?
    // If not, switch entry and upptr
    // Upward pointer to upptr remains unchanged
    if (upptr != entry) {
        *fillptr = upptr;
	upptr->chkbit = entry->chkbit;
	upptr->oneptr = entry->oneptr;
	upptr->zeroptr = entry->zeroptr;
    }

    size--;
}

/* Clear the entire Patricia tree. This is a recursive
 * operation, which deletes all the nodes.
 */

void PatTree::clear()

{
    clear_subtree(root);
    init();
}

/* Clear the subtree rooted at the given entry.
 * Works recursively.
 */

void PatTree::clear_subtree(PatEntry *entry)

{
    if (!entry)
        return;
    if (entry->zeroptr && entry->zeroptr->chkbit > entry->chkbit)
        clear_subtree(entry->zeroptr);
    if (entry->oneptr && entry->oneptr->chkbit > entry->chkbit)
        clear_subtree(entry->oneptr);
    delete entry;
}
