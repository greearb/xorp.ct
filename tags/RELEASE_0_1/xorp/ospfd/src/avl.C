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

/* Routine implementing AVL trees.
 * These are handled by the AVLitem and AVLtree
 * classes.
 */

#include "machdep.h"
#include "stack.h"
#include "avl.h"

/* Constructor for an AVLitem, Initialize the values
 * of the keys, and set the pointers to NULL and the balance
 * factor to 0.
 */

AVLitem::AVLitem(uns32 a, uns32 b) : _index1(a), _index2(b)

{
    right = 0;
    left = 0;
    balance = 0;
    in_db = 0;
    refct = 0;
}

/* Destrustor does nothing, but defined here so that inherited
 * classes can have their destructors called when AVL item
 * is deleted.
 */

AVLitem::~AVLitem()

{
}

/* Find an item in the AVL tree, given its two keys (indexes).
 */

AVLitem *AVLtree::find(uns32 key1, uns32 key2) const

{
    AVLitem *ptr;

    for (ptr = _root; ptr; ) {
	if (key1 > ptr->_index1)
	    ptr = ptr->right;
	else if (key1 < ptr->_index1)
	    ptr = ptr->left;
	else if (key2 > ptr->_index2)
	    ptr = ptr->right;
	else if (key2 < ptr->_index2)
	    ptr = ptr->left;
	else
	    break;
    }

    return(ptr);
}


/* Find the item in the AVL tree that immediately precedes the
 * given keys (indexes).
 */

AVLitem *AVLtree::previous(uns32 key1, uns32 key2) const

{
    AVLitem *ptr;
    AVLitem *last_right;

    if (key1 == 0 && key2 == 0)
	return(0);
    else if (key2 != 0)
	key2--;
    else {
	key2 = 0xffffffffL;
	key1--;
    }

    last_right = 0;
    
    for (ptr = _root; ptr; ) {
	if ((key1 > ptr->_index1) ||
	    (key1 == ptr->_index1 && key2 > ptr->_index2)) {
	    last_right = ptr;
	    ptr = ptr->right;
	}
	else if ((key1 < ptr->_index1) ||
		 (key2 < ptr->_index2))
	    ptr = ptr->left;
	else
	    return(ptr);
    }

    return(last_right);
}


/* Clear the whole AVL tree, by walking the ordered link
 * list.
 */

void AVLtree::clear()

{
    AVLitem *ptr;
    AVLitem *next;

    for (ptr = sllhead; ptr; ptr = next) {
	next = ptr->sll;
	ptr->in_db = 0;
	ptr->chkref();
    }

    _root = 0;
    sllhead = 0;
    count = 0;
    instance++;
}

/* Add item to balanced tree. Search for place in tree, remembering
 * the balance point (the last place where the tree balance was
 * non-zero). Insert the item, and then readjust the balance factors
 * starting at the balance point. If the balance at the balance point is
 * now +2 or -2, must shift to get the tree back in balance. There
 * are four cases: simple left shift, double left shift, and the right
 * shift counterparts.
 */

void AVLtree::add(AVLitem *item)

{
    AVLitem **parent_ptr;
    AVLitem **balance_ptr;
    AVLitem *ptr;
    uns32 index1;
    uns32 index2;
    AVLitem *sllprev;

    item->in_db = 1;
    index1 = item->_index1;
    index2 = item->_index2;
    balance_ptr = &_root;
    instance++;

    for (parent_ptr = &_root; (ptr = *parent_ptr); ) {
	// Remember balance point's parent
	if (ptr->balance != 0)
	    balance_ptr = parent_ptr;
	// Search for insertion point
	if (index1 > ptr->_index1)
	    parent_ptr = &ptr->right;
	else if (index1 < ptr->_index1)
	    parent_ptr = &ptr->left;
	else if (index2 > ptr->_index2)
	    parent_ptr = &ptr->right;
	else if (index2 < ptr->_index2)
	    parent_ptr = &ptr->left;
	else {
	    // Replace current entry
	    *parent_ptr = item;
	    item->right = ptr->right;
	    item->left = ptr->left;
	    item->balance = ptr->balance;
	    // Update ordered singly linked list
	    item->sll = ptr->sll;
	    if (!(sllprev = previous(index1, index2)))
		sllhead = item;
	    else
		sllprev->sll = item;
	    ptr->in_db = 0;
	    ptr->chkref();
	    return;
	}
    }

    // Insert into tree
    *parent_ptr = item;
    count++;

    // Readjust balance, from balance point on down
    for (ptr = *balance_ptr; ptr != item; ) {
	if (index1 > ptr->_index1) {
	    ptr->balance += 1;
	    ptr = ptr->right;
	}
	else if (index1 < ptr->_index1) {
	    ptr->balance -= 1;
	    ptr = ptr->left;
	}
	else if (index2 > ptr->_index2) {
	    ptr->balance += 1;
	    ptr = ptr->right;
	}
	else if (index2 < ptr->_index2) {
	    ptr->balance -= 1;
	    ptr = ptr->left;
	}
    }

    // If necessary, shift to return balance
    ptr = *balance_ptr;
    if (ptr->balance == 2) {
	if (ptr->right->balance == -1)
	    dbl_left_shift(balance_ptr);
	else
	    left_shift(balance_ptr);
    }
    else if (ptr->balance == -2) {
	if (ptr->left->balance == 1)
	    dbl_right_shift(balance_ptr);
	else
	    right_shift(balance_ptr);
    }

    // Update ordered singly linked list
    if (!(sllprev = previous(index1, index2))) {
	item->sll = sllhead;
	sllhead = item;
    }
    else {
	item->sll = sllprev->sll;
	sllprev->sll = item;
    }
}


