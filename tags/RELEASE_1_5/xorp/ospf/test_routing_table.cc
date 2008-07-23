// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/ospf/test_routing_table.cc,v 1.12 2008/01/04 03:16:58 pavlin Exp $"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/tlv.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <map>
#include <list>
#include <set>
#include <deque>

#include "libproto/spt.hh"

#include "ospf.hh"
#include "debug_io.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"


// Make sure that all tests free up any memory that they use. This will
// allow us to use the leak checker program.

/**
 * Simple test to verify that assigning a RouteEntry works.
 */
template <typename A>
bool
rt1(TestInfo& info, OspfTypes::Version version)
{
    RouteEntry<A> rt1;
    RouteEntry<A> rt2;

    OspfTypes::VertexType destination_type = OspfTypes::Network;
    bool discard = true;
    bool direct = true;
    uint32_t address = 108;
    OspfTypes::AreaID area = 55;
    typename RouteEntry<A>::PathType path_type = RouteEntry<A>::type2;
    uint32_t cost = 2007;
    uint32_t type_2_cost = 1000;
    A nexthop;
    switch(nexthop.ip_version()) {
    case 4:
	nexthop = "192.150.187.78";
	break;
    case 6:
	nexthop = "2001:468:e21:c800:220:edff:fe61:f033";
	break;
    default:
	XLOG_FATAL("Unknown IP version %d", nexthop.ip_version());
	break;
    }
    uint32_t nexthop_id = 2000000;
    uint32_t advertising_router = 1500;
    Lsa::LsaRef lsar(new RouterLsa(version));
    bool filtered = true;

    rt1.set_destination_type(destination_type);
    rt1.set_discard(discard);
    rt1.set_directly_connected(direct);
    rt1.set_address(address);
    rt1.set_area(area);
    rt1.set_path_type(RouteEntry<A>::type2);
    rt1.set_cost(cost);
    rt1.set_type_2_cost(type_2_cost);
    rt1.set_nexthop(nexthop);
    rt1.set_nexthop_id(nexthop_id);
    rt1.set_advertising_router(advertising_router);
    rt1.set_lsa(lsar);
    rt1.set_filtered(filtered);

    rt2 = rt1;

    if (destination_type != rt2.get_destination_type())  {
	DOUT(info) << "Failed to copy value of destination type\n";
	return false;
    }

    if (discard != rt2.get_discard()) {
	DOUT(info) << "Failed to copy discard value\n";
	return false;
    }

    if (direct != rt2.get_directly_connected()) {
	DOUT(info) << "Failed to copy directly connected value\n";
	return false;
    }

    if (address != rt2.get_address())  {
	DOUT(info) << "Failed to copy value of address\n";
	return false;
    }

    if (area != rt2.get_area())  {
	DOUT(info) << "Failed to copy value of area\n";
	return false;
    }

    if (path_type != rt2.get_path_type())  {
	DOUT(info) << "Failed to copy value of path type\n";
	return false;
    }

    if (cost != rt2.get_cost())  {
	DOUT(info) << "Failed to copy value of cost\n";
	return false;
    }

    if (type_2_cost != rt2.get_type_2_cost())  {
	DOUT(info) << "Failed to copy value of type 2 cost\n";
	return false;
    }

    if (nexthop != rt2.get_nexthop())  {
	DOUT(info) << "Failed to copy value of nexthop\n";
	return false;
    }

    if (nexthop_id != rt2.get_nexthop_id())  {
	DOUT(info) << "Failed to copy value of nexthop ID\n";
	return false;
    }

    if (advertising_router != rt2.get_advertising_router())  {
	DOUT(info) << "Failed to copy value of advertising router\n";
	return false;
    }

    if (lsar != rt2.get_lsa())  {
	DOUT(info) << "Failed to copy value of LSA\n";
	return false;
    }

    if (filtered != rt2.get_filtered()) {
	DOUT(info) << "Failed to copy filtered value\n";
	return false;
    }

    return true;
}

