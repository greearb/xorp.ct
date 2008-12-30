// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/subnet_route.hh,v 1.27 2008/10/02 21:56:21 bms Exp $

#ifndef __BGP_SUBNET_ROUTE_HH__
#define __BGP_SUBNET_ROUTE_HH__

#include "path_attribute.hh"
#include "attribute_manager.hh"
#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"

#include "policy/backend/policytags.hh"
#include "policy/backend/policy_filter.hh"

#define SRF_IN_USE		0x00000001
#define SRF_WINNER		0x00000002
#define SRF_FILTERED		0x00000004
#define SRF_DELETED		0x00000008
#define SRF_NH_RESOLVED		0x00000010
#define SRF_AGGR_BRIEF_MODE	0x00000080
#define SRF_AGGR_PREFLEN_MASK	0x0000ff00
#define SRF_REFCOUNT		0xffff0000

// Aggregation flags
// XXX Marko any better ideas where to put those?
#define SR_AGGR_IGNORE			0xff
#define SR_AGGR_IBGP_ONLY		0xe0
#define SR_AGGR_EBGP_AGGREGATE		0xd0
#define SR_AGGR_EBGP_NOT_AGGREGATED	0xd1
#define SR_AGGR_EBGP_WAS_AGGREGATED	0xd2

//Defining paranoid emables some additional checks to ensure we don't
//try to reuse deleted data, or follow an obsolete parent_route
//pointer.
template<class A>
class SubnetRouteRef;
template<class A>
class SubnetRouteConstRef;

class RouteMetaData {
public:
    RouteMetaData(const RouteMetaData& metadata);

    RouteMetaData();

    ~RouteMetaData() {
	// prevent accidental reuse after deletion
	_flags = 0xffffffff;
    }

    /**
     * @return whether or not this route is in use.  "in use" here
     * does not mean the route won the decision process, but rather
     * that it was at least a contender for decision, and was not
     * filtered in the incoming filter bank.
     */
    inline bool in_use() const {return (_flags & SRF_IN_USE) != 0;}

    /**
     * Record whether or not this route is in use.  "in use" here
     * does not mean the route won the decision process, but rather
     * that it was at least a contender for decision, and was not
     * filtered in the incoming filter bank.
     *
     * @param used true if the route is "in use".
     */
    void set_in_use(bool used) {
	if (used) {
	    _flags |= SRF_IN_USE;
	} else {
	    _flags &= ~SRF_IN_USE;
	}
    }

    /**
     * returns true if the route was chosen by the routing decision
     * process as the winning route for this subnet.
     *
     * is_winner should NOT be called by anything other than the
     * DecisionTable because caching means that it may not return the
     * right answer anywhere else
     */
    inline bool is_winner() const {return (_flags & SRF_WINNER) != 0;}

    /**
     * when a route is chosen by the routing decision process as the
     * winning route for this subnet, set_is_winner should be called
     * to record this fact and to record the igp_metric at the time
     * the route was chosen.
     *
     * set_is_winner should NOT be called by anything other than the
     * DecisionTable because caching means that it may not return the
     * right answer anywhere else
     */
    void set_is_winner(uint32_t igp_metric) {
	_flags |= SRF_WINNER;
	_igp_metric = igp_metric;
    }

    /**
     * when a route fails to be chosen by the routing decision process as the
     * winning route for this subnet, set_is_not_winner should be called
     * to record this fact.
     */
    inline void set_is_not_winner() {
	_flags &= ~SRF_WINNER;
    }

    /**
     * record whether or not a route's nexthop resolved
     */
    void set_nexthop_resolved(bool resolvable) {
	if (resolvable) {
	    _flags |= SRF_NH_RESOLVED;
	} else {
	    _flags &= ~SRF_NH_RESOLVED;
	}
    }

    /**
     * did the route's nexthop resolve when it was passed through the
     * NextHop resolver table.
     */
    inline bool nexthop_resolved() const {
	return (_flags & SRF_NH_RESOLVED) != 0;
    }

    /**
     * is_filtered returns true if the route was filtered out by the
     * incoming filter bank, false otherwise.  As such it only makes
     * sense calling this on routes that are stored in the RibIn.
     */
    inline bool is_filtered() const {return (_flags & SRF_FILTERED) != 0;}

