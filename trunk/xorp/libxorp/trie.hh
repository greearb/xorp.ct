// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2002 International Computer Science Institute
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

// $XORP: xorp/libxorp/trie.hh,v 1.4 2003/01/25 01:11:22 mjh Exp $

#ifndef __LIBXORP_TRIE_HH__
#define __LIBXORP_TRIE_HH__

#include "ipnet.hh"

// Macros 
//#define VALIDATE_XORP_TRIE
//#define DEBUG_LOGGING

#include "debug.h"
#include "minitraits.hh"

/*
 * This module implements a trie to support route lookups.
 *
 * The template should be invoked with two classes, the basetype "A"
 * for the search Key (which is a subnet, IPNet<A>), and the Payload.
 */


/**
 * @short TrieNode definition
 *
 * TrieNode's are the elements of a Trie.
 * Each node is associated to a Key and possibly a Payload.
 * Nodes with a Payload ("full") can have 0, 1 or 2 children.
 * Nodes without a Payload ("empty") can only be internal nodes,
 * and MUST have 2 children (or they have no reason to exist).
 *
 * Children have a Key which is strictly contained in their
 * parent's Key -- more precisely, they are in either the left
 * or the right half of the parent's Key. The branch to which
 * a child is attached (left or right) is defined accordingly.
 */

template <class A, class Payload> 
class TrieNode {
public:
    typedef IPNet<A> Key;
    typedef typename MiniTraits<Payload>::NonConst PPayload;

    /**
     * Constructors
     */
    TrieNode() : _up(0), _left(0), _right(0), _k(Key()), _p(0) {}
    TrieNode(const Key& key, const Payload& p, TrieNode* up = 0) :
	_up(up), _left(0), _right(0), _k(key), _p(new PPayload(p)) {}

    TrieNode(const Key& key, TrieNode* up = 0) :
	_up(up), _left(0), _right(0), _k(key), _p(0) {}

    ~TrieNode() 				{ delete _p; }

    /**
     * add a node to a subtree
     * @return a pointer to the node.
     */
    static TrieNode *insert(TrieNode **root,
			    const Key& key,
			    const Payload& p,
			    bool& replaced);

    /**
     * erase current node, replumb. Returns the new root.
     */
    TrieNode *erase();

    /**
     * main search routine. Given a key, returns a node.
     */
    TrieNode *find(const Key& key) ;
    const TrieNode *const_find(const Key& key) const {
	return const_cast<TrieNode*>(this)->find(key);
    }

    /**
     * aux search routine.
     * Given a key, returns a subtree contained in the key, irrespective
     * of the presence of a payload in the node.
     */
    TrieNode *find_subtree(const Key &key);

    /**
     * Given a key, find the node with that key and a payload.
     * If the next doesn't exist or does not have a payload, find
     * the next node in the iterator sequence. XXX check the description.
     */
    TrieNode* lower_bound(const Key &key);

    bool has_payload() const			{ return _p != NULL;	}
    const Payload &const_p() const		{ return *_p;		}
    Payload &p()    			        { return *_p;		}

    void set_payload(const Payload& p) {
	delete _p;
	_p = new PPayload(p);
    }

    const Key &k() const			{ return _k;		}

    void print(int indent, const char *msg) const;
    string str() const;

    /**
     * helper function to delete an entire subtree (including the root).
     */
    void delete_subtree()			{
	if (_left)
	    _left->delete_subtree();
	if (_right)
	    _right->delete_subtree();
	delete this;	/* and we are gone too */
    }

    /**
     * debugging, validates a node by checking pointers and Key invariants.
     */
    void validate(const TrieNode *parent) const	{
	UNUSED(parent);
#ifdef VALIDATE_XORP_TRIE
	if (_up != parent) {
	    fprintf(stderr, "bad parent _up %x vs %x",
		    (int)_up, (int)parent);
	    abort();
	}
	if (_up && _k.contains(_up->_k)) {
	    fprintf(stderr, "bad subnet order");
	    abort();
	}
	if (_p == NULL && (!_left || !_right)) {
	    fprintf(stderr, "useless internal node");
	    abort();
	}
	if (_left)
	    _left->validate(this);
	if (_right)
	    _right->validate(this);
#endif
    }

