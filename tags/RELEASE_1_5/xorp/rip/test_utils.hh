// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rip/test_utils.hh,v 1.14 2008/01/04 03:17:33 pavlin Exp $

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
