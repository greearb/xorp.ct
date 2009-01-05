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

// $XORP: xorp/rip/route_entry.hh,v 1.23 2008/10/02 21:58:17 bms Exp $

#ifndef __RIP_ROUTE_ENTRY_HH__
#define __RIP_ROUTE_ENTRY_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipnet.hh"
#include "libxorp/timer.hh"

#include "policy/backend/policytags.hh"


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
	       const string& ifname,
	       const string& vifname,
	       uint16_t	   cost,
	       Origin*&	   o,
	       uint16_t    tag);

    RouteEntry(const Net&  n,
	       const Addr& nh,
	       const string& ifname,
	       const string& vifname,
	       uint16_t	   cost,
	       Origin*&	   o,
	       uint16_t    tag,
	       const PolicyTags& policytags);
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
    const IPNet<A>& net() const		{ return _net; }

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
    const A& nexthop() const		{ return _nh; }

    /**
     * Get the outgoing interface name.
     *
     * @return the outgoing interface name.
     */
    const string& ifname() const	{ return _ifname; }

    /**
     * Set the outgoing interface name.
     *
     * @param ifname the outgoing interface name.
     * @return true if the stored interface name changed, false otherwise.
     */
    bool set_ifname(const string& ifname);

    /**
     * Get the outgoing vif name.
     *
     * @return the outgoing vif name.
     */
    const string& vifname() const	{ return _vifname; }

    /**
     * Set the outgoing vif name.
     *
     * @param vifname the outgoing vif name.
     * @return true if the stored vif name changed, false otherwise.
     */
    bool set_vifname(const string& vifname);
    
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
    const Origin* origin() const 	{ return _origin; }

    /**
     * Get the origin.
     *
     * @return a pointer to the origin associated with the route entry.
     */
    Origin* origin()			{ return _origin; }

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
    uint16_t tag() const 		{ return _tag; }

    /**
     * Set a Timer Event associated with this route.
     */
    void set_timer(const XorpTimer& t) 	{ _timer = t; }

    /**
     * Get Timer associated with route.
     */
    const XorpTimer& timer() const 	{ return _timer; }

    /**
     * @return policy-tags associated with route.
     */
    const PolicyTags& policytags() const	{ return _policytags; }

    PolicyTags& policytags()			{ return _policytags; }

    /**
     * Replace policy-tags of route
     * @return true if tags were modified.
     * @param tags new policy-tags.
     */
    bool set_policytags(const PolicyTags& tags);

    /**
     * @return true if route was rejected by policy filter.
     */
    bool filtered() const			{ return _filtered; }
    /**
     * Set if route is accepted or rejected.
     *
     * @param v true if route is filtered
     */
    void set_filtered(bool v)			{ _filtered = v;    }

private:
    friend class RouteEntryRef<A>;
    void ref()				{ _ref_cnt++; }
    uint16_t unref()			{ return --_ref_cnt; }
    uint16_t ref_cnt() const		{ return _ref_cnt; }

protected:
    RouteEntry(const RouteEntry&);			// Not implemented.
    RouteEntry& operator=(const RouteEntry&);		// Not implemented.

    void dissociate();
    void associate(Origin* o);


protected:
    Net		_net;
    Addr	_nh;
    string	_ifname;
    string	_vifname;
    uint16_t	_cost;
    Origin*	_origin;
    uint16_t	_tag;
    uint16_t	_ref_cnt;

    XorpTimer	_timer;
    
    PolicyTags	_policytags;
    bool	_filtered;
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
    void release() {
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

    RouteEntryRef& operator=(const RouteEntryRef& o) {
	release();
	_rt = o._rt;
	if (_rt) _rt->ref();
	return *this;
    }

    RouteEntry<A>* get() const { return _rt; }

    RouteEntry<A>* operator->() const { return _rt; }

    bool operator==(const RouteEntryRef& o) const { return _rt == o._rt; }
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
    RouteEntryOrigin(bool is_rib_origin);
    virtual ~RouteEntryOrigin();

    /**
     * Test if RIB is the originator.
     *
     * @return true if RIB is the originator, otherwise false.
     */
    bool is_rib_origin() const { return _is_rib_origin; }

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
     * Clear/remove all routes associated with this RouteEntryOrigin.
     */
    void clear();

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

private:
    bool	_is_rib_origin;		// True if the origin is RIB
};



#endif // __RIP_ROUTE_ENTRY_HH__
