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

#ident "$XORP: xorp/bgp/subnet_route.cc,v 1.1.1.1 2002/12/11 23:55:50 hodson Exp $"

#include "bgp_module.h"
#include "subnet_route.hh"


template<class A> AttributeManager<A> SubnetRoute<A>::_att_mgr;

template<class A>
SubnetRoute<A>::SubnetRoute<A>(const SubnetRoute<A>& route_to_clone) 
{
    debug_msg("SubnetRoute constructor1 giving %x\n", (uint)this);
    //note that we need an explicit constructor here, rather than
    //relying in C++ for the default constructor, or we get the
    //reference counts wrong in the attribute manager

    _net = route_to_clone.net();

    const PathAttributeList<A>* atts;
    atts = route_to_clone.attributes();
    //the attribute manager handles memory management, and ensuring
    //that only one copy of each attribute list is ever stored
    _attributes = _att_mgr.add_attribute_list(atts);

    _flags = route_to_clone._flags;
    _igp_metric = route_to_clone._igp_metric;
}

template<class A>
SubnetRoute<A>::SubnetRoute<A>(const IPNet<A> &n, 
			       const PathAttributeList<A>* atts)
    : _net(n) {
    debug_msg("SubnetRoute constructor2 giving %x\n", (uint)this);
    //the attribute manager handles memory management, and ensuring
    //that only one copy of each attribute list is ever stored
    _attributes = _att_mgr.add_attribute_list(atts);

    _flags = 0;
    //the in_use flag is set to false so reduce the work we have to
    //re-do when we have to touch all the routes in a route_table.  It
    //should default to true if we don't know for sure that a route is
    //not used as this is always safe, if somewhat inefficient.
    _flags |= SRF_IN_USE;
    _igp_metric = 0xffffffff;
}

template<class A>
SubnetRoute<A>::SubnetRoute<A>(const IPNet<A> &n, 
			       const PathAttributeList<A>* atts,
			       uint32_t igp_metric)
    : _net(n) {
    debug_msg("SubnetRoute constructor3 giving %x\n", (uint)this);
    //the attribute manager handles memory management, and ensuring
    //that only one copy of each attribute list is ever stored
    _attributes = _att_mgr.add_attribute_list(atts);

    _flags = 0;
    //the in_use flag is set to false so reduce the work we have to
    //re-do when we have to touch all the routes in a route_table.  It
    //should default to true if we don't know for sure that a route is
    //not used as this is always safe, if somewhat inefficient.
    _flags |= SRF_IN_USE;
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
SubnetRoute<A>::~SubnetRoute<A>() {
    debug_msg("SubnetRoute destructor called for %x\n", (uint)this);
    _att_mgr.delete_attribute_list(_attributes);
}

template<class A>
void 
SubnetRoute<A>::set_is_winner(uint32_t igp_metric) const {
    _flags |= SRF_WINNER;
    _igp_metric = igp_metric;
}

template<class A>
void 
SubnetRoute<A>::set_is_not_winner() const {
    _flags &= ~SRF_WINNER;
}

template<class A>
void 
SubnetRoute<A>::set_in_use(bool used) const {
    if (used) {
	_flags |= SRF_IN_USE;
    } else {
	_flags &= ~SRF_IN_USE;
    }
#ifdef DEBUG_FLAGS
    printf("set_in_use: %p = ", this);
    if (used)
	printf("true");
    else 
	printf("false");
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
    printf("set_filtered: %p = ", this);
    if (filtered)
	printf("true");
    else 
	printf("false");
    printf("\n%s\n", str().c_str());
#endif
}

template<class A>
string
SubnetRoute<A>::str() const {
    string s;
    s = "SubnetRoute:\n";
    s += "  Net: " + _net.str() + "\n";
    s += "  PAList: " + _attributes->str() + "\n";
#ifdef DEBUG_FLAGS
    if (is_winner()) {
	s += "  route is winner";
    } else {
	s += "  route is not winner";
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
#endif
    return s;
}


template class SubnetRoute<IPv4>;
template class SubnetRoute<IPv6>;
