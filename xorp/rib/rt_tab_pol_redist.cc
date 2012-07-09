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

#include "rt_tab_pol_redist.hh"


template <class A>
const string PolicyRedistTable<A>::table_name = "policy-redist-table";


template <class A>
PolicyRedistTable<A>::PolicyRedistTable(RouteTable<A>* parent, XrlRouter& rtr,
					PolicyRedistMap& rmap,
					bool multicast)
    : RouteTable<A>(table_name),
      _parent(parent),
      _xrl_router(rtr),
      _eventloop(_xrl_router.eventloop()),
      _redist_map(rmap),
      _redist_client(&_xrl_router),
      _multicast(multicast)
{
    if (_parent->next_table() != NULL) {
        this->set_next_table(_parent->next_table());
        this->next_table()->set_parent(this);
    }
    _parent->set_next_table(this);
}

template <class A>
int
PolicyRedistTable<A>::add_route(const IPRouteEntry<A>& route)
{
    debug_msg("[RIB] PolicyRedistTable ADD ROUTE: %s\n",
	      route.str().c_str());

    // get protocols involved in redistribution with these tags
    set<string> protos;
    _redist_map.get_protocols(protos, route.policytags());

    // if there are any, then redistribute
    if (!protos.empty())
	add_redist(route, protos);

    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next != NULL);

    return next->add_route(route);
}

template <class A>
int
PolicyRedistTable<A>::delete_route(const IPRouteEntry<A>* route)
{
    XLOG_ASSERT(route != NULL);

    debug_msg("[RIB] PolicyRedistTable DELETE ROUTE: %s\n",
	      route->str().c_str());

    // get protocols involved in redistribution of this route
    set<string> protos;
    _redist_map.get_protocols(protos, route->policytags());

    // if there are any, stop redistributing
    if (!protos.empty())
	del_redist(*route, protos);

    RouteTable<A>* next = this->next_table();
    XLOG_ASSERT(next != NULL);

    return next->delete_route(route);
}

template <class A>
const IPRouteEntry<A>*
PolicyRedistTable<A>::lookup_route(const IPNet<A>& net) const
{
    XLOG_ASSERT(_parent != NULL);

    return _parent->lookup_route(net);
}


template <class A>
const IPRouteEntry<A>*
PolicyRedistTable<A>::lookup_route(const A& addr) const
{
    XLOG_ASSERT(_parent != NULL);

    return _parent->lookup_route(addr);
}


template <class A>
RouteRange<A>*
PolicyRedistTable<A>::lookup_route_range(const A& addr) const
{
    XLOG_ASSERT(_parent != NULL);

    return _parent->lookup_route_range(addr);
}


template <class A>
void
PolicyRedistTable<A>::set_parent(RouteTable<A>* new_parent)
{
    _parent = new_parent;
}


template <class A>
string
PolicyRedistTable<A>::str() const
{
    ostringstream oss;
    oss << "------" << endl;
    oss << "PolicyRedistTable" << endl;
    oss << "parent: " << const_cast<PolicyRedistTable<A>* >(this)->parent()->tablename() << endl;
    if (this->next_table())
	oss << "next table: " << this->next_table()->tablename() << endl;
    else
	oss << "no next table" << endl;
    return oss.str();
}


template <class A>
void
PolicyRedistTable<A>::add_redist(const IPRouteEntry<A>& route,
				 const Set& protos)
{
    // send a redistribution request for all protocols in the set.
    for (Set::const_iterator i = protos.begin(); i != protos.end(); ++i)
	add_redist(route, *i);
}


template <class A>
void
PolicyRedistTable<A>::del_redist(const IPRouteEntry<A>& route,
				 const Set& protos)
{
    // stop redistribution for all protocols in set.
    for (Set::const_iterator i = protos.begin(); i != protos.end(); ++i)
	del_redist(route, *i);
}


template <class A>
void
PolicyRedistTable<A>::xrl_cb(const XrlError& e, string action) {
    UNUSED(action);
    if (e != XrlError::OKAY()) {
	XLOG_WARNING("Unable to complete XRL: %s", action.c_str());
    }
}

template <class A>
void
PolicyRedistTable<A>::add_redist(const IPRouteEntry<A>& route,
				    const string& proto)
{
    string error = "add_route for " + A::ip_version_str() + " " + proto + " route: " + route.str();

    debug_msg("[RIB] PolicyRedistTable add_redist %s %s to %s\n",
	A::ip_version_str().c_str(), route.str().c_str(), proto.c_str());

    _redist_client.send_add_route(proto.c_str(), route.net(),
				    !_multicast, _multicast, // XXX
				    route.nexthop_addr(), route.metric(),
				    route.policytags().xrl_atomlist(),
				    callback(this, &PolicyRedistTable<A>::xrl_cb, error));
}


template <class A>
void
PolicyRedistTable<A>::del_redist(const IPRouteEntry<A>& route,
				    const string& proto)
{
    string error = "del_route for " + A::ip_version_str() + " " + proto + " route: " + route.str();

    debug_msg("[RIB] PolicyRedistTable del_redist %s %s to %s\n",
	A::ip_version_str().c_str(), route.str().c_str(), proto.c_str());

    _redist_client.send_delete_route(proto.c_str(), route.net(),
				       !_multicast, _multicast,	// XXX
				       callback(this, &PolicyRedistTable<A>::xrl_cb, error));
}


template <class A>
void
PolicyRedistTable<A>::replace_policytags(const IPRouteEntry<A>& route,
					 const PolicyTags& prevtags,
					 RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    set<string> del_protos;
    set<string> add_protos;

    // who doesn't have to redistribute this route anymore ?
    _redist_map.get_protocols(del_protos, prevtags);

    // who has to redistribute this route ?
    _redist_map.get_protocols(add_protos, route.policytags());

    // ok since it is "tags only change and nothing else"
    // we can be smart i.e. weed out the intersection of del/add
    // but not implement yet.

    // commit changes
    if (!del_protos.empty())
        del_redist(route, del_protos);
    if (!add_protos.empty())
	add_redist(route, add_protos);
}


template class PolicyRedistTable<IPv4>;
template class PolicyRedistTable<IPv6>;
