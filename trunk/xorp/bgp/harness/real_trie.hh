// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/bgp/harness/real_trie.hh,v 1.12 2008/07/23 05:09:43 pavlin Exp $

#ifndef __BGP_HARNESS_REAL_TRIE_HH_
#define __BGP_HARNESS_REAL_TRIE_HH_

template <class A>
class RealTrie {
public:
    RealTrie() : _debug(true), _pretty(true)  {
    }

    ~RealTrie();

    bool empty() const;

    typedef typename XorpCallback3<void, const UpdatePacket*,
			  const IPNet<A>&,
			  const TimeVal&>::RefPtr TreeWalker;

    void tree_walk_table(const TreeWalker& tw) const {
	A topbit;
	++topbit;
	topbit = topbit << A::addr_bitlen() - 1;
	tree_walk_table(tw, &_head, A::ZERO(), 0, topbit);
    };

    bool insert(const IPNet<A>& net, TriePayload& p);
    bool insert(A address, size_t mask_length, TriePayload& p);
    void del() {
	del(&_head);
    }
    bool del(const IPNet<A>& net);
    bool del(A address, size_t mask_length);
    TriePayload find(const IPNet<A>& net) const;
    TriePayload find(A address, size_t mask_length) const;
    TriePayload find(const A& address) const;
private:
    struct Tree {
	Tree *ptrs[2];
	TriePayload p;

	Tree() {
	    ptrs[0] = ptrs[1] = 0;
	}
    };

    void tree_walk_table(const TreeWalker&, const Tree *ptr, 
			 A address, size_t prefix_len, A orbit) const;
    void del(Tree *ptr);
    bool del(Tree *ptr, A address, size_t mask_length);
    
    bool _debug;
    bool _pretty;

    Tree _head;
};

template <class A>
RealTrie<A>::~RealTrie()
{
    del(&_head);
}

template <class A>
bool
RealTrie<A>::empty() const
{
    return (0 ==_head.ptrs[0]) && (0 ==_head.ptrs[1]);
}

template <class A>
void
RealTrie<A>::tree_walk_table(const TreeWalker& tw, const Tree *ptr,
			     A address, size_t prefix_len, A orbit) const
{
    debug_msg("Enter: %s/%u\n", address.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    if(0 == ptr) {
	return;
    }

    TimeVal tv;
    const UpdatePacket *update = ptr->p.get(tv);
    if(0 != update) {
 	debug_msg("Found %s/%u\n", address.str().c_str(),
		  XORP_UINT_CAST(prefix_len));
	tw->dispatch(update, IPNet<A>(address, prefix_len), tv);
    }

    ++prefix_len;
    A save_orbit = orbit;
    orbit = orbit >> 1;

    tree_walk_table(tw, ptr->ptrs[0], address, prefix_len, orbit);

    address = address | save_orbit;
    tree_walk_table(tw, ptr->ptrs[1], address, prefix_len, orbit);
}

template <class A>
bool
RealTrie<A>::insert(const IPNet<A>& net, TriePayload& p)
{
    return insert(net.masked_addr(), net.prefix_len(), p);
}

template <class A>
bool
RealTrie<A>::insert(A address, size_t mask_length, TriePayload& p)
{
    debug_msg("%s/%u\n", address.str().c_str(),
	      XORP_UINT_CAST(mask_length));
#ifdef	PARANOIA
    if(0 == p.get()) {
	debug_msg("insert: Attempt to store an invalid entry\n");
	return false;
    }
#endif
    Tree *ptr = &_head;
    for(size_t i = 0; i < mask_length; i++) {
	int index;
	if(address.bits(A::addr_bitlen() - 1, 1))
	    index = 1;
	else
	    index = 0;
	address = address << 1;

	debug_msg("address %s prefix_len %u depth %u index %d\n",
		  address.str().c_str(), XORP_UINT_CAST(mask_length), 
		  XORP_UINT_CAST(i), index);

	
	if(0 == ptr->ptrs[index]) {
	    ptr->ptrs[index] = new Tree();
	    if(0 == ptr->ptrs[index]) {
		if(_debug)
		    debug_msg("insert: new failed\n");
		return false;
	    }
	}

	ptr = ptr->ptrs[index];
    }

    if(0 != ptr->p.get()) {
	debug_msg("insert: value already assigned\n");
	return false;
    }

    ptr->p = p;

    return true;
}

template <class A>
void
RealTrie<A>::del(Tree *ptr)
{
    if(0 == ptr)
	return;
    del(ptr->ptrs[0]);
    delete ptr->ptrs[0];
    ptr->ptrs[0] = 0;
    del(ptr->ptrs[1]);
    delete ptr->ptrs[1];
    ptr->ptrs[1] = 0;
}

template <class A>
bool
RealTrie<A>::del(const IPNet<A>& net)
{
    return del(net.masked_addr(), net.prefix_len());
}

template <class A>
bool
RealTrie<A>::del(A address, size_t mask_length)
{
    return del(&_head, address, mask_length);
}

template <class A>
bool
RealTrie<A>::del(Tree *ptr, A address, size_t mask_length)
{
    if((0 == ptr) && (0 != mask_length)) {
	if(_debug)
	    debug_msg("del:1 not in table\n");
	return false;
    }
    int index;
    if(address.bits(A::addr_bitlen() - 1, 1))
	index = 1;
    else
	index = 0;
    address = address << 1;

    if(0 == mask_length) {
	if(0 == ptr) {
	    if(_debug)
		debug_msg("del:1 zero pointer\n");
	    return false;
	}
	if(0 == ptr->p.get()) {
	    if(_debug)
		debug_msg("del:2 not in table\n");
	    return false;
	}
	ptr->p = TriePayload();	// Invalidate this entry.
	return true;
    }

    if(0 == ptr) {
	if(_debug)
	    debug_msg("del:2 zero pointer\n");
	return false;
    }

    Tree *next = ptr->ptrs[index];

    bool status = del(next, address, --mask_length);

    if(0 == next) {
	if(_debug)
	    debug_msg("del: no next pointer\n");
	return false;
    }

    if((0 == next->p.get()) && (0 == next->ptrs[0]) &&
       (0 == next->ptrs[1])) {
	delete ptr->ptrs[index];
	ptr->ptrs[index] = 0;
    }

    return status;
}

template <class A>
TriePayload 
RealTrie<A>::find(const IPNet<A>& net) const
{
    return find(net.masked_addr(), net.prefix_len());
}

template <class A>
TriePayload 
RealTrie<A>::find(A address, size_t mask_length) const
{
    const Tree *ptr = &_head;
    Tree *next;

    debug_msg("find: %s/%u\n", address.str().c_str(), 
	      XORP_UINT_CAST(mask_length));

    // The loop should not require bounding. Defensive
    for(size_t i = 0; i <= A::addr_bitlen(); i++) {
	int index;
	if(address.bits(A::addr_bitlen() - 1, 1))
	    index = 1;
	else
	    index = 0;
	address = address << 1;

	if(_pretty)
	    debug_msg("%d", index);
	TriePayload p = ptr->p;
	if(mask_length == i)
	    return p;
	if(0 == (next = ptr->ptrs[index])) {
	    if(_pretty)
		debug_msg("\n");
	    return TriePayload();
	}
	ptr = next;
    }
    debug_msg("find: should never happen\n");
    return TriePayload();
}
#endif // __BGP_HARNESS_REAL_TRIE_HH_
