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

// $XORP: xorp/rib/rt_tab_extint.hh,v 1.8 2004/02/11 08:48:48 pavlin Exp $

#ifndef __RIB_RT_TAB_EXTINT_HH__
#define __RIB_RT_TAB_EXTINT_HH__

#include "rt_tab_base.hh"


/**
 * @short Make two route @ref RouteTables behave like one, while
 * resolving nexthops that are not immediate neighbors
 *
 * ExtIntTable takes two routing tables and combines them together to
 * form a single table, where routes for the same subnet with a lower
 * admin distance override those with a higher admin distance.  The
 * two parent tables are different: the Internal table takes routes
 * only from IGP protocols, and so the nexthops of routes it provides
 * are always immediate neighbors.  The External table takes routes
 * from EGP protocols, and so the nexthops of routes it provides may
 * not be immediate neighbors.  The ExtIntTable resolves the nexthops
 * of all routes it propagates downstream.  If a nexthop cannot be
 * resolved, it is not propagated downstream, but is stored pending
 * the arrival of a route that would permit the nexthop to be
 * resolved.
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
 *
 * A RIB normally only has one ExtIntTable.
 */
template<class A>
class ExtIntTable : public RouteTable<A> {
public:
    /**
     * ExtIntTable Constructor.
     *
     * @param ext_table parent RouteTables supplying EGP routes.
     * @param int_table parent RouteTables supplying IGP routes.
     */
    ExtIntTable(RouteTable<A>* ext_table,
		RouteTable<A>* int_table);

    /**
     * An add_route request from a parent table causes a lookup on the
     * other parent table.  If the route is better than the one from the
     * other table, or no route exists in the other table then it is
     * passed downstream if nexthop resolution is successful.
     *
     * @param route the new route.
     * @param caller the parent table sending the new route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);

    /**
     * An delete_route request from a parent table also causes a
     * lookup on the other parent table.  The delete_route is
     * propagated downstream.  If an alternative route is found and
     * nexthop resolution on it is successful, then it is then
     * propagated downsteam as an add_route to replace the deleted
     * route.
     *
     * @param route the route to be deleted.
     * @param caller the parent table sending the delete_route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_route(const IPRouteEntry<A>* route, RouteTable<A>* caller);

    /**
     * Lookup a specific subnet.  The lookup will first look in the
     * ExtIntTable 's table of resolved routes - if there is a route
     * here, then this is returned.  Otherwise the lookup will be sent
     * to both parent tables.  If both give an answer, then the route
     * with the better admin_distance is returned, so long as it gives
     * a nexthop that is resolvable.
     *
     * @param net the subnet to look up.
     * @return a pointer to the route entry if it exists, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;

    /**
     * Lookup an IP address to get the most specific (longest prefix
     * length) route that matches this address.  The lookup will be
     * sent to both parent tables and the ExtIntTable's internal table
     * of resolved_routes.  The most specific answer is returned, so
     * long as the nexthop resolves.  If more than one route has the
     * same prefix length, then the route with the better
     * admin_distance is returned.
     *
     * @param addr the IP address to look up.
     *
     * @return a pointer to the best most specific route entry if any
     * entry matches and its nexthop resolves, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const A& addr) const;

    /**
     * Lookup an IP address to get the most specific (longest prefix
     * length) route that matches this address, along with the
     * RouteRange information for this address and route.  As with
     * lookup_route, this involves querying ExtIntTable's
     * resolved_routes table and possibly both parent tables.  The
     * best, most specific route is returned if the nexthop is
     * resolvable, and the tightest bounds on the answer are returned.
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
    TableType type() const	{ return EXTINT_TABLE; }

    /**
     * replumb the parent tables, so that new_parent replaces old_parent
     *
     * @param old_parent the old parent table.
     * @param new_parent the new parent table.
     */
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);

    /**
     * Render this ExtIntTable as a string for debugging purposes.
     */
    string str() const;

private:
    typedef typename ResolvedIPRouteEntry<A>::RouteBackLink RouteBackLink;

    const ResolvedIPRouteEntry<A>* lookup_in_resolved_table(
	const IPNet<A>& ipv4net);

    void resolve_unresolved_nexthops(const IPRouteEntry<A>& route);

    const ResolvedIPRouteEntry<A>* resolve_and_store_route(
	const IPRouteEntry<A>& route,
	const IPRouteEntry<A>* nexthop_route);

    bool delete_unresolved_nexthop(const IPRouteEntry<A>* route);

    void recalculate_nexthops(const IPRouteEntry<A>& route);

    const ResolvedIPRouteEntry<A>* lookup_by_igp_parent(
	const IPRouteEntry<A>* route);

    const ResolvedIPRouteEntry<A>* lookup_next_by_igp_parent(
	const IPRouteEntry<A>* route,
	const ResolvedIPRouteEntry<A>* previous);

    const IPRouteEntry<A>* lookup_route_in_igp_parent(
	const IPNet<A>& subnet) const;
    const IPRouteEntry<A>* lookup_route_in_igp_parent(const A& addr) const;

    RouteTable<A>*				_ext_table;
    RouteTable<A>*				_int_table;
    Trie<A, const ResolvedIPRouteEntry<A>* >	_ip_route_table;
    multimap<A, const IPRouteEntry<A>* >	_ip_unresolved_nexthops;

    // _ip_igp_parents gives us a fast way of finding a route
    // affected by a change in an igp parent route
    multimap<const IPRouteEntry<A>*, ResolvedIPRouteEntry<A>* > _ip_igp_parents;

    // _resolving_routes is a Trie of all the routes that are used to
    // resolve external routes
    Trie<A, const IPRouteEntry<A>* > _resolving_routes;
};

#endif // __RIB_RT_TAB_EXTINT_HH__