    /**
     * set_filtered record whether or not the route was filtered out
     * by the incoming filter bank.  As such it only
     * makes sense calling this on routes that are stored in the
     * RibIn.
     *
     * @param filtered true if the route was filtered, false otherwise.
     */
    void set_filtered(bool filtered) {
	if (filtered) {
	    _flags |= SRF_FILTERED;
	} else {
	    _flags &= ~SRF_FILTERED;
	}
    }

    /**
     * is_deleted returns true if the route has already been deleted
     * (but the class instance representing it has not been because
     * it's reference count is non-zero) 
     */
    inline bool is_deleted() const {return (_flags & SRF_DELETED) != 0;}

    inline void set_deleted() {_flags |= SRF_DELETED;}

    /**
     * @returns the IGP routing protocol metric that applied when the
     * route won the decision process.  If the route has not won, this
     * value is undefined.
     */
    inline uint32_t igp_metric() const {return _igp_metric;}

    inline void set_igp_metric(uint32_t igp_metric) {
	_igp_metric = igp_metric;
    }

    inline void dont_aggregate() {
	_flags |= SRF_AGGR_PREFLEN_MASK;
    }

    inline uint16_t refcount() const { 
	return (_flags & SRF_REFCOUNT) >> 16; 
    }

    //set our reference count to one (our own self-reference)
    //and clear the deleted flag
    inline void reset_flags() {
	_flags ^= (_flags & (SRF_REFCOUNT | SRF_DELETED));
    }

    /**
     * @return policy tags associated with route.
     */
    inline const PolicyTags& policytags() const {
	return _policytags;
    }

    /**
     * Replaced policy tags of route.
     *
     * @param tags new policy tags for route.
     */
    inline void set_policytags(const PolicyTags& tags) {
	_policytags = tags;
    }

    inline const RefPf& policyfilter(uint32_t i) const {
	return _pfilter[i];
    }

    inline void set_policyfilter(uint32_t i, const RefPf& pf) {
	_pfilter[i] = pf;
    }
    
    /**
     * Set the "brief" mode flag on an candidate for aggregation.
     */
    void set_aggr_brief_mode() {
	_flags |= SRF_AGGR_BRIEF_MODE;
    }

    /**
     * Clear the "brief" mode flag on an candidate for aggregation.
     */
    void clear_aggr_brief_mode() {
	_flags &= ~SRF_AGGR_BRIEF_MODE;
    }

    /**
     * Read the "brief" aggregation mode flag.
     */
    bool aggr_brief_mode() const {
	return (_flags & SRF_AGGR_BRIEF_MODE);
    }

    /**
     * Set the target prefix length on an candidate for aggregation.
     * The field is also used for storing aggregation markers. 
     *
     * @param preflen prefix length of the requested aggregate route.
     */
    void set_aggr_prefix_len(uint32_t preflen) {
	_flags = (_flags & ~SRF_AGGR_PREFLEN_MASK) | 
		 ((preflen << 8) & SRF_AGGR_PREFLEN_MASK);
    }

    /**
     * Read the aggregation prefix length marker.
     * The field is also used for storing aggregation markers.
     */
    uint32_t aggr_prefix_len() const {
	return (_flags & SRF_AGGR_PREFLEN_MASK) >> 8;
    }

    bool bump_refcount(int delta) {
	XLOG_ASSERT(delta == 1 || delta == -1);
	uint16_t refs = refcount();
	if (delta == 1) {
	    XLOG_ASSERT(refs < 0xffff);
	} else {
	    XLOG_ASSERT(refs > 0);
	}
	refs += delta;

	//re-insert the new ref count
	_flags = (_flags ^ (_flags&SRF_REFCOUNT)) | (refs << 16);

	//handle delayed deletion
	if ((refs==0) && ((_flags & SRF_DELETED) != 0)) {
	    return true;
	}
	return false;
    }

private:
    /**
     * Flag definitions: 
     *
     * SRF_IN_USE indicates whether this route is currently
     * used for anything.  The route might not be used if it wasn't
     * chosen by the BGP decision mechanism, or if a downstream filter
     * caused a modified version of this route to be installed in a
     * cache table 
     *
     * SRF_WINNER indicates that the route won the decision process.  
     *
     * SRF_FILTERED indicates that the route was filtered downstream.
     * Currently this is only used for RIB-IN routes that are filtered
     * in the inbound filter bank
     *
     * SRF_REFCOUNT (16 bits) maintains a reference count of the number
     * of objects depending on this SubnetRoute instance.  Deletion
     * will be delayed until the reference count reaches zero
     */
    uint32_t _flags;

