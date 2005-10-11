// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/ospf/test_routing.cc,v 1.3 2005/09/13 18:47:33 atanu Exp $"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>
#include <set>
#include <deque>

#include "ospf_module.h"

#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "debug_io.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "test_common.hh"

// Make sure that all tests free up any memory that they use. This will
// allow us to use the leak checker program.

void
p2p(OspfTypes::Version version, RouterLsa *rlsa, OspfTypes::RouterID id,
    uint32_t metric)
{
     RouterLink rl(version);

     rl.set_type(RouterLink::p2p);
     rl.set_metric(metric);

    switch(version) {
    case OspfTypes::V2:
	// The link id is the router id and the link data is the
	// interface address. They may be the same, they are not
	// interchangeable. Adding one to the link data should allow
	// us the catch any misuse in the code.
	rl.set_link_id(id);
	rl.set_link_data(id + 1);
	break;
    case OspfTypes::V3:
	rl.set_interface_id(id);
	rl.set_neighbour_interface_id(0);
	rl.set_neighbour_router_id(id);
	break;
    }
    rlsa->get_router_links().push_back(rl);
}

void
stub(OspfTypes::Version version, RouterLsa *rlsa, OspfTypes::RouterID id,
    uint32_t metric)
{
     RouterLink rl(version);

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	return;
	break;
    }

     rl.set_type(RouterLink::stub);
     rl.set_metric(metric);

     id <<= 16;

     switch(version) {
     case OspfTypes::V2:
	 rl.set_link_id(id);
	 rl.set_link_data(0xffff0000);
	break;
     case OspfTypes::V3:
	 rl.set_interface_id(id);
	 rl.set_neighbour_interface_id(0);
	 rl.set_neighbour_router_id(id);
	 break;
     }
     rlsa->get_router_links().push_back(rl);
}

// This is the origin.

Lsa::LsaRef
create_RT6(OspfTypes::Version version)
{
     RouterLsa *rlsa = new RouterLsa(version);
     Lsa_header& header = rlsa->get_header();

     uint32_t options = compute_options(version, OspfTypes::NORMAL);

     // Set the header fields.
     switch(version) {
     case OspfTypes::V2:
	 header.set_options(options);
	 break;
     case OspfTypes::V3:
	 rlsa->set_options(options);
	 break;
     }
     header.set_link_state_id(6);
     header.set_advertising_router(6);

     // Link to RT3
     p2p(version, rlsa, 3, 6);

     // Link to RT5
     p2p(version, rlsa, 5, 6);

     // Link to RT10 XXX need to look at this more carefully.
     p2p(version, rlsa, 10, 7);

     rlsa->encode();

     rlsa->set_self_originating(true);

     return Lsa::LsaRef(rlsa);
}

Lsa::LsaRef
create_RT3(OspfTypes::Version version)
{
     RouterLsa *rlsa = new RouterLsa(version);
     Lsa_header& header = rlsa->get_header();

     uint32_t options = compute_options(version, OspfTypes::NORMAL);

     // Set the header fields.
     switch(version) {
     case OspfTypes::V2:
	 header.set_options(options);
	 break;
     case OspfTypes::V3:
	 rlsa->set_options(options);
	 break;
     }
     header.set_link_state_id(3);
     header.set_advertising_router(3);

     // Link to RT6
     p2p(version, rlsa, 6, 8);

     // Network to N4
     stub(version, rlsa, 4, 2);

     // Network to N3
//      network(version, rlsa, 3, 1);

     rlsa->encode();

     return Lsa::LsaRef(rlsa);
}

// Some of the routers from Figure 2. in RFC 2328. Single area.
// This router is R6.

template <typename A> 
bool
routing1(TestInfo& info, OspfTypes::Version version)
{
    DOUT(info) << "hello" << endl;

    EventLoop eventloop;
    DebugIO<A> io(info, version, eventloop);
    io.startup();
    
    Ospf<A> ospf(version, eventloop, &io);
    ospf.set_router_id(set_id("0.0.0.6"));

    OspfTypes::AreaID area = set_id("128.16.64.16");
    const uint16_t interface_prefix_length = 16;
    const uint16_t interface_mtu = 1500;

    PeerManager<A>& pm = ospf.get_peer_manager();

    // Create an area
    pm.create_area_router(area, OspfTypes::NORMAL);

    // Create a peer associated with this area.
    const string interface = "eth0";
    const string vif = "vif0";

    A src;
    switch(src.ip_version()) {
    case 4:
	src = "192.150.187.78";
	break;
    case 6:
	src = "2001:468:e21:c800:220:edff:fe61:f033";
	break;
    default:
	XLOG_FATAL("Unknown IP version %d", src.ip_version());
	break;
    }

    PeerID peerid = pm.
	create_peer(interface, vif, src, interface_prefix_length,
		    interface_mtu,
		    OspfTypes::BROADCAST,
		    area);

    // Bring the peering up
    if (!pm.set_state_peer(peerid, true)) {
	DOUT(info) << "Failed enable peer\n";
	return false;
    }

    AreaRouter<A> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    ar->testing_replace_router_lsa(create_RT6(version));

    ar->testing_add_lsa(create_RT3(version));

    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    ar->testing_delete_lsa(create_RT3(version));

    // At the time of writing the OSPFv3 routing table computations
    // were not complete, when they are remove this test.
    if (OspfTypes::V2 == version) {
	// At this point there should be a single route in the routing
	// table.
	const uint32_t routes = 1;
	if (routes != io.routing_table_size()) {
	    DOUT(info) << "Expecting " << routes << " routes " << "got " <<
		io.routing_table_size() << endl;
	    return false;
	}
	if (!io.routing_table_verify(IPNet<A>("0.4.0.0/16"),
				    A("0.0.0.7"), 8, false, false)) {
	    DOUT(info) << "Mismatch in routing table\n";
	    return false;
	}
    }

    // Now delete the routes.

    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    // Take the peering down
    if (!pm.set_state_peer(peerid, false)) {
	DOUT(info) << "Failed to disable peer\n";
	return false;
    }

    // Delete the peer.
    if (!pm.delete_peer(peerid)) {
	DOUT(info) << "Failed to delete peer\n";
	return false;
    }

    // Delete the area
    if (!pm.destroy_area_router(area)) {
	DOUT(info) << "Failed to delete area\n";
	return false;
    }

    // The routing table should be empty now.
    if (0 != io.routing_table_size()) {
	DOUT(info) << "Expecting no routes " << "got " <<
	    io.routing_table_size() << endl;
	return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"r1V2", callback(routing1<IPv4>, OspfTypes::V2)},
	{"r1V3", callback(routing1<IPv6>, OspfTypes::V3)},
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

    return t.exit();
}
