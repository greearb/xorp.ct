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

// $XORP: xorp/fea/fte.hh,v 1.12 2004/11/05 00:47:27 bms Exp $

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
    explicit Fte(int family) : _net(family), _nexthop(family) { zero(); }
    Fte(const N&	net,
	const A&	nexthop,
	const string&	ifname,
	const string&	vifname,
	uint32_t	metric,
	uint32_t	admin_distance,
	bool		xorp_route)
	: _net(net), _nexthop(nexthop), _ifname(ifname), _vifname(vifname),
	  _metric(metric), _admin_distance(admin_distance),
	  _xorp_route(xorp_route), _is_deleted(false), _is_unresolved(false) {}
    Fte(const N& net)
	: _net(net), _nexthop(A::ZERO(net.af())),
	  _metric(0), _admin_distance(0), _xorp_route(false) {}

    const N&	net() const		{ return _net; }
    const A&	nexthop() const 	{ return _nexthop; }
    const string& ifname() const	{ return _ifname; }
    const string& vifname() const	{ return _vifname; }
    uint32_t	metric() const		{ return _metric; }
    uint32_t	admin_distance() const	{ return _admin_distance; }
    bool	xorp_route() const 	{ return _xorp_route; }
    bool	is_deleted() const	{ return _is_deleted; }
    void	mark_deleted()		{ _is_deleted = true; }
    bool	is_unresolved() const	{ return _is_unresolved; }
    void	mark_unresolved()	{ _is_unresolved = true; }

    /**
     * Reset all members
     */
    void zero() {
	_net = N(A::ZERO(_net.af()), 0);
	_nexthop = A::ZERO(_nexthop.af());
	_ifname.erase();
	_vifname.erase();
	_metric = 0;
	_admin_distance = 0;
	_xorp_route = false;
	_is_deleted = false;
	_is_unresolved = false;
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
     * dst = 127.0.0.1 nexthop = 127.0.0.1 netmask = 255.255.255.255 if = lo0
     * metric = 10 admin_distance = 20
     */
    string str() const {
	return c_format("net = %s nexthop = %s ifname = %s vifname = %s "
			"metric = %u admin_distance = %u xorp_route = %s "
			"is_deleted = %s is_unresolved = %s",
			_net.str().c_str(), _nexthop.str().c_str(),
			_ifname.c_str(), _vifname.c_str(),
			_metric, _admin_distance,
			_xorp_route ? "true" : "false",
			_is_deleted ? "true" : "false",
			_is_unresolved ? "true" : "false");
    }

private:
    N		_net;			// Network
    A		_nexthop;		// Nexthop address
    string	_ifname;		// Interface name
    string	_vifname;		// Virtual interface name
    uint32_t	_metric;		// Route metric
    uint32_t	_admin_distance;	// Route admin distance
    bool	_xorp_route;		// This route was installed by XORP
    bool	_is_deleted;		// True if the entry was deleted
    bool	_is_unresolved;		// True if the entry is actually
					// a notification of an unresolved
					// route to the destination.
};

typedef Fte<IPv4, IPv4Net> Fte4;
typedef Fte<IPv6, IPv6Net> Fte6;
typedef Fte<IPvX, IPvXNet> FteX;

#endif	// __FEA_FTE_HH__
