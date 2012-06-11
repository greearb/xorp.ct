// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rt_tab_origin.hh"
#include "rt_tab_deletion.hh"


//
// A = Address Type. E.g., IPv4 or IPv6
//
template<class A>
OriginTable<A>::OriginTable(const string&	tablename,
			    uint32_t		admin_distance,
			    ProtocolType	protocol_type,
			    EventLoop&		eventloop)
    : RouteTable<A>(tablename),
      _admin_distance(admin_distance),
      _protocol_type(protocol_type),
      _eventloop(eventloop),
      _gen(0)
{
    XLOG_ASSERT(admin_distance <= 255);
    XLOG_ASSERT((protocol_type == IGP) || (protocol_type == EGP));

    _ip_route_table = new Trie<A, const IPRouteEntry<A>* >();
    _gen++;
}

template<class A>
OriginTable<A>::~OriginTable()
{
    // Delete all the routes in the trie
    delete_all_routes();
    delete _ip_route_table;
}

template<class A>
int
OriginTable<A>::add_route(const IPRouteEntry<A>& route)
{
    debug_msg("OT[%s]: Adding route %s\n", this->tablename().c_str(),
	      route.str().c_str());

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

    //
    // The actual map holds pointers, but we also do allocation and
    // deallocation here. The reason for this is that using the map to
    // hold objects themselves results in us doing too many copies on
    // lookup, but we also don't want the table to be referencing
    // something external that may go away.
    //
    IPRouteEntry<A>* routecopy = new IPRouteEntry<A>(route);
    routecopy->set_admin_distance(_admin_distance);

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
    if (this->next_table() != NULL) {
	this->next_table()->add_route(*routecopy,
			       reinterpret_cast<RouteTable<A>* >(this));
    }

    return XORP_OK;
}

template<class A>
int
OriginTable<A>::add_route(const IPRouteEntry<A>&, RouteTable<A>*)
{
    XLOG_UNREACHABLE();
    return XORP_ERROR;
}

template<class A>
int
OriginTable<A>::delete_route(const IPNet<A>& net)
{
    debug_msg("OT[%s]: Deleting route %s\n", this->tablename().c_str(),
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
	if (this->next_table() != NULL)
	    this->next_table()->delete_route(found, this);

	// Finally we're done, and can cleanup
	delete found;
	return XORP_OK;
    }
    XLOG_ERROR("OT: attempt to delete a route that doesn't exist: %s",
	       net.str().c_str());
    return XORP_ERROR;
}

template<class A>
int
OriginTable<A>::delete_route(const IPRouteEntry<A>*, RouteTable<A>*)
{
    XLOG_UNREACHABLE();
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
    // Put existing ip_route_table to one side.  The plumbing changes that
    // accompany the creation and plumbing of the deletion table may trigger
    // upstream tables to query whether trie has changed.
    //

    Trie<A, const IPRouteEntry<A>* >* old_ip_route_table = _ip_route_table;
    _ip_route_table = new Trie<A, const IPRouteEntry<A>* >();

    //
    // Pass our entire routing table into a DeletionTable, which will
    // handle the background deletion task.  The DeletionTable will
    // plumb itself in.
    //
    new DeletionTable<A>("Delete(" + this->tablename() + ")",
			 this, old_ip_route_table, _eventloop);
}

template<class A>
const IPRouteEntry<A>*
OriginTable<A>::lookup_route(const IPNet<A>& net) const
{
    debug_msg("------------------\nlookup_route in table %s\n",
	this->tablename().c_str());
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
	this->tablename().c_str());
    debug_msg("OriginTable (%u): Looking up route for addr %s\n",
	      XORP_UINT_CAST(_admin_distance), addr.str().c_str());

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
	      this->tablename().c_str(), addr.str().c_str(),
	      bottom_addr.str().c_str());
    debug_msg("Origin Table: %s returning upper bound for %s of %s\n",
	      this->tablename().c_str(), addr.str().c_str(),
	      top_addr.str().c_str());
    return rr;
}

template<class A>
string
OriginTable<A>::str() const
{
    string s;

    s = "-------\nOriginTable: " + this->tablename() + "\n" +
    	( _protocol_type == IGP ? "IGP\n" : "EGP\n" ) ;
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class OriginTable<IPv4>;
typedef OriginTable<IPv4> IPv4OriginTable;

template class OriginTable<IPv6>;
typedef OriginTable<IPv6> IPv6OriginTable;
