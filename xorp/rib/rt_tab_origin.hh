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

// $XORP: xorp/rib/rt_tab_origin.hh,v 1.23 2008/10/02 21:58:13 bms Exp $

#ifndef __RIB_RT_TAB_ORIGIN_HH__
#define __RIB_RT_TAB_ORIGIN_HH__

#include "libxorp/eventloop.hh"

#include "rt_tab_base.hh"


/**
 * @short RouteTable that receives and stores raw routes sent by
 * routing protocols.
 *
 * OriginTable is a @ref RouteTable that accepts route_add requests
 * from a single routing protocol, stores the route, and propagates it
 * downstream.  It also answers lookup_route and lookup_route_range
 * requests from downstream using the routes it has stored.
 *
 * Its template class, A, must be either the IPv4 class of the IPv6
 * class.
 */
template<class A>
class OriginTable : public RouteTable<A> {
public:
    typedef Trie<A, const IPRouteEntry<A>*> RouteContainer;

public:
    /**
     * OriginTable constructor.
     *
     * @param tablename typically the name of the routing protocol that
     * supplies routes to this origin table, or "connected" for the
     * OriginTable that holds directly connected routes, or "static"
     * for the OriginTable that holds locally configured static
     * routes.
     * @param admin_distance the default administrative distance for
     * routes in this table.
     * @param protocol_type the routing protocol type (@ref ProtocolType).
     * @param eventloop the main event loop.
     */
    OriginTable(const string& tablename, uint32_t admin_distance,
		ProtocolType protocol_type, EventLoop& eventloop);

    /**
     * OriginTable destructor.
     */
    ~OriginTable();

    /**
     * Add a route to the OriginTable.  The route must not already be
     * in the OriginTable.
     *
     * @param route the route entry to be added.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_route(const IPRouteEntry<A>& route);

    /**
     * Generic @ref RouteTable method that is not used on OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_route(const IPRouteEntry<A>&, RouteTable<A>* );

    /**
     * Delete a route from the OriginTable.
     *
     * @param net the subnet of the route entry to be deleted.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_route(const IPNet<A>& net);

    /**
     * Generic @ref RouteTable method that is not used on OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_route(const IPRouteEntry<A>* , RouteTable<A>* );

    /**
     * Delete all the routes that are in this OriginTable.  The
     * deletion is not propagated downstream, so this is only useful
     * when shutting down the RIB.
     */
    void delete_all_routes();

    /**
     * Delete all the routes that are in this OriginTable, and
     * propagate the deletions downstream.
     */
    void routing_protocol_shutdown();

    /**
     * Lookup a specific subnet to see if it is in this OriginTable.
     *
     * @param net the subnet to look up.
     * @return a pointer to the route entry if it exists, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;

    /**
     * Lookup an IP address to get the most specific (longest prefix
     * length) route in the OriginTable that matches this address.
     *
     * @param addr the IP address to look up.
     * @return a pointer to the most specific route entry if any entry
     * matches, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const A& addr) const;

    /**
     * Lookup an IP addressto get the most specific (longest prefix
     * length) route in the OriginTable that matches this address,
     * along with the RouteRange information for this address and
     * route.
     *
     * @see RouteRange
     * @param addr the IP address to look up.
     * @return a pointer to a RouteRange class instance containing the
     * relevant answer.  It is up to the recipient of this pointer to
     * free the associated memory.
     */
    RouteRange<A>* lookup_route_range(const A& addr) const;

    /**
     * @return the default administrative distance for this OriginTable
     */
    uint32_t admin_distance() const	{ return _admin_distance; }

    /**
     * @return the routing protocol type (@ref ProtocolType).
     */
    int protocol_type() const		{ return _protocol_type; }

    /**
     * @return the table type (@ref TableType).
     */
    TableType type() const		{ return ORIGIN_TABLE; }

    /**
     * Generic @ref RouteTable method that is not used on OriginTable.
     */
    void replumb(RouteTable<A>* , RouteTable<A>* ) {}

    /**
     * Render the OriginTable as a string for debugging purposes
     */
    string str() const;

    /**
     * Get the number of times routing protocol has been shutdown and
     * restarted.
     */
    uint32_t generation() const;

    /**
     * Get the number of routes held internally.
     */
    uint32_t route_count() const;

    /**
     * Get the trie.
     */
    const RouteContainer& route_container() const;

private:
    uint32_t		_admin_distance;	// 0 .. 255
    ProtocolType 	_protocol_type;	// IGP or EGP
    EventLoop&   	_eventloop;
    RouteContainer*	_ip_route_table;
    uint32_t	 	_gen;
};


template <class A>
inline uint32_t
OriginTable<A>::generation() const
{
    return _gen;
}

template <class A>
inline uint32_t
OriginTable<A>::route_count() const
{
    return _ip_route_table->route_count();
}

template <class A>
inline const typename OriginTable<A>::RouteContainer&
OriginTable<A>::route_container() const
{
    return *_ip_route_table;
}

#endif // __RIB_RT_TAB_ORIGIN_HH__
