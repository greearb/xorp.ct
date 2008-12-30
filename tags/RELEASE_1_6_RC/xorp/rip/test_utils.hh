// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rip/test_utils.hh,v 1.15 2008/07/23 05:11:37 pavlin Exp $

#ifndef __RIP_TEST_UTILS_HH__
#define __RIP_TEST_UTILS_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include <functional>
#include <set>

#include "peer.hh"
#include "route_db.hh"


template <typename A>
inline A random_addr();

/**
 * Generate a random IPv4 address.
 */
template <>
IPv4
random_addr<IPv4>()
{
    uint32_t h = htonl(xorp_random());
    return IPv4(h);
}

/**
 * Generate a random IPv6 address.
 */
template <>
IPv6
random_addr<IPv6>()
{
    uint32_t x[4];
    x[0] = htonl(xorp_random());
    x[1] = htonl(xorp_random());
    x[2] = htonl(xorp_random());
    x[3] = htonl(xorp_random());
    return IPv6(x);
}

/**
 * Generate a set of routes.
 */
template <typename A>
void
make_nets(set<IPNet<A> >& nets, uint32_t n_nets)
{
    // attempt at deterministic nets sequence
    while (nets.size() != n_nets) {
	A addr = random_addr<A>();
	IPNet<A> net = IPNet<A>(addr, 1 + xorp_random() % A::ADDR_BITLEN);
	nets.insert(net);
    }
}

/**
 * @short Unary function object to split routes evenly into 2 sets.
 */
template <typename A>
struct SplitNets : public unary_function<A,void>
{
    SplitNets(set<IPNet<A> >& a, set<IPNet<A> >& b) : _s1(a), _s2(b), _n(0)
    {}

    void operator() (const IPNet<A>& net)
    {
	if (_n % 2) {
	    _s1.insert(net);
	} else {
	    _s2.insert(net);
	}
	_n++;
    }
protected:
    set<IPNet<A> >& 	_s1;
    set<IPNet<A> >& 	_s2;
    uint32_t 		_n;
};

/**
 * @short Unary function object to inject routes into a route database.
 */
template <typename A>
struct RouteInjector : public unary_function<A,void>
{
    RouteInjector(RouteDB<A>& r, const A& my_addr, const string& ifname,
		  const string& vifname, uint32_t cost,
		  Peer<A>* peer)
	: _r(r), _m(my_addr), _ifname(ifname), _vifname(vifname),
	  _c(cost), _p(peer), _injected(0) {}

    void operator() (const IPNet<A>& net)
    {
	if (_r.update_route(net, _m, _ifname, _vifname, _c, 0, _p,
			    PolicyTags(), false) == true) {
	    _injected++;
	} else {
	    fprintf(stderr, "Failed to update %s\n", net.str().c_str());
	}
    }
    uint32_t injected() const { return _injected; }

protected:
    RouteDB<A>& _r;
    A		_m;
    string	_ifname;
    string	_vifname;
    uint32_t	_c;
    Peer<A>*	_p;
    uint32_t	_injected;
};

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
	fflush(stdout);							\
    }									\
} while(0)

#endif // __RIP_TEST_UTILS_HH__