    /**
     * If the route is a winner (SRF_WINNER is set), then
     * DecisionTable will fill in the IGP metric that was used in
     * deciding the route was a winner 
     */
    uint32_t _igp_metric;

    PolicyTags _policytags;
    RefPf _pfilter[3];
};

/**
 * @short SubnetRoute holds a BGP routing table entry.
 *
 * SubnetRoute is the basic class used to hold a BGP routing table
 * entry in BGP's internal representation.  It's templated so the same
 * code can be used to hold IPv4 or IPv6 routes - their representation
 * is essentially the same internally, even though they're encoded
 * differently on the wire by the BGP protocol.  A route essentially
 * consists of the subnet (address and prefix) referred to by the
 * route, a BGP path attribute list, and some metadata for our own
 * use.  When a route update from BGP comes it, it is split into
 * multiple subnet routes, one for each NLRI (IPv4) or MP_REACH (IPv6)
 * attribute.  SubnetRoute is also the principle way routing
 * information is passed around internally in our BGP implementation.
 * 
 * SubnetRoute is reference-counted - delete should NOT normally be
 * called directly on a SubnetRoute; instead unref should be called,
 * which will decrement the reference count, and delete the instance
 * if the reference count has reached zero.
 */

template<class A>
class SubnetRoute
{
    friend class SubnetRouteRef<A>;
    friend class SubnetRouteConstRef<A>;
public:
    /**
     * @short SubnetRoute copy constructor
     */
    SubnetRoute(const SubnetRoute<A>& route_to_clone);

    /**
     * @short SubnetRoute constructor
     *
     * @param net the subnet (address and prefix) this route refers to.
     * @param attributes pointer to the path attribute list associated with          * this route.
     * @param parent_route the SubnetRoute that this route was derived
     * from.  For example, if a filter takes one SubnetRoute and
     * generates a modified version, the parent_route of the new one
     * should point to the original.  If this is set to non-NULL, care
     * must be taken to ensure the original route is never deleted
     * before the derived route.
     */
    SubnetRoute(const IPNet<A> &net, 
		PAListRef<A> attributes,
		const SubnetRoute<A>* parent_route);

    /**
     * @short SubnetRoute constructor
     *
     * @param net the subnet (address and prefix) this route refers to.
     * @param attributes pointer to the path attribute list associated with          * this route.
     * @param parent_route the SubnetRoute that this route was derived
     * from.  For example, if a filter takes one SubnetRoute and
     * generates a modified version, the parent_route of the new one
     * should point to the original.  If this is set to non-NULL, care
     * must be taken to ensure the original route is never deleted
     * before the derived route.
     * @param igp_metric the IGP routing protocol metric to reach the
     * nexthop obtained from the RIB.
     */
    SubnetRoute(const IPNet<A> &net, 
		PAListRef<A> attributes,
		const SubnetRoute<A>* parent_route,
		uint32_t igp_metric);

    /**
     * @short Equality comparison for SubnetRoutes
     *
     * Equality comparison for SubnetRoutes.  Only the subnet and
     * attributes are compared, not the metadata such as flags,
     * igp_metric, etc
     *
     * @param them Route to be compared against.
     */
    bool operator==(const SubnetRoute<A>& them) const;

    /**
     * @return the subnet this route refers to.
     */
    const IPNet<A>& net() const {return _net;}

#if 0
    /**
     * @return the address of the nexthop for this route.
     */
    // remove this because we shouldn't be pulling attributes directly
    // out of the stored version - use a FastPathAttributeList
    // instead.
    const A& nexthop() const {return _attributes.nexthop();}
#endif

    /**
     * @return a pointer to the path attribute list for this route.
     */
    PAListRef<A> attributes() const {return _attributes;}