/**
 * Verify that an internal routing entry correctly recomputes the
 * winning route.
 */
template <typename A> 
bool
ire1(TestInfo& info, OspfTypes::Version /*version*/)
{
    RouteEntry<A> rt1;
    RouteEntry<A> rt2;
    InternalRouteEntry<A> ire;

    rt1.set_area(1);
    rt2.set_area(2);

    ire.add_entry(1, rt1);
    ire.add_entry(2, rt2);

    RouteEntry<A>& win = ire.get_entry();
    if (win.get_area() != rt2.get_area()) {
	DOUT(info) << "The entry with the largest area ID should win\n";
	return false;
    }

    return true;
}

/**
 * Verify that copying a internal routing entry works.
 */
template <typename A> 
bool
ire2(TestInfo& info, OspfTypes::Version /*version*/)
{
    RouteEntry<A> rt1;
    RouteEntry<A> rt2;
    InternalRouteEntry<A> *ire = new InternalRouteEntry<A>;

    rt1.set_area(1);
    rt2.set_area(2);
    rt1.set_cost(5);
    rt2.set_cost(5);

    ire->add_entry(1, rt1);
    ire->add_entry(2, rt2);

    RouteEntry<A>& win = ire->get_entry();
    if (win.get_area() != rt2.get_area()) {
	DOUT(info) << "The entry with the largest area ID should win\n";
	delete ire;
	return false;
    }

    InternalRouteEntry<A> ire_copy = *ire;
    delete ire;

    RouteEntry<A>& rtc = ire_copy.get_entry();
    if (rtc.get_cost() != 5) {
	DOUT(info) << "Cost should be 5 not " << rtc.get_cost() << endl;
	return false;
    }

    return true;
}

/**
 * Verify that the trie code works as advertised.
 */
template <typename A> 
bool
trie1(TestInfo& info, OspfTypes::Version version)
{
    Trie<A, InternalRouteEntry<A> > *current;
    current = new Trie<A, InternalRouteEntry<A> >;

    typename Trie<A, InternalRouteEntry<A> >::iterator i;

    InternalRouteEntry<A> ire;

    IPNet<A> net;
    switch(net.masked_addr().ip_version()) {
    case 4:
	net = IPNet<A>("192.150.187.108/32");
	break;
    case 6:
	net = IPNet<A>("2001:468:e21:c800:220:edff:fe61:f033/128");
	break;
    default:
	XLOG_FATAL("Unknown IP version %d", net.masked_addr().ip_version());
	break;
    }

    current->insert(net, ire);
    i = current->lookup_node(net);
    if (current->end() == i) {
	DOUT(info) << "Key not found\n";
	delete current;
	return false;
    }

    RouterLsa *rlsa = new RouterLsa(version);
    Lsa::LsaRef lsar(rlsa);

    InternalRouteEntry<A>& irentry1 = i.payload();
    A nexthop;
    RouteEntry<A> rt1;
    rt1.set_lsa(lsar);
    rt1.set_cost(1);
    nexthop = rt1.get_nexthop();
    rt1.set_nexthop(++nexthop);
    RouteEntry<A> rt2;
    rt2.set_lsa(lsar);
    rt2.set_cost(2);
    rt2.set_nexthop(++nexthop);

    OspfTypes::AreaID az = set_id("0.0.0.0");
    OspfTypes::AreaID a1 = set_id("0.0.0.1");
    
    irentry1.add_entry(az, rt1);

    i = current->lookup_node(net);
    if (current->end() == i) {
	DOUT(info) << "Key not found\n";
	delete current;
	return false;
    }
    InternalRouteEntry<A>& irentry2 = i.payload();
    irentry2.add_entry(a1, rt2);

    i = current->lookup_node(net);
    if (current->end() == i) {
	DOUT(info) << "Key not found\n";
	delete current;
	return false;
    }
    InternalRouteEntry<A>& irentry3 = i.payload();
    bool winner_changed;
    irentry3.delete_entry(az, winner_changed);

    typename Trie<A, InternalRouteEntry<A> >::iterator tic;
    for (tic = current->begin(); tic != current->end(); tic++) {
	RouteEntry<A>& rte = tic.payload().get_entry();
	DOUT(info) << "Key: " << tic.key().str() << 
	    " Payload: " << rte.str() << endl;
    }

    delete current;

    return true;
}

