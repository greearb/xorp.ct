// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rib/route.hh,v 1.3 2003/02/23 06:45:13 pavlin Exp $

#ifndef __RIB_ROUTE_HH__
#define __RIB_ROUTE_HH__

#include "libxorp/xorp.h"
#include <set>
#include <map>
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/vif.hh"
#include "libxorp/nexthop.hh"
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
     * @param nh the NextHop router to which packets matching this
     * entry should be forwarded.
     * @param proto the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     */
    RouteEntry(Vif *vif, NextHop *nh, const Protocol& proto, uint16_t metric);

    /**
     * Destructor
     */
    virtual ~RouteEntry();

    /**
     * Get the VIF
     *
     * @return the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.  
     */
    Vif *vif() const { return _vif; }

    /**
     * Get the NextHop
     *
     * @return the NextHop router to which packets matching this
     * entry should be forwarded.
     */
    inline NextHop *nexthop() const { return _nh; }

    /** 
     * Set the NextHop
     *
     * @param nh the NextHop router to be set on this route.
     */
    void set_nexthop(NextHop *nh) { _nh = nh; }

    /**
     * Get the Administrative Distance
     *
     * @return the Administrative Distance associated with this route.
     * Admin Distance is a parameter typically assigned on a
     * per-routing-protocol basis.  When two routes for the same
     * subnet come from different routing protocols, the one with the
     * lower admin distance is prefered
     */
    inline uint16_t admin_distance() const { return _admin_distance; }

    /**
     * Set the Administrative Distance
     *
     * @param ad the administrative distance to apply to this route.
     */
    void set_admin_distance(uint16_t ad) { _admin_distance = ad; }

    /**
     * Get the routing protocol
     *
     * @return the routing protocol that originated this route.
     */
    const Protocol &protocol() const { return _proto; }

    /**
     * Display the route for debugging purposes
     */
    virtual string str() const = 0;

    /**
     * Set the routing protocol metric on this route
     *
     * @param metric the routing protocol metric to be set on this route.
     */
    void set_metric(uint16_t metric) { _metric = metric; }

    /**
     * Get a metric for the route that can be directly compared, even
     * when the routes originate from different protocols.  Lower
     * metrics are prefered.
     *
     * @return a metric where the most significant bits are the admin
     * distance and the least significant bits are the routing
     * protocol metric.
     */
    uint32_t global_metric() const {
	return ((_admin_distance << 16) | _metric);
    }

    /**
     * Get the routing protocol metric.
     *
     * @return the routing protocol metric for this route
     */
    uint32_t metric() const {
	return _metric;
    }
protected:
    Vif *_vif;
    NextHop *_nh;
    uint16_t _admin_distance; // lower is better
    uint16_t _metric; // lower is better

    // The routing protocol that instantiated this route
    const Protocol& _proto;

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
     * Constructor for IPRouteEntry
     *
     * @param net the Subnet (address and mask) of the routing table entry.
     * @param vif the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.  
     * @param nh the NextHop router to which packets matching this
     * entry should be forwarded.
     * @param proto the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     */
    IPRouteEntry(const IPNet<A> &net, Vif *vif, NextHop *nh,
		 const Protocol& proto, uint16_t metric) :
	RouteEntry(vif, nh, proto, metric), _net(net) {}
    /**
     * Destructor for Routing Table Entry
     */
    ~IPRouteEntry() {}

    /**
     * Indicates if the route is for directly connected subnet.
     * 
     * @return true is the subnet is directly connected to this
     * router, otherwise false
     */
    inline bool directly_connected() const {
	return _vif ? _vif->is_same_subnet(IPvXNet(_net)) : false;
    }

    /**
     * Get the route entry's subnet
     *
     * @return the route entry's subnet
     */
    inline const IPNet<A> &net() const { return _net; }

    /**
     * Get the prefix length of the route entry's subnet
     *
     * @return the prefix length (in bits) of the route entry's subnet
     */
    inline size_t prefix_len() const { return _net.prefix_len(); }

    /**
     * Get the route entry as a string for debugging purposes.
     *
     * @return a human readable representation of the route entry
     */
    string str() const {
	string dst = (_net.is_valid()) ? _net.str() : string("NULL");
	string vif = (_vif) ? string(_vif->name()) : string("NULL");
	return string("Dst: ") + dst + string(" Vif: ") + vif +
	    string(" NextHop: ") + _nh->str() + string(" Metric: ") +
	    c_format("%d", _metric);
    }
protected:
    IPNet<A> _net;
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
    typedef
    multimap<const IPRouteEntry<A>*, ResolvedIPRouteEntry<A> *> RouteBackLink;

public:
    /**
     * Constructor for IPRouteEntry
     *
     * @param net the Subnet (address and mask) of the routing table entry.
     * @param vif the Virtual Interface on which packets matching this
     * routing table entry should be forwarded.  
     * @param nh the NextHop router to which packets matching this
     * entry should be forwarded.  This should be a local nexthop.
     * @param proto the routing protocol that originated this route.
     * @param metric the routing protocol metric for this route.
     * @param igp_parent the route entry used to resolve the non-local
     * nexthop in the egp_parent into a local nexthop.
     * @param egp_parent the orginal route entry with a non-local nexthop.
     */
    ResolvedIPRouteEntry(const IPNet<A> &net, Vif *vif, NextHop *nh,
			 const Protocol& proto, uint16_t metric,
			 const IPRouteEntry<A>* igp_parent,
			 const IPRouteEntry<A>* egp_parent) :
	IPRouteEntry<A>(net, vif, nh, proto, metric),
	_igp_parent(igp_parent),
	_egp_parent(egp_parent)
    { }

    /**
     * Get the igp_parent
     *
     * @return the IGP parent route entry that was used to resolve the
     * EGP parent route entry's non-local nexthop into a local
     * nexthop.  
     */
    const IPRouteEntry<A> *igp_parent() const { return _igp_parent; }

    /**
     * Get the EGP parent
     *
     * @return the EGP parent, which is the original route entry that
     * had a non-local nexthop 
     */
    const IPRouteEntry<A> *egp_parent() const { return _egp_parent; }

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
    void set_backlink(typename RouteBackLink::iterator backlink) 
    { 
	_backlink = backlink; 
    }

    /**
     * Get the backlink.  
     * @see ResolvedIPRouteEntry<A>::set_backlink
     *
     * @return the backlink iterator.
     */
    typename RouteBackLink::iterator backlink() const { return _backlink; }
private:
    mutable const IPRouteEntry<A>* _igp_parent;
    mutable const IPRouteEntry<A>* _egp_parent;

    /* _backlink is used for removing the corresponding entry from the
       RouteTable's map that is indexed by igp_parent.  Without it,
       route deletion would be expensive*/
    typename RouteBackLink::iterator _backlink;
};

typedef ResolvedIPRouteEntry<IPv4> ResolvedIPv4RouteEntry;
typedef ResolvedIPRouteEntry<IPv6> ResolvedIPv6RouteEntry;

#endif // __RIB_ROUTE_HH__
