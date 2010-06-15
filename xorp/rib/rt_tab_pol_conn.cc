// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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
    : RouteTable<A>(table_name), _parent(parent), _policy_filters(pfs)
{
    if (_parent->next_table()) {
	this->set_next_table(_parent->next_table());

	this->next_table()->replumb(_parent, this);
    }
    _parent->set_next_table(this);
}

template <class A>
PolicyConnectedTable<A>::~PolicyConnectedTable ()
{
    for (typename RouteContainer::iterator i = _route_table.begin();
	i != _route_table.end(); ++i) {
	delete i.payload();
    }

    _route_table.delete_all_nodes();
}

template <class A>
int
PolicyConnectedTable<A>::add_route(const IPRouteEntry<A>& route, 
				RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    debug_msg("[RIB] PolicyConnectedTable ADD ROUTE: %s\n", 
	      route.str().c_str());


    // store original
    IPRouteEntry<A>* original = new IPRouteEntry<A>(route);
    _route_table.insert(original->net(), original);

    // make a copy so we may modify it
    IPRouteEntry<A> route_copy(*original);
    do_filtering(route_copy); 
   
    
    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    // Send the possibly modified route down
    return next->add_route(route_copy, this);
}			

template <class A>
int 
PolicyConnectedTable<A>::delete_route(const IPRouteEntry<A>* route, 
				   RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);
    XLOG_ASSERT(route != NULL);

    debug_msg("[RIB] PolicyConnectedTable DELETE ROUTE: %s\n",
	      route->str().c_str());

    // delete our copy
    typename RouteContainer::iterator i;
    i = _route_table.lookup_node(route->net());

    XLOG_ASSERT(i != _route_table.end());

    const IPRouteEntry<A>* re = i.payload();
    _route_table.erase(route->net());
    delete re;

    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    // make a copy so we may modify it (e.g., by setting its policy tags)
    IPRouteEntry<A> route_copy(*route);
    do_filtering(route_copy); 

    // propagate the delete
    return next->delete_route(&route_copy, this);
}

template <class A>
const IPRouteEntry<A>* 
PolicyConnectedTable<A>::lookup_route(const IPNet<A>& net) const
{
    XLOG_ASSERT(_parent);

    typename RouteContainer::iterator i;
    i = _route_table.lookup_node(net);

    // check if we have route [we should have same routes as origin table].
    if (i == _route_table.end())
	return _parent->lookup_route(net); // should return null probably
    
    return i.payload();
}


template <class A>
const IPRouteEntry<A>* 
PolicyConnectedTable<A>::lookup_route(const A& addr) const
{
    XLOG_ASSERT(_parent);

    typename RouteContainer::iterator i;
    i = _route_table.find(addr);

    // same as above
    if (i == _route_table.end())
	return _parent->lookup_route(addr); // return null

    return i.payload();
}


template <class A>
RouteRange<A>* 
PolicyConnectedTable<A>::lookup_route_range(const A& addr) const
{
    XLOG_ASSERT(_parent);

    // XXX: no policy tags in ranges for now
    return _parent->lookup_route_range(addr);
}


template <class A>
void
PolicyConnectedTable<A>::replumb(RouteTable<A>* old_parent, 
			      RouteTable<A>* new_parent)
{
    XLOG_ASSERT(old_parent == _parent);

    _parent = new_parent;
}			      


template <class A>
string
PolicyConnectedTable<A>::str() const
{
    return "not implement yet";
}

template <class A>
void
PolicyConnectedTable<A>::push_routes()
{
    debug_msg("[RIB] PolicyConnectedTable PUSH ROUTES\n");
    
    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    vector<IPRouteEntry<A>*> new_routes;

    // XXX: not a background task
    // go through original routes and refilter them
    for (typename RouteContainer::iterator i = _route_table.begin();
	i != _route_table.end(); ++i) {

	const IPRouteEntry<A>* prev = i.payload();

	// make a copy so filter may [possibly] modify it
	const IPRouteEntry<A>* orig = _parent->lookup_route(prev->net());
	IPRouteEntry<A>* copy = new IPRouteEntry<A>(*orig);

	do_filtering(*copy);

	// only policytags may change
	next->replace_policytags(*copy, prev->policytags(), this);

	// delete old route
	delete prev;

	// keep the new route so we may re-store it after this iteration
	new_routes.push_back(copy);
    }

    // store all new routes
    for (typename vector<IPRouteEntry<A>*>::iterator i = new_routes.begin();
	i != new_routes.end(); ++i) {

	// replace route
	IPRouteEntry<A>* route = *i;
	_route_table.erase(route->net());
	_route_table.insert(route->net(), route);
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
