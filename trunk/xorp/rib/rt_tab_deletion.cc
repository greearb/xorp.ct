// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rib/rt_tab_deletion.cc,v 1.2 2003/09/27 22:32:46 mjh Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"

#include "rt_tab_deletion.hh"


//
// A = Address Type. E.g., IPv4 or IPv6
//
template<class A>
DeletionTable<A>::DeletionTable<A>(const string& tablename,
				   RouteTable<A>* parent,
				   Trie<A, const IPRouteEntry<A>* >* ip_route_trie,
				   EventLoop& eventloop)
    : RouteTable<A>(tablename),
      _parent(parent),
      _eventloop(eventloop),
      _ip_route_table(ip_route_trie)
{
    XLOG_ASSERT(_parent != NULL);
    _next_table = _parent->next_table();
    _next_table->replumb(parent, this);
    parent->set_next_table(this);

    // Callback immediately, but after network events or expired timers
    _background_deletion_timer = _eventloop.new_oneoff_after_ms(
	0,
	callback(this, &DeletionTable<A>::background_deletion_pass));
}

template<class A>
DeletionTable<A>::~DeletionTable<A>() 
{
    // Delete all the routes in the trie.
    delete_all_routes();
    delete _ip_route_table;
}

template<class A>
int 
DeletionTable<A>::add_route(const IPRouteEntry<A>& route, 
			    RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->lookup_node(route.net());
    if (iter !=  _ip_route_table->end()) {
	//
	// We got an add route for a route that was waiting to be
	// deleted.  Process this now - pass the deletion downstream,
	// remove the route from our trie, then pass the new route
	// downstream.
	//
	const IPRouteEntry<A>* our_route = iter.payload();
	_ip_route_table->erase(route.net());
	_next_table->delete_route(our_route, this);
	delete our_route;
    }

    return _next_table->add_route(route, this);
}

template<class A>
int 
DeletionTable<A>::delete_route(const IPRouteEntry<A>* route, 
			       RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    // The route MUST NOT be in our trie.
    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->lookup_node(route->net());
    XLOG_ASSERT(iter == _ip_route_table->end());

    return _next_table->delete_route(route, this);
}

template<class A>
void
DeletionTable<A>::delete_all_routes() 
{
    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    for (iter = _ip_route_table->begin();
	 iter != _ip_route_table->end();
	 ++iter) {
	delete iter.payload();
    }
    _ip_route_table->delete_all_nodes();
}

template<class A>
const IPRouteEntry<A>*
DeletionTable<A>::lookup_route(const IPNet<A>& net) const
{
    const IPRouteEntry<A>* parent_route = _parent->lookup_route(net);

    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->lookup_node(net);

    if (parent_route != NULL) {
	//
	// If we succeeded in looking up the route in the parent table,
	// then the route MUST NOT be in our deletion table.
	//
	XLOG_ASSERT(iter == _ip_route_table->end());
	return parent_route;
    } else {
	//
	// While we hold routes to be deleted, we haven't told anyone
	// downstream they've gone yet.  We have to pretend they're
	// still there (for consistency reasons) until we've got round
	// to telling downstream that they've actually gone.
	//
	return (iter == _ip_route_table->end()) ? NULL : iter.payload();
    }
}

template<class A>
const IPRouteEntry<A>*
DeletionTable<A>::lookup_route(const A& addr) const
{
    const IPRouteEntry<A>* parent_route = _parent->lookup_route(addr);

    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->find(addr);

    if (parent_route != NULL) {
	if (iter == _ip_route_table->end()) {
	    return parent_route;
	} else {
	    //
	    // Both our parent and ourselves have a route.  We need to
	    // return the more specific route.  If the two are the same
	    // this is a fatal error.
	    //
	    const IPRouteEntry<A>* our_route = iter.payload();
	    XLOG_ASSERT(our_route->prefix_len() != parent_route->prefix_len());

	    if (our_route->prefix_len() > parent_route->prefix_len()) {
		return our_route;
	    } else {
		return parent_route;
	    }
	}
	XLOG_UNREACHABLE();
    }

    //
    // While we hold routes to be deleted, we haven't told anyone
    // downstream they've gone yet.  We have to pretend they're
    // still there (for consistency reasons) until we've got round
    // to telling downstream that they've actually gone.
    //
    return (iter == _ip_route_table->end()) ? NULL : iter.payload();
}

template<class A>
RouteRange<A>*
DeletionTable<A>::lookup_route_range(const A& addr) const
{
    const IPRouteEntry<A>* route;
    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->find(addr);

    if (iter == _ip_route_table->end())
	route = NULL;
    else
	route = iter.payload();

    A bottom_addr, top_addr;
    _ip_route_table->find_bounds(addr, bottom_addr, top_addr);
    RouteRange<A>* rr = new RouteRange<A>(addr, route, top_addr, bottom_addr);
    debug_msg("Deletion Table: %s returning lower bound for %s of %s\n",
	      tablename().c_str(), addr.str().c_str(),
	      bottom_addr.str().c_str());
    debug_msg("Deletion Table: %s returning upper bound for %s of %s\n",
	      tablename().c_str(), addr.str().c_str(), top_addr.str().c_str());

    // Merge our own route range with that of our parent.
    RouteRange<A>* parent_rr = _parent->lookup_route_range(addr);
    rr->merge(parent_rr);
    delete parent_rr;
    return rr;
}

template<class A> 
void
DeletionTable<A>::background_deletion_pass()
{
    if (_ip_route_table->begin() == _ip_route_table->end()) {
	unplumb_self();
	return;
    }

    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->begin();
    const IPRouteEntry<A>* our_route = iter.payload();
    _ip_route_table->erase(our_route->net());
    _next_table->delete_route(our_route, this);
    delete our_route;

    // Callback immediately, but after network events or expired timers
    _background_deletion_timer = _eventloop.new_oneoff_after_ms(
	0,
	callback(this, &DeletionTable<A>::background_deletion_pass));
}

template<class A> 
void
DeletionTable<A>::unplumb_self()
{
    _parent->set_next_table(_next_table);
    _next_table->replumb(this, _parent);
    delete this;
}

template<class A>
void
DeletionTable<A>::replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent)
{
    XLOG_ASSERT(_parent == old_parent);
    _parent = new_parent;
}

template<class A>
string
DeletionTable<A>::str() const
{
    string s;

    s = "-------\nDeletionTable: " + _tablename + "\n";
    if (_next_table == NULL)
	s += "no next table\n";
    else
	s += "next table = " + _next_table->tablename() + "\n";
    return s;
}

template class DeletionTable<IPv4>;
typedef DeletionTable<IPv4> IPv4DeletionTable;

template class DeletionTable<IPv6>;
typedef DeletionTable<IPv6> IPv6DeletionTable;
