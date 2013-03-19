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
#include "libxorp/eventloop.hh"

#include "rt_tab_deletion.hh"


//
// A = Address Type. E.g., IPv4 or IPv6
//
template<class A>
DeletionTable<A>::DeletionTable(const string& tablename,
				RouteTable<A>* parent,
				RouteTrie* ip_route_trie,
				EventLoop& eventloop)
    : RouteTable<A>(tablename),
      _parent(parent),
      _eventloop(eventloop),
      _ip_route_table(ip_route_trie)
{
    XLOG_ASSERT(_parent != NULL);
    this->set_next_table(_parent->next_table());
    parent->set_next_table(this);
}

template<class A>
DeletionTable<A>::~DeletionTable()
{
    // Delete all the routes in the trie.
    delete_all_routes();
    delete _ip_route_table;
}

template<class A>
int
DeletionTable<A>::add_igp_route(const IPRouteEntry<A>& route)
{
    typename RouteTrie::iterator iter;
    iter = _ip_route_table->lookup_node(route.net());
    if (iter != _ip_route_table->end()) {
	//
	// We got an add route for a route that was waiting to be
	// deleted.  Process this now - pass the deletion downstream,
	// remove the route from our trie, then pass the new route
	// downstream.
	//
	const IPRouteEntry<A>* our_route = *iter;
	_ip_route_table->erase(route.net());
	this->next_table()->delete_igp_route(our_route, true);
	delete our_route;
    }

    return this->next_table()->add_igp_route(route);
}

template<class A>
int
DeletionTable<A>::add_egp_route(const IPRouteEntry<A>& route)
{
    typename RouteTrie::iterator iter;
    iter = _ip_route_table->lookup_node(route.net());
    if (iter != _ip_route_table->end()) {
	//
	// We got an add route for a route that was waiting to be
	// deleted.  Process this now - pass the deletion downstream,
	// remove the route from our trie, then pass the new route
	// downstream.
	//
	const IPRouteEntry<A>* our_route = *iter;
	_ip_route_table->erase(route.net());
	this->next_table()->delete_egp_route(our_route, true);
	delete our_route;
    }

    return this->next_table()->add_egp_route(route);
}

template<class A>
int
DeletionTable<A>::delete_igp_route(const IPRouteEntry<A>* route, bool b)
{
    // The route MUST NOT be in our trie.
    XLOG_ASSERT(_ip_route_table->lookup_node(route->net()) == _ip_route_table->end());

    return this->next_table()->delete_igp_route(route, b);
}

template<class A>
int
DeletionTable<A>::delete_egp_route(const IPRouteEntry<A>* route, bool b)
{
    // The route MUST NOT be in our trie.
    XLOG_ASSERT(_ip_route_table->lookup_node(route->net()) == _ip_route_table->end());

    return this->next_table()->delete_egp_route(route, b);
}

template<class A>
void
DeletionTable<A>::delete_all_routes()
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
DeletionTable<A>::background_deletion_pass()
{
    if (_ip_route_table->begin() == _ip_route_table->end()) {
	unplumb_self();
	return;
    }

    typename RouteTrie::iterator iter;
    iter = _ip_route_table->begin();
    const IPRouteEntry<A>* our_route = *iter;
    _ip_route_table->erase(our_route->net());
    this->generic_delete_route(our_route);
    delete our_route;

    this->set_background_timer();
}

template<class A>
void
DeletionTable<A>::unplumb_self()
{
    _parent->set_next_table(this->next_table());
    delete this;
}

template<class A>
void
DeletionTable<A>::set_parent(RouteTable<A>* new_parent)
{
    _parent = new_parent;
}

template<class A>
string
DeletionTable<A>::str() const
{
    string s;

    s = "-------\nDeletionTable: " + this->tablename() + "\n";
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class DeletionTable<IPv4>;
typedef DeletionTable<IPv4> IPv4DeletionTable;

template class DeletionTable<IPv6>;
typedef DeletionTable<IPv6> IPv6DeletionTable;
