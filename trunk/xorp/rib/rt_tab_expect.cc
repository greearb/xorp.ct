// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rib/rt_tab_expect.cc,v 1.8 2004/06/10 22:41:40 hodson Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"

#include "rt_tab_expect.hh"


template <typename A>
class ExpectedRouteChange {
public:
    ExpectedRouteChange(bool add, const IPRouteEntry<A>& route);
    bool matches_add(const IPRouteEntry<A>& route) const;
    bool matches_delete(const IPRouteEntry<A>* route) const;
    string str() const;

private:
    bool		_add; 			// true = add, false = delete
    IPRouteEntry<A>	_route;
};

template<class A>
ExpectedRouteChange<A>::ExpectedRouteChange<A>(bool add,
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
    IPNextHop<A>* expected_nh = dynamic_cast<IPNextHop<A>* >(_route.nexthop());
    XLOG_ASSERT(expected_nh != NULL);

    IPNextHop<A>* actual_nh = dynamic_cast<IPNextHop<A>* >(route.nexthop());
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
    IPNextHop<A>* expected_nh = dynamic_cast<IPNextHop<A>* >(_route.nexthop());
    XLOG_ASSERT(expected_nh != NULL);

    IPNextHop<A>* actual_nh = dynamic_cast<IPNextHop<A>* >(route->nexthop());
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
