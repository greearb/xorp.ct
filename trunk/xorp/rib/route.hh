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

// $XORP: xorp/rib/route.hh,v 1.13 2004/06/10 22:41:39 hodson Exp $

#ifndef __RIB_ROUTE_HH__
#define __RIB_ROUTE_HH__

#include <set>
#include <map>

#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/vif.hh"
#include "libxorp/nexthop.hh"

#include "policy/backend/policytags.hh"

#include "protocol.hh"


/**
 * @short Base class for RIB routing table entries.
 *
 * This is the base class from which RIB routing table entries are derived.
 * It's not useful by itself.
 */
class RouteEntry {
public:
    /**
     * Constructor for a route entry.
     *
     * @param vif the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.
     * @param nexthop the NextHop router to which packets matching this
     * entry should be forwarded.
     * @param protocol the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     */
    RouteEntry(Vif* vif, NextHop* nexthop, const Protocol& protocol,
	       uint16_t metric)
	: _vif(vif), _nexthop(nexthop), _protocol(protocol),
	  _admin_distance(255), _metric(metric)
    {
    }


    /**
     * Destructor
     */
    virtual ~RouteEntry() {}

    /**
     * Get the VIF.
     *
     * @return the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.
     */
    inline Vif* vif() const { return _vif; }

    /**
     * Get the NextHop router.
     *
     * @return the NextHop router to which packets matching this
     * entry should be forwarded.
     */
    inline NextHop* nexthop() const { return _nexthop; }

    /**
     * Set the NextHop router.
     *
     * @param v the NextHop router to be set on this route.
     */
    inline void set_nexthop(NextHop* v) { _nexthop = v; }

    /**
     * Get the Administrative Distance.
     *
     * @return the Administrative Distance associated with this route.
     * Admin Distance is a parameter typically assigned on a
     * per-routing-protocol basis.  When two routes for the same
     * subnet come from different routing protocols, the one with the
     * lower admin distance is prefered.
     */
    inline uint16_t admin_distance() const { return _admin_distance; }

    /**
     * Set the Administrative Distance.
     *
     * @param ad the administrative distance to apply to this route.
     */
    inline void set_admin_distance(uint16_t ad) { _admin_distance = ad; }

    /**
     * Get the routing protocol.
     *
     * @return the routing protocol that originated this route.
     * @see Protocol.
     */
    inline const Protocol& protocol() const { return _protocol; }

    /**
     * Display the route for debugging purposes.
     */
    virtual string str() const = 0;

    /**
     * Set the routing protocol metric on this route.
     *
     * @param metric the routing protocol metric to be set on this route.
     */
    inline void set_metric(uint16_t metric) { _metric = metric; }

    /**
     * Get the routing protocol metric.
     *
     * @return the routing protocol metric for this route.
     */
    inline uint32_t metric() const { return _metric; }

protected:
    Vif*		_vif;
    NextHop*		_nexthop;		// The next-hop router
    // The routing protocol that instantiated this route
    const Protocol&	_protocol;
    uint16_t		_admin_distance;	// Lower is better
    uint16_t		_metric;		// Lower is better
};

/**
 * @short Routing Table Entry
 *
 * This class stores a regular RIB routing table entry for either an
 * IPv4 or an IPv6 route.  It is a template class, where A is either a
 * the IPv4 class or the IPv6 class.
 */
template <class A>
class IPRouteEntry : public RouteEntry {
public:
    /**
     * Constructor for IPRouteEntry.
     *
     * @param net the Subnet (address and mask) of the routing table entry.
     * @param vif the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.
     * @param nexthop the NextHop router to which packets matching this
     * entry should be forwarded.
     * @param protocol the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     */
    IPRouteEntry(const IPNet<A>& net, Vif* vif, NextHop* nexthop,
		 const Protocol& protocol, uint16_t metric)
	: RouteEntry(vif, nexthop, protocol, metric), _net(net) {}

    /**
     * Constructor for IPRouteEntry.
     *
     * @param net the Subnet (address and mask) of the routing table entry.
     * @param vif the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.
     * @param nexthop the NextHop router to which packets matching this
     * entry should be forwarded.
     * @param protocol the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     * @param policytags the policy-tags for this route.
     */
    IPRouteEntry(const IPNet<A>& net, Vif* vif, NextHop* nexthop,
		 const Protocol& protocol, uint16_t metric,
		 const PolicyTags& policytags)
	: RouteEntry(vif, nexthop, protocol, metric), _net(net),
	  _policytags(policytags) {}

    /**
     * Destructor for Routing Table Entry
     */
    ~IPRouteEntry() {}

    /**
     * Get the route entry's subnet addres.
     *
     * @return the route entry's subnet address.
     */
    inline const IPNet<A>& net() const { return _net; }

    /**
     * Get the prefix length of the route entry's subnet address.
     *
     * @return the prefix length (in bits) of the route entry's subnet address.
     */
    inline size_t prefix_len() const { return _net.prefix_len(); }

    /**
     * Get the route entry's next-hop router address.
     *
     * @return the route entry's next-hop router address. If there is no
     * next-hop router, then the return value is IPv4#ZERO() or IPv6#ZERO().
     */
    inline const A& nexthop_addr() const {
	IPNextHop<A>* nh = reinterpret_cast<IPNextHop<A>* >(nexthop());
	if (nh != NULL)
	    return nh->addr();
	else {
	    return A::ZERO();
	}
    }

