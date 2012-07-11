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

// $XORP: xorp/rib/rt_tab_deletion.hh,v 1.14 2008/10/02 21:58:12 bms Exp $

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
    typedef Trie<A, const IPRouteEntry<A>* > RouteTrie;
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
		  RouteTrie* ip_route_trie,
		  EventLoop& eventloop);

    /**
     * DeletionTable destructor.
     */
    virtual ~DeletionTable();

    /**
     * Add a route.  If the route was stored in the DeletionTable,
     * we'll remove it and propagate the delete and add downstream.
     *
     * @param route the route entry to be added.
     * @param caller the caller route table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_igp_route(const IPRouteEntry<A>& route);
    int add_egp_route(const IPRouteEntry<A>& route);

    /**
     * Delete a route.  This route MUST NOT be in the DeletionTable trie.
     *
     * @param route the route entry to be deleted.
     * @param caller the caller route table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_igp_route(const IPRouteEntry<A>* route);
    int delete_egp_route(const IPRouteEntry<A>* route);

    /**
     * Delete all the routes that are in this DeletionTable.  The
     * deletion is not propagated downstream, so this is only useful
     * when shutting down the RIB.
     */
    void delete_all_routes();

    /**
     * Delete a route, and reschedule background_deletion_pass again
     * on a zero-second timer until all the routes have been deleted
     */
    virtual void background_deletion_pass();

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
    void set_parent(RouteTable<A>* new_parent);

    /**
     * Render the DeletionTable as a string for debugging purposes.
     */
    string str() const;

    RouteTable<A>* parent() { return _parent; }
    const RouteTable<A>* parent() const { return _parent; }

protected:
    virtual int generic_add_route(const IPRouteEntry<A>& route) = 0;
    virtual int generic_delete_route(const IPRouteEntry<A>* route) = 0;
    virtual void set_background_timer() = 0;

    RouteTable<A>*	_parent;
    EventLoop&		_eventloop;
    RouteTrie*		_ip_route_table;
};

template <class A, ProtocolType PROTOCOL_TYPE>
class TypedDeletionTable { };

template <class A>
class TypedDeletionTable<A, IGP> : public DeletionTable<A> {
public:
    TypedDeletionTable(const string& tablename, RouteTable<A>* parent,
	typename DeletionTable<A>::RouteTrie* ip_route_trie, EventLoop& eventloop) :
	DeletionTable<A>(tablename, parent, ip_route_trie, eventloop),
	_background_deletion_timer(this->_eventloop.new_oneoff_after_ms(
		0, callback(this, &TypedDeletionTable<A, IGP>::background_deletion_pass))) {}

    ~TypedDeletionTable() {}

    int generic_add_route(const IPRouteEntry<A>& route) { return this->next_table()->add_igp_route(route); }
    int generic_delete_route(const IPRouteEntry<A>* route) { return this->next_table()->delete_igp_route(route); }

    //XXX This is hack, because callback can't register call
    //    to function from base class from the derived class
    void background_deletion_pass() { DeletionTable<A>::background_deletion_pass(); }
protected:
    void set_background_timer() {
	// Callback immediately, but after network events or expired timers
	_background_deletion_timer = this->_eventloop.new_oneoff_after_ms(0,
		callback(this, &TypedDeletionTable<A, IGP>::background_deletion_pass));
    }

    XorpTimer		_background_deletion_timer;
};

template <class A>
class TypedDeletionTable<A, EGP> : public DeletionTable<A> {
public:
    TypedDeletionTable(const string& tablename, RouteTable<A>* parent,
	typename DeletionTable<A>::RouteTrie* ip_route_trie, EventLoop& eventloop) :
	DeletionTable<A>(tablename, parent, ip_route_trie, eventloop),
	_background_deletion_timer(this->_eventloop.new_oneoff_after_ms(
		0, callback(this, &TypedDeletionTable<A, EGP>::background_deletion_pass))) {}

    ~TypedDeletionTable() {}

    int generic_add_route(const IPRouteEntry<A>& route) { return this->next_table()->add_egp_route(route); }
    int generic_delete_route(const IPRouteEntry<A>* route) { return this->next_table()->delete_egp_route(route); }

    //XXX This is hack, because callback can't register call
    //    to function from base class from the derived class
    void background_deletion_pass() { DeletionTable<A>::background_deletion_pass(); }
protected:
    void set_background_timer() {
	// Callback immediately, but after network events or expired timers
	_background_deletion_timer = this->_eventloop.new_oneoff_after_ms(0,
		callback(this, &TypedDeletionTable<A, EGP>::background_deletion_pass));
    }

    XorpTimer		_background_deletion_timer;
};

#endif // __RIB_RT_TAB_DELETION_HH__
