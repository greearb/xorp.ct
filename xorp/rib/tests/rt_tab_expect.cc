// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rt_tab_expect.hh"


template<class A>
ExpectedRouteChange<A>::ExpectedRouteChange(bool add,
					    const IPRouteEntry<A>& route)
    : _add(add),
      _route(route)
{
}

template<class A>
bool
ExpectedRouteChange<A>::matches_add(const IPRouteEntry<A>& route) const
{
    if (!_add)
	return false;
    if (route.net() != _route.net())
	return false;
    IPNextHop<A>* expected_nh = _route.nexthop();
    XLOG_ASSERT(expected_nh != NULL);

    IPNextHop<A>* actual_nh = route.nexthop();
    XLOG_ASSERT(actual_nh != NULL);

    if ((expected_nh->addr()) != (actual_nh->addr()))
	return false;
    return true;
}

template<class A>
bool
ExpectedRouteChange<A>::matches_delete(const IPRouteEntry<A>* route) const
{
    if (_add)
	return false;
    if (route->net() != _route.net())
	return false;
    IPNextHop<A>* expected_nh = _route.nexthop();
    XLOG_ASSERT(expected_nh != NULL);

    IPNextHop<A>* actual_nh = route->nexthop();
    XLOG_ASSERT(actual_nh != NULL);

    if ((expected_nh->addr()) != (actual_nh->addr()))
	return false;
    return true;
}

template<class A>
string
ExpectedRouteChange<A>::str() const
{
    string s;
    if (_add)
	s = "Add of ";
    else
	s = "Delete of ";
    s += _route.str();
    return s;
}


/*--------------------------------------------------------------------*/

template<class A>
ExpectTable<A>::ExpectTable(const string&   tablename,
			       RouteTable<A>*  parent)
    : RouteTable<A>(tablename)
{
    _parent = parent;

    // Plumb ourselves into the table graph
    _parent->set_next_table(this);

    // There's no downstream table
    this->set_next_table(NULL);
}

template<class A>
ExpectTable<A>::~ExpectTable()
{
    XLOG_ASSERT(_expected_route_changes.empty());
}

template<class A>
void
ExpectTable<A>::expect_add(const IPRouteEntry<A>& route)
{
    _expected_route_changes.push_back(ExpectedRouteChange<A>(true, route));
}

template<class A>
void
ExpectTable<A>::expect_delete(const IPRouteEntry<A>& route)
{
    _expected_route_changes.push_back(ExpectedRouteChange<A>(false, route));
}

template<class A>
int
ExpectTable<A>::add_route(const IPRouteEntry<A>& 	route,
			  RouteTable<A>* 		caller)
{
    XLOG_ASSERT(caller == _parent);
    debug_msg("DT[%s]: Adding route %s\n", this->tablename().c_str(),
	      route.str().c_str());
    if (_expected_route_changes.empty()) {
	XLOG_FATAL("ExpectTable: unexpected add_route received");
    }
    if (_expected_route_changes.front().matches_add(route)) {
	_expected_route_changes.pop_front();
	return XORP_OK;
    }
    XLOG_FATAL("ExpectTable: unexpected add_route received. "
	       "Expected: %s; Received: Add of %s",
	       _expected_route_changes.front().str().c_str(),
	       route.str().c_str());
    return XORP_ERROR;
}

template<class A>
int
ExpectTable<A>::delete_route(const IPRouteEntry<A>* 	route,
			  RouteTable<A>* 		caller)
{
    XLOG_ASSERT(caller == _parent);
    debug_msg("DT[%s]: Deleting route %s\n", this->tablename().c_str(),
	      route->str().c_str());
    if (_expected_route_changes.empty()) {
	XLOG_FATAL("ExpectTable: unexpected delete_route received");
    }
    if (_expected_route_changes.front().matches_delete(route)) {
	_expected_route_changes.pop_front();
	return XORP_OK;
    }
    XLOG_FATAL("ExpectTable: unexpected delete_route received. "
	       "Expected: %s; Received: Delete of %s",
	       _expected_route_changes.front().str().c_str(),
	       route->str().c_str());
    return XORP_ERROR;
}

template<class A>
const IPRouteEntry<A>*
ExpectTable<A>::lookup_route(const IPNet<A>& net) const
{
    return _parent->lookup_route(net);
}

template<class A>
const IPRouteEntry<A>*
ExpectTable<A>::lookup_route(const A& addr) const
{
    return _parent->lookup_route(addr);
}

template<class A>
void
ExpectTable<A>::replumb(RouteTable<A>* old_parent,
		       RouteTable<A>* new_parent)
{
    XLOG_ASSERT(_parent == old_parent);
    _parent = new_parent;
}

template<class A>
RouteRange<A>*
ExpectTable<A>::lookup_route_range(const A& addr) const
{
    return _parent->lookup_route_range(addr);
}

template<class A> string
ExpectTable<A>::str() const
{
    string s;
    s = "-------\nExpectTable: " + this->tablename() + "\n";
    s += "parent = " + _parent->tablename() + "\n";
    return s;
}

template class ExpectTable<IPv4>;
template class ExpectTable<IPv6>;
