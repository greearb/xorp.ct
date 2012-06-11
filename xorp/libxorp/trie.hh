// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/trie.hh,v 1.29 2008/10/02 21:57:36 bms Exp $

#ifndef __LIBXORP_TRIE_HH__
#define __LIBXORP_TRIE_HH__

#include "ipnet.hh"

// Macros
//#define VALIDATE_XORP_TRIE
//#define DEBUG_LOGGING

#include "xlog.h"
#include "debug.h"
#include "minitraits.hh"

#ifndef XORP_USE_USTL
#include <stack>
#endif

#define trie_debug_msg(x...) /* debug_msg(x) */
#define trie_debug_msg_indent(x)

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

    explicit TrieNode(const Key& key, TrieNode* up = 0) :
	_up(up), _left(0), _right(0), _k(key), _p(0) {}

    ~TrieNode()
    {
	if (_p)
	    delete_payload(_p);
    }

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

    TrieNode* get_left()			{ return this->_left;   }
    TrieNode* get_right()			{ return this->_right;  }
    TrieNode* get_parent()			{ return this->_up;   }
    bool has_payload() const			{ return _p != NULL;	}
    const Payload &p() const			{ return *_p;		}
    Payload &p()    				{ return *_p;		}

    void set_payload(const Payload& p) {
	if (_p)
	    delete_payload(_p);
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
     * @return the leftmost node under this node
     */

    TrieNode * leftmost() {
	TrieNode *n = this;
	while (n->_left || n->_right)
	    n = (n->_left ? n->_left : n->_right);
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
    /* delete_payload is a separate method to allow specialization */
    void delete_payload(Payload* p) {
	delete p;
    }

    void dump(const char *msg) const
    {
	UNUSED(msg);
	trie_debug_msg(" %s %s %s\n",
		       msg,
		       _k.str().c_str(), _p ? "PL" : "[]");
	trie_debug_msg("  U   %s\n",
		       _up ? _up->_k.str().c_str() : "NULL");
	trie_debug_msg("  L   %s\n",
		       _left ? _left->_k.str().c_str() : "NULL");
	trie_debug_msg("  R   %s\n",
		       _right ? _right->_k.str().c_str() : "NULL");

    }

    TrieNode	*_up, *_left, *_right;
    Key		_k;
    PPayload 	*_p;
};

/**
 * Postorder Iterator on a trie.
 *
 * _cur points to the current object, _root contains the search key for
 * root of the subtree we want to scan. The iterator skips over empty
 * nodes, and visits the subtree in depth-first, left-to-right order.
 * The keys returned by this iterator are not sorted by prefix length.
 */
template <class A, class Payload>
class TriePostOrderIterator {
public:
    typedef IPNet<A> Key;
    typedef TrieNode<A, Payload> Node;

    /**
     * Constructors
     */
    TriePostOrderIterator()				{}

    /**
     * constructor for exact searches: both the current node and the search
     * key are taken from n, so the iterator will only loop once.
     */
    explicit TriePostOrderIterator(Node *n)			{
	_cur = n;
	if (n)
	    _root = n->k();
    }

    /**
     * construct for subtree scanning: the root key is set explicitly,
     * and the current node is set according to the search order.
     */
    TriePostOrderIterator(Node *n, const Key &k)		{
	_root = k;
	_cur = n;
	if (_cur) begin();
    }

    /**
     * move to the starting position according to the visiting order
     */
    TriePostOrderIterator * begin() {
	Node * n = _cur;
	while (n->get_parent() && _root.contains(n->get_parent()->k()))
	    n = n->get_parent();
	_cur =  n->leftmost();
	return this;
    }

    /**
     * Postfix increment
     *
     * Updates position of iterator in tree.
     * @return position of iterator before increment.
     */
    TriePostOrderIterator operator ++(int)		{ // postfix
	TriePostOrderIterator x = *this;
	next();
	return x;
    }

    /**
     * Prefix increment
     *
     * Updates position of iterator in tree.
     * @return position of iterator after increment.
     */
    TriePostOrderIterator& operator ++()		{ // prefix
	next();
	return *this;
    }

    Payload& operator*()		{ return payload(); }
    const Payload& operator*() const	{ return payload(); }

    Payload* operator->()		{ return &payload(); }
    const Payload* operator->() const	{ return &payload(); }

    Node *cur() const			{ return _cur;		};

    bool operator==(const TriePostOrderIterator & x) const {
	return (_cur == x._cur);
    }

    bool has_payload() const		{ return _cur->has_payload(); }
    Payload & payload()			{ return _cur->p(); };
    const Payload & payload() const	{ return _cur->p(); };
    const Key & key() const		{ return _cur->k(); };

private:

    bool node_is_left(Node * n) const;
    void next();

    Node	*_cur;
    Key		_root;
};

/**
 * Preorder Iterator on a trie.
 *
 * _cur points to the current object, _root contains the search key for
 * root of the subtree we want to scan. The iterator does preorder traversal,
 * that is, current node first, then left then right.  This guarantees that
 * keys returned are sorted by prefix length.
 */
template <class A, class Payload>
class TriePreOrderIterator {
public:
    typedef IPNet<A> Key;
    typedef TrieNode<A, Payload> Node;

    /**
     * Constructors
     */
    TriePreOrderIterator()				{}

    /**
     * constructor for exact searches: both the current node and the search
     * key are taken from n, so the iterator will only loop once.
     */
    explicit TriePreOrderIterator(Node *n)			{
	_cur = n;
	if (_cur) _root = n->k();
    }

    /**
     * construct for subtree scanning: the root key is set explicitly,
     * and the current node is set according to the search order.
     */
    TriePreOrderIterator(Node *n, const Key &k)		{
	_root = k;
	_cur = n;
	if (_cur) begin();
    }

    /**
     * move to the starting position according to the visiting order
     */
    TriePreOrderIterator * begin() {
	while (!_stack.empty()) _stack.pop();
	while (_cur->get_parent() && _root.contains(_cur->get_parent()->k()))
	    _cur = _cur->get_parent();
	_stack.push(_cur);
	next();
	return this;
    }

    /**
     * Postfix increment
     *
     * Updates position of iterator in tree.
     * @return position of iterator before increment.
     */
    TriePreOrderIterator operator ++(int)		{ // postfix
	TriePreOrderIterator x = *this;
	next();
	return x;
    }

    /**
     * Prefix increment
     *
     * Updates position of iterator in tree.
     * @return position of iterator after increment.
     */
    TriePreOrderIterator& operator ++()		{ // prefix
	next();
	return *this;
    }

    Payload& operator*()		{ return payload(); }
    const Payload& operator*() const	{ return payload(); }

    Payload* operator->()		{ return &payload(); }
    const Payload* operator->() const	{ return &payload(); }

    Node *cur() const			{ return _cur;		};

    bool operator==(const TriePreOrderIterator & x) const {
	return (_cur == x._cur);
    }

    bool has_payload() const		{ return _cur->has_payload(); }
    Payload & payload()			{ return _cur->p(); };
    const Payload & payload() const	{ return _cur->p(); };
    const Key & key() const		{ return _cur->k(); };

private:


    bool node_is_left(Node * n) const;
    void next();

    Node	 *_cur;
    Key		 _root;
    stack<Node*> _stack;
};

/**
 * The Trie itself
 *
 * The trie support insertion and deletion of Key,Payload pairs,
 * and lookup by Key (which can be an address or a subnet).
 *
 * Additional methods are supported to provide access via iterators.
 */

template <class A, class Payload, class __Iterator =
    TriePostOrderIterator<A,Payload> >
class Trie {
public:
    typedef IPNet<A> Key;
    typedef TrieNode<A,Payload> Node;
    typedef __Iterator iterator;

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
     * Set root node associated with iterator to the root node of the
     * trie.  Needed whilst trie iterators have concept of root nodes
     * find methods return iterators with root bound to key and
     * means they can never continue iteration beyond of root.
     *
     * @return iterator with non-restricted root node.
     */
    iterator unbind_root(iterator i) const	{
	return iterator(i.cur(), _root->k());
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
	// there are no less specific routes than the default route
	if (key.prefix_len() == 0)
	    return end();

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
 * @return a pointer to the newly inserted node.
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

    trie_debug_msg("++ insert %s\n", x.str().c_str());
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
	    //trie_debug_msg("case A:  |--x--|   |--y--|\n");
	    Key k = Key::common_subnet(x, y);
	    newroot = new TrieNode(k, parent);	// create new root
	    newroot->_right = *root;		// old root goes right
	    newroot->_right->_up = newroot;
	    newroot->_left = me = new TrieNode(x, p, newroot);
	    break;
	} else if (y_h < x_l) {				/* case B */
	    //trie_debug_msg("case B:  |--y--|   |--x--|\n");
	    Key k = Key::common_subnet(x, y);
	    newroot = new TrieNode(k, parent);	// create new root
	    newroot->_left = *root;
	    newroot->_left->_up = newroot;
	    newroot->_right = me = new TrieNode(x, p, newroot);
	    break;
	} else if (x_l >= y_l && x_h <= y_m) {		/* case C */
	    //trie_debug_msg("case C:  |--x-.----|\n");
	    parent = *root;
	    root = &(newroot->_left);
	} else if (x_l > y_m && x_h <= y_h) {		/* case D */
	    //trie_debug_msg("case D:  |----.-x--|\n");
	    parent = *root;
	    root = &(newroot->_right);
	} else if (y_l > x_m && y_h <= x_h) {		/* case E */
	    //trie_debug_msg("case E:  |----.-Y--|\n");
	    newroot = me = new TrieNode(x, p, parent);
	    newroot->_right = *root;
	    newroot->_right->_up = newroot;
	    break;
	} else if (y_l >= x_l && y_h <= x_m) {		/* case F */
	    //trie_debug_msg("case F:  |--Y-.----|\n");
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
 * @return a pointer to the root of the trie.
 */
template <class A, class Payload>
TrieNode<A, Payload> *
TrieNode<A, Payload>::erase()
{
    TrieNode *me, *parent, *child;

    if (_p) {
	delete_payload(_p);
	_p = NULL;
    }

    trie_debug_msg("++ erase %s\n", this->_k.str().c_str());
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
	} else {		// no payload, skip to the next (in postorder)
				// node in the entire tree (null Key as root)
	    // printf("exact match on empty node - calling next\n");
	    TriePostOrderIterator<A,Payload> iterator(cand, Key());
	    ++iterator;
	    return iterator.cur();
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
	if (key.contains(r->_k))
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
    trie_debug_msg_indent(indent);

    if (this == NULL) {
	trie_debug_msg("%sNULL\n", msg);
	return;
    }
    trie_debug_msg("%skey: %s %s\n",
		   msg, _k.str().c_str(), _p ? "PL" : "[]");
    trie_debug_msg("    U: %s\n", _up ? _up->_k.str().c_str() : "NULL");
    _left->print(indent+4, "L: ");
    _right->print(indent+4, "R: ");
    trie_debug_msg_indent(0);
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

template <class A, class Payload, class __Iterator>
void
Trie<A,Payload,__Iterator>::print() const
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

template <class A, class Payload>
bool
TriePostOrderIterator<A,Payload>::node_is_left(Node* n) const
{
    return n->get_parent() && n  == n->get_parent()->get_left();
}

template <class A, class Payload>
void
TriePostOrderIterator<A,Payload>::next()
{
    Node * n = _cur;
    do {
	if (n->get_parent() == NULL) {
	    _cur = NULL;
	    return;		// cannot backtrack, finished
	}
	bool was_left_child = node_is_left(n);
	n = n->get_parent();
	// backtrack one level, then explore the leftmost path
	// on the right branch if not done already.
	if (was_left_child && n->get_right()) {
	    n = n->get_right()->leftmost();
	}
	if (_root.contains(n->k()) == false) {
	    _cur = NULL;
	    return;
	}
    } while (n->has_payload() == false);	// found a good node.
    _cur = n;
}


template <class A, class Payload>
void
TriePreOrderIterator<A,Payload>::next()
{
    if (_stack.empty()) {
	_cur = NULL;
	return;
    }

    do {
	_cur = _stack.top();
	_stack.pop();
	if( _cur->get_right( ) != NULL )
	    _stack.push(_cur->get_right());
	if( _cur->get_left() != NULL )
	    _stack.push(_cur->get_left());
    } while (_cur->has_payload() == false);	// found a good node.
}

#endif // __LIBXORP_TRIE_HH__
