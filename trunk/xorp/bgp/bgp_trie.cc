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

#ident "$XORP: xorp/bgp/bgp_trie.cc,v 1.24 2008/07/23 05:09:31 pavlin Exp $"

// #define DEBUG_LOGGING

#include "bgp_module.h"
#include "bgp_trie.hh"

template<class A>
ChainedSubnetRoute<A>::
ChainedSubnetRoute(const SubnetRoute<A>& route,
		   const ChainedSubnetRoute<A>* prev)
    : SubnetRoute<A>(route)
{
    if (prev != NULL) {
	set_prev(prev);
	set_next(prev->next());
	_prev->set_next(this);
	_next->set_prev(this);
    } else {
	_prev = this;
	_next = this;
    }
}

template<class A>
ChainedSubnetRoute<A>::
ChainedSubnetRoute(const ChainedSubnetRoute<A>& original)
    : SubnetRoute<A>(original)
{
    _prev = &original;
    _next = original.next();
    original.set_next(this);
    _next->set_prev(this);
}

template<class A>
bool
ChainedSubnetRoute<A>::unchain() const {
    _prev->set_next(_next);
    _next->set_prev(_prev);
    return _next != this;
}

/*************************************************************************/

template<class A>
BgpTrie<A>::BgpTrie()
{
}

template<class A>
BgpTrie<A>::~BgpTrie()
{
    if (this->route_count() > 0) {
	XLOG_FATAL("BgpTrie being deleted while still containing data\n");
    }
}

template<class A>
typename BgpTrie<A>::iterator
BgpTrie<A>::insert(const IPNet& net, const SubnetRoute<A>& route)
{
    typename PathmapType::iterator pmi = _pathmap.find(route.attributes());
    const ChainedSubnetRoute* found = (pmi == _pathmap.end()) ? NULL : pmi->second;
    ChainedSubnetRoute* chained_rt 
	= new ChainedSubnetRoute(route, found);

    debug_msg("storing route for %s with attributes %p", net.str().c_str(),
	   route.attributes());

    // The trie will copy chained_rt.  The copy constructor will insert
    // the copy into the chain after chained_rt.
    iterator iter = ((RouteTrie*)this)->insert(net, *chained_rt);

    if (found == NULL) {
	debug_msg(" on new chain");
	_pathmap[route.attributes()] = &(iter.payload());
    }
    debug_msg("\n");
    chained_rt->unchain();
    chained_rt->unref();
    return iter;
}

template<class A>
void
BgpTrie<A>::erase(const IPNet& net)
{
    // unlink the node from the _pathmap chain
    iterator iter = this->lookup_node(net);
    XLOG_ASSERT(iter != this->end());
    const ChainedSubnetRoute *found = &(iter.payload());
    XLOG_ASSERT(iter.key() == net);
    XLOG_ASSERT(found->net() == net);

    debug_msg("deleting route for %s with attributes %p", net.str().c_str(),
	   found->attributes());
    typename PathmapType::iterator pmi = _pathmap.find(found->attributes());
    if (pmi == _pathmap.end()) {
	XLOG_ERROR("Error deleting route for %s with attributes %p %s", 
		   net.str().c_str(), found->attributes(), 
		   found->attributes()->str().c_str());
	XLOG_INFO("Pathmap dump follows: \n");
	for (pmi == _pathmap.begin(); pmi != _pathmap.end(); pmi++) {
	    XLOG_INFO("%p %s\n\n", pmi->first, pmi->second->str().c_str());
	}
	XLOG_FATAL("Exiting\n");
    }
    if (pmi->second == found) {		// this was the head node
	if (found->next() == found) {	 // it's the only node in the chain
	    _pathmap.erase(pmi);
	    debug_msg(" and erasing chain\n");
	} else {			// there are other nodes
	    _pathmap[found->attributes()] = found->next();
	    found->unchain();
	    debug_msg(" chain remains but head moved\n");
	}
    } else {
	found->unchain();
	debug_msg(" chain remains\n");
    }
    debug_msg("\n");

    // now delete it from the Actual Trie
    ((RouteTrie*)this)->erase(iter);
}

template<class A>
void
BgpTrie<A>::delete_all_nodes()			
{
    while (_pathmap.empty() == false)
	_pathmap.erase(_pathmap.begin());
    ((RouteTrie*)this)->delete_all_nodes();
}

template class BgpTrie<IPv4>;
template class BgpTrie<IPv6>;