/**
 * Call trie1 one hundred times.
 */
template <typename A> 
bool
trie2(TestInfo& info, OspfTypes::Version version)
{
    for (int i = 0; i < 100; i++)
	if (!trie1<A>(info, version))
	    return false;

    return true;
}

/**
 * Verify that no host routes pointing at a router make it through to
 * the routing table.
 */
template <typename A>
bool
routing1(TestInfo& info, OspfTypes::Version /*version*/)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());

    OspfTypes::AreaID az = set_id("0.0.0.0");
    OspfTypes::AreaID a1 = set_id("0.0.0.1");
    
    RoutingTable<A>& routing_table = ospf.get_routing_table();

    RouterLsa *rlsa = new RouterLsa(version);
    Lsa::LsaRef lsar(rlsa);

    RouteEntry<A> route_entry1;
    RouteEntry<A> route_entry2;

    route_entry1.set_directly_connected(true);
    route_entry2.set_directly_connected(true);

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    routing_table.add_entry(a1, IPNet<A>("192.150.187.108/32"), route_entry1);
    routing_table.end();

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    routing_table.add_entry(a1, IPNet<A>("192.150.187.108/32"), route_entry1);
    routing_table.end();

    /****************************************/
    routing_table.begin(az);

    route_entry2.set_area(az);
    route_entry2.set_lsa(lsar);

    routing_table.add_entry(az, IPNet<A>("192.150.187.108/32"), route_entry2);
    routing_table.end();

    /****************************************/
    routing_table.begin(a1);
    routing_table.end();

    /****************************************/
    routing_table.begin(az);
    routing_table.end();

    if (0 != io.routing_table_size()) {
	DOUT(info) << "Routing table should be empty not " << 
	    io.routing_table_size() << endl;
	return false;
    }

    return true;
}

/**
 * At the time of writing OSPFv3 behaved differently to OSPFv2 with
 * respect to router entries. In OSPFv2 router entries are added as
 * host routes as well as being indexed by the advertising router. In
 * OSPFv3 router entries are only in the Adv database. Just verify
 * that add and replace work correctly for OSPFv3
 */
