// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/rib/rt_tab_pol_conn.hh,v 1.2 2004/09/17 20:02:27 pavlin Exp $

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
