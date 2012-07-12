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

#include "protocol.hh"

#include "rt_tab_origin.hh"
#include "rt_tab_deletion.hh"


//
// A = Address Type. E.g., IPv4 or IPv6
//
template<class A>
OriginTable<A>::OriginTable(const string&	tablename,
			    uint16_t		admin_distance,
			    EventLoop&		eventloop)
    : RouteTable<A>(tablename),
      _admin_distance(admin_distance),
      _eventloop(eventloop),
      _gen(0)
{
    XLOG_ASSERT(admin_distance <= 255);

    _ip_route_table = new RouteTrie();
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
OriginTable<A>::add_route(IPRouteEntry<A>* route)
{
    debug_msg("OT[%s]: Adding route %s\n", this->tablename().c_str(),
	    route->str().c_str());

    if (lookup_ip_route(route->net()) != NULL) {
	delete (route);
	return XORP_ERROR;
    }

    route->set_admin_distance(_admin_distance);

    _ip_route_table->insert(route->net(), route);

    // Propagate to next table
    XLOG_ASSERT(this->next_table() != NULL);
    this->generic_add_route(*route);

    return XORP_OK;
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

    typename RouteTrie::iterator iter;
    iter = _ip_route_table->lookup_node(net);
    if (iter != _ip_route_table->end()) {
	const IPRouteEntry<A>* found = *iter;
	_ip_route_table->erase(net);
	// Propagate to next table
	XLOG_ASSERT(this->next_table() != NULL);
	this->generic_delete_route(found);

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
    typename RouteTrie::iterator iter;
    for (iter = _ip_route_table->begin();
	 iter != _ip_route_table->end();
	 ++iter) {
	delete *iter;
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

    RouteTrie* old_ip_route_table = _ip_route_table;
    _ip_route_table = new RouteTrie();

    //
    // Pass our entire routing table into a DeletionTable, which will
    // handle the background deletion task.  The DeletionTable will
    // plumb itself in.
    //
    this->allocate_deletion_table(old_ip_route_table);
}

template<class A>
const IPRouteEntry<A>*
OriginTable<A>::lookup_ip_route(const IPNet<A>& net) const
{
    debug_msg("------------------\nlookup_route in table %s\n",
	this->tablename().c_str());
    debug_msg("OriginTable: Looking up route %s\n", net.str().c_str());
    typename RouteTrie::iterator iter = _ip_route_table->lookup_node(net);
    return (iter == _ip_route_table->end()) ? NULL : *iter;
}

template<class A>
const IPRouteEntry<A>*
OriginTable<A>::lookup_ip_route(const A& addr) const
{
    debug_msg("------------------\nlookup_route in table %s\n",
	this->tablename().c_str());
    debug_msg("OriginTable (%u): Looking up route for addr %s\n",
	      XORP_UINT_CAST(_admin_distance), addr.str().c_str());

    typename RouteTrie::iterator iter = _ip_route_table->find(addr);

    return (iter == _ip_route_table->end()) ? NULL : *iter;
}

template<class A>
string
OriginTable<A>::str() const
{
    string s;

    s = "-------\nOriginTable: " + this->tablename() + "\n" +
	( this->protocol_type() == IGP ? "IGP\n" : "EGP\n" ) ;
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template <class A>
void
TypedOriginTable<A, IGP>::allocate_deletion_table(typename OriginTable<A>::RouteTrie* ip_route_trie)
{
    new TypedDeletionTable<A, IGP>("Delete(" + this->tablename() + ")", this,
	    ip_route_trie, this->_eventloop);
}

template <class A>
void
TypedOriginTable<A, EGP>::allocate_deletion_table(typename OriginTable<A>::RouteTrie* ip_route_trie)
{
    new TypedDeletionTable<A, EGP>("Delete(" + this->tablename() + ")", this,
	    ip_route_trie, this->_eventloop);
}

template class OriginTable<IPv4>;
typedef OriginTable<IPv4> IPv4OriginTable;

template class OriginTable<IPv6>;
typedef OriginTable<IPv6> IPv6OriginTable;

template class TypedOriginTable<IPv4, IGP>;
template class TypedOriginTable<IPv4, EGP>;
template class TypedOriginTable<IPv6, IGP>;
template class TypedOriginTable<IPv6, EGP>;
