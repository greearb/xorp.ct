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



#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ref_trie.hh"
#include "libxorp/timeval.hh"
#include "libxorp/timer.hh"

#include "path_attribute.hh"
#include "subnet_route.hh"


template<>
inline void
RefTrieNode<IPv4, const SubnetRoute<IPv4> >
::delete_payload(const SubnetRoute<IPv4>* p) 
{
    debug_msg("delete_payload %p\n", p);
    p->unref();
}

template<>
inline void
RefTrieNode<IPv6, const SubnetRoute<IPv6> >
::delete_payload(const SubnetRoute<IPv6>* p) 
{
    p->unref();
}

const int routes = 200000; 	// Number of routes

template <class A>
bool
test_subnet_route1(TestInfo& info, IPNet<A> net)
{
    DOUT(info) << info.test_name() << endl;

    FPAListRef fpa = new FastPathAttributeList<A>();
    A nexthop;
    NextHopAttribute<A> nha(nexthop);
    fpa->add_path_attribute(nha);
    ASPathAttribute aspa(ASPath("1,2,3"));
    fpa->add_path_attribute(aspa);
    PAListRef<A> pa = new PathAttributeList<A>(fpa);
    RefTrie<A, const SubnetRoute<A> > route_table;

    for(int i = 0; i < routes; i++) {
	SubnetRoute<A> *route = new SubnetRoute<A>(net, pa, 0);
	route_table.insert(net, *route);
	route->unref();
	++net;
    }

    TimeVal now;
    TimeVal start;
    TimeVal used;

    TimerList::system_gettimeofday(&now);
    start = now;
    route_table.delete_all_nodes();
    TimerList::system_gettimeofday(&now);
    used = now - start;

    DOUT(info) << " To delete " << routes << 
	" routes with the same path attribute list took " <<
	used.str() << " seconds" << endl;

    return true;
}

template <class A>
bool
test_subnet_route2(TestInfo& info, IPNet<A> net)
{
    DOUT(info) << info.test_name() << endl;

    FPAListRef fpa = new FastPathAttributeList<A>();
    A nexthop;
    NextHopAttribute<A> nha(nexthop);
    fpa->add_path_attribute(nha);
    ASPathAttribute aspa(ASPath("1,2,3"));
    fpa->add_path_attribute(aspa);
    PAListRef<A> pa = new PathAttributeList<A>(fpa);
    RefTrie<A, const SubnetRoute<A> > route_table;

    for(int i = 0; i < routes; i++) {
 	++nexthop;
 	fpa->replace_nexthop(nexthop);
	pa.release();
	pa = new PathAttributeList<A>(fpa);
	SubnetRoute<A> *route = new SubnetRoute<A>(net, pa, 0);
	route_table.insert(net, *route);
	route->unref();
	++net;
    }

    TimeVal now;
    TimeVal start;
    TimeVal used;

    TimerList::system_gettimeofday(&now);
    start = now;
    route_table.delete_all_nodes();
    TimerList::system_gettimeofday(&now);
    used = now - start;

    DOUT(info) << " To delete " << routes << 
	" routes with the a different path attribute list per route took " <<
	used.str() << "seconds" << endl;

    return true;
}

/*
** This function is never called it exists to instantiate the
** templatised functions.
*/
void
dummy_subnet_route()
{
    XLOG_FATAL("Not to be called");

    callback(test_subnet_route1<IPv4>);
    callback(test_subnet_route1<IPv6>);
    callback(test_subnet_route2<IPv4>);
    callback(test_subnet_route2<IPv6>);
}