    /**
     * helper methods for iterators.
     * Visit order: start from the leaves and then go up.
     * begin() moves to the first node we want to explore, which is
     *	the leftmost node in the subtree.
     * next() finds the 'next' node in the visit order.
     * Invariant for next(): being on _cur means that we have visited its
     * subtrees (and the node itself, of course, which is the current one).
     */

    bool is_left() const		{ return _up && this == _up->_left; }

    TrieNode *leftmost() {
	TrieNode *n = this;
	while (n->_left || n->_right)
	    n = (n->_left ? n->_left : n->_right);
	return n;
    }

    TrieNode *begin() { 
	return leftmost();
    }

    TrieNode *next(const Key &root) {
	TrieNode *n = this;

	n->dump("next");
	do {
	    bool was_left_child = n->is_left();
	    n = n->_up;
	    if (n == NULL)			// cannot backtrack,
		return NULL;		// we are done.
	    // backtrack one level, then explore the leftmost path
	    // on the right branch if not done already.
	    if (was_left_child && n->_right)
		n = n->_right->leftmost();
	    if (!root.contains(n->_k))	// backtrack past root ?
		return NULL;
	} while (n->has_payload() == false);	// found a good node.
	return n;
    }

    /**
     * @return the boundaries ("lo" and "hi") of the largest range that
     * contains 'a' and maps to the same route entry.
     *
     * Algorithm:
     * <PRE>
     *		n = find(a);
     * 		if we have no route (hence no default), provide a fake 0/0;
     *		set lo and hi to the boundaries of the current node.
     *
     * if n.is_a_leaf() we are done (results are the extremes of the entry)
     * Otherwise: we are in an intermediate node, and a can be in positions
     * 1..5 if the node has 2 children, or 1'..3' if it has 1 child.
     *
     *	n:		|---------------.----------------|
     *  a:                1    2        3      4     5
     *                       |--X--|         |--Y--|
     *
     *  a:                1'    2'        3'
     *                       |--X--|
     *
     * Behaviour is the following:
     *  case 1 and 1':	lo already set, hi = (lowest address in X)-1
     *  case 2 and 2': set n = X and repeat
     *  case 3: lo = (highest addr in X)+1, hi = (lowest addr in Y)-1
     *  case 3': lo = (highest addr in X)+1, hi is already set
     *  case 4: set n = Y and repeat
     *  case 5:	lo = (highest addr in Y)+1, hi is already set
     * </PRE>
     */
    void find_bounds(const A& a, A &lo, A &hi) const	{
	TrieNode def = TrieNode();
	const TrieNode *n = const_find(Key(a, a.addr_bitlen()));

	if (n == NULL) {	// create a fake default entry
	    def._left = const_cast<TrieNode *>(this);
	    def._right = NULL;
	    n = &def;
	}
	lo = n->_k.masked_addr();
	hi = n->_k.top_addr();
	for (const TrieNode *prev = NULL; prev != n;) {
	    prev = n;
	    TrieNode *x = (n->_left ? n->_left : n->_right);
	    if (x == NULL)
		break;
	    if (a < x->_k.masked_addr()) {		// case 1 and 1'
		hi = x->low(); --hi;
	    } else if (a <= x->_k.top_addr()) {		// case 2 and 2'
		n = x; // and continue
	    } else if (n->_left == NULL || n->_right == NULL) { // case 3'
		lo = x->high(); ++lo;
	    } else if (a < n->_right->_k.masked_addr()) {	// case 3
		lo = x->high(); ++lo;
		hi = n->_right->low(); --hi;
	    } else if (a <= n->_right->_k.top_addr()) {	// case 4:
		n = n->_right; // and continue
	    } else {					// case 5:
		lo = n->_right->high(); ++lo;
	    }
	}
    }

    /**
     * @return the lowest address in a subtree which has a route.
     * Search starting from left or right until a full node is found.
     */
    A low() const 					{
	const TrieNode *n = this;
	while (!(n->has_payload()) && (n->_left || n->_right))
	    n = (n->_left ? n->_left : n->_right);
	return n->_k.masked_addr();
    }

