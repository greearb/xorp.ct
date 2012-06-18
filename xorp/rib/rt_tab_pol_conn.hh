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

// $XORP: xorp/rib/rt_tab_pol_conn.hh,v 1.9 2008/10/02 21:58:13 bms Exp $

#ifndef __RIB_RT_TAB_POL_CONN_HH__
#define __RIB_RT_TAB_POL_CONN_HH__

#include "rt_tab_base.hh"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#include "policy/backend/policy_filters.hh"

/**
 * @short This table will filter connected routes.
 *
 * This table stores all routes it receives from the origin table.
 * It then filters them modifying only policytags.
 * It has the ability to push routes through the filter again, causing only
 * policytags on routes to be updated [no route deletion / replacement].
 */
template<class A>
class PolicyConnectedTable : public RouteTable<A> {
public:
    static const string table_name;

    /**
     * @param parent parent table.
     * @param pfs the connected routes policy filters.
     */
    PolicyConnectedTable(RouteTable<A>* parent, PolicyFilters& pfs);
    ~PolicyConnectedTable();

    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* route, RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    TableType type() const { return POLICY_CONNECTED_TABLE; }
    const RouteTable<A>* parent() const { return _parent; }
    RouteTable<A>* parent() { return _parent; }
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;

    /**
     * Push all the routes through the filter again
     */
    void push_routes();

private:
    /**
     * Route may be modified [its policy tags].
     * No need to check for route being accepted / rejected -- it is always
     * accepted [only source match filtering].
     *
     * @param r route to filter.
     */
    void do_filtering(IPRouteEntry<A>& r);


    typedef Trie<A, const IPRouteEntry<A>*> RouteContainer;


    RouteTable<A>*	_parent;

    RouteContainer	_route_table;	// Copy of routes
					// we have this so we may push routes.

    PolicyFilters&	_policy_filters; // Reference to connected route filters.
};

#endif // __RIB_RT_TAB_POL_CONN_HH__
