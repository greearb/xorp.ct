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

// $XORP: xorp/rib/rt_tab_origin.hh,v 1.10 2004/02/06 22:44:12 pavlin Exp $

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
    /** 
     * OriginTable constructor.
     *
     * @param tablename typically the name of the routing protcol that
     * supplies routes to this origin table, or "connected" for the
     * OriginTable that holds directly connected routes, or "static"
     * for the OriginTable that holds locally configured static
     * routes.  
     * @param admin_distance the default administrative distance for
     * routes in this table.
     * @param protocol_type the routing protocol type (@ref ProtocolType).
     * @param eventloop the main event loop.
     */
    OriginTable(const string& tablename, int admin_distance,
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
    int add_route(const IPRouteEntry<A>&, RouteTable<A>* ) {
	XLOG_UNREACHABLE();
	return XORP_ERROR;
    }

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
    int delete_route(const IPRouteEntry<A>* , RouteTable<A>* ) {
	XLOG_UNREACHABLE();
	return XORP_ERROR;
    }

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
    int admin_distance() const		{ return _admin_distance; }

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

private:
    int		 _admin_distance;		// 0 .. 255
    ProtocolType _protocol_type;		// IGP or EGP
    EventLoop&   _eventloop;
    Trie<A, const IPRouteEntry<A>* >* _ip_route_table;
};

#endif // __RIB_RT_TAB_ORIGIN_HH__
