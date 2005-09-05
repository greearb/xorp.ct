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

#ident "$XORP: xorp/ospf/test_peering.cc,v 1.43 2005/09/05 18:58:04 atanu Exp $"

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

// Make sure that all tests free up any memory that they use. This will
// allow us to use the leak checker program.

template <typename A> 
bool
routing1(TestInfo& info, OspfTypes::Version version)
{
    DOUT(info) << "hello" << endl;

    EventLoop eventloop;
    DebugIO<A> io(info, version, eventloop);
    
    Ospf<A> ospf(version, eventloop, &io);
    ospf.set_router_id(set_id("0.0.0.1"));

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

    ar->debugging_routing_total_recompute();

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