template <typename A>
bool
routing2(TestInfo& info, OspfTypes::Version /*version*/)
{
    OspfTypes::Version version = OspfTypes::V3;

    EventLoop eventloop;
    DebugIO<IPv6> io(info, version, eventloop);
    io.startup();

    Ospf<IPv6> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());

    OspfTypes::AreaID az = set_id("0.0.0.0");
    OspfTypes::AreaID a1 = set_id("0.0.0.1");
    
    RoutingTable<A>& routing_table = ospf.get_routing_table();

    RouterLsa *rlsa = new RouterLsa(version);
    Lsa::LsaRef lsar(rlsa);

    IPNet<IPv6> netv("5f00:0000:c001:0200::/56");

    IPNet<IPv6> net;

    RouteEntry<A> route_entry1;
    RouteEntry<A> route_entry2;

    route_entry1.set_directly_connected(true);
    route_entry2.set_directly_connected(true);

    route_entry1.set_destination_type(OspfTypes::Router);
    route_entry2.set_destination_type(OspfTypes::Router);

    route_entry1.set_router_id(set_id("0.0.0.1"));
    route_entry2.set_router_id(set_id("0.0.0.1"));

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    // Add an entry with a non zero net
    if (routing_table.add_entry(a1, netv, route_entry1)) {
	DOUT(info) << "Accepted an entry with a non-zero net " <<
	    route_entry1.str() <<
	    endl;
	return false;
    }

    route_entry2.set_area(a1);
    route_entry2.set_lsa(lsar);

    // Replace an entry with a non zero net
    if (routing_table.replace_entry(a1, netv, route_entry2)) {
	DOUT(info) << "Accepted an entry with a non-zero net " <<
	    route_entry1.str() <<
	    endl;
	return false;
    }
	
    routing_table.end();

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    // Add an ordinary entry should work
    if (!routing_table.add_entry(a1, net, route_entry1)) {
	DOUT(info) << "Failed to add a simple entry " << route_entry1.str() <<
	    endl;
	return false;
    }
	
    route_entry2.set_area(a1);
    route_entry2.set_lsa(lsar);

    // Add an entry with an existing advertising router this should generate
    // an error.
    if (routing_table.add_entry(a1, net, route_entry2)) {
	DOUT(info) << "Accepted an entry with the same advertising router "
		   << route_entry2.str() << endl;
	return false;
    }

    routing_table.end();

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    // Add an ordinary entry should work
    if (!routing_table.add_entry(a1, net, route_entry1)) {
	DOUT(info) << "Failed to add a simple entry " << route_entry1.str() <<
	    endl;
	return false;
    }
	
    route_entry2.set_area(a1);
    route_entry2.set_lsa(lsar);

    // Replace entry
    if (!routing_table.replace_entry(a1, net, route_entry2)) {
	DOUT(info) << "Replacing entry failed " << route_entry2.str() << endl;
	return false;
    }

    routing_table.end();

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    // Perform a replace with nothing to replace.
    if (routing_table.replace_entry(a1, net, route_entry1)) {
	DOUT(info) << "Replaced an entry that didn't exist " <<
	    route_entry1.str() <<
	    endl;
	return false;
    }

    routing_table.end();

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    routing_table.add_entry(a1, net, route_entry1);
    routing_table.end();

    /****************************************/
    routing_table.begin(a1);

    route_entry1.set_area(a1);
    route_entry1.set_lsa(lsar);

    routing_table.add_entry(a1, net, route_entry1);
    routing_table.end();

    /****************************************/
    routing_table.begin(az);

    route_entry2.set_area(az);
    route_entry2.set_lsa(lsar);

    routing_table.add_entry(az, net, route_entry2);
    routing_table.end();

    /****************************************/
    routing_table.begin(a1);
    routing_table.end();

    /****************************************/
    routing_table.begin(az);
    routing_table.end();

    if (0 != io.routing_table_size()) {
	DOUT(info) << "Routing table should be empty not " << 
	    io.routing_table_size() << endl;
	return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"rt1v2", callback(rt1<IPv4>, OspfTypes::V2)},
	{"rt1v3", callback(rt1<IPv6>, OspfTypes::V3)},
	{"ire1v2", callback(ire1<IPv4>, OspfTypes::V2)},
	{"ire1v3", callback(ire1<IPv6>, OspfTypes::V3)},
	{"ire2v2", callback(ire2<IPv4>, OspfTypes::V2)},
	{"ire2v3", callback(ire2<IPv6>, OspfTypes::V3)},
	{"trie1v2", callback(trie1<IPv4>, OspfTypes::V2)},
 	{"trie1v3", callback(trie1<IPv6>, OspfTypes::V3)},
	{"trie2v2", callback(trie2<IPv4>, OspfTypes::V2)},
	{"trie2v3", callback(trie2<IPv6>, OspfTypes::V3)},
	{"r1v2", callback(routing1<IPv4>, OspfTypes::V2)},
// 	{"r1v3", callback(routing1<IPv6>, OspfTypes::V3)},
//	{"r2v2", callback(routing1<IPv4>, OspfTypes::V2)},
//  	{"r2v3", callback(routing2<IPv6>, OspfTypes::V3)},
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		if (test == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return t.exit();
}