    /**
     * @return the highest address in a subtree which has a route.
     * Search starting from right or left until a full node is found.
     */
    A high() const		 			{
	const TrieNode *n = this;
	while (!(n->has_payload()) && (n->_right || n->_left))
	    n = (n->_right ? n->_right : n->_left);
	return n->_k.top_addr();
    }

private:

    void dump(const char *msg) const			{
#if 1
	debug_msg(" %s %s %s\n",
		  msg,
		  _k.str().c_str(), _p ? "PL" : "[]");
	debug_msg("  U   %s\n",
		  _up ? _up->_k.str().c_str() : "NULL");
	debug_msg("  L   %s\n",
		  _left ? _left->_k.str().c_str() : "NULL");
	debug_msg("  R   %s\n",
		  _right ? _right->_k.str().c_str() : "NULL");
#endif
    }

    TrieNode	*_up, *_left, *_right;
    Key		_k;
    PPayload 	*_p;
};

/**
 * Iterator on a trie.
 *
 * _cur points to the current object, _root contains the search key for
 * root of the subtree we want to scan. The iterator skips over empty
 * nodes, and visits the subtree in depth-first, left-to-right order.
 * This does not guarantees that keys returned are sorted by prefix length.
 */
template <class A, class Payload>
class TrieIterator {
public:
    typedef IPNet<A> Key;
    typedef TrieNode<A, Payload> Node;

    /**
     * Constructors
     */
    TrieIterator()				{}

    /**
     * constructor for exact searches: both the current node and the search
     * key are taken from n, so the iterator will only loop once.
     */
    TrieIterator(Node *n)			{
	_cur = n;
	if (n)
	    _root = n->k();
    }

    /**
     * construct for subtree scanning: the root key is set explicitly,
     * and the current node is set according to the search order.
     */
    TrieIterator(Node *n, const Key &k)		{
	_root = k;
	_cur = n ? n->begin() : NULL;
    }

    /**
     * Postfix increment
     *
     * Updates position of iterator in tree.
     * @return position of iterator before increment.
     */
    TrieIterator operator ++(int)		{ // postfix
	TrieIterator x = *this;
	_cur = _cur->next(_root);
	return x;
    }

    /**
     * Prefix increment
     *
     * Updates position of iterator in tree.
     * @return position of iterator after increment.
     */
    TrieIterator& operator ++()		{ // prefix
	_cur = _cur->next(_root);
	return *this;
    }

    Node *cur() const			{ return _cur;		};

    bool operator==(const TrieIterator & x) const { return (_cur == x._cur); }

    Payload & payload()  		          { return _cur->p(); };
    const Key & key() const 		          { return _cur->k(); };

private:
    Node	*_cur;
    Key		_root;
};

/**
 * The Trie itself
 *
 * The trie support insertion and deletion of Key,Payload pairs,
 * and lookup by Key (which can be an address or a subnet).
 *
 * Additional methods are supported to provide access via iterators.
 */

template <class A, class Payload>
class Trie {
public:
    typedef IPNet<A> Key;
    typedef TrieIterator<A,Payload> iterator;
    typedef TrieNode<A,Payload> Node;

    /**
     * stl map interface
     */
    Trie() : _root(0), _payload_count(0)	{}

    ~Trie()					{ delete_all_nodes(); }

    /**
     * insert a key,payload pair, returns an iterator
     * to the newly inserted node.
     * Prints a warning message if the new entry overwrites an
     * existing full node.
     */
    iterator insert(const Key & net, const Payload& p) {
	bool replaced = false;
	Node *out = Node::insert(&_root, net, p, replaced);
	if (replaced) {
	    fprintf(stderr, "overwriting a full node"); //XXX
	} else {
	    _payload_count++;
	}
	return iterator(out);
    }

    /**
     * delete the node with the given key.
     */
 
    void erase(const Key &k)			{ erase(find(k)); }

    /**
     * delete the node pointed by the iterator.
     */
    void erase(iterator i)			{
	if (_root && i.cur() && i.cur()->has_payload()) {
	    _payload_count--;
	    _root = const_cast<Node *>(i.cur())->erase();
	    // XXX should invalidate i ?
	}
    }

    /**
     * given a key, returns an iterator to the entry with the
     * longest matching prefix.
     */
    iterator find(const Key &k) const		{
	return iterator(_root->find(k));
    }

