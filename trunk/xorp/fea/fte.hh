// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/fea/fte.hh,v 1.1 2003/05/02 07:50:42 pavlin Exp $

#ifndef	__FEA_FTE_HH__
#define __FEA_FTE_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"


/**
 * @short Forwarding Table Entry.
 *
 * Representation of a forwarding table entry.
 */
template<typename A, typename N>
class Fte {
public:
    Fte() { zero(); }
    explicit Fte(int family) : _net(family), _gateway(family) { zero(); }
    Fte(const N&	net,
	const A&	gateway,
	const string&	ifname,
	const string&	vifname,
	uint32_t	metric,
	uint32_t	admin_distance,
	bool		xorp_route = false) :
	_net(net), _gateway(gateway), _ifname(ifname), _vifname(vifname),
	_metric(metric), _admin_distance(admin_distance),
	_xorp_route(xorp_route) {}
    Fte(const N& net) : _net(net), _gateway(A::ZERO(net.af())) {}

    const N& net() const		{ return _net; }
    const A& gateway() const 		{ return _gateway; }
    const string& ifname() const	{ return _ifname; }
    const string& vifname() const	{ return _vifname; }
    uint32_t metric() const		{ return _metric; }
    uint32_t admin_distance() const	{ return _admin_distance; }
    bool xorp_route() const 		{ return _xorp_route; }

    /**
     * Reset all members
     */
    void zero() {
	_net = N(A::ZERO(_net.af()), 0);
	_gateway = A::ZERO(_gateway.af());
	_ifname.erase();
	_vifname.erase();
	_metric = 0;
	_admin_distance = 0;
	_xorp_route = false;
    }

    /**
     * @return true if this is a host route.
     */
    inline bool is_host_route() const {
 	return _net.prefix_len() == _net.masked_addr().addr_bitlen();
    }

    /**
     * @return A string representation of the entry.
     *
     * dst = 127.0.0.1 gateway = 127.0.0.1 netmask = 255.255.255.255 if = lo0
     * metric = 10 admin_distance = 20
     */
    string str() const {
	return c_format("net = %s gateway = %s ifname = %s vifname = %s "
			"metric = %u admin_distance = %u xorp_route %s",
			_net.str().c_str(), _gateway.str().c_str(),
			_ifname.c_str(), _vifname.c_str(),
			_metric, _admin_distance, _xorp_route ? "true" :
			"false");
    }

private:
    N		_net;		// Network
    A		_gateway; 	// Gateway address
    string	_ifname;	// Interface name
    string	_vifname;	// Virtual interface name
    uint32_t	_metric;	// Route metric
    uint32_t	_admin_distance; // Route admin distance
    bool	_xorp_route;	// This route was installed by XORP.
};

typedef Fte<IPv4, IPv4Net> Fte4;
typedef Fte<IPv6, IPv6Net> Fte6;
typedef Fte<IPvX, IPvXNet> FteX;

#endif	// __FEA_FTE_HH__
