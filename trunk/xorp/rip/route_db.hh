// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8: 

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/rip/route_db.hh,v 1.15 2004/09/17 13:57:15 abittau Exp $

#ifndef __RIP_ROUTE_DB_HH__
#define __RIP_ROUTE_DB_HH__

#include "config.h"
#include <map>

#include "libxorp/ref_ptr.hh"
#include "route_entry.hh"

#include "policy/backend/policy_filters.hh"

class EventLoop;

template <typename A>
class UpdateQueue;

template <typename A>
class PacketRouteEntry;

template <typename A>
class RouteWalker;

/**
 * @short A network comparitor class for the purposes of ordering
 * networks in sorted containers.
 */
template <typename A>
struct NetCmp {
    typedef IPNet<A> Net;
    bool operator() (const Net& l, const Net& r) const;
};

/**
 * @short Class that manages routes.
 *
 * The RouteDB class holds routes and manages their updates.
 * Successful updates are placed in the update queue contained within
 * the RouteDB instance.  The UpdateQueue is used for generating
 * triggered update messages.
 *
 * The @ref RouteWalker class provides a way to walk the routes held.
 */
template <typename A>
class RouteDB {
public:
    typedef A					Addr;
    typedef IPNet<A> 				Net;
    typedef RouteEntry<A>			Route;
    typedef RouteEntryOrigin<A>			RouteOrigin;
    typedef RouteEntryRef<A>			DBRouteEntry;
    typedef RouteEntryRef<A>			ConstDBRouteEntry;
    typedef PacketRouteEntry<A>			PacketizedRoute;
    typedef map<Net, DBRouteEntry, NetCmp<A> >	RouteContainer;
    typedef map<Net, Route*, NetCmp<A> >        RouteContainerNoRef;

public:
    RouteDB(EventLoop& e, PolicyFilters& pfs);
    ~RouteDB();

    /**
     * Update Route Entry in database for specified route.
     *
     * If the route does not exist or the values provided differ from the
     * existing route, then an update is placed in the update queue.
     *
     * @param net the network route being updated.
     * @param nexthop the corresponding nexthop address.
     * @param cost the corresponding metric value as received from the
     *	      route originator.
     * @param tag the corresponding route tag.
     * @param origin the route originator proposing update.
     * @param policytags the policytags of this route.
     * @return true if an update occurs, false otherwise.
     */
    bool update_route(const Net&	net,
		      const Addr&	nexthop,
		      uint32_t		cost,
		      uint32_t		tag,
		      RouteOrigin*	origin,
		      const PolicyTags& policytags);
    /**
     * A copy of RIB routes need to be kept, as they are not advertised
     * periodically. If a RIB route gets replaced with a better route from
     * another peer, it will be lost. By storing RIB routes, it is possible to
     * re-advertise RIB routes which have lost, but are now optimal.
     *
     * @param net network of the route being added.
     * @param nexthop the corresponding nexthop address.
     * @param cost the corresponding metric value.
     * @param the corresponding route tag.
     * @param origin the route originator [RIB in this case].
     * @param policytags the policytags of this route.
     */
    void add_rib_route(const Net&	net,
		       const Addr&	nexthop,
		       uint32_t		cost,
		       uint32_t		tag,
		       RouteOrigin*	origin,
		       const PolicyTags& policytags);
    
    /**
     * Permanently delete a RIB route.  This occurs if redistribution of this
     * route ceased.
     *
     * @param net network of the route being deleted.
     */
    void delete_rib_route(const Net& net);

    /**
     * Flatten route entry collection into a Vector.
     *
     * @param routes vector where routes are to be appended.
     */
    void dump_routes(vector<ConstDBRouteEntry>& routes);

    /**
     * Flush routes.
     */
    void flush_routes();

    /**
     * @return count of routes in database.
     */
    uint32_t route_count() const;

    /**
     * @return pointer to route entry if it exists, 0 otherwise.
     */
    const Route* find_route(const Net& n) const;

    /**
     * Accessor.
     * @return reference to UpdateQueue.
     */
    UpdateQueue<A>& update_queue();

