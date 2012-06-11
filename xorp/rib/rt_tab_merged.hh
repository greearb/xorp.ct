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

// $XORP: xorp/rib/rt_tab_merged.hh,v 1.15 2008/10/02 21:58:12 bms Exp $

#ifndef __RIB_RT_TAB_MERGED_HH__
#define __RIB_RT_TAB_MERGED_HH__

#include "rt_tab_base.hh"


/**
 * @short Make two route @ref RouteTables behave like one
 *
 * MergedTable takes two routing tables and combines them together to
 * form a single table, where routes for the same subnet with a lower
 * admin distance override those with a higher admin distance.
 *
 * The two parent tables are not actually merged.
 *
 * An add_route request from a parent tables causes a lookup on the
 * other parent table.  If the route is better than the one from the
 * other table, or no route exists in the other table, then the new
 * route is passed downstream.
 *
 * An delete_route request from a parent table also causes a lookup on
 * the other parent table.  The delete_route is propagated downstream.
 * If an alternative route is found, then that is then propagated
 * downsteam as an add_route to replace the deleted route.
 *
 * Lookups from downsteam cause lookups on both parent tables.  The
 * better response is given.
 */
template <class A>
class MergedTable : public RouteTable<A> {
public:
    /**
     * MergedTable Constructor.
     *
     * @param table_a one of two parent RouteTables.
     * @param table_b one of two parent RouteTables.
     */
    MergedTable(RouteTable<A>* table_a, RouteTable<A>* table_b);

    /**
     * An add_route request from a parent table causes a lookup on the
     * other parent table.  If the route is better than the one from the
     * other table, or no route exists in the other table then it is
     * passed downstream.
     *
     * @param route the new route.
     * @param caller the parent table sending the new route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_route(const IPRouteEntry<A>& route,
		  RouteTable<A>* caller);

    /**
     * An delete_route request from a parent table also causes a
     * lookup on the other parent table.  The delete_route is
     * propagated downstream.  If an alternative route is found, then
     * that is then propagated downsteam as an add_route to replace
     * the deleted route.
     *
     * @param route the route to be deleted.
     * @param caller the parent table sending the delete_route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_route(const IPRouteEntry<A>* route,
		     RouteTable<A>* caller);

    /**
     * Lookup a specific subnet.  The lookup will be sent to both
     * parent tables.  If both give an answer, then the route with the
     * better admin_distance is returned.
     *
     * @param net the subnet to look up.
     * @return a pointer to the route entry if it exists, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;

    /**
     * Lookup an IP address to get the most specific (longest prefix
     * length) route that matches this address.  The lookup will be
     * sent to both parent tables.  If both give an answer, then the
     * more specific route is returned.  If both routes have the same
     * prefix length, then the route with the better admin_distance
     * is returned.
     *
     * @param addr the IP address to look up.
     *
     * @return a pointer to the best most specific route entry if any
     * entry matches, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const A& addr) const;

    /**
     * Lookup an IP address to get the most specific (longest prefix
     * length) route that matches this address, along with the
     * RouteRange information for this address and route.  As with
     * lookup_route, this involves querying both parent tables.  The
     * best, most specific route is returned, and the tightest bounds
     * on the answer are returned.
     *
     * @see RouteRange
     * @param addr the IP address to look up.
     * @return a pointer to a RouteRange class instance containing the
     * relevant answer.  It is up to the recipient of this pointer to
     * free the associated memory.
     */
    RouteRange<A>* lookup_route_range(const A& addr) const;

    /**
     * @return the table type (@ref TableType).
     */
    TableType type() const	{ return MERGED_TABLE; }

    /**
     * replumb the parent tables, so that new_parent replaces old_parent.
     *
     * @param old_parent the old parent table.
     * @param new_parent the new parent table.
     */
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);

    /**
     * Render this MergedTable as a string for debugging purposes.
     */
    string str() const;

private:
    RouteTable<A>* _table_a;
    RouteTable<A>* _table_b;
};

#endif // __RIB_RT_TAB_MERGED_HH__