    /**
     * Get the policy-tags for this route.
     *
     * @return the policy-tags for this route.
     */
    inline const PolicyTags& policytags() const {
	return _policytags;
    }

    /**
     * Replace policy-tags in the route.
     *
     * @param ptags The new policy-tags for this route.
     */
    inline void set_policytags(const PolicyTags& ptags) { _policytags = ptags; }

    /**
     * Get the route entry as a string for debugging purposes.
     *
     * @return a human readable representation of the route entry.
     */
    string str() const {
	string dst = (_net.is_valid()) ? _net.str() : string("NULL");
	string vif = (_vif) ? string(_vif->name()) : string("NULL");
	return string("Dst: ") + dst + string(" Vif: ") + vif +
	    string(" NextHop: ") + _nexthop->str() + string(" Metric: ") +
	    c_format("%d", _metric) + string(" PolicyTags: ") +
	    _policytags.str();
    }

protected:
    IPNet<A> _net;		// The route entry's subnet address
    PolicyTags _policytags;	// Tags used for policy route redistribution
};

typedef IPRouteEntry<IPv4> IPv4RouteEntry;
typedef IPRouteEntry<IPv6> IPv6RouteEntry;

/**
 * @short Extended RouteEntry, used by ExtIntTable.
 *
 * This class stored an extended routing table entry, for use in
 * ExtIntTable.  When a route with a non-local nexthop arrives, the
 * ExtIntTable attempts to discover a local nexthop by finding the
 * route that packets to the non-local nexthop would use.  This entry
 * is used to store the resulting route, with a local nexthop, and
 * with links to the parent routes that were used to provide the
 * information in this route entry.
 *
 * This is a template class, where A is either a the IPv4 class or the
 * IPv6 class.
 */
template <class A>
class ResolvedIPRouteEntry : public IPRouteEntry<A> {
public:
    typedef multimap<const IPRouteEntry<A>* , ResolvedIPRouteEntry<A>* > RouteBackLink;

public:
    /**
     * Constructor for IPRouteEntry.
     *
     * @param net the Subnet (address and mask) of the routing table entry.
     * @param vif the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.
     * @param nexthop the NextHop router to which packets matching this
     * entry should be forwarded.  This should be a local nexthop.
     * @param protocol the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     * @param igp_parent the route entry used to resolve the non-local
     * nexthop in the egp_parent into a local nexthop.
     * @param egp_parent the orginal route entry with a non-local nexthop.
     */
    ResolvedIPRouteEntry(const IPNet<A>& net, Vif* vif, NextHop* nexthop,
			 const Protocol& protocol, uint16_t metric,
			 const IPRouteEntry<A>* igp_parent,
			 const IPRouteEntry<A>* egp_parent)
	: IPRouteEntry<A>(net, vif, nexthop, protocol, metric,PolicyTags()),
	_igp_parent(igp_parent),
	_egp_parent(egp_parent) { }

    /**
     * Get the igp_parent.
     *
     * @return the IGP parent route entry that was used to resolve the
     * EGP parent route entry's non-local nexthop into a local nexthop.
     */
    inline const IPRouteEntry<A>* igp_parent() const { return _igp_parent; }

    /**
     * Get the EGP parent.
     *
     * @return the EGP parent, which is the original route entry that
     * had a non-local nexthop.
     */
    inline const IPRouteEntry<A>* egp_parent() const { return _egp_parent; }

    /**
     * Set the backlink.  When a resolved route is created, the
     * ExtIntTable will store a link to it in a multimap that is
     * indexed by the IGP parent.  This will allow all the routes
     * affected by a change in the IGP parent to be found easily.
     * However, if the EGP parent goes away, we need to remove the
     * links from this multimap, and the backlink provides an iterator
     * into the multimap that makes this operation very efficient.
     *
     * @param backlink the ExtIntTable multimap iterator for this route.
     */
    inline void set_backlink(typename RouteBackLink::iterator v);

    /**
     * Get the backlink.
     * @see ResolvedIPRouteEntry<A>::set_backlink
     *
     * @return the backlink iterator.
     */
    inline typename RouteBackLink::iterator backlink() const;

private:
    mutable const IPRouteEntry<A>* _igp_parent;
    mutable const IPRouteEntry<A>* _egp_parent;

    // _backlink is used for removing the corresponding entry from the
    // RouteTable's map that is indexed by igp_parent.  Without it,
    // route deletion would be expensive.
    typename RouteBackLink::iterator _backlink;
};

template <typename A>
inline void
ResolvedIPRouteEntry<A>::set_backlink(typename RouteBackLink::iterator v)
{
    _backlink = v;
}

template <typename A>
inline typename ResolvedIPRouteEntry<A>::RouteBackLink::iterator
ResolvedIPRouteEntry<A>::backlink() const
{
    return _backlink;
}

typedef ResolvedIPRouteEntry<IPv4> ResolvedIPv4RouteEntry;
typedef ResolvedIPRouteEntry<IPv6> ResolvedIPv6RouteEntry;

#endif // __RIB_ROUTE_HH__