/* Remove item from balanced tree. First, reduce to the case where the
 * item being removed has a NULL right or left pointer. If this is not the
 * case, simply find the item that is jus prvious (in sorted order); this is
 * guaranteed to have a NULL right pointer. Then swap the entries before
 * deleting. After deleting, go up the stack adjusting the balance
 * when necessary.
 *
 * We can stop whenevr the balance factor ceases to change.
 */

void AVLtree::remove(AVLitem *item)

{
    AVLitem **parent_ptr;
    AVLitem *ptr;
    uns32 index1;
    uns32 index2;
    Stack stack;
    AVLitem **item_parent;
    AVLitem **prev;
    AVLitem *sllprev;

    index1 = item->_index1;
    index2 = item->_index2;

    for (parent_ptr = &_root; (ptr = *parent_ptr); ) {
	// Have we found element to be deleted?
	if (ptr == item)
	    break;
	// Add to stack for later balancing
	stack.push((void *) parent_ptr);
	// Search for item
	if (index1 > ptr->_index1)
	    parent_ptr = &ptr->right;
	else if (index1 < ptr->_index1)
	    parent_ptr = &ptr->left;
	else if (index2 > ptr->_index2)
	    parent_ptr = &ptr->right;
	else if (index2 < ptr->_index2)
	    parent_ptr = &ptr->left;
    }

    // Deletion failed
    if (ptr != item)
	return;

    instance++;
    count--;
    // Update ordered singly linked list
    if (!(sllprev = previous(index1, index2)))
	sllhead = item->sll;
    else
	sllprev->sll = item->sll;

    // Remove item from btree
    if (!item->right)
	*parent_ptr = item->left;
    else if (!item->left)
	*parent_ptr = item->right;
    // If necessary, swap with previous to get NULL pointer
    else {
	item_parent = parent_ptr;
	stack.push((void *) parent_ptr);
	parent_ptr = &item->left;
	stack.mark();
	for (ptr = *parent_ptr; ptr->right; ptr = *parent_ptr) {
	    stack.push((void *) parent_ptr);
	    parent_ptr = &ptr->right;
	}
	// Remove item from btree
	*parent_ptr = ptr->left;
	// Swap item and previous
	*item_parent = ptr;
	ptr->right = item->right;
	ptr->left = item->left;
	ptr->balance = item->balance;
	// Replace stack element that was item
	if (parent_ptr == &item->left)
	    parent_ptr = &ptr->left;
	else
	    stack.replace_mark((void *) &ptr->left);
    }

    // Go back up stack, adjusting balance where necessary;
    for (; (prev = (AVLitem **) stack.pop()) != 0; parent_ptr = prev) {
	AVLitem *parent;
	parent = *prev;
	if (parent_ptr == &parent->left)
	    parent->balance++;
	else if (parent_ptr == &parent->right)
	    parent->balance--;
	// tree has shrunken if balance now zero
	if (parent->balance == 0)
	    continue;
	// If out-of-balance, shift
	// Continue only if tree has then shrunken
	else if (parent->balance == 2) {
	    if (parent->right->balance == -1)
		dbl_left_shift(prev);
	    else if (parent->right->balance == 1)
		left_shift(prev);
	    else {
		left_shift(prev);
		break;
	    }
	}
	else if (parent->balance == -2) {
	    if (parent->left->balance == 1)
		dbl_right_shift(prev);
	    else if (parent->left->balance == -1)
		right_shift(prev);
	    else {
		right_shift(prev);
		break;
	    }
	}
	else
	    break;
    }

    item->in_db = 0;
}


