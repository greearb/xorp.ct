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

#ident "$XORP: xorp/bgp/subnet_route.cc,v 1.25 2008/07/23 05:09:38 pavlin Exp $"

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "subnet_route.hh"


template<class A> AttributeManager<A> SubnetRoute<A>::_att_mgr;

template<class A>
SubnetRoute<A>::SubnetRoute(const SubnetRoute<A>& route_to_clone) 
{
    debug_msg("SubnetRoute constructor1 giving %p\n", this);
    //note that we need an explicit constructor here, rather than
    //relying in C++ for the default constructor, or we get the
    //reference counts wrong in the attribute manager

    _net = route_to_clone._net;
    _parent_route = route_to_clone._parent_route;

    const PathAttributeList<A>* atts;
    atts = route_to_clone.attributes();
    //the attribute manager handles memory management, and ensuring
    //that only one copy of each attribute list is ever stored
    _attributes = _att_mgr.add_attribute_list(atts);

    _flags = route_to_clone._flags;
    //set our reference count to one (our own self-reference)
    //and clear the deleted flag
    _flags ^= (_flags & (SRF_REFCOUNT | SRF_DELETED));

    //update parent refcount
    if (_parent_route)
	_parent_route->bump_refcount(1);
    _igp_metric = route_to_clone._igp_metric;

    _policytags = route_to_clone._policytags;

    for (int i = 0; i < 3; i++)
	_pfilter[i] = route_to_clone._pfilter[i];
}

template<class A>
SubnetRoute<A>::SubnetRoute(const IPNet<A> &n, 
			       const PathAttributeList<A>* atts,
			       const SubnetRoute<A>* parent_route)
    : _net(n), _parent_route(parent_route) {
    debug_msg("SubnetRoute constructor2 giving %p\n", this);
    //the attribute manager handles memory management, and ensuring
    //that only one copy of each attribute list is ever stored
    _attributes = _att_mgr.add_attribute_list(atts);

    _flags = 0;
    //the in_use flag is set to false so reduce the work we have to
    //re-do when we have to touch all the routes in a route_table.  It
    //should default to true if we don't know for sure that a route is
    //not used as this is always safe, if somewhat inefficient.
    _flags |= SRF_IN_USE;

    // we must set the aggregate_prefix_len to SR_AGGRLEN_IGNORE to
    // indicate that the route has not been (yet) marked for aggregation
    // the statement bellow is equivalent to
    // this->set_aggr_prefix_len(SR_AGGRLEN_IGNORE)
    _flags |= SRF_AGGR_PREFLEN_MASK;

    if (_parent_route) {
	_parent_route->bump_refcount(1);
    }

    _igp_metric = 0xffffffff;
}

template<class A>
SubnetRoute<A>::SubnetRoute(const IPNet<A> &n, 
			       const PathAttributeList<A>* atts,
			       const SubnetRoute<A>* parent_route,
			       uint32_t igp_metric)
    : _net(n), _parent_route(parent_route) {
    debug_msg("SubnetRoute constructor3 giving %p\n", this);
    //the attribute manager handles memory management, and ensuring
    //that only one copy of each attribute list is ever stored
    _attributes = _att_mgr.add_attribute_list(atts);

    _flags = 0;
    //the in_use flag is set to false so reduce the work we have to
    //re-do when we have to touch all the routes in a route_table.  It
    //should default to true if we don't know for sure that a route is
    //not used as this is always safe, if somewhat inefficient.
    _flags |= SRF_IN_USE;

    // we must set the aggregate_prefix_len to SR_AGGRLEN_IGNORE to
    // indicate that the route has not been (yet) marked for aggregation
    // the statement bellow is equivalent to
    // this->set_aggr_prefix_len(SR_AGGRLEN_IGNORE)
    _flags |= SRF_AGGR_PREFLEN_MASK;

    if (parent_route) {
	_parent_route->bump_refcount(1);
    }

    _igp_metric = igp_metric;
}

template<class A>
bool
SubnetRoute<A>::operator==(const SubnetRoute<A>& them) const {
    //only compare net and attributes, not flags
    if (!(_net == them._net))
	return false;
    if (!(*_attributes == *(them._attributes)))
	return false;
    return true;
}

