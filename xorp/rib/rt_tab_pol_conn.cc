// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rt_tab_pol_conn.hh"
#include "rib_varrw.hh"


template <class A>
const string PolicyConnectedTable<A>::table_name = "policy-connected-table";


template <class A>
PolicyConnectedTable<A>::PolicyConnectedTable (RouteTable<A>* parent,
					       PolicyFilters& pfs)
    : RouteTable<A>(table_name), _policy_filters(pfs)
{
    if (parent->next_table()) {
	this->set_next_table(parent->next_table());
    }
    parent->set_next_table(this);
}

template <class A>
PolicyConnectedTable<A>::~PolicyConnectedTable ()
{
    _route_table.delete_all_nodes();
}

template <class A>
void
PolicyConnectedTable<A>::generic_add_route(const IPRouteEntry<A>& route)
{
    debug_msg("[RIB] PolicyConnectedTable ADD ROUTE: %s\n",
	      route.str().c_str());

    // store original
    IPRouteEntry<A>* original = const_cast<IPRouteEntry<A>* >(&route);
    _route_table.insert(original->net(), original);

    do_filtering(*original);
}

template <class A>
int
PolicyConnectedTable<A>::add_igp_route(const IPRouteEntry<A>& route)
{
    this->generic_add_route(route);

    XLOG_ASSERT(this->next_table());

    // Send the possibly modified route down
    return this->next_table()->add_igp_route(route);
}

template <class A>
int
PolicyConnectedTable<A>::add_egp_route(const IPRouteEntry<A>& route)
{
    this->generic_add_route(route);

    XLOG_ASSERT(this->next_table());

    // Send the possibly modified route down
    return this->next_table()->add_egp_route(route);
}

template <class A>
void
PolicyConnectedTable<A>::generic_delete_route(const IPRouteEntry<A>* route)
{
    XLOG_ASSERT(route != NULL);

    debug_msg("[RIB] PolicyConnectedTable DELETE ROUTE: %s\n",
	      route->str().c_str());

    XLOG_ASSERT(_route_table.lookup_node(route->net()) != _route_table.end());

    _route_table.erase(route->net());

    do_filtering(*(const_cast<IPRouteEntry<A>* >(route)));
}

template <class A>
int
PolicyConnectedTable<A>::delete_igp_route(const IPRouteEntry<A>* route)
{
    this->generic_delete_route(route);

    XLOG_ASSERT(this->next_table());
    // propagate the delete
    return this->next_table()->delete_igp_route(route);
}

template <class A>
int
PolicyConnectedTable<A>::delete_egp_route(const IPRouteEntry<A>* route)
{
    this->generic_delete_route(route);

    XLOG_ASSERT(this->next_table());
    // propagate the delete
    return this->next_table()->delete_egp_route(route);
}

template <class A>
string
PolicyConnectedTable<A>::str() const
{
    ostringstream oss;
    oss << "------" << endl;
    oss << "PolicyConnectedTable" << endl;
    if (this->next_table())
	oss << "next table: " << this->next_table()->tablename() << endl;
    else
	oss << "no next table" << endl;
    return oss.str();
}

template <class A>
void
PolicyConnectedTable<A>::push_routes()
{
    debug_msg("[RIB] PolicyConnectedTable PUSH ROUTES\n");

    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    // XXX: not a background task
    // go through original routes and refilter them
    for (typename RouteContainer::iterator i = _route_table.begin();
	i != _route_table.end(); ++i) {

	IPRouteEntry<A>* prev = *i;

	do_filtering(*prev);

	// only policytags may change
	next->replace_policytags(*prev, prev->policytags());
    }
}


template <class A>
void
PolicyConnectedTable<A>::do_filtering(IPRouteEntry<A>& route)
{
    try {
	debug_msg("[RIB] PolicyConnectedTable Filtering: %s\n",
		  route.str().c_str());

	RIBVarRW<A> varrw(route);

	// only source match filtering!
	_policy_filters.run_filter(filter::EXPORT_SOURCEMATCH, varrw);

    } catch(const PolicyException& e) {
	XLOG_FATAL("PolicyException: %s", e.str().c_str());
	XLOG_UNFINISHED();
    }
}


template class PolicyConnectedTable<IPv4>;
template class PolicyConnectedTable<IPv6>;