/* Part of the tree balancing process. Single left shift restores balance
 * when the parent has balance of +2, and its right child has balance
 * of +1 or 0. If right child has balance of -1, a double left shift
 * is necessary.
 */

void left_shift(AVLitem **balance_ptr)

{
    AVLitem *node_b;
    AVLitem *node_c;

    node_b = *balance_ptr;
    node_c = node_b->right;
    *balance_ptr = node_c;
    node_b->right = node_c->left;
    node_c->left = node_b;

    if (node_c->balance == 1) {
	node_c->balance = 0;
	node_b->balance = 0;
    }
    else {
	node_c->balance = -1;
	node_b->balance = 1;
    }
}


/* Double left shift. Necessary when the parent balance is +2, and the
 * right child's balance is -1. Returns depth change of tree.
 */

void dbl_left_shift(AVLitem **balance_ptr)

{
    AVLitem *node_b;
    AVLitem *node_d;
    AVLitem *node_c;

    node_b = *balance_ptr;
    node_d = node_b->right;
    node_c = node_d->left;
    *balance_ptr = node_c;
    node_b->right = node_c->left;
    node_d->left = node_c->right;
    node_c->right = node_d;
    node_c->left = node_b;

    if (node_c->balance == -1) {
	node_b->balance = 0;
	node_c->balance = 0;
	node_d->balance = 1;
    }
    else if (node_c->balance == 1) {
	node_b->balance = -1;
	node_c->balance = 0;
	node_d->balance = 0;
    }
    else {
	node_b->balance = 0;
	node_c->balance = 0;
	node_d->balance = 0;
    }
}	


/* Part of the tree balancing process. Single right shift is the mirror
 * image of the single left shift above, and is invoked when the parent
 * has balance of -2, and its left child a balance of -1 or 0. If the
 * left child's balance is 1, a double right shift is necessary.
 */

void right_shift(AVLitem **balance_ptr)

{
    AVLitem *node_c;
    AVLitem *node_b;

    node_c = *balance_ptr;
    node_b = node_c->left;
    *balance_ptr = node_b;
    node_c->left = node_b->right;
    node_b->right = node_c;

    if (node_b->balance == -1) {
	node_b->balance = 0;
	node_c->balance = 0;
    }
    else {
	node_b->balance = 1;
	node_c->balance = -1;
    }
}

/* Double right shift. Mirror image of the double left shift above.
 */

void dbl_right_shift(AVLitem **balance_ptr)

{
    AVLitem *node_d;
    AVLitem *node_b;
    AVLitem *node_c;

    node_d = *balance_ptr;
    node_b = node_d->left;
    node_c = node_b->right;
    *balance_ptr = node_c;
    node_d->left = node_c->right;
    node_b->right = node_c->left;
    node_c->left = node_b;
    node_c->right = node_d;

    if (node_c->balance == 1) {
	node_d->balance = 0;
	node_c->balance = 0;
	node_b->balance = -1;
    }
    else if (node_c->balance == -1) {
	node_d->balance = 1;
	node_c->balance = 0;
	node_b->balance = 0;
    }
    else {
	node_d->balance = 0;
	node_c->balance = 0;
	node_b->balance = 0;
    }
}	

/* Establish point at which AVL search will begin. First item returned
 * by next will have keys immediately following those specified to this
 * routine.
 */

void AVLsearch::seek(uns32 key1, uns32 key2)

{
    c_index1 = key1;
    c_index2 = key2;

    // Initially set up for next to fail
    current = 0;

    // If at end, next should fail
    if (!tree->_root || (++key2 == 0 && ++key1 == 0))
	return;
    
    // Now set current equal to previous item to key1, key2
    if ((current = tree->previous(key1, key2))) {
	c_index1 = current->_index1;
	c_index2 = current->_index2;
    }
}

/* Iterator for an AVL tree, using the ordered singly linked
 * list. A return of NULL indicates that
 * the entire tree has been searched.
 */

AVLitem *AVLsearch::next()

{
    if (!tree->_root)
	return(0);
    if (instance != tree->instance) {
        if (current)
	    seek(c_index1, c_index2);
	// We're now synced up with tree
	instance = tree->instance;
    }

    if (!current)
	current = tree->sllhead;
    else if (!current->sll)
	return(0);
    else
	current = current->sll;

    c_index1 = current->_index1;
    c_index2 = current->_index2;
    return(current);
}
