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

// $XORP: xorp/policy/policy_route.hh,v 1.2 2003/02/13 00:51:03 mjh Exp $

#ifndef __POLICY_POLICY_ROUTE_HH__
#define __POLICY_POLICY_ROUTE_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipnet.hh"
#include "bgp/path_attribute.hh"

template <typename A>
class PolicyRoute {
public:
    PolicyRoute(const IPNet<A> &net, 
		const A& nexthop, 
		const string& origin_protocol);
    const IPNet<A>& net() const {return _net;}
    const A& nexthop() const {return _nexthop;}
    const string& origin_protocol() const {return _origin_protocol;}
private:
    IPNet<A> _net;
    A _nexthop;
    string _origin_protocol;
};

template <typename A>
class BGPPolicyRoute : public PolicyRoute<A> {
public:
    BGPPolicyRoute(const IPNet<A> &net, 
		   const A& nexthop, 
		   const AsPath& as_path, 
		   OriginType origin,
		   bool _ibgp);
    void set_med(uint32_t med) 
    {
	_med = med; 
	_has_med = true;
    }
    bool has_med() const {return _has_med;}
    uint32_t med() const {return _med;}

    void set_localpref(uint32_t localpref) 
    {
	_localpref = localpref; 
	_has_localpref = true;
    }
    bool has_localpref() const {return _has_localpref;}
    uint32_t localpref() const {return _localpref;}

    void set_communities(set <uint32_t> communities) 
    {
	_communities = communities; 
    }
    const set <uint32_t>& communities() const {return _communities;}
private:
    AsPath _as_path;
    OriginType _origin;
    bool _ibgp;
    bool _has_med;
    uint32_t _med;
    bool _has_localpref;
    uint32_t _localpref;
    set <uint32_t> _communities;
};

#endif // __POLICY_POLICY_ROUTE_HH__