template<class A>
SubnetRoute<A>::~SubnetRoute() {
    debug_msg("SubnetRoute destructor called for %p\n", this);
    _att_mgr.delete_attribute_list(_attributes);

    assert(refcount() == 0);
    if (_parent_route)
	_parent_route->bump_refcount(-1);

    //prevent accidental reuse after deletion.
    _net = IPNet<A>();
    _parent_route = (const SubnetRoute<A>*)0xbad;
    _attributes = (const PathAttributeList<A>*)0xbad;
    _flags = 0xffffffff;
}

template<class A>
void 
SubnetRoute<A>::unref() const {
    if ((_flags & SRF_DELETED) != 0) {
	XLOG_FATAL("SubnetRoute %p: multiple unref's\n", this);
    }
    
    if (refcount() == 0) 
	delete this;
    else {
	_flags |= SRF_DELETED;
    }
}

template<class A>
void 
SubnetRoute<A>::set_parent_route(const SubnetRoute<A> *parent) 
{
    assert(parent != this);
    if (_parent_route)
	_parent_route->bump_refcount(-1);
    _parent_route = parent;
    if (_parent_route)
	_parent_route->bump_refcount(1);
}

template<class A>
void 
SubnetRoute<A>::set_is_winner(uint32_t igp_metric) const {
    _flags |= SRF_WINNER;
    _igp_metric = igp_metric;
    if (_parent_route) {
	_parent_route->set_is_winner(igp_metric);
    }
}

template<class A>
void 
SubnetRoute<A>::set_is_not_winner() const {
    _flags &= ~SRF_WINNER;
    if (_parent_route) {
	_parent_route->set_is_not_winner();
    }
}

template<class A>
void 
SubnetRoute<A>::set_in_use(bool used) const {
    if (used) {
	_flags |= SRF_IN_USE;
    } else {
	_flags &= ~SRF_IN_USE;
    }
    if (_parent_route) {
	_parent_route->set_in_use(used);
    }

#ifdef DEBUG_FLAGS
    printf("set_in_use: %p = %s", this, bool_c_str(used));
    printf("\n%s\n", str().c_str());
#endif
}

template<class A>
void 
SubnetRoute<A>::set_nexthop_resolved(bool resolvable) const {
    if (resolvable) {
	_flags |= SRF_NH_RESOLVED;
    } else {
	_flags &= ~SRF_NH_RESOLVED;
    }
    if (_parent_route) {
	_parent_route->set_nexthop_resolved(resolvable);
    }

#ifdef DEBUG_FLAGS
    printf("set_nexthop_resolved: %p = %s", this, bool_c_str(resolvable));
    printf("\n%s\n", str().c_str());
#endif
}

template<class A>
void 
SubnetRoute<A>::set_filtered(bool filtered) const {
    if (filtered) {
	_flags |= SRF_FILTERED;
    } else {
	_flags &= ~SRF_FILTERED;
    }

#ifdef DEBUG_FLAGS
    printf("set_filtered: %p = %s", this, bool_c_str(filtered));
    printf("\n%s\n", str().c_str());
#endif
}

template<class A>
string
SubnetRoute<A>::str() const {
    string s;
    s = "SubnetRoute:\n";
    s += "  Net: " + _net.str() + "\n";
    s += "  PAList: " + _attributes->str();
#ifdef DEBUG_FLAGS
    s += "\n";
    s += "  Policytags: " + _policytags.str() + "\n";
    if (is_winner()) {
	s += "  route is winner\n";
    } else {
	s += "  route is not winner\n";
    }
    if (is_filtered()) {
	s += "  route is filtered";
    } else {
	s += "  route is not filtered";
    }
    if (in_use()) {
	s += "  route is in use";
    } else {
	s += "  route is not in use";
    }
    if (nexthop_resolved()) {
	s += "  route's nexthop resolved";
    } else {
	s += "  route's nexthop did not resolve";
    }
#endif
    return s;
}

template<class A>
const RefPf&
SubnetRoute<A>::policyfilter(uint32_t i) const
{
    if (_parent_route)
	return _parent_route->policyfilter(i);
    return _pfilter[i];
}

template<class A>
void
SubnetRoute<A>::set_policyfilter(uint32_t i, const RefPf& f) const
{
    if (_parent_route) {
	_parent_route->set_policyfilter(i, f);
    }
    _pfilter[i] = f;
}

template class SubnetRoute<IPv4>;
template class SubnetRoute<IPv6>;
