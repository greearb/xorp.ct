// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/rib/rt_tab_merged.hh,v 1.3 2003/03/16 07:18:59 pavlin Exp $

#ifndef __RIB_RT_TAB_MERGED_HH__
#define __RIB_RT_TAB_MERGED_HH__

#include "rt_tab_base.hh"

/**
 * @short Make two route @ref RouteTables behave like one
 *
 * MergedTable takes two routing tables and combines them together to
 * form a single table, where routes for the same subnet with a lower
 * admin distance override those with a higher admin distance
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
     * @param tablename human-readable tablename to aid debugging
     * @param table_a one of two parent RouteTables
     * @param table_b one of two parent RouteTables
     */
    MergedTable(const string&  tablename, 
		RouteTable<A> *table_a, 
		RouteTable<A> *table_b);

    /**
     * An add_route request from a parent table causes a lookup on the
     * other parent table.  If the route is better than the one from the
     * other table, or no route exists in the other table then it is
     * passed downstream.
     *
     * @param route the new route
     * @param caller the parent table sending the new route
     */
    int add_route(const IPRouteEntry<A>& route, 
		  RouteTable<A> *caller);

    /**
     * An delete_route request from a parent table also causes a
     * lookup on the other parent table.  The delete_route is
     * propagated downstream.  If an alternative route is found, then
     * that is then propagated downsteam as an add_route to replace
     * the deleted route.
     *
     * @param route the route to be deleted.
     * @param caller the parent table sending the delete_route.  
     */
    int delete_route(const IPRouteEntry<A> *route,
		     RouteTable<A> *caller);

    /**
     * Lookup a specific subnet.  The lookup will be sent to both
     * parent tables.  If both give an answer, then the route with the
     * better admin_distance is returned.
     *
     * @param net the subnet to look up.
     * @return a pointer to the route entry if it exists, NULL otherwise.  
     */
    const IPRouteEntry<A> *lookup_route(const IPNet<A>& net) const;

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
    const IPRouteEntry<A> *lookup_route(const A& addr) const;

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
    RouteRange<A> *lookup_route_range(const A& addr) const;

    /**
     * @return MERGED_TABLE
     */
    int type() const {return MERGED_TABLE;}

    /**
     * replumb the parent tables, so that new_parent replaces old_parent
     *
     * @param old_parent
     * @param new_parent
     */
    void replumb(RouteTable<A> *old_parent, 
		 RouteTable<A> *new_parent);

    /**
     * Render this MergedTable as a string for debugging purposes
     */
    string str() const;
private:
    RouteTable<A> *_table_a;
    RouteTable<A> *_table_b;
};

#endif // __RIB_RT_TAB_MERGED_HH__
