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

#ident "$XORP: xorp/rib/rt_tab_origin.cc,v 1.13 2004/02/11 08:48:49 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"

#include "rt_tab_origin.hh"
#include "rt_tab_deletion.hh"


//
// A = Address Type. E.g., IPv4 or IPv6
//
template<class A>
OriginTable<A>::OriginTable<A>(const string& tablename,
			       int admin_distance,
			       ProtocolType protocol_type,
			       EventLoop& eventloop)
    : RouteTable<A>(tablename),
      _admin_distance(admin_distance),
      _protocol_type(protocol_type),
      _eventloop(eventloop)
{
    XLOG_ASSERT(admin_distance >= 0 && admin_distance <= 255);
    XLOG_ASSERT((protocol_type == IGP) || (protocol_type == EGP));

    _ip_route_table = new Trie<A, const IPRouteEntry<A>* >();
}

template<class A>
OriginTable<A>::~OriginTable<A>()
{
    // Delete all the routes in the trie
    delete_all_routes();
    delete _ip_route_table;
}

template<class A>
int
OriginTable<A>::add_route(const IPRouteEntry<A>& route)
{
    debug_msg("OT[%s]: Adding route %s\n", tablename().c_str(),
	      route.str().c_str());

    //
    // The actual map holds pointers, but we also do allocation and
    // deallocation here. The reason for this is that using the map to
    // hold objects themselves results in us doing too many copies on
    // lookup, but we also don't want the table to be referencing
    // something external that may go away.
    //
    IPRouteEntry<A>* routecopy = new IPRouteEntry<A>(route);
    routecopy->set_admin_distance(_admin_distance);

#if 0
    //
    // BGP can send multiple add routes for the same entry without any
    // corresponding deletes. So if this route is already in the table
    // remove it.
    //
    if (lookup_route(route.net()) != NULL)
	delete_route(route.net());
#else
    if (lookup_route(route.net()) != NULL)
	return XORP_ERROR;
#endif

    // Now add the route to this table
    debug_msg("BEFORE:\n");
#ifdef DEBUG_LOGGING
    _ip_route_table->print();
#endif
    _ip_route_table->insert(route.net(), routecopy);
    debug_msg("AFTER:\n");
#ifdef DEBUG_LOGGING
    _ip_route_table->print();
#endif

    // Propagate to next table
    if (next_table() != NULL) {
	next_table()->add_route(*routecopy,
			       reinterpret_cast<RouteTable<A>* >(this));
    }

    return XORP_OK;
}

template<class A>
int
OriginTable<A>::delete_route(const IPNet<A>& net)
{
    debug_msg("OT[%s]: Deleting route %s\n", tablename().c_str(),
	   net.str().c_str());
#ifdef DEBUG_LOGGING
    _ip_route_table->print();
#endif

    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->lookup_node(net);
    if (iter != _ip_route_table->end()) {
	const IPRouteEntry<A>* found = iter.payload();
	_ip_route_table->erase(net);
	// Propagate to next table
	if (next_table() != NULL)
	    next_table()->delete_route(found, this);

	// Finally we're done, and can cleanup
	delete found;
	return XORP_OK;
    }
    XLOG_ERROR("OT: attempt to delete a route that doesn't exist: %s",
	       net.str().c_str());
    return XORP_ERROR;
}

template<class A>
void
OriginTable<A>::delete_all_routes()
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
void
OriginTable<A>::routing_protocol_shutdown()
{
    //
    // Pass our entire routing table into a DeletionTable, which will
    // handle the background deletion task.  The DeletionTable will
    // plumb itself in.
    //
    DeletionTable<A>* dt;
    dt = new DeletionTable<A>("Delete(" + tablename() + ")",
			      this,
			      _ip_route_table,
			      _eventloop);
    // Create a new routing table, ready for when the routing protocol
    // comes back up.
    _ip_route_table = new Trie<A, const IPRouteEntry<A>* >();
}

template<class A>
const IPRouteEntry<A>*
OriginTable<A>::lookup_route(const IPNet<A>& net) const
{
    debug_msg("------------------\nlookup_route in table %s\n",
	tablename().c_str());
    debug_msg("OriginTable: Looking up route %s\n", net.str().c_str());
    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->lookup_node(net);
    return (iter == _ip_route_table->end()) ? NULL : iter.payload();
}

template<class A>
const IPRouteEntry<A>*
OriginTable<A>::lookup_route(const A& addr) const
{
    debug_msg("------------------\nlookup_route in table %s\n",
	tablename().c_str());
    debug_msg("OriginTable (%d): Looking up route for addr %s\n",
	   _admin_distance, addr.str().c_str());

    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->find(addr);
    if (iter == _ip_route_table->end()) {
	debug_msg("No match found\n");
    }
    return (iter == _ip_route_table->end()) ? NULL : iter.payload();
}

template<class A>
RouteRange<A>*
OriginTable<A>::lookup_route_range(const A& addr) const
{
    const IPRouteEntry<A>* route;
    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table->find(addr);

    route = (iter == _ip_route_table->end()) ? NULL : iter.payload();

    A bottom_addr, top_addr;
    _ip_route_table->find_bounds(addr, bottom_addr, top_addr);
    RouteRange<A>* rr = new RouteRange<A>(addr, route, top_addr, bottom_addr);
    debug_msg("Origin Table: %s returning lower bound for %s of %s\n",
	      tablename().c_str(), addr.str().c_str(),
	      bottom_addr.str().c_str());
    debug_msg("Origin Table: %s returning upper bound for %s of %s\n",
	      tablename().c_str(), addr.str().c_str(), top_addr.str().c_str());
    return rr;
}

template<class A>
string
OriginTable<A>::str() const
{
    string s;

    s = "-------\nOriginTable: " + tablename() + "\n" +
    	( _protocol_type == IGP ? "IGP\n" : "EGP\n" ) ;
    if (next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + next_table()->tablename() + "\n";
    return s;
}

template class OriginTable<IPv4>;
typedef OriginTable<IPv4> IPv4OriginTable;

template class OriginTable<IPv6>;
typedef OriginTable<IPv6> IPv6OriginTable;
