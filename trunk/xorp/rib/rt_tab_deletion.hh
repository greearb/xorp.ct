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

// $XORP: xorp/rib/rt_tab_deletion.hh,v 1.5 2004/02/11 08:48:48 pavlin Exp $

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

    RouteTable* parent() { return _parent; }

private:
    RouteTable<A>*	_parent;
    EventLoop&		_eventloop;
    Trie<A, const IPRouteEntry<A>* >* _ip_route_table;
    XorpTimer		_background_deletion_timer;
};

#endif // __RIB_RT_TAB_DELETION_HH__
