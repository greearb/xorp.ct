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

// $XORP: xorp/rip/route_entry.hh,v 1.2 2003/07/09 00:11:02 hodson Exp $

#ifndef __RIP_ROUTE_ENTRY_HH__
#define __RIP_ROUTE_ENTRY_HH__

#include "config.h"
#include "libxorp/ipnet.hh"
#include "libxorp/timer.hh"

template<typename A> class RouteEntryOrigin;

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
	       uint32_t	   cost,
	       Origin*&	   o,
	       uint32_t    tag);

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
    inline const IPNet<A>& net() const { return _net; }

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
    inline const A& nexthop() const { return _nh; }

    /**
     * Set the cost metric.
     *
     * @param cost the new cost to be associated with the Route Entry.
     *
     * @return true if stored cost changed, false otherwise.
     */
    bool set_cost(uint32_t cost);

    /**
     * Get the cost metric.
     *
     * @return the cost associated with the route entry.
     */
    uint32_t cost() const { return _cost; }

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
    inline const Origin* origin() const { return _origin; }

    /**
     * Get the origin.
     *
     * @return a pointer to the origin associated with the route entry.
     */
    inline Origin* origin() { return _origin; }

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
    inline uint16_t tag() const { return _tag; }

    /**
     * Set a Timer Event associated with this route.
     */
    inline void set_timer(const XorpTimer& t) { _timer = t; }

    /**
     * Get Timer associated with route.
     */
    inline const XorpTimer& timer() const { return _timer; }

protected:
    RouteEntry(const RouteEntry&);			// Not implemented.
    RouteEntry& operator=(const RouteEntry&);		// Not implemented.

    inline void dissociate();
    inline void associate(Origin* o);

protected:
    Net		_net;
    Addr	_nh;
    uint32_t	_cost;
    Origin*	_origin;
    uint16_t	_tag;

    XorpTimer	_timer;
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
    struct RouteEntryStore;

public:
    RouteEntryOrigin();
    virtual ~RouteEntryOrigin();

    /**
     * Associate route with this RouteEntryOrigin.
     * @param r route to be stored.
     * @return true on success, false if route is already associated.
     */
    bool associate(const Route* r);

    /**
     * Dissociate route from this RouteEntryOrigin.
     * @param r route to be dissociated.
     * @return true on success, false if route is not associated.
     */
    bool dissociate(const Route* r);

    /**
     * @return number of routes associated with this RouteEntryOrigin.
     */
    size_t route_count() const;

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


/**
 * A comparitor for the purposes of sorting containers of RouteEntry objects.
 * It examines the data rather than using the address of pointers.  The
 * latter approach makes testing difficult on different platforms since the
 * tests may inadvertantly make assumptions about the memory layout.  The
 * comparison is arbitrary, it just has to be consistent and reversible.
 *
 * IFF speed proves to be an issue, RouteEntry can be changed to be an element
 * in an intrusive linked list that has a sentinel embedded in the
 * RouteEntryOrigin.
 */
template <typename A>
struct RtPtrComparitor {
    bool operator() (const RouteEntry<A>* a, const RouteEntry<A>* b) const
    {
	const IPNet<A>& l = a->net();
	const IPNet<A>& r = b->net();
	if (l.prefix_len() < r.prefix_len())
	    return true;
	if (l.prefix_len() > r.prefix_len())
	    return false;
	return l.masked_addr() < r.masked_addr();
    }
};

#endif // __RIP_ROUTE_ENTRY_HH__