    /**
     * given an address, returns an iterator to the entry with the
     * longest matching prefix.
     */
    iterator find(const A& a) const		{
	return find(Key(a, a.addr_bitlen()));
    }

    iterator lower_bound(const Key &k) const {
#ifdef NOTDEF
	iterator i = lookup_node(k);
	if (i != end())
	    return i;
#endif
	return iterator(_root->lower_bound(k));
    }

    iterator begin() const		{ return iterator(_root, IPNet<A>()); }
    const iterator end() const			{ return iterator(0); }

    void delete_all_nodes()			{
	if (_root)
	    _root->delete_subtree();
	_root = NULL;
	_payload_count = 0;
    }

    /**
     * lookup a subnet, must return exact match if found, end() if not.
     *
     */
    iterator lookup_node(const Key & k) const	{
	Node *n = _root->find(k);
	return (n && n->k() == k) ? iterator(n) : end();
    }

    /**
     * returns an iterator to the subtree rooted at or below
     * the key passed as parameter.
     */
    iterator search_subtree(const Key &key) const {
	return iterator(_root->find_subtree(key), key);
    }

    /**
     * find_less_specific asks the question: if I were to add this
     * net to the trie, what would be its parent node?
     * net may or may not already be in the trie.
     * Implemented as a find() with a less specific key.
     */
    iterator find_less_specific(const Key &key)	const {
	Key x(key.masked_addr(), key.prefix_len() - 1);

	return iterator(_root->find(x));
    }

    /**
     * return the lower and higher address in the range that contains a
     * and would map to the same route.
     */
    void find_bounds(const A& a, A &lo, A &hi) const	{
	_root->find_bounds(a, lo, hi);
    }
#if 0	// compatibility stuff, has to go
    /*
     * return the lower and higher address in the range that contains a
     * and would map to the same route.
     */
    A find_lower_bound(const A a) const		{
	A lo, hi;
	_root->find_bounds(a, lo, hi);
	return lo;
    }
    A find_higher_bound(const A a) const		{
	A lo, hi;
	_root->find_bounds(a, lo, hi);
	return hi;
    }

#endif // compatibility
    int route_count() const			{ return _payload_count; }

    void print() const;

private:
    void validate()				{
	if (_root)
	    _root->validate(NULL);
    }

    Node	*_root;
    int		_payload_count;
};


/**
 * add subnet/payload to the tree at *root.
 *
 * @returns a pointer to the newly inserted node.
 */
