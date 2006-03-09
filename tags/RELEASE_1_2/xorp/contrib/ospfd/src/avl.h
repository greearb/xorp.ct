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

/*
 * Defines for AVL balanced trees
 * These are not entirely general, but instead always
 * have as keys two unsigned 32-bit integers
 */

/* Each element on the AVL tree is represented as
 * a AVLitem
 */

class AVLitem {
    uns32 _index1;	// Most significant half of index
    uns32 _index2;	// Least significant half
    AVLitem *right; 	// Right pointer
    AVLitem *left; 	// Left pointer
    int16 balance:3,	// AVL balance factor
	in_db:1;	// In database?
protected:
    int16 refct; 	// Reference count
public:
    AVLitem *sll; 	// Next in ordered list

    AVLitem(uns32, uns32);
    virtual ~AVLitem();

    inline int valid();
    inline void ref();
    inline void deref();
    inline void chkref();
    inline bool not_referenced();
    inline uns32 index1();
    inline uns32 index2();
    inline AVLitem *go_right();
    inline AVLitem *go_left();

    friend class AVLtree;
    friend class AVLsearch;
    friend void left_shift(AVLitem **);
    friend void dbl_left_shift(AVLitem **);
    friend void right_shift(AVLitem **);
    friend void dbl_right_shift(AVLitem **);
};

inline int AVLitem::valid()
{
    return(in_db);
}
inline void AVLitem::ref()
{
    refct++;
}
inline bool AVLitem::not_referenced()
{
    return(refct == 0);
}
inline uns32 AVLitem::index1()
{
    return(_index1);
}
inline uns32 AVLitem::index2()
{
    return(_index2);
}
inline AVLitem *AVLitem::go_right()
{
    return(right);
}
inline AVLitem *AVLitem::go_left()
{
    return(left);
}

/* Dereference an AVL entry, freeing it if both a) its reference
 * count is now zero and b) it is no longer in the database.
 */

inline void AVLitem::deref()

{
    if (refct >= 1)
	refct--;
    if (refct == 0 && !in_db)
	delete this;
}

/* Similar to above, except don't decrement the reference count.
 * Free the element if its reference count is zero and its no longer
 * in the database.
 */

inline void AVLitem::chkref()			

{
    if (refct == 0 && !in_db)
	delete this;
}

/* Each balanced AVL tree has a root element, and functions to
 * add and delete elements.
 */

class AVLtree {
    AVLitem *_root; 	// Root element
    uns32 count; 	// # elements on tree
    uns32 instance;	// Changes on add and deletes
public:
    AVLitem *sllhead;	// Order list head

    inline AVLtree();
    inline AVLitem *root();// Access AVL tree root
    inline int size();
    AVLitem *find(uns32 key1, uns32 key2=0) const;
    AVLitem *previous(uns32 key1, uns32 key2=0) const;
    void add(AVLitem *); // Add element to tree
    void remove(AVLitem *); // Remove element from tree
    void clear();

friend class AVLsearch;
};

inline AVLtree::AVLtree() : _root(0), count(0), instance(0), sllhead(0)
{
}
inline AVLitem *AVLtree::root()
{
    return(_root);
}
inline int AVLtree::size()
{
    return(count);
}

/* Iterator through an AVL tree, performing a depth first search.
 * Note that we could have cached a place in the tree (via a stack)
 * instead of just keeping indexes; that would have been more efficient,
 * but would force the iterator to lock out tree updates.
 */

class AVLsearch {
    AVLtree *tree;	 // Tree to search
    uns32 instance;	// Corresponding tree instance
    uns32 c_index1;	// current place
    uns32 c_index2;
    AVLitem *current;
public:
    inline AVLsearch(AVLtree *);
    void seek(uns32 key1, uns32 key2);
    inline void seek(AVLitem *item);
    AVLitem *next();
};

inline AVLsearch::AVLsearch(AVLtree *avltree)
: tree(avltree), c_index1(0), c_index2(0), current(0)

{    
    instance = tree->instance;
}
void AVLsearch::seek(AVLitem *item)
{
    seek(item->index1(), item->index2());
}
