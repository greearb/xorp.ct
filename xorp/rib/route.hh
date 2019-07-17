// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __RIB_ROUTE_HH__
#define __RIB_ROUTE_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/vif.hh"
#include "libxorp/nexthop.hh"

#include "policy/backend/policytags.hh"

#include "protocol.hh"

#ifdef USE_BOOST

#include <boost/shared_ptr.hpp>
#define smart_ptr boost::shared_ptr

#else

#include "libxorp/ref_ptr.hh"
#define smart_ptr ref_ptr

#endif

template<class A>
class RibVif;

/**
 * @short Base class for RIB routing table entries.
 *
 * This is the base class from which RIB routing table entries are derived.
 * It's not useful by itself.
 */
template<class A>
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
    RouteEntry(RibVif<A>* vif, const Protocol* protocol,
		uint32_t metric, const PolicyTags& policytags,
		const IPNet<A>& net, uint16_t admin_distance = UNKNOWN_ADMIN_DISTANCE);

    RouteEntry(RibVif<A>* vif, const Protocol* protocol,
		uint32_t metric, const IPNet<A>& net,
		uint16_t admin_distance = UNKNOWN_ADMIN_DISTANCE);

    RouteEntry(RibVif<A>* vif, const Protocol* protocol,
		uint32_t metric, smart_ptr<PolicyTags>& policytags,
		const IPNet<A>& net, uint16_t admin_distance = UNKNOWN_ADMIN_DISTANCE);

    RouteEntry(const RouteEntry<A>& r);

    RouteEntry& operator=(const RouteEntry<A>& r);

    /**
     * Destructor
     */
    virtual ~RouteEntry();

    /**
     * Get the VIF.
     *
     * @return the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.
     */
    RibVif<A>* vif() const { return _vif; }

    /**
     * Get the NextHop router.
     *
     * @return the NextHop router to which packets matching this
     * entry should be forwarded.
     */
    virtual NextHop* nexthop() const = 0;

    /**
     * Get the Administrative Distance.
     *
     * @return the Administrative Distance associated with this route.
     * Admin Distance is a parameter typically assigned on a
     * per-routing-protocol basis.  When two routes for the same
     * subnet come from different routing protocols, the one with the
     * lower admin distance is prefered.
     */
    uint16_t admin_distance() const { return _admin_distance; }

    /**
     * Set the Administrative Distance.
     *
     * @param ad the administrative distance to apply to this route.
     */
    void set_admin_distance(uint16_t ad) { _admin_distance = ad; }

    /**
     * Get the routing protocol.
     *
     * @return the routing protocol that originated this route.
     * @see Protocol.
     */
    const Protocol* protocol() const { return _protocol; }

    /**
     * Display the route for debugging purposes.
     */
    virtual string str() const = 0;

    /**
     * Set the routing protocol metric on this route.
     *
     * @param metric the routing protocol metric to be set on this route.
     */
    void set_metric(uint32_t metric) { _metric = metric; }

    /**
     * Get the routing protocol metric.
     *
     * @return the routing protocol metric for this route.
     */
    uint32_t metric() const { return _metric; }

    virtual const IPNet<A>& net() const { return _net; };

    size_t prefix_len() const  { return _net.prefix_len(); };

    virtual const A& nexthop_addr() const = 0;

    /**
     * Get the policy-tags for this route.
     *
     * @return the policy-tags for this route.
     */
    PolicyTags& policytags() { return *_policytags; }
    const PolicyTags& policytags() const { return *_policytags; }

    smart_ptr<PolicyTags>& policytags_shared() { return _policytags; }

protected:
    RibVif<A>* _vif;

    const Protocol* _protocol;		// The routing protocol that instantiated this route

    uint16_t	_admin_distance;	// Lower is better
    uint32_t	_metric;		// Lower is better
    smart_ptr<PolicyTags> _policytags;	// Tags used for policy route redistribution
    IPNet<A>	_net;			// The route entry's subnet address
};

/**
 * @short Routing Table Entry
 *
 * This class stores a regular RIB routing table entry for either an
 * IPv4 or an IPv6 route.  It is a template class, where A is either a
 * the IPv4 class or the IPv6 class.
 */
template <class A>
class IPRouteEntry : public RouteEntry<A> {
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
    IPRouteEntry(const IPNet<A>& net, RibVif<A>* vif, IPNextHop<A>* nexthop,
		 const Protocol* protocol, uint32_t metric)
	: RouteEntry<A>(vif, protocol, metric, net), _nexthop(nexthop) { XLOG_ASSERT(nexthop); }

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
    IPRouteEntry(const IPNet<A>& net, RibVif<A>* vif, IPNextHop<A>* nexthop,
		 const Protocol* protocol, uint32_t metric,
		 const PolicyTags& policytags)
	: RouteEntry<A>(vif, protocol, metric, policytags, net), _nexthop(nexthop) { XLOG_ASSERT(nexthop); }

    IPRouteEntry(const IPNet<A>& net, RibVif<A>* vif,
	smart_ptr<IPNextHop<A> >& nexthop, const Protocol* protocol, uint32_t metric,
	smart_ptr<PolicyTags>& policytags, uint16_t admin_distance)
	: RouteEntry<A>(vif, protocol, metric, policytags, net, admin_distance), _nexthop(nexthop) { }

    IPRouteEntry(const IPRouteEntry<A>& rhs);

    IPRouteEntry<A>& operator=(const IPRouteEntry<A>& rhs);

    /**
     * Destructor for Routing Table Entry
     */
    virtual ~IPRouteEntry() { }

    /**
     * Get the NextHop router.
     *
     * @return the NextHop router to which packets matching this
     * entry should be forwarded.
     */
    IPNextHop<A>* nexthop() const { return _nexthop.get(); }