template <class A, class Payload> 
TrieNode<A, Payload> *
TrieNode<A, Payload>::insert(TrieNode **root,
			     const Key& x,
			     const Payload& p,
			     bool& replaced)
{
    /*
     * Loop until done in the following:
     *
     * If *root == NULL, create a new TrieNode containing x and we are DONE.
     * Otherwise consider the possible cases of overlaps between the subnets
     * in *root (call it y) and x (+ indicates the middle of the interval):
     *
     *   y = (*root)          .|===+===|
     *
     *   x	0	      .|---+---|
     *   x	A       |--|  .    .
     *   x	B             .    .     |--|
     *   x	C             . |-|.
     *   x	D             .    .|-|
     *   x	E  |----------+----------|
     *   x	F             |----------+-----------|
     *
     * case 0:	Same subnet. Store payload if *root if empty, replace otherwise.
     * case A:	allocate a new empty root, make old *root the right child,
     *		make a new node with x the left child. DONE.
     * case B:	allocate a new empty root, make old *root the left child,
     *		make a new node with x the right child. DONE.
     * case C:	repeat with root = &((*root)->left)
     * case D:	repeat with root = &((*root)->right)
     * case E:	*root = new node with x, old *root the right child, DONE.
     * case F:	*root = new node with x, old *root the left child, DONE.
     *
     * In all case, when we exit the loop, newroot contains the new value to
     * be assigned to *root;
     */

    TrieNode **oldroot = root;	// do we need it ?
    TrieNode *newroot = NULL, *parent = NULL, *me = NULL;

    debug_msg("++ insert %s\n", x.str().c_str());
    for (;;) {
	newroot = *root;
	if (newroot == NULL) {
	    me = newroot = new TrieNode(x, p, parent);
	    break;
	}
	parent = newroot->_up;
	Key y = newroot->_k;
	if (x == y) {					/* case 0 */
	    replaced = newroot->has_payload();
	    newroot->set_payload(p);
	    me = newroot;
	    break;
        }

	// boundaries of x and y, and their midpoints.

	A x_m = x.masked_addr() | ( ~(x.netmask()) >> 1 );
	A y_m = y.masked_addr() | ( ~(y.netmask()) >> 1 );
	A x_l = x.masked_addr();
	A x_h = x.top_addr();
	A y_l = y.masked_addr();
	A y_h = y.top_addr();

	if (x_h < y_l) {			 	/* case A */
	    //debug_msg("case A:  |--x--|   |--y--|\n");
	    Key k = Key::common_subnet(x, y);
	    newroot = new TrieNode(k, parent);	// create new root
	    newroot->_right = *root;		// old root goes right
	    newroot->_right->_up = newroot;
	    newroot->_left = me = new TrieNode(x, p, newroot);
	    break;
	} else if (y_h < x_l) {				/* case B */
	    //debug_msg("case B:  |--y--|   |--x--|\n");
	    Key k = Key::common_subnet(x, y);
	    newroot = new TrieNode(k, parent);	// create new root
	    newroot->_left = *root;
	    newroot->_left->_up = newroot;
	    newroot->_right = me = new TrieNode(x, p, newroot);
	    break;
	} else if (x_l >= y_l && x_h <= y_m) {		/* case C */
	    //debug_msg("case C:  |--x-.----|\n");
	    parent = *root;
	    root = &(newroot->_left);
	} else if (x_l > y_m && x_h <= y_h) {		/* case D */
	    //debug_msg("case D:  |----.-x--|\n");
	    parent = *root;
	    root = &(newroot->_right);
	} else if (y_l > x_m && y_h <= x_h) {		/* case E */
	    //debug_msg("case E:  |----.-Y--|\n");
	    newroot = me = new TrieNode(x, p, parent);
	    newroot->_right = *root;
	    newroot->_right->_up = newroot;
	    break;
	} else if (y_l >= x_l && y_h <= x_m) {		/* case F */
	    //debug_msg("case F:  |--Y-.----|\n");
	    newroot = me = new TrieNode(x, p, parent);
	    newroot->_left = *root;
	    newroot->_left->_up = newroot;
	    break;
	} else
	    abort();	// impossible case in TrieNode::insert()
    }
    *root = newroot;
    if (*oldroot)
	(*oldroot)->validate(NULL);
    // (*oldroot)->print(0, "");
    return me;
}


/**
 * Remove this node, cleanup useless internal nodes.
 *
 * @returns a pointer to the root of the trie.
 */
template <class A, class Payload>
TrieNode<A, Payload> *
TrieNode<A, Payload>::erase()
{
    TrieNode *me, *parent, *child;

    if (_p) {
	delete _p;
	_p = NULL;
    }

    debug_msg("++ erase %s\n", this->_k.str().c_str());
    /*
     * If the node ("me") exists, has no payload and at most one child,
     * then it is a useless internal node which needs to be removed by
     * linking the child to the parent. If the child is NULL, we need
     * to repeat the process up.
     */
    for (me = this; me && me->_p == NULL &&
	     (me->_left == NULL || me->_right == NULL); ) {

	// me->dump("erase");			// debugging

	// Find parent and the one possible child (both can be NULL).
	parent = me->_up;
	child = me->_left ? me->_left : me->_right;

	if (child != NULL)		// if the child exists, link it to
	    child->_up = parent;	// its new parent

	if (parent == NULL)		// no parent, child becomes new root
	    parent = child;
	else { // i have a parent, link my child to it (left or right)
	    if (parent->_left == me)
		parent->_left = child;
	    else
		parent->_right = child;
	}
	delete me;			// nuke the node
	me = parent;
    }
    // now navigate up to find and return the new root of the trie
    for ( ; me && me->_up ; me = me->_up)
	;
    return me;
}

/**
 * Finds the most specific entry in the subtree rooted at r
 * that contains the desired key and has a Payload
 */