    /**
     * @return whether or not this route is in use.  "in use" here
     * does not mean the route won the decision process, but rather
     * that it was at least a contender for decision, and was not
     * filtered in the incoming filter bank.
     */
    bool in_use() const {return _metadata.in_use();}

    /**
     * Record whether or not this route is in use.  "in use" here
     * does not mean the route won the decision process, but rather
     * that it was at least a contender for decision, and was not
     * filtered in the incoming filter bank.
     *
     * @param used true if the route is "in use".
     */
    void set_in_use(bool used) const;

    /**
     * returns true if the route was chosen by the routing decision
     * process as the winning route for this subnet.
     *
     * is_winner should NOT be called by anything other than the
     * DecisionTable because caching means that it may not return the
     * right answer anywhere else
     */
    bool is_winner() const {return _metadata.is_winner();}

    /**
     * when a route is chosen by the routing decision process as the
     * winning route for this subnet, set_is_winner should be called
     * to record this fact and to record the igp_metric at the time
     * the route was chosen.
     *
     * set_is_winner should NOT be called by anything other than the
     * DecisionTable because caching means that it may not return the
     * right answer anywhere else
     */
    void set_is_winner(uint32_t igp_metric) const;

    /**
     * when a route fails to be chosen by the routing decision process as the
     * winning route for this subnet, set_is_not_winner should be called
     * to record this fact.
     */
    void set_is_not_winner() const;

    /**
     * record whether or not a route's nexthop resolved
     */
    void set_nexthop_resolved(bool resolved) const;

    /**
     * did the route's nexthop resolve when it was passed through the
     * NextHop resolver table.
     */
    bool nexthop_resolved() const {return _metadata.nexthop_resolved();}

    /**
     * is_filtered returns true if the route was filtered out by the
     * incoming filter bank, false otherwise.  As such it only makes
     * sense calling this on routes that are stored in the RibIn.
     */
    bool is_filtered() const {return _metadata.is_filtered();}

    /**
     * set_filtered record whether or not the route was filtered out
     * by the incoming filter bank.  As such it only
     * makes sense calling this on routes that are stored in the
     * RibIn.
     *
     * @param filtered true if the route was filtered, false otherwise.
     */
    void set_filtered(bool filtered) const;

    /**
     * is_deleted returns true if the route has already been deleted
     * (but the class instance representing it has not been because
     * it's reference count is non-zero) 
     */
    bool is_deleted() const {return _metadata.is_deleted();}

    /**
     * @returns a string representation of the route for debugging purposes
     */
    string str() const;


    /**
     * @returns the IGP routing protocol metric that applied when the
     * route won the decision process.  If the route has not won, this
     * value is undefined.
     */
    uint32_t igp_metric() const {return _metadata.igp_metric();}

    /** 
     * @returns the original version of this route, before any filters
     * were applied to it to modify it.  If no filters have been
     * applied, this may be this route itself.
     */
    const SubnetRoute<A> *original_route() const {
	if (_parent_route)
	    return _parent_route;
	else
	    return this;
    }

    /**
     * @returns the original version of this route, before any filters
     * were applied to it to modify it.  If no filters have been
     * applied, NULL will be returned.
     */
    const SubnetRoute<A>* parent_route() const { return _parent_route; }

    /**
     * @short record the original version of this route, before any
     * filters were applied.
     * 
     * @param parent the original version of this route.
     */
    void set_parent_route(const SubnetRoute<A> *parent);

    /**
     * @short delete this route
     *
     * unref is called to delete a SubnetRoute, and should be called
     * instead of calling delete on the route.  SubnetRoutes are
     * reference-counted, so unref decrements the reference count and
     * deletes the storage only if the reference count reaches zero.
     */
    void unref() const;

    uint16_t refcount() const { return _metadata.refcount(); }

    /**
     * @return policy tags associated with route.
     */
    const PolicyTags& policytags() const {
	return _metadata.policytags();
    }

    /**
     * Replaced policy tags of route.
     *
     * @param tags new policy tags for route.
     */
    void set_policytags(const PolicyTags& tags) const {
	_metadata.set_policytags(tags);
    }

