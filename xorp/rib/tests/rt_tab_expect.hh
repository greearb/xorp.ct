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

// $XORP: xorp/rib/rt_tab_expect.hh,v 1.12 2008/10/02 21:58:12 bms Exp $

#ifndef __RIB_RT_TAB_EXPECT_HH__
#define __RIB_RT_TAB_EXPECT_HH__

#include "rt_tab_base.hh"

template<class A> class ExpectedRouteChange;

/**
 * @short A Route Table for comparing route updates received against
 * expected.
 *
 * Users of this class specify expected updates with @ref expect_add
 * and @ref expect_delete.  As the updates come through they are
 * compared against those expect and generate an assertion failure if
 * those arriving do not match those expected.  An assertion failure
 * will also occur if there are unmatched updates upon instance
 * destruction.
 *
 * This class is strictly for debugging and testing purposes.
 */
template<class A>
class ExpectTable : public RouteTable<A> {
public:
    ExpectTable(const string& tablename, RouteTable<A>* parent);
    ~ExpectTable();

    const list<ExpectedRouteChange<A> >& expected_route_changes() const {
	return _expected_route_changes;
    }

    void expect_add(const IPRouteEntry<A>& route);
    void expect_delete(const IPRouteEntry<A>& route);
    int add_route(const IPRouteEntry<A>& route);
    int delete_route(const IPRouteEntry<A>* route);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    TableType type() const { return EXPECT_TABLE; }
    RouteTable<A>* parent() { return _parent; }
    const RouteTable<A>* parent() const { return _parent; }
    void set_parent(RouteTable<A>* new_parent);
    string str() const;

private:
    RouteTable<A>*			_parent;
    list<ExpectedRouteChange<A> >	_expected_route_changes;
};

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

#endif // __RIB_RT_TAB_EXPECT_HH__