template <class A, class Payload> 
TrieNode<A, Payload> *
TrieNode<A,Payload>::find(const Key &key)
{
    TrieNode * cand = NULL;
    TrieNode * r = this;

    for ( ; r && r->_k.contains(key) ; ) {
	if (r->_p)
	    cand = r;		// we have a candidate.
	if (r->_left && r->_left->_k.contains(key))
	    r = r->_left;
	else			// should check that right contains(key), but
	    r = r->_right;	// the loop condition will do it for us.
    }
    return cand;
}

/**
 * See the comment in the class definition.
 */
template <class A, class Payload> 
TrieNode<A, Payload> *
TrieNode<A,Payload>::lower_bound(const Key &key)
{
    TrieNode * cand = NULL;
    TrieNode * r = this;
    //printf("lower bound: %s\n", key.str().c_str());
    for ( ; r && r->_k.contains(key) ; ) {
	cand = r;		// any node is good, irrespective of payload.
	if (r->_left && r->_left->_k.contains(key))
	    r = r->_left;
	else			// should check that right contains(key), but
	    r = r->_right;	// the loop condition will do it for us.
    }
    if (cand == NULL)
	cand = this;

    if (cand->_k == key) {	// we found an exact match
	if (cand->_p) {		// we also have a payload, so we are done.
	    // printf("exact match\n");
	    return cand;
	} else {		// no payload, skip to the next node in the
				// entire tree (use a null Key as root)
	    // printf("exact match on empty node - calling next\n");
	    return cand->next(Key());
	}
    }
    
    // printf("no exact match\n");
    // No exact match exists.
    // cand holds what would be the parent of the node, if it existed.
    while (cand != NULL) {
	// printf("cand = %s\n", cand->str().c_str());
	if (cand->_left && (key < cand->_left->_k)) {
	    return cand->_left->leftmost();
	}
	if (cand->_right && (key < cand->_right->_k)) {
	    return cand->_right->leftmost();
	}
	cand = cand->_up;
    }
    return NULL;
}

/**
 * Finds the subtree of key.
 */
template <class A, class Payload> 
TrieNode<A, Payload> *
TrieNode<A,Payload>::find_subtree(const Key &key)
{
    TrieNode *r = this;
    TrieNode *cand = r && key.contains(r->_k) ? r : NULL;

    for ( ; r && r->_k.contains(key) ; ) {
        cand = r;		// we have a candidate.
	if (r->_left && r->_left->_k.contains(key))
	    r = r->_left;
	else			// should check that right contains(key), but
	    r = r->_right;	// the loop condition will do it for us.
    }
    return cand;
}

template <class A, class Payload> 
void 
TrieNode<A,Payload>::print(int indent, const char *msg) const
{
#ifdef DEBUG_LOGGING
    debug_msg_indent(indent);

    if (this == NULL) {
	debug_msg("%sNULL\n", msg);
	return;
    }
    debug_msg("%skey: %s %s\n",
	      msg, _k.str().c_str(), _p ? "PL" : "[]");
    debug_msg("    U: %s\n", _up ? _up->_k.str().c_str() : "NULL");
    _left->print(indent+4, "L: ");
    _right->print(indent+4, "R: ");
    debug_msg_indent(0);
#endif /* DEBUG_LOGGING */
    UNUSED(indent);
    UNUSED(msg);
}

template <class A, class Payload> 
string
TrieNode<A,Payload>::str() const
{
    string s;
    if (this == NULL) {
	s = "NULL";
	return s;
    }
    s = c_format("key: %s %s\n", _k.str().c_str(), _p ? "PL" : "[]");
    s += c_format("    U: %s\n", _up ? _up->_k.str().c_str() : "NULL");
    return s;
}

template <class A, class Payload> 
void 
Trie<A,Payload>::print() const
{
    //this is called print - it should NOT use debug_msg!!!
    printf("---- print trie ---\n");
    // _root->print(0, "");
    iterator ti;
    for (ti = begin() ; ti != end() ; ti++)
	printf("*** node: %-26s %s\n",
	       ti.cur()->k().str().c_str(),
	       ti.cur()->has_payload() ? "PL" : "[]");
    printf("---------------\n");
}

#endif // __LIBXORP_TRIE_HH__
