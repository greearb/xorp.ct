// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rib/rt_tab_pol_conn.cc,v 1.1 2004/09/17 14:00:04 abittau Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "rib_module.h"
#include "rt_tab_pol_conn.hh"
#include "rib_varrw.hh"

#include <sstream>



template <class A>
const string PolicyConnectedTable<A>::table_name = "policy-connected-table";


template <class A>
PolicyConnectedTable<A>::PolicyConnectedTable (RouteTable<A>* parent,
					       PolicyFilters& pfs) :
    RouteTable<A>(table_name), _parent(parent), _policy_filters(pfs)  {
    
    if (_parent->next_table()) {
        set_next_table(_parent->next_table());

        this->next_table()->replumb(_parent, this);
    }
    _parent->set_next_table(this);
}

template <class A>
PolicyConnectedTable<A>::~PolicyConnectedTable () {
    for(typename RouteContainer::iterator i = _route_table.begin();
	i != _route_table.end(); ++i) {
	delete i.payload();
    }

    _route_table.delete_all_nodes();
}

template <class A>
int
PolicyConnectedTable<A>::add_route(const IPRouteEntry<A>& route, 
				RouteTable<A>* caller) {

    XLOG_ASSERT(caller == _parent);

    debug_msg("[RIB] PolicyConnectedTable ADD ROUTE: %s\n", 
	      route.str().c_str());


    // store original
    IPRouteEntry<A>* original = new IPRouteEntry<A>(route);
    _route_table.insert(original->net(),original);

    // make a copy so we may modify it
    IPRouteEntry<A> route_copy(*original);
    do_filtering(route_copy); 
   
    
    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    // send the possibly modified route down.
    return next->add_route(route_copy,this);

}				

template <class A>
int 
PolicyConnectedTable<A>::delete_route(const IPRouteEntry<A>* route, 
				   RouteTable<A>* caller) {
    
    XLOG_ASSERT(caller == _parent);
    XLOG_ASSERT(route);

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

    // propagate the delete
    return next->delete_route(route,this);
}

template <class A>
const IPRouteEntry<A>* 
PolicyConnectedTable<A>::lookup_route(const IPNet<A>& net) const {
    XLOG_ASSERT(_parent);

    typename RouteContainer::iterator i;
    i = _route_table.lookup_node(net);

    // check if we have route [we should have same routes as origin table].
    if(i == _route_table.end());
	return _parent->lookup_route(net); // should return null probably
    
    return i.payload();
}


template <class A>
const IPRouteEntry<A>* 
PolicyConnectedTable<A>::lookup_route(const A& addr) const {
    XLOG_ASSERT(_parent);

    typename RouteContainer::iterator i;
    i = _route_table.find(addr);

    // same as above
    if(i == _route_table.end())
	return _parent->lookup_route(addr); // return null

    return i.payload();
}


template <class A>
RouteRange<A>* 
PolicyConnectedTable<A>::lookup_route_range(const A& addr) const {
    XLOG_ASSERT(_parent);

    // XXX: no policy tags in ranges for now
    return _parent->lookup_route_range(addr);
}


template <class A>
void
PolicyConnectedTable<A>::replumb(RouteTable<A>* old_parent, 
			      RouteTable<A>* new_parent) {
    XLOG_ASSERT(old_parent == _parent);

    _parent = new_parent;

}			      


template <class A>
string
PolicyConnectedTable<A>::str() const {
    return "not implement yet";
}

template <class A>
void
PolicyConnectedTable<A>::push_routes() {
    debug_msg("[RIB] PolicyConnectedTable PUSH ROUTES\n");

    
    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    vector<IPRouteEntry<A>*> new_routes;

    // XXX: not a background task
    // go through original routes and refilter them
    for(typename RouteContainer::iterator i = _route_table.begin();
	i != _route_table.end(); ++i) {
   
	const IPRouteEntry<A>* prev = i.payload();

	// make a copy so filter may [possibly] modify it
	IPRouteEntry<A>* copy = new IPRouteEntry<A>(*prev);

	do_filtering(*copy);
	
	
	// only policytags may change
	next->replace_policytags(*copy,prev->policytags(),this);

	// delete old route
	delete prev;

	// keep the new route so we may re-store it after this iteration
	new_routes.push_back(copy);
    }

    // store all new routes
    for(typename vector<IPRouteEntry<A>*>::iterator i = new_routes.begin();
	i != new_routes.end(); ++i) {

	// replace route
	IPRouteEntry<A>* route = *i;
	_route_table.erase(route->net());
	_route_table.insert(route->net(),route);
    }	

}


template <class A>
void
PolicyConnectedTable<A>::do_filtering(IPRouteEntry<A>& route) {

try {
    debug_msg("[RIB] PolicyConnectedTable Filtering: %s\n",
	      route.str().c_str());
    
    RIBVarRW<A> varrw(route);

    // only source match filtering!
    ostringstream trace;
    _policy_filters.run_filter(filter::EXPORT_SOURCEMATCH,
			       varrw, &trace);
 
    debug_msg("[RIB] Filter trace:\n%s\n[RIB] end of trace.\n",
	      trace.str().c_str());
  
} catch(const PolicyException& e) {
    XLOG_FATAL("PolicyException: %s",e.str().c_str());
    abort(); // FIXME
}

}




template class PolicyConnectedTable<IPv4>;
template class PolicyConnectedTable<IPv6>;
