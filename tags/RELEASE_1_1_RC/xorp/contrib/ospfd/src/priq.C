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

/* Routines implementing a priority queue. There are two standard
 * operations. priq_merge() merges two priority queues. Addition
 * of a single element is a special case of the merge (second priority
 * queue consisting of a single element). Removing the smallest queue
 * element (always the head) is also implementing by merging the left
 * and right children. The second operation is removing a queue element
 * (not necessarily the head) and is implemented by priq_delete().
 *
 * This priority queue leans to the left. Following right pointers
 * always gets you to NULL is O(log(n) operations. Each queue element
 * has the following items: left pointer, right pointer, distance to NULL,
 * and the item's cost. The properties of the queue are:
 *
 * 	COST(a) <= COST(LEFT(a)) and COST(a) <= COST(RIGHT(a))
 * 	DIST(LEFT(a)) >= DIST(RIGHT(a))
 * 	DIST(a) = 1 + DIST(RIGHT(a))
 *
 * It's this last clause that makes the tree lean to the left.
 * Priority queues are covered in Section 5.2.3 of Knuth Vol. III.
 */

#include "machdep.h"
#include "priq.h"

/* Add an element to a priority queue.
 */

void PriQ::priq_add(PriQElt *item)

{
    PriQ temp;

    item->left = 0;
    item->right = 0;
    item->parent = 0;
    item->dist = 1;
    temp.root = item;
    priq_merge(temp);
}

/* Merge two priority queues. Compare their heads. Assign the smallest
 * as the new head, and then merge its right subtree with the other
 * queue. This process then repeats. After reaching NULL (which, since
 * we're always dealing with right pointers, happens in O(log(n)). Go
 * back up the tree (which is why we keep parent pointers) adjusting
 * DIST(a) and flipping left and right when necessary.
 */

void PriQ::priq_merge(PriQ & otherq)

{
    PriQElt *parent;
    PriQElt *temp;
    PriQElt *x;
    PriQElt *y;

    if (!otherq.root) {
	if (root) root->parent = 0;
	return;
    }
    else if (!root) {
	root = otherq.root;
	if (root) root->parent = 0;
	return;
    }
    else {
	y = otherq.root;
	if (y->costs_less(root)) {
	    temp = root;
	    root = y;
	    y = temp;
	}

	// Make sure that root's parent is NULL
	root->parent = 0;

	// Merge right pointer with other queue
	parent = root;
	x = root->right;

	while (x) {
	    if (y->costs_less(x)) {
		temp = x;
		x = y;
		y = temp;
	    }
	    x->parent = parent;
	    parent->right = x;
	    parent = x;
	    x = x->right;
	}

	parent->right = y;
	y->parent = parent;
	priq_adjust(parent, false);
    }
}

/* Delete an item from the middle of the priority queue. Merge
 * the two branches leading from the deleted node, and then go back
 * up the tree, rebalancing when necessary.
 *
 * To enable going back up the tree, parent pointers have been added
 * to the priority queue items.
 */

void PriQ::priq_delete(PriQElt *item)

{
    PriQElt *parent;
    PriQ q1;
    PriQ q2;

    parent = item->parent;
    q1.root = item->right;
    q2.root = item->left;
    q1.priq_merge(q2);

    if (!parent)
	root = q1.root;
    else if (parent->right == item)
	parent->right = q1.root; 
    else
	parent->left = q1.root;
    if (q1.root)
	q1.root->parent = parent;

    priq_adjust(parent, true);
}

/* Go back up the queue, readjusting the DIST() and
 * left and right pointers as necessary.
 * Used by both the priority queue merge and delete routines.
 */

void PriQ::priq_adjust(PriQElt *balance_pt, int deleting)

{
    PriQElt *left;
    PriQElt *right;

    for (; balance_pt ; balance_pt = balance_pt->parent) {
	uns32 new_dist;

	left = balance_pt->left;
	right = balance_pt->right;
	
	if (!right)
	    new_dist = 1;
	else if (!left) {
	    balance_pt->left = right;
	    balance_pt->right = 0;
	    new_dist = 1;
	}
	else if (left->dist < right->dist) {
	    balance_pt->left = right;
	    balance_pt->right = left;
	    new_dist = left->dist + 1;
	}
	else {
	    new_dist = right->dist + 1;
	}

	if (new_dist != balance_pt->dist)
	    balance_pt->dist = new_dist;
	else if (deleting)
	    break;
    }
}

/* Take the top element off the priority queue.
 * Maintain the priority queue structure by merging
 * the left and right halves of the tree.
 */

PriQElt *PriQ::priq_rmhead()

{
    PriQElt *top;
    PriQ temp1;
    PriQ temp2;

    if (!(top = root))
	return(0);

    temp1.root = top->left;
    temp2.root = top->right;
    temp1.priq_merge(temp2);
    root = temp1.root;
    return(top);
}
