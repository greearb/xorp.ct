// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/fea/fte.hh,v 1.26 2008/10/02 21:56:47 bms Exp $

#ifndef	__FEA_FTE_HH__
#define __FEA_FTE_HH__

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
	  _xorp_route(xorp_route), _is_deleted(false), _is_unresolved(false),
	  _is_connected_route(false) {}
    Fte(const N& net)
	: _net(net), _nexthop(A::ZERO(net.af())),
	  _metric(0), _admin_distance(0),
	  _xorp_route(false), _is_deleted(false), _is_unresolved(false),
	  _is_connected_route(false) {}

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
    bool	is_connected_route() const { return _is_connected_route; }
    void	mark_connected_route()	{ _is_connected_route = true; }

    /**
     * Reset all members.
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
	_is_connected_route = false;
    }

    /**
     * @return true if this is a host route.
     */
    bool is_host_route() const {
 	return _net.prefix_len() == _net.masked_addr().addr_bitlen();
    }

    /**
     * @return a string representation of the entry.
     */
    string str() const {
	return c_format("net = %s nexthop = %s ifname = %s vifname = %s "
			"metric = %u admin_distance = %u xorp_route = %s "
			"is_deleted = %s is_unresolved = %s "
			"is_connected_route = %s",
			_net.str().c_str(), _nexthop.str().c_str(),
			_ifname.c_str(), _vifname.c_str(),
			XORP_UINT_CAST(_metric),
			XORP_UINT_CAST(_admin_distance),
			bool_c_str(_xorp_route),
			bool_c_str(_is_deleted),
			bool_c_str(_is_unresolved),
			bool_c_str(_is_connected_route));
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
    bool	_is_connected_route;	// True if this is a route for
					// directly-connected subnet.
};

typedef Fte<IPv4, IPv4Net> Fte4;
typedef Fte<IPv6, IPv6Net> Fte6;
typedef Fte<IPvX, IPvXNet> BaseFteX;

class FteX : public BaseFteX {
public:
    /**
     * Constructor.
     *
     * @param net the network address.
     * @param nexthop the next-hop router address.
     * @param ifname the interface name.
     * @param vifname the virtual interface name.
     * @param metric the route metric.
     * @param admin_distance the route admin distance.
     * @param xorp_route true if this is a XORP route.
     */
    FteX(const IPvXNet&	net,
	 const IPvX&	nexthop,
	 const string&	ifname,
	 const string&	vifname,
	 uint32_t	metric,
	 uint32_t	admin_distance,
	 bool		xorp_route)
	: BaseFteX(net,
		   nexthop,
		   ifname,
		   vifname,
		   metric,
		   admin_distance,
		   xorp_route) {}

    /**
     * Constructor for a specified address family.
     */
    explicit FteX(int family) : BaseFteX(family) {}

#ifdef XORP_USE_USTL
    FteX() : BaseFteX(AF_INET) {}
#endif

    /**
     * Copy constructor for Fte4 entry.
     */
    FteX(const Fte4& fte4)
	: BaseFteX(IPvXNet(fte4.net()),
		   IPvX(fte4.nexthop()),
		   fte4.ifname(),
		   fte4.vifname(),
		   fte4.metric(),
		   fte4.admin_distance(),
		   fte4.xorp_route()) {
	if (fte4.is_deleted())
	    mark_deleted();
	if (fte4.is_unresolved())
	    mark_unresolved();
	if (fte4.is_connected_route())
	    mark_connected_route();
    }

    /**
     * Copy constructor for Fte6 entry.
     */
    FteX(const Fte6& fte6)
	: BaseFteX(IPvXNet(fte6.net()),
		   IPvX(fte6.nexthop()),
		   fte6.ifname(),
		   fte6.vifname(),
		   fte6.metric(),
		   fte6.admin_distance(),
		   fte6.xorp_route()) {
	if (fte6.is_deleted())
	    mark_deleted();
	if (fte6.is_unresolved())
	    mark_unresolved();
	if (fte6.is_connected_route())
	    mark_connected_route();
    }

    /**
     * Get an Fte4 entry.
     *
     * @return the corresponding Fte4 entry.
     */
    Fte4 get_fte4() const throw (InvalidCast) {
	Fte4 fte4(net().get_ipv4net(),
		  nexthop().get_ipv4(),
		  ifname(),
		  vifname(),
		  metric(),
		  admin_distance(),
		  xorp_route());
	if (is_deleted())
	    fte4.mark_deleted();
	if (is_unresolved())
	    fte4.mark_unresolved();
	if (is_connected_route())
	    fte4.mark_connected_route();
	return fte4;
    }

    /**
     * Get an Fte6 entry.
     *
     * @return the corresponding Fte6 entry.
     */
    Fte6 get_fte6() const throw (InvalidCast) {
	Fte6 fte6(net().get_ipv6net(),
		  nexthop().get_ipv6(),
		  ifname(),
		  vifname(),
		  metric(),
		  admin_distance(),
		  xorp_route());
	if (is_deleted())
	    fte6.mark_deleted();
	if (is_unresolved())
	    fte6.mark_unresolved();
	if (is_connected_route())
	    fte6.mark_connected_route();
	return fte6;
    }
};

#endif	// __FEA_FTE_HH__
