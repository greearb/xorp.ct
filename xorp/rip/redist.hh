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

// $XORP: xorp/rip/redist.hh,v 1.14 2008/10/02 21:58:17 bms Exp $

#ifndef __RIP_ROUTE_REDIST_HH__
#define __RIP_ROUTE_REDIST_HH__

#include "route_entry.hh"

template <typename A>
class RouteDB;

template <typename A>
class RouteWalker;

/**
 * @short Route Origination class for @ref RouteRedistributor instances.
 *
 * Route Origination class for @ref RouteRedistributor instances.
 * Overrides time constant accessors for @ref RouteEntryOrigin.  By
 * default the time constants for route expiry and deletion is never.
 * If set_allow_timers() is called future updates will have timers
 * associated with them.  set_allow_timers() is called when the
 * RedistTable is going to be withdrawn.  This allows routes to be
 * advertised as unreachable via host before they are deleted.
 *
 * Non-copyable due to inheritance from RouteEntryOrigin<A>.
 */
template <typename A>
class RedistRouteOrigin :
    public RouteEntryOrigin<A>
{
public:
    RedistRouteOrigin() : RouteEntryOrigin<A>(true) {}

    /**
     * Retrieve number of seconds before routes associated with this
     * RedistRouteOrigin expire.
     *
     * Always returns 0 to signify routes do not automatically expire.
     */
    uint32_t expiry_secs() const;

    /**
     * Retrieve number of seconds before route should be deleted after
     * expiry.
     */
    uint32_t deletion_secs() const;
};


/**
 * @short Store for redistributed routes.
 */
template <typename A>
class RouteRedistributor :
    public NONCOPYABLE
{
public:
    typedef A Addr;
    typedef IPNet<A> Net;

public:
    /**
     * Constructor for RouteRedistributor
     *
     * @param route_db the route database to add and expire routes in.
     */
    RouteRedistributor(RouteDB<A>&	route_db);

    ~RouteRedistributor();

    /**
     * Add a route to be redistributed with specific cost and tag values.
     *
     * @param net network described by route.
     * @param nexthop router capable of forwarding route.
     * @param ifname the corresponding interface name toward the destination.
     * @param vifname the corresponding vif name toward the destination.
     * @param policytags policy-tags associated with route.
     *
     * @return true on success, false if route could not be added to
     *         the RouteDatabase.  Failure may occur if route already exists
     *	       or a lower cost route exists.
     */
    bool add_route(const Net&		net,
		   const Addr&		nexthop,
		   const string&	ifname,
		   const string&	vifname,
		   uint16_t		cost,
		   uint16_t		tag,
		   const PolicyTags&	policytags);

    /**
     * Trigger route expiry.
     *
     * @param network described by route.
     *
     * @return true on success, false if route did not come from this
     * RouteRedistributor instance.
     */
    bool expire_route(const Net& net);

    /**
     * Accessor.
     *
     * @return number of routes
     */
    uint32_t route_count() const;

    /**
     * Withdraw routes.  Triggers a walking of associated routes and
     * their expiration from the RIP route database.
     */
    void withdraw_routes();

    /**
     * @return true if actively withdrawing routes, false otherwise.
     */
    bool withdrawing_routes() const;

private:
    /**
     * Periodic timer callback for withdrawing a batch of routes.  The timer
     * is triggered by @ref withdraw_routes().
     */
    bool withdraw_batch();

protected:
    RouteDB<A>&			_route_db;
    RedistRouteOrigin<A>*	_rt_origin;

    RouteWalker<A>* 		_wdrawer;	// Route Withdrawl Walker
    XorpTimer			_wtimer;	// Clock for Withdrawls
};

#endif // __RIP_REDIST_HH__
