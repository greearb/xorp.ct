// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rib/rt_tab_deletion.hh,v 1.13 2008/07/23 05:11:31 pavlin Exp $

#ifndef __RIB_RT_TAB_DELETION_HH__
#define __RIB_RT_TAB_DELETION_HH__

#include "libxorp/timer.hh"

#include "rt_tab_base.hh"


class EventLoop;

/**
 * @short RouteTable that performs background deletion of routes when
 * a routing protocol goes down leaving routes in the RIB.
 *
 * Its template class, A, must be either the IPv4 class of the IPv6 class.
 */
template<class A>
class DeletionTable : public RouteTable<A> {
public:
    /**
     * DeletionTable constructor.
     *
     * @param tablename used for debugging.
     * @param parent Upstream routing table (usually an origin table).
     * @param ip_route_trie the entire route trie from the OriginTable
     * that contains routes we're going to delete (as a background task).
     */
    DeletionTable(const string& tablename,
		  RouteTable<A>* parent,
		  Trie<A,  const IPRouteEntry<A>* >* ip_route_trie,
		  EventLoop& eventloop);

    /**
     * DeletionTable destructor.
     */
    ~DeletionTable();

    /**
     * Add a route.  If the route was stored in the DeletionTable,
     * we'll remove it and propagate the delete and add downstream.
     *
     * @param route the route entry to be added.
     * @param caller the caller route table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);

    /**
     * Delete a route.  This route MUST NOT be in the DeletionTable trie.
     *
     * @param route the route entry to be deleted.
     * @param caller the caller route table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_route(const IPRouteEntry<A>* route, RouteTable<A>* caller);

    /**
     * Delete all the routes that are in this DeletionTable.  The
     * deletion is not propagated downstream, so this is only useful
     * when shutting down the RIB.
     */
    void delete_all_routes();

    /**
     * Lookup a specific subnet to see if it is in this DeletionTable
     * or the upstream tables.
     *
     * @param net the subnet to look up.
     * @return a pointer to the route entry if it exists, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;

    /**
     * Lookup an IP address to get the most specific (longest prefix
     * length) route in the DeletionTable or the upstream tables that
     * matches this address.
     *
     * @param addr the IP address to look up.
     * @return a pointer to the most specific route entry if any entry
     * matches, NULL otherwise.
     */
    const IPRouteEntry<A>* lookup_route(const A& addr) const;

    /**
     * Lookup an IP addressto get the most specific (longest prefix
     * length) route in the union of the DeletionTable and the
     * upstream tables that matches this address, along with the
     * RouteRange information for this address and route.
     *
     * @see RouteRange
     * @param addr the IP address to look up.
     * @return a pointer to a RouteRange class instance containing the
     * relevant answer.  It is up to the recipient of this pointer to
     * free the associated memory.
     */
    RouteRange<A>* lookup_route_range(const A& addr) const;

    /**
     * Delete a route, and reschedule background_deletion_pass again
     * on a zero-second timer until all the routes have been deleted
     */
    void background_deletion_pass();

    /**
     * Remove ourself from the plumbing and delete ourself.
     */
    void unplumb_self();

    /**
     * @return the table type (@ref TableType).
     */
    TableType type() const	{ return DELETION_TABLE; }

    /**
     * Change the parent of this route table.
     */
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);

    /**
     * Render the DeletionTable as a string for debugging purposes.
     */
    string str() const;

    RouteTable<A>* parent() { return _parent; }

private:
    RouteTable<A>*	_parent;
    EventLoop&		_eventloop;
    Trie<A, const IPRouteEntry<A>* >* _ip_route_table;
    XorpTimer		_background_deletion_timer;
};

#endif // __RIB_RT_TAB_DELETION_HH__