    const RefPf& policyfilter(uint32_t i) const;
    void set_policyfilter(uint32_t i, const RefPf& pf) const;
    
    /**
     * Set the "brief" mode flag on an candidate for aggregation.
     */
    void set_aggr_brief_mode() const {
	_metadata.set_aggr_brief_mode();
    }

    /**
     * Clear the "brief" mode flag on an candidate for aggregation.
     */
    void clear_aggr_brief_mode() const {
	_metadata.clear_aggr_brief_mode();
    }

    /**
     * Read the "brief" aggregation mode flag.
     */
    bool aggr_brief_mode() const {
	return _metadata.aggr_brief_mode();
    }

    /**
     * Set the target prefix length on an candidate for aggregation.
     * The field is also used for storing aggregation markers. 
     *
     * @param preflen prefix length of the requested aggregate route.
     */
    void set_aggr_prefix_len(uint32_t preflen) {
	_metadata.set_aggr_prefix_len(preflen);
    }

    /**
     * Read the aggregation prefix length marker.
     * The field is also used for storing aggregation markers.
     */
    uint32_t aggr_prefix_len() const {
	return _metadata.aggr_prefix_len();
    }

protected:
    /**
     * @short protected SubnetRoute destructor.
     *
     * The destructor is protected because you are not supposed to
     * directly delete a SubnetRoute.  Instead you should call unref()
     * and the SubnetRoute will be deleted when it's reference count
     * reaches zero.
     */
    ~SubnetRoute();
private:

    void bump_refcount(int delta) const {
	if (_metadata.bump_refcount(delta))
	    delete this;
    }
    //prevent accidental use of default assignment operator
    const SubnetRoute<A>& operator=(const SubnetRoute<A>&);


    /**
     * _net is the subnet (address and prefix) for this route.
     */
    IPNet<A> _net;

    /**
     * _attributes is a pointer to the path attribute list for this
     * route.  The actual data storage for the path attribute list is
     * handled inside the attribute manager
     */
    PAListRef<A> _attributes;

    /**
     * _parent_route is a pointer to the original version of this
     * route, before any filters modified it.  It may be NULL,
     * indicating this is the original version.
     */
    const SubnetRoute<A> *_parent_route;

    mutable RouteMetaData _metadata;
};


/**
 * @short a class to hold a reference to a SubnetRoute
 * 
 * SubnetRouteRef holds a reference to a SubnetRoute.  When it is
 * deleted or goes out of scope, the SubnetRoute's reference count
 * will be decreased, and the SubnetRoute may be deleted if the
 * reference count reaches zero.
 */
template<class A>
class SubnetRouteRef
{
public:
    SubnetRouteRef(SubnetRoute<A>* route) : _route(route)
    {
	if (_route)
	    _route->bump_refcount(1);
    }
    SubnetRouteRef(const SubnetRouteRef<A>& ref)
    {
	_route = ref._route;
	if (_route)
	    _route->bump_refcount(1);
    }
    ~SubnetRouteRef() {
	if (_route)
	    _route->bump_refcount(-1);
    }
    SubnetRoute<A>* route() const {return _route;}
private:
    //_route can be NULL
    SubnetRoute<A>* _route;
};

/**
 * @short a class to hold a const reference to a SubnetRoute
 * 
 * SubnetRouteRef holds a const reference to a SubnetRoute.  When it
 * is deleted or goes out of scope, the SubnetRoute's reference count
 * will be decreased, and the SubnetRoute may be deleted if the
 * reference count reaches zero.
 */
template<class A>
class SubnetRouteConstRef 
{
public:
    SubnetRouteConstRef(const SubnetRoute<A>* route) : _route(route)
    {
	if (_route)
	    _route->bump_refcount(1);
    }
    SubnetRouteConstRef(const SubnetRouteConstRef<A>& ref)
    {
	_route = ref._route;
	if (_route)
	    _route->bump_refcount(1);
    }
    ~SubnetRouteConstRef() {
	if (_route)
	    _route->bump_refcount(-1);
    }
    const SubnetRoute<A>* route() const {return _route;}
private:
    //_route can be NULL
    const SubnetRoute<A>* _route;
};

#endif // __BGP_SUBNET_ROUTE_HH__