    smart_ptr<IPNextHop<A> >& nexthop_shared() { return _nexthop; }

    /**
     * Get the route entry's next-hop router address.
     *
     * @return the route entry's next-hop router address. If there is no
     * next-hop router, then the return value is IPv4#ZERO() or IPv6#ZERO().
     */
    const A& nexthop_addr() const { return _nexthop->addr(); }

    /**
     * Get the route entry as a string for debugging purposes.
     *
     * @return a human readable representation of the route entry.
     */
    string str() const;
    void* operator new(size_t size);
    void operator delete(void* ptr);
protected:
    smart_ptr<IPNextHop<A> > _nexthop;
private:
    static MemoryPool<IPRouteEntry<A> >& memory_pool();
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
    typedef multimap<const IPNet<A> , ResolvedIPRouteEntry<A>* > RouteBackLink;

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
    ResolvedIPRouteEntry(const IPRouteEntry<A>* resolving_parent,
			 const IPRouteEntry<A>* egp_parent)
	: IPRouteEntry<A>(egp_parent->net(), resolving_parent->vif(),
		const_cast<IPRouteEntry<A>*>(resolving_parent)->nexthop_shared(),
		egp_parent->protocol(), egp_parent->metric(),
		const_cast<IPRouteEntry<A>*>(egp_parent)->policytags_shared(),
		egp_parent->admin_distance()),
	  _resolving_parent(resolving_parent),
	  _egp_parent(egp_parent) { }

    ResolvedIPRouteEntry(const ResolvedIPRouteEntry<A>& r);
    ResolvedIPRouteEntry& operator=(const ResolvedIPRouteEntry<A>& r);

    /**
     * Get the igp_parent.
     *
     * @return the IGP parent route entry that was used to resolve the
     * EGP parent route entry's non-local nexthop into a local nexthop.
     */
    const IPRouteEntry<A>* resolving_parent() const { return _resolving_parent; }

    /**
     * Get the EGP parent.
     *
     * @return the EGP parent, which is the original route entry that
     * had a non-local nexthop.
     */
    const IPRouteEntry<A>* egp_parent() const { return _egp_parent; }

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
    void set_backlink(typename RouteBackLink::iterator v) { _backlink = v; }

    /**
     * Get the backlink.
     * @see ResolvedIPRouteEntry<A>::set_backlink
     *
     * @return the backlink iterator.
     */
    typename RouteBackLink::iterator backlink() const { return _backlink; }

    void* operator new(size_t size);
    void operator delete(void* ptr);

private:
    static MemoryPool<ResolvedIPRouteEntry<A> >& memory_pool();

    const IPRouteEntry<A>* _resolving_parent;
    const IPRouteEntry<A>* _egp_parent;

    // _backlink is used for removing the corresponding entry from the
    // RouteTable's map that is indexed by igp_parent.  Without it,
    // route deletion would be expensive.
    typename RouteBackLink::iterator _backlink;
};

typedef ResolvedIPRouteEntry<IPv4> ResolvedIPv4RouteEntry;
typedef ResolvedIPRouteEntry<IPv6> ResolvedIPv6RouteEntry;

/**
 * @short Extended Unresolved RouteEntry, used by ExtIntTable.
 *
 * This class stored an extended unresolved routing table entry, for use in
 * ExtIntTable.  When a route with a non-local nexthop arrives, the
 * ExtIntTable attempts to discover a local nexthop by finding the
 * route that packets to the non-local nexthop would use.  If the local
 * nexthop is not found, this entry is used to store the unresolved route.
 *
 * This is a template class, where A is either a the IPv4 class or the
 * IPv6 class.
 */
template <class A>
class UnresolvedIPRouteEntry {
public:
    typedef multimap<A, UnresolvedIPRouteEntry<A>* > RouteBackLink;

public:
    /**
     * Constructor for a given IPRouteEntry.
     *
     * @param route the IPRouteEntry route.
     */
    UnresolvedIPRouteEntry(const IPRouteEntry<A>* route)
	: _route(route) {}

    /**
     * Get the route.
     *
     * @return the route.
     */
    const IPRouteEntry<A>* route() const { return _route; }

    /**
     * Set the backlink.  When an unresolved route is created, the
     * ExtIntTable will store a link to it in a multimap that is
     * indexed by the unresolved nexthop.  This will allow all the routes
     * affected by a change (e.g., resolving the nexthop) to be found easily.
     * However, if the EGP parent goes away, we need to remove the
     * links from this multimap, and the backlink provides an iterator
     * into the multimap that makes this operation very efficient.
     *
     * @param backlink the ExtIntTable multimap iterator for this route.
     */
    void set_backlink(typename RouteBackLink::iterator v) { _backlink = v; }

    /**
     * Get the backlink.
     * @see UnresolvedIPRouteEntry<A>::set_backlink
     *
     * @return the backlink iterator.
     */
    typename RouteBackLink::iterator backlink() const { return _backlink; }

    void* operator new(size_t size);
    void operator delete(void* ptr);

private:
    static MemoryPool<UnresolvedIPRouteEntry<A> >& memory_pool();
    //
    // _backlink is used for removing the corresponding entry from the
    // RouteTable's map that is indexed by the unresolved nexthop.
    // Without it, route deletion would be expensive.
    //
    typename RouteBackLink::iterator	_backlink;
    const IPRouteEntry<A>*		_route;
};

typedef UnresolvedIPRouteEntry<IPv4> UnresolvedIPv4RouteEntry;
typedef UnresolvedIPRouteEntry<IPv6> UnresolvedIPv6RouteEntry;

#endif // __RIB_ROUTE_HH__
