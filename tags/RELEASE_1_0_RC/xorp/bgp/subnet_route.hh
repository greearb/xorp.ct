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

// $XORP: xorp/bgp/subnet_route.hh,v 1.13 2004/05/19 10:27:01 mjh Exp $

#ifndef __BGP_SUBNET_ROUTE_HH__
#define __BGP_SUBNET_ROUTE_HH__

#include "path_attribute.hh"
#include "attribute_manager.hh"
#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"

#define SRF_IN_USE 0x0001
#define SRF_WINNER 0x0002
#define SRF_FILTERED 0x0004
#define SRF_DELETED 0x0008
#define SRF_NH_RESOLVED 0x0010
#define SRF_REFCOUNT 0xffff0000

//Defining paranoid emables some additional checks to ensure we don't
//try to reuse deleted data, or follow an obsolete parent_route
//pointer.
template<class A>
class SubnetRouteRef;
template<class A>
class SubnetRouteConstRef;

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
		const PathAttributeList<A> *attributes,
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
		const PathAttributeList<A> *attributes,
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

    /**
     * @return the address of the nexthop for this route.
     */
    const A& nexthop() const {return _attributes->nexthop();}

    /**
     * @return a pointer to the path attribute list for this route.
     */
    const PathAttributeList<A> *attributes() const {return _attributes;}

    /**
     * @return whether or not this route is in use.  "in use" here
     * does not mean the route won the decision process, but rather
     * that it was at least a contender for decision, and was not
     * filtered in the incoming filter bank.
     */
    bool in_use() const {return (_flags & SRF_IN_USE) != 0;}

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
    bool is_winner() const {return (_flags & SRF_WINNER) != 0;}

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
    bool nexthop_resolved() const {return (_flags & SRF_NH_RESOLVED) != 0;}

    /**
     * is_filtered returns true if the route was filtered out by the
     * incoming filter bank, false otherwise.  As such it only makes
     * sense calling this on routes that are stored in the RibIn.
     */
    bool is_filtered() const {return (_flags & SRF_FILTERED) != 0;}

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
    bool is_deleted() const {return (_flags & SRF_DELETED) != 0;}

    /**
     * @returns a string representation of the route for debugging purposes
     */
    string str() const;

    /**
     * DEBUGGING ONLY
     */
    int number_of_managed_atts() const {
	return _att_mgr.number_of_managed_atts();
    }

    /**
     * @returns the IGP routing protocol metric that applied when the
     * route won the decision process.  If the route has not won, this
     * value is undefined.
     */
    uint32_t igp_metric() const {return _igp_metric;}

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
    inline const SubnetRoute<A>* parent_route() const {
	return _parent_route;
    }

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

    inline uint16_t refcount() const {
	return (_flags & SRF_REFCOUNT)>>16;
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

    inline void bump_refcount(int delta) const {
	assert(delta == 1 || delta == -1);
	uint16_t refs = refcount();
	if (delta == 1) {
	    assert(refs < 0xffff);
	} else {
	    assert(refs > 0);
	}
	refs += delta;

	//re-insert the new ref count
	_flags = (_flags ^ (_flags&SRF_REFCOUNT)) | (refs << 16);

	//handle delayed deletion
	if ((refs==0) && ((_flags & SRF_DELETED) != 0)) {
	    delete this;
	}
    }
    //prevent accidental use of default assignment operator
    const SubnetRoute<A>& operator=(const SubnetRoute<A>&);

    /**
     * static attribute manager shared by all SubnetRoutes.  This is
     * used to ensure that we only need to store each path attribute
     * list once, because we often have multiple subnet routes with
     * the same path attribute list.
     */
    static AttributeManager<A> _att_mgr;

    /**
     * _net is the subnet (address and prefix) for this route.
     */
    IPNet<A> _net;

    /**
     * _attributes is a pointer to the path attribute list for this
     * route.  The actual data storage for the path attribute list is
     * handled inside the attribute manager
     */
    const PathAttributeList<A> *_attributes;

    /**
     * _parent_route is a pointer to the original version of this
     * route, before any filters modified it.  It may be NULL,
     * indicating this is the original version.
     */
    const SubnetRoute<A> *_parent_route;

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
    mutable uint32_t _flags;

    /**
     * If the route is a winner (SRF_WINNER is set), then
     * DecisionTable will fill in the IGP metric that was used in
     * deciding the route was a winner 
     */
    mutable uint32_t _igp_metric;
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
