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

// $XORP: xorp/bgp/subnet_route.hh,v 1.8 2003/03/10 23:20:06 hodson Exp $

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
#define SRF_REFCOUNT 0xffff0000

//Defining paranoid emables some additional checks to ensure we don't
//try to reuse deleted data, or follow an obsolete parent_route
//pointer.
template<class A>
class SubnetRouteRef;
template<class A>
class SubnetRouteConstRef;

template<class A>
class SubnetRoute
{
    friend class SubnetRouteRef<A>;
    friend class SubnetRouteConstRef<A>;
public:
    SubnetRoute(const SubnetRoute<A>& route_to_clone);
    SubnetRoute(const IPNet<A> &net, 
		const PathAttributeList<A> *attributes,
		const SubnetRoute<A>* parent_route);
    SubnetRoute(const IPNet<A> &net, 
		const PathAttributeList<A> *attributes,
		const SubnetRoute<A>* parent_route,
		uint32_t igp_metric);
    bool operator==(const SubnetRoute<A>& them) const;
    const IPNet<A>& net() const {return _net;}
    const A& nexthop() const {return _attributes->nexthop();}
    const PathAttributeList<A> *attributes() const {return _attributes;}
    bool in_use() const {return (_flags & SRF_IN_USE) != 0;}
    void set_in_use(bool used) const;

    /* the winner methods are used to set the route flag indicating
       that the route won the decision process.  These methods should
       NOT be called by anything other than the DecisionTable because
       caching means that they may not do the right thing anywhere
       else */
    bool is_winner() const {return (_flags & SRF_WINNER) != 0;}
    void set_is_winner(uint32_t igp_metric) const;
    void set_is_not_winner() const;

    bool is_filtered() const {return (_flags & SRF_FILTERED) != 0;}
    void set_filtered(bool filtered) const;
    string str() const;

    int number_of_managed_atts() const {
	return _att_mgr.number_of_managed_atts();
    }
    uint32_t igp_metric() const {return _igp_metric;}

    const SubnetRoute<A> *original_route() const {
	if (_parent_route)
	    return _parent_route;
	else
	    return this;
    }
    inline const SubnetRoute<A>* parent_route() const {
	return _parent_route;
    }


    void unref() const;

    void set_parent_route(const SubnetRoute<A> *parent);

    inline uint16_t refcount() const {
	return (_flags & SRF_REFCOUNT)>>16;
    }
protected:
    //The destructor is protected because you are not supposed to
    //directly delete a SubnetRoute.  Instead you should call unref()
    //and the SubnetRoute will be deleted when it's reference count
    //reaches zero.
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

    static AttributeManager<A> _att_mgr;
    IPNet<A> _net;
    const PathAttributeList<A> *_attributes;

    const SubnetRoute<A> *_parent_route;

    /*Flags: */
    /* SRF_IN_USE indicates whether this route is currently used for
       anything.  The route might not be used if it wasn't chosen by
       the BGP decision mechanism, or if a downstream filter caused a
       modified version of this route to be installed in a cache table */

    /* SRF_WINNER indicates that the route won the decision process.  */

    /* SRF_FILTERED indicates that the route was filtered downstream.
       Currently this is only used for RIB-IN routes that are filtered
       in the inbound filter bank */

    /* SRF_REFCOUNT (16 bits) maintains a reference count of the
       number of objects depending on this SubnetRoute instance.
       Deletion will be delayed until the reference count reaches zero */
    mutable uint32_t _flags;

    /* If the route is a winner (SRF_WINNER is set), then
       DecisionTable will fill in the IGP metric that was used in
       deciding the route was a winner */
    mutable uint32_t _igp_metric;
};

template<class A>
class SubnetRouteRef
{
public:
    SubnetRouteRef(SubnetRoute<A>* route) : _route(route)
    {
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

template<class A>
class SubnetRouteConstRef 
{
public:
    SubnetRouteConstRef(const SubnetRoute<A>* route) : _route(route)
    {
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
