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

#ident "$XORP$"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/test_main.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ref_trie.hh"
#include "libxorp/timeval.hh"
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

    PathAttributeList<A> pa;
    A nexthop;
    pa.add_path_attribute(NextHopAttribute<A>(nexthop));
    pa.add_path_attribute(ASPathAttribute(AsPath("1,2,3")));
    RefTrie<A, const SubnetRoute<A> > route_table;

    for(int i = 0; i < routes; i++) {
	pa.rehash();
	SubnetRoute<A> *route = new SubnetRoute<A>(net, &pa, 0);
	route_table.insert(net, *route);
	route->unref();
	++net;
    }

    timeval now;
    TimeVal start;
    TimeVal used;

    gettimeofday(&now, 0);
    start = TimeVal(now);
    route_table.delete_all_nodes();
    gettimeofday(&now, 0);
    used = TimeVal(now) - start;

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

    PathAttributeList<A> pa;
    A nexthop;
    pa.add_path_attribute(NextHopAttribute<A>(nexthop));
    pa.add_path_attribute(ASPathAttribute(AsPath("1,2,3")));
    RefTrie<A, const SubnetRoute<A> > route_table;

    for(int i = 0; i < routes; i++) {
 	++nexthop;
 	pa.replace_nexthop(nexthop);
	pa.rehash();
	SubnetRoute<A> *route = new SubnetRoute<A>(net, &pa, 0);
	route_table.insert(net, *route);
	route->unref();
	++net;
    }

    timeval now;
    TimeVal start;
    TimeVal used;

    gettimeofday(&now, 0);
    start = TimeVal(now);
    route_table.delete_all_nodes();
    gettimeofday(&now, 0);
    used = TimeVal(now) - start;

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