    /**
     * Accessor.
     * @return const reference to UpdateQueue.
     */
    const UpdateQueue<A>& update_queue() const;

    inline EventLoop& eventloop() 			{ return _eventloop; }

    /**
     * Push routes through policy filters for re-filtering.
     */
    void push_routes();

    /**
     * Do policy filtering
     * @param r route to filter.
     * @return true if route was accepted, false otherwise.
     */
    bool do_filtering(Route* r);

protected:
    RouteDB(const RouteDB&);			// not implemented
    RouteDB& operator=(const RouteDB&);		// not implemented

    void expire_route(Route* r);
    void set_expiry_timer(Route* r);

    void delete_route(Route* r);
    void set_deletion_timer(Route* r);

protected:
    RouteContainer& routes();

protected:
    EventLoop&		_eventloop;
    RouteContainer	_routes;
    UpdateQueue<A>*	_uq;
    PolicyFilters&	_policy_filters;


    // Rib routes are not "readvertised", so consider if a rib route loses, and
    // then the winning route expires... we will have no route for that
    // destination... while we should.
    //
    // Also need to be able to re-filter original routes
    RouteContainerNoRef	_rib_routes;
    RouteOrigin*	_rib_origin;
    
    friend class RouteWalker<A>;
};

/**
 * @short Asynchronous RouteDB walker.
 *
 * The RouteWalker class walks the routes in a RouteDB.  It assumes
 * the walking is broken up into a number of shorter walks, and that
 * each short walk is triggered from a XorpTimer.  The end of a short
 * walk causes state to saved and is signalled using the pause()
 * method.  When the next short walk is ready to start, resume()
 * should be called.  These calls save and resume state are relatively
 * expensive.
 */
template <typename A>
class RouteWalker
{
public:
    typedef A			  		Addr;
    typedef IPNet<A>		  		Net;
    typedef typename RouteDB<A>::RouteContainer	RouteContainer;
    typedef typename RouteDB<A>::Route		Route;

    enum State { RUNNING, PAUSED };

public:
    RouteWalker(RouteDB<A>& route_db);

    ~RouteWalker();

    /**
     * @return current state of instance.
     */
    inline State state() const			{ return _state; }

    /**
     * Move iterator to next available route.
     *
     * @return true on success, false if route not available or instance is
     *         not in the RUNNING state.
     */
    const Route* next_route();

    /**
     * Get current route.
     *
     * @return pointer to route if available, 0 if route not available or
     *	       not in RUNNING state.
     */
    const Route* current_route();

    /**
     * Pause route walking operation.  The instance state is
     * transitioned from RUNNING to PAUSED on the assumption that
     * route walking will be resumed at some point in the future (@ref
     * resume).  If the current route has a deletion timer associated
     * with it that would expire within pause_ms, the timer expiry is
     * pushed back so it will expire at a time after the expected
     * resume time.  Thus in most cases a walk can safely resume from
     * where it was paused.
     *
     * @param pause_ms the expected time before resume is called.
     */
    void pause(uint32_t pause_ms);

    /**
     * Resume route walking.  The instance state is transitioned from
     * PAUSED to RUNNING.  The internal iterator is checked for validity and
     * recovery steps taken should the route pointed to have been deleted.
     */
    void resume();

    /**
     * Effect a reset.  The internal iterator is moved to the first
     * stored route and the state is set to RUNNING.
     */
    void reset();

protected:
    RouteWalker(const RouteWalker&);				// Not impl
    RouteWalker& operator=(const RouteWalker&);			// Not impl

private:
    static const Net NO_NET;

private:
    RouteDB<A>& _route_db;	// RouteDB to be walked.
    State	_state;		// Current state (RUNNING/PAUSED).
    Net		_last_visited;	// Last route output before entering PAUSED.
    				// if set to RouteWalker::no_net there was
    				// no valid route when paused.
    typename RouteContainer::iterator _pos;	// Current route when RUNNING.
};

#endif // __RIP_ROUTE_DB_HH__
