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

// $XORP: xorp/bgp/bgp_trie.hh,v 1.2 2002/12/13 22:38:53 rizzo Exp $

#ifndef __BGP_TRIE_HH__
#define __BGP_TRIE_HH__

#include "subnet_route.hh"
#include <map>
#include "libxorp/trie.hh"

template <class A>
class Path_Att_Ptr_Cmp {
public:
    bool operator() (const PathAttributeList<A> *a,
		     const PathAttributeList<A> *b) const {
	return *a < *b;
    }
};

template<class A>
class ChainedSubnetRoute : public SubnetRoute<A> {
public:
    ChainedSubnetRoute(const IPNet<A> &net,
		       const PathAttributeList<A> *attributes) :
	SubnetRoute<A>(net, attributes), _prev(0), _next(0) {}

    ChainedSubnetRoute(const SubnetRoute<A>& route,
		       const ChainedSubnetRoute<A>* prev);

    ChainedSubnetRoute(const ChainedSubnetRoute& csr);

    ~ChainedSubnetRoute()	{}

    const ChainedSubnetRoute<A> *prev() const { return _prev; }
    const ChainedSubnetRoute<A> *next() const { return _next; }

    bool unchain() const;

protected:
    inline void set_next(const ChainedSubnetRoute<A> *next) const {
	_next = next;
    }

    inline void set_prev(const ChainedSubnetRoute<A> *prev) const {
	_prev = prev;
    }

    ChainedSubnetRoute& operator=(const ChainedSubnetRoute& csr); // Not impl.

private:
    // it looks odd to have these be mutable and the methods to set
    // them be const, but that's because the chaining is really
    // conceptually part of the container, not the payload.  It these
    // aren't mutable we can't modify the chaining and have the payload
    // be const.
    mutable const ChainedSubnetRoute<A> *_prev;
    mutable const ChainedSubnetRoute<A> *_next;
};

/**
 * The BgpTrie is an augmented, specialized trie that allows us to
 * lookup by network address or by path attribute list.  We need this
 * because we can't efficiently extract entries with the same path
 * attribute list from a regular trie.  Each set of nodes with the same
 * path attribute pointer are linked together into a chain (a circular
 * doubly-linked list).  The BgpTrie holds a pointer to any one of
 * those nodes.
 */
template<class A>
class BgpTrie {
public:
    typedef IPNet<A> IPNet;
    typedef ChainedSubnetRoute<A> ChainedSubnetRoute;
    typedef map<const PathAttributeList<A> *,
	const ChainedSubnetRoute*, Path_Att_Ptr_Cmp<A> > PathmapType;
    typedef Trie<A, const ChainedSubnetRoute> RouteTrie;
    typedef typename RouteTrie::iterator iterator;

    BgpTrie()	{}

    iterator insert(const IPNet& net, const SubnetRoute<A>& route);

    void erase(const IPNet& net);

    void delete_all_nodes()			{
	    while (_pathmap.empty() == false)
		_pathmap.erase(_pathmap.begin());
	    _trie.delete_all_nodes();
    }

    iterator lookup_node(const IPNet& net) const	{
	    return _trie.lookup_node(net);
    }

    iterator lower_bound(const IPNet& net) const	{
	    return _trie.lower_bound(net);
    }

    iterator find(const A& addr) const		{ return _trie.find(addr); }

#if 0
    // find_less_specific asks the question: if I were to add this
    // net to the trie, what would be its parent node?  net may or
    // may not already be in the trie
    iterator find_less_specific(const IPNet& net) const {
	    return _trie.find_less_specific(net);
    }

    void find_bounds(const A& net, A& lo, A& hi) const {
	    _trie.find_bounds(net, lo, hi);
    }
#endif
    int route_count() const			{ return _trie.route_count(); }

    iterator begin() const			{ return _trie.begin(); }
    iterator end() const			{ return _trie.end(); }
    iterator search_subtree(const IPNet& net) const {
	    return _trie.search_subtree(net);
    }

    const PathmapType& pathmap() const { return _pathmap; }

private:
    RouteTrie	_trie;
    PathmapType	_pathmap;
};

#endif // __BGP_TRIE_HH__
