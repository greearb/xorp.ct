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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "rib_module.h"
#include "rt_tab_pol_redist.hh"



template <class A>
const string PolicyRedistTable<A>::table_name = "policy-redist-table";


template <class A>
PolicyRedistTable<A>::PolicyRedistTable (RouteTable<A>* parent, XrlRouter& rtr,
					 PolicyRedistMap& rmap,
					 bool multicast) :
    RouteTable<A>(table_name), _parent(parent), _xrl_router(rtr), 
    _eventloop(_xrl_router.eventloop()),
    _redist_map(rmap),
    _redist4_client(&_xrl_router),
    _redist6_client(&_xrl_router),
    _multicast(multicast){

    if (_parent->next_table()) {
        set_next_table(_parent->next_table());

        this->next_table()->replumb(_parent, this);
    }
    _parent->set_next_table(this);
}

template <class A>
int
PolicyRedistTable<A>::add_route(const IPRouteEntry<A>& route, 
				RouteTable<A>* caller) {

    XLOG_ASSERT(caller == _parent);

    debug_msg("[RIB] PolicyRedistTable ADD ROUTE: %s\n",
	      route.str().c_str());

    // get protocols involved in redistribution with these tags
    set<string> protos;
    _redist_map.get_protocols(protos,route.policytags());

    // if there are any, then redistribute
    if(!protos.empty())
	add_redist(route,protos);

    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    return next->add_route(route,this);
}				

template <class A>
int 
PolicyRedistTable<A>::delete_route(const IPRouteEntry<A>* route, 
				   RouteTable<A>* caller) {
    
    XLOG_ASSERT(caller == _parent);
    XLOG_ASSERT(route);

    debug_msg("[RIB] PolicyRedistTable DELETE ROUTE: %s\n",
	      route->str().c_str());

    // get protocols involved in redistribution of this route
    set<string> protos;
    _redist_map.get_protocols(protos,route->policytags());

    // if there are any, stop redistributing
    if(!protos.empty())
	del_redist(*route,protos);

    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next);

    return next->delete_route(route,this);
}

template <class A>
const IPRouteEntry<A>* 
PolicyRedistTable<A>::lookup_route(const IPNet<A>& net) const {
    XLOG_ASSERT(_parent);

    return _parent->lookup_route(net);
}


template <class A>
const IPRouteEntry<A>* 
PolicyRedistTable<A>::lookup_route(const A& addr) const {
    XLOG_ASSERT(_parent);

    return _parent->lookup_route(addr);
}


template <class A>
RouteRange<A>* 
PolicyRedistTable<A>::lookup_route_range(const A& addr) const {
    XLOG_ASSERT(_parent);

    return _parent->lookup_route_range(addr);
}


template <class A>
void
PolicyRedistTable<A>::replumb(RouteTable<A>* old_parent, 
			      RouteTable<A>* new_parent) {
    XLOG_ASSERT(old_parent == _parent);

    _parent = new_parent;

}			      


template <class A>
string
PolicyRedistTable<A>::str() const {
    return "not implement yet";
}


template <class A>
void
PolicyRedistTable<A>::add_redist(const IPRouteEntry<A>& route, 
				 const Set& protos) {
    
    // send a redistribution request for all protocols in the set.
    for(Set::iterator i = protos.begin(); i != protos.end(); ++i)
	add_redist(route,*i);
}


template <class A>
void
PolicyRedistTable<A>::del_redist(const IPRouteEntry<A>& route,
				 const Set& protos) {
    // stop redistribution for all protocols in set.
    for(Set::iterator i = protos.begin(); i != protos.end(); ++i)
	del_redist(route,*i);

}


template <class A>
void
PolicyRedistTable<A>::xrl_cb(const XrlError& e, string action) {
    if( e != XrlError::OKAY()) {
	XLOG_WARNING("Unable to complete XRL: %s", action.c_str());
    }
}			     

template <>
void
PolicyRedistTable<IPv4>::add_redist(const IPRouteEntry<IPv4>& route,
				    const string& proto) {

    string error = "add_route4 for " + proto + " route: " + route.str();

    debug_msg("[RIB] PolicyRedistTable add_redist IPv4 %s to %s\n",
	      route.str().c_str(),proto.c_str());

    _redist4_client.send_add_route4(proto.c_str(), route.net(),
				    !_multicast,_multicast, // XXX
				    route.nexthop_addr(), route.metric(),
				    route.policytags().xrl_atomlist(),
				    callback(this,&PolicyRedistTable<IPv4>::xrl_cb,error));
}


template <>
void
PolicyRedistTable<IPv4>::del_redist(const IPRouteEntry<IPv4>& route,
				    const string& proto) {
    
    string error = "del_route4 for " + proto + " route: " + route.str();

    debug_msg("[RIB] PolicyRedistTable del_redist IPv4 %s to %s\n",
	      route.str().c_str(),proto.c_str());

    _redist4_client.send_delete_route4(proto.c_str(), route.net(),
				       !_multicast,_multicast,	// XXX
				       callback(this,&PolicyRedistTable<IPv4>::xrl_cb,error));
}

template <>
void
PolicyRedistTable<IPv6>::add_redist(const IPRouteEntry<IPv6>& route,
				    const string& proto) {

    string error = "add_route6 for " + proto + " route: " + route.str();

    debug_msg("[RIB] PolicyRedistTable add_redist IPv6 %s to %s\n",
	      route.str().c_str(),proto.c_str());

    _redist6_client.send_add_route6(proto.c_str(), route.net(),
				    !_multicast,_multicast, // XXX: mutex ?
				    route.nexthop_addr(), route.metric(),
				    route.policytags().xrl_atomlist(),
				    callback(this,&PolicyRedistTable<IPv6>::xrl_cb,error));
}


template <>
void
PolicyRedistTable<IPv6>::del_redist(const IPRouteEntry<IPv6>& route,
				    const string& proto) {
    
    string error = "del_route6 for " + proto + " route: " + route.str();

    debug_msg("[RIB] PolicyRedistTable del_redist IPv6 %s to %s\n",
	      route.str().c_str(),proto.c_str());

    _redist6_client.send_delete_route6(proto.c_str(), route.net(),
				       !_multicast,_multicast, // XXX: mutex ?
				       callback(this,&PolicyRedistTable<IPv6>::xrl_cb,error));
}


template <class A>
void
PolicyRedistTable<A>::replace_policytags(const IPRouteEntry<A>& route,
					 const PolicyTags& prevtags,
					 RouteTable<A>* caller) {
    XLOG_ASSERT(caller == _parent);

    set<string> del_protos;
    set<string> add_protos;

    // who doesn't have to redistribute this route anymore ?
    _redist_map.get_protocols(del_protos,prevtags);

    // who has to redistribute this route ?
    _redist_map.get_protocols(add_protos,route.policytags());

    // ok since it is "tags only change and nothing else"
    // we can be smart i.e. weed out the intersection of del/add
    // but not implement yet.

    // commit changes
    if(!del_protos.empty())
        del_redist(route,del_protos);
    if(!add_protos.empty())
	add_redist(route,add_protos);
}					 




template class PolicyRedistTable<IPv4>;
template class PolicyRedistTable<IPv6>;
