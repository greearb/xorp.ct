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
 * Provides hardwired time constants for route expiry (never) and
 * deletion (default value).
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
     * @return Always returns 0 as redistributed routes persist until
     * explicitly expired.
     */
    uint32_t expiry_secs() const;

    /**
     * Retrieve number of seconds before route should be deleted after
     * expiry.
     * @return Always returns DEFAULT_DELETION_SECS.
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
     * @param tag to be associated with redistributed routes.
     */
    RouteRedistributor(RouteDB<A>&	route_db,
		       const string&	protocol,
		       uint16_t		tag);

    ~RouteRedistributor();

    /**
     * Add a route to be redistributed.
     *
     * @param net network described by route.
     * @param nexthop router capable of forwarding route.
     * @param cost of route.
     *
     * @return true on success, false if route could not be added to
     *         the RouteDatabase.  Failure may occur if route already exists
     *	       or a lower cost route exists.
     */
    bool add_route(const Net& net, const Addr& nexthop, uint32_t cost = 0);

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

private:
    RouteRedistributor(const RouteRedistributor& );		// not impl
    RouteRedistributor& operator=(const RouteRedistributor& );	// not impl

protected:
    RouteDB<A>&		 _route_db;
    string		 _protocol;
    uint16_t		 _tag;
    RouteEntryOrigin<A>* _rt_origin;
};

#endif // __RIP_REDIST_HH__
