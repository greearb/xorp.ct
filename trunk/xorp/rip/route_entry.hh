// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rip/route_entry.hh,v 1.5 2004/04/04 18:34:53 hodson Exp $

#ifndef __RIP_ROUTE_ENTRY_HH__
#define __RIP_ROUTE_ENTRY_HH__

#include "config.h"
#include "libxorp/ipnet.hh"
#include "libxorp/timer.hh"

template<typename A> class RouteEntryOrigin;

template<typename A> class RouteEntryRef;

/**
 * RIP Route Entry Class.
 *
 * This class is used for storing RIPv2 and RIPng route entries.  It is
 * a template class taking an address family type as a template argument.
 * Only IPv4 and IPv6 types may be supplied.
 */
template<typename A>
class RouteEntry {
public:
    typedef A Addr;
    typedef IPNet<A> Net;
    typedef RouteEntryOrigin<A> Origin;

public:
    /**
     * Constructor.
     *
     * The constructor set the internal state according to the parameters and
     * if the Origin is non-null makes the appropriate call to tell the
     * Origin of it's existence.
     */
    RouteEntry(const Net&  n,
	       const Addr& nh,
	       uint16_t	   cost,
	       Origin*&	   o,
	       uint16_t    tag);

    /**
     * Destructor.
     *
     * Cleans up state associated with RouteEntry.  If the Origin associated
     * with the RouteEntry is not-null, the Origin is informed of the
     * destruction.
     */
    ~RouteEntry();

    /**
     * Get network.
     */
    inline const IPNet<A>& net() const		{ return _net; }

    /**
     * Set next hop.
     *
     * @param nh the new nexthop to be associated with Route Entry.
     *
     * @return true if the stored nexthop changed, false otherwise.
     */
    bool set_nexthop(const A& nh);

    /**
     * Get next hop.
     *
     * @return nexthop address associated with the route entry.
     */
    inline const A& nexthop() const		{ return _nh; }

    /**
     * Set the cost metric.
     *
     * @param cost the new cost to be associated with the Route Entry.
     *
     * @return true if stored cost changed, false otherwise.
     */
    bool set_cost(uint16_t cost);

    /**
     * Get the cost metric.
     *
     * @return the cost associated with the route entry.
     */
    uint16_t cost() const			{ return _cost; }

    /**
     * Set the origin.  If the origin is different from the stored origin,
     * the RouteEntry dissociates itself from the current origin and
     * informs the new origin of it's existence.
     *
     * @param origin the new origin to be associated with the route entry.
     *
     * @return true if the stored origin changed, false otherwise.
     */
    bool set_origin(Origin* origin);

    /**
     * Get the origin.
     *
     * @return a pointer to the origin associated with the route entry.
     */
    inline const Origin* origin() const 	{ return _origin; }

    /**
     * Get the origin.
     *
     * @return a pointer to the origin associated with the route entry.
     */
    inline Origin* origin()			{ return _origin; }

    /**
     * Set the tag value.
     *
     * @param tag the tag value to be associated with the route entry.
     *
     * @return true if the stored tag changed, false otherwise.
     */
    bool set_tag(uint16_t tag);

    /**
     * Get the tag.
     *
     * @return tag value associated with the route entry.
     */
    inline uint16_t tag() const 		{ return _tag; }

    /**
     * Set a Timer Event associated with this route.
     */
    inline void set_timer(const XorpTimer& t) 	{ _timer = t; }

    /**
     * Get Timer associated with route.
     */
    inline const XorpTimer& timer() const 	{ return _timer; }

private:
    friend class RouteEntryRef<A>;
    inline void ref()				{ _ref_cnt++; }
    inline uint16_t unref()			{ return --_ref_cnt; }
    inline uint16_t ref_cnt() const		{ return _ref_cnt; }

protected:
    RouteEntry(const RouteEntry&);			// Not implemented.
    RouteEntry& operator=(const RouteEntry&);		// Not implemented.

    inline void dissociate();
    inline void associate(Origin* o);

protected:
    Net		_net;
    Addr	_nh;
    uint16_t	_cost;
    Origin*	_origin;
    uint16_t	_tag;
    uint16_t	_ref_cnt;

    XorpTimer	_timer;
};


/**
 * @short RouteEntry reference class.
 *
 * A reference pointer that manipulates a counter embedded within
 * RouteEntry objects and deletes them when the counter goes to zero.  Having
 * the counter embedded in the object makes it easier to reference count
 * than using ref_ptr.
 */
template <typename A>
class RouteEntryRef {
private:
    RouteEntry<A>* _rt;

protected:
    inline void release()
    {
	if (_rt && _rt->unref() == 0) delete _rt;
    }

public:
    RouteEntryRef(RouteEntry<A>* r) : _rt(r)		{ _rt->ref(); }

    RouteEntryRef() : _rt(0) 				{}

    ~RouteEntryRef()					{ release(); }

    RouteEntryRef(const RouteEntryRef& o) : _rt(o._rt)
    {
	if (_rt) _rt->ref();
    }

    inline RouteEntryRef& operator=(const RouteEntryRef& o)
    {
	release();
	_rt = o._rt;
	if (_rt) _rt->ref();
	return *this;
    }

    inline RouteEntry<A>* get() const
    {
        return _rt;
    }

    inline RouteEntry<A>* operator->() const
    {
        return _rt;
    }

    inline bool operator==(const RouteEntryRef& o) const
    {
	return _rt == o._rt;
    }
};


/**
 * Base class for originators of RIP route entires.
 *
 * This class is used for storing RIPv2 and RIPng route entries.  It is
 * a template class taking an address family type as a template argument.
 * Only IPv4 and IPv6 types may be supplied.
 */
template <typename A>
class RouteEntryOrigin {
public:
    typedef RouteEntry<A> Route;
    typedef IPNet<A>	  Net;
    struct RouteEntryStore;

public:
    RouteEntryOrigin();
    virtual ~RouteEntryOrigin();

    /**
     * Associate route with this RouteEntryOrigin.
     * @param r route to be stored.
     * @return true on success, false if route is already associated.
     */
    bool associate(Route* r);

    /**
     * Dissociate route from this RouteEntryOrigin.
     * @param r route to be dissociated.
     * @return true on success, false if route is not associated.
     */
    bool dissociate(Route* r);

    /**
     * Find route if RouteOrigin has a route for given network.
     * @param n network.
     * @return true if entry exists in store, false otherwise.
     */
    Route* find_route(const Net& n) const;

    /**
     * @return number of routes associated with this RouteEntryOrigin.
     */
    uint32_t route_count() const;

    /**
     * Dump associated routes into a vector (debugging use only).
     */
    void dump_routes(vector<const Route*>& routes) const;

    /**
     * Retrieve number of seconds before routes associated with this
     * RouteEntryOrigin should be marked as expired.  A return value of 0
     * indicates routes are of infinite duration, eg static routes.
     */
    virtual uint32_t expiry_secs() const = 0;

    /**
     * Retrieve number of seconds before route should be deleted after
     * expiry.
     */
    virtual uint32_t deletion_secs() const = 0;

private:
    RouteEntryOrigin(const RouteEntryOrigin& reo);		// Not impl
    RouteEntryOrigin& operator=(const RouteEntryOrigin&);	// Not impl

protected:
    struct RouteEntryStore* _rtstore;
};



#endif // __RIP_ROUTE_ENTRY_HH__
