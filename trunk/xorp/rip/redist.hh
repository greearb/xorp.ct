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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_ROUTE_REDIST_HH__
#define __RIP_ROUTE_REDIST_HH__

#include "route_entry.hh"

template <typename A>
class RouteDB;

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
 */
template <typename A>
class RedistRouteOrigin : public RouteEntryOrigin<A>
{
public:
    RedistRouteOrigin() {}

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

private:
    RedistRouteOrigin(const RedistRouteOrigin&);		// not impl
    RedistRouteOrigin& operator=(const RedistRouteOrigin&);	// not impl
};


/**
 * @short Store for redistributed routes.
 */
template <typename A>
class RouteRedistributor {
public:
    typedef A Addr;
    typedef IPNet<A> Net;

public:
    /**
     * Constructor for RouteRedistributor
     *
     * @param route_db the route database to add and expire routes in.
     * @param protocol name of protocol redistributor handles.
     * @param cost cost to associated with redistributed routes.
     * @param tag tag to be associated with redistributed routes.
     */
    RouteRedistributor(RouteDB<A>&	route_db,
		       const string&	protocol,
		       uint16_t		cost,
		       uint16_t		tag);

    ~RouteRedistributor();

    /**
     * Add a route to be redistributed.
     *
     * @param net network described by route.
     * @param nexthop router capable of forwarding route.
     *
     * @return true on success, false if route could not be added to
     *         the RouteDatabase.  Failure may occur if route already exists
     *	       or a lower cost route exists.
     */
    bool add_route(const Net& net, const Addr& nexthop);

    /**
     * Add a route to be redistributed with specific cost and tag values.
     *
     * @param net network described by route.
     * @param nexthop router capable of forwarding route.
     *
     * @return true on success, false if route could not be added to
     *         the RouteDatabase.  Failure may occur if route already exists
     *	       or a lower cost route exists.
     */
    bool add_route(const Net&	net,
		   const Addr&	nexthop,
		   uint16_t	cost,
		   uint16_t	tag);

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
     * @return protocol name.
     */
    const string& protocol() const;

    /**
     * Accessor.
     *
     * @return tag routes are redistributed with.
     */
    uint16_t tag() const;

    /**
     * Accessor.
     *
     * @return cost routes are redistributed with.
     */
    uint16_t cost() const;

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
    RouteRedistributor(const RouteRedistributor& );		// not impl
    RouteRedistributor& operator=(const RouteRedistributor& );	// not impl

    /**
     * Periodic timer callback for withdrawing a batch of routes.  The timer
     * is triggered by @ref withdraw_routes().
     */
    bool withdraw_batch();

protected:
    RouteDB<A>&			_route_db;
    string			_protocol;
    uint16_t			_cost;
    uint16_t			_tag;
    RedistRouteOrigin<A>*	_rt_origin;

    RouteWalker<A>* 		_wdrawer;	// Route Withdrawl Walker
    XorpTimer			_wtimer;	// Clock for Withdrawls
};

#endif // __RIP_REDIST_HH__
