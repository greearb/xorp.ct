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

#ident "$XORP: xorp/rib/test_register.cc,v 1.17 2004/10/06 21:21:06 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "rib_manager.hh"
#include "rib.hh"
#include "dummy_register_server.hh"


RIB<IPv4>* rib_ptr;
bool verbose = false;

static void
verify_route(const string& ipv4, const string& vifname,
	     const string& nexthop, uint32_t metric,
	     const RibVerifyType matchtype)
{
    if (verbose) {
	printf("-----------------------------------------------------------\n");
	printf("VERIFY: type %d addr %s -> if: %s, NH: %s\n", (int)matchtype,
	       ipv4.c_str(), vifname.c_str(), nexthop.c_str());
    }
    if (rib_ptr->verify_route(IPv4(ipv4.c_str()), vifname,
			      IPv4(nexthop.c_str()), metric,
			      matchtype) != XORP_OK) {
	printf("route verify failed\n");
	abort();
    }
}

static void
test_route_range(const string& ipv4,
		 const string& verifytypestr,
		 const string& vifname,
		 const string& nexthop,
		 const string& lower,
		 const string& upper)
{
    RouteRange<IPv4>* rr;
    RibVerifyType verifytype;

    if (verifytypestr == "miss")
	verifytype = RibVerifyType(MISS);
    else if (verifytypestr == "discard")
	verifytype = RibVerifyType(DISCARD);
    else if (verifytypestr == "ip")
	verifytype = RibVerifyType(IP);
    else {
	printf ("**RouteRange invalid match type specification\n");
	abort();
    }

    rr = rib_ptr->route_range_lookup(IPv4(ipv4.c_str()));
    if (verbose)
	printf("**RouteRange for %s\n", ipv4.c_str());
    if (rr->route() != NULL) {
	DiscardNextHop* dnh;
	dnh = reinterpret_cast<DiscardNextHop* >(rr->route()->nexthop());
	if (verifytype != RibVerifyType(DISCARD) && dnh == NULL) {
	    printf("**RouteRange for %s\n", ipv4.c_str());
	    printf("Expected discard route\n");
	    abort();
	}
	if (rr->route()->vif() == NULL) {
	    printf("BAD Vif NULL != \"%s\"\n", vifname.c_str());
	    abort();
	} else if (rr->route()->vif()->name() != vifname) {
	    printf("**RouteRange for %s\n", ipv4.c_str());
	    printf("BAD Vif \"%s\" != \"%s\"\n",
		   rr->route()->vif()->name().c_str(),
		   vifname.c_str()
		   );
	    abort();
	}
	IPNextHop<IPv4>* nh;
	nh = reinterpret_cast<IPNextHop<IPv4>* >(rr->route()->nexthop());
	if (nh->addr().str() != nexthop) {
	    printf("**RouteRange for %s\n", ipv4.c_str());
	    printf("Found Nexthop: %s\n", nh->addr().str().c_str());
	    printf("BAD Nexthop\n");
	    printf("Got: %s\n", nh->addr().str().c_str());
	    printf("Should be: %s\n", nexthop.c_str());
	    abort();
	}
    } else {
	if (verifytype != RibVerifyType(MISS)) {
	    printf("**RouteRange for %s\n", ipv4.c_str());
	    printf("Expected route miss, got a route\n");
	    abort();
	}
    }
    if (lower != rr->bottom().str()) {
	printf("**RouteRange for %s\n", ipv4.c_str());
	printf("Incorrect lower bound\n");
	printf("**Lower bound: %s\n", rr->bottom().str().c_str());
	printf("**should be: %s\n", lower.c_str());
	abort();
    }
    if (upper != rr->top().str()) {
	printf("**RouteRange for %s\n", ipv4.c_str());
	printf("Incorrect upper bound\n");
	printf("**Upper bound: %s\n", rr->top().str().c_str());
	printf("**should be: %s\n", upper.c_str());
	abort();
    }
    // printf("****PASS****\n");
}


int
main (int /* argc */, char* argv[])
{

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    EventLoop eventloop;
    XrlStdRouter xrl_std_router_rib(eventloop, "rib");

    //
    // The RIB manager
    //
    RibManager rib_manager(eventloop, xrl_std_router_rib, "fea");
    rib_manager.enable();

    RIB<IPv4> rib(UNICAST, rib_manager, eventloop);
    rib_ptr = &rib;

    Vif vif0("vif0");
    Vif vif1("vif1");
    Vif vif2("vif2");

    DummyRegisterServer register_server;
    rib.initialize(register_server);
    rib.add_igp_table("connected", "", "");

    rib.new_vif("vif0", vif0);
    rib.add_vif_address("vif0", IPv4("10.0.0.1"), IPv4Net("10.0.0.0", 24),
			IPv4::ZERO(), IPv4::ZERO());
    rib.new_vif("vif1", vif1);
    rib.add_vif_address("vif1", IPv4("10.0.1.1"), IPv4Net("10.0.1.0", 24),
			IPv4::ZERO(), IPv4::ZERO());
    rib.new_vif("vif2", vif2);
    rib.add_vif_address("vif2", IPv4("10.0.2.1"), IPv4Net("10.0.2.0", 24),
			IPv4::ZERO(), IPv4::ZERO());

    rib.add_igp_table("static", "", "");
    rib.add_igp_table("ospf", "", "");
    rib.add_egp_table("ebgp", "", "");
    rib.add_egp_table("ibgp", "", "");

    rib.print_rib();

    // rib.add_route("connected", IPv4Net("10.0.0.0", 24), IPv4("10.0.0.1"));
    // rib.add_route("connected", IPv4Net("10.0.1.0", 24), IPv4("10.0.1.1"));
    // rib.add_route("connected", IPv4Net("10.0.2.0", 24), IPv4("10.0.2.1"));

    verify_route("10.0.0.4", "vif0", "10.0.0.1", 0, RibVerifyType(IP));
    verify_route("10.0.1.4", "vif1", "10.0.1.1", 0, RibVerifyType(IP));
    verify_route("10.0.2.4", "vif2", "10.0.2.1", 0, RibVerifyType(IP));

    rib.add_route("static", IPv4Net("1.0.0.0", 16), IPv4("10.0.0.2"), 
		  "", "", 0, PolicyTags());
    rib.add_route("static", IPv4Net("1.0.3.0", 24), IPv4("10.0.1.2"), 
		  "", "", 1, PolicyTags());
    rib.add_route("static", IPv4Net("1.0.9.0", 24), IPv4("10.0.2.2"), 
		  "", "", 2, PolicyTags());

    verify_route("1.0.5.4", "vif0", "10.0.0.2", 0, RibVerifyType(IP));
    verify_route("1.0.3.4", "vif1", "10.0.1.2", 1, RibVerifyType(IP));

    // Test with routes just in "static" and "connected" tables
    test_route_range("1.0.4.1", "ip", "vif0", "10.0.0.2", "1.0.4.0", "1.0.8.255");
    test_route_range("1.0.3.1", "ip", "vif1", "10.0.1.2", "1.0.3.0", "1.0.3.255");
    test_route_range("1.0.7.1", "ip", "vif0", "10.0.0.2", "1.0.4.0", "1.0.8.255");
    test_route_range("2.0.0.1", "miss", "lo0", "0.0.0.0", "1.1.0.0",
		     "9.255.255.255");

    // Add a route to another IGP table
    rib.add_route("ospf", IPv4Net("1.0.6.0", 24), IPv4("10.0.2.2"), 
		  "", "", 3, PolicyTags());
    test_route_range("1.0.4.1", "ip", "vif0", "10.0.0.2", "1.0.4.0", "1.0.5.255");
    test_route_range("1.0.8.1", "ip", "vif0", "10.0.0.2", "1.0.7.0", "1.0.8.255");
    test_route_range("1.0.6.1", "ip", "vif2", "10.0.2.2", "1.0.6.0", "1.0.6.255");

    // Add an EGP route
    rib.add_route("ebgp", IPv4Net("5.0.5.0", 24), IPv4("1.0.3.1"), 
		  "", "", 4, PolicyTags());
    test_route_range("5.0.5.1", "ip", "vif1", "10.0.1.2", "5.0.5.0", "5.0.5.255");
    test_route_range("2.0.0.1", "miss", "lo0", "0.0.0.0", "1.1.0.0", "5.0.4.255");

    rib.add_route("ebgp", IPv4Net("1.0.0.0", 20), IPv4("1.0.6.1"), 
		  "", "", 5, PolicyTags());
    test_route_range("1.0.5.1", "ip", "vif2", "10.0.2.2", "1.0.4.0", "1.0.5.255");
    test_route_range("1.0.6.1", "ip", "vif2", "10.0.2.2", "1.0.6.0", "1.0.6.255");

    rib.add_route("ospf", IPv4Net("1.0.5.64", 26), IPv4("10.0.1.2"), 
		  "", "", 6, PolicyTags());
    test_route_range("1.0.5.1", "ip", "vif2", "10.0.2.2", "1.0.4.0", "1.0.5.63");


    // Add a couple of registrations
    RouteRegister<IPv4>* rreg;
    rreg = rib.route_register(IPv4("1.0.5.1"), string("foo"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());
    rreg = rib.route_register(IPv4("1.0.5.2"), string("foo2"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());


    // Check we can delete and add back
    if (rib.route_deregister(IPv4Net("1.0.5.0", 26), string("foo"))
	!= XORP_OK) {
	abort();
    }
    rreg = rib.route_register(IPv4("1.0.5.1"), string("foo"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());


    // Add a route that should have no effect
    rib.add_route("ospf", IPv4Net("1.0.11.0", 24), IPv4("10.0.1.2"), 
		  "", "", 1, PolicyTags());
    if (verbose)
	printf("##########################\n");
    if (!register_server.verify_no_info())
	abort();

    // Add a route that should cause both registrations to be invalidated
    rib.add_route("ospf", IPv4Net("1.0.5.0", 24), IPv4("10.0.1.2"), 
		  "", "", 1, PolicyTags());
    if (!register_server.verify_invalidated("foo 1.0.5.0/26 mcast:false"))
	abort();
    if (!register_server.verify_invalidated("foo2 1.0.5.0/26 mcast:false"))
	abort();
    if (!register_server.verify_no_info())
	abort();

    // Verify that the deregister now fails because the registrations
    // became invalid
    if (rib.route_deregister(IPv4Net("1.0.5.0", 26), string("foo"))
	== XORP_OK) {
	abort();
    }
    if (rib.route_deregister(IPv4Net("1.0.5.0", 26), string("foo2"))
	== XORP_OK) {
	abort();
    }

    // Re-register interest
    rreg = rib.route_register(IPv4("1.0.5.1"), string("foo"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());
    rreg = rib.route_register(IPv4("1.0.5.2"), string("foo2"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());

    //
    // Delete the routes we just added.
    // This one should have no effect.
    //
    rib.delete_route("ospf", IPv4Net("1.0.11.0", 24));
    if (!register_server.verify_no_info())
	abort();

    // This one should cause the registrations to be invalidated
    rib.delete_route("ospf", IPv4Net("1.0.5.0", 24));
    if (!register_server.verify_invalidated("foo 1.0.5.0/26 mcast:false"))
	abort();
    if (!register_server.verify_invalidated("foo2 1.0.5.0/26 mcast:false"))
	abort();
    if (!register_server.verify_no_info())
	abort();

    // Test registration where the address doesn't resolve
    rib.add_route("ospf", IPv4Net("1.0.11.0", 24), IPv4("10.0.1.2"), 
		  "", "", 1, PolicyTags());
    rib.add_route("ospf", IPv4Net("1.0.5.0", 24), IPv4("10.0.1.2"), 
		  "", "", 1, PolicyTags());
    rib.delete_route("ebgp", IPv4Net("1.0.0.0", 20));
    rib.delete_route("static", IPv4Net("1.0.0.0", 16));
    rib.delete_route("ospf", IPv4Net("1.0.6.0", 24));
    if (!register_server.verify_no_info())
	abort();

    // Register interest in all three non-existent ranges
    rreg = rib.route_register(IPv4("1.0.0.1"), string("foo"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());
    rreg = rib.route_register(IPv4("1.0.6.1"), string("foo"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());
    rreg = rib.route_register(IPv4("1.0.7.1"), string("foo2"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());
    rreg = rib.route_register(IPv4("1.0.13.1"), string("foo"));
    if (verbose)
	printf("%s\n", rreg->str().c_str());

    rib.add_route("ospf", IPv4Net("1.0.6.0", 24), IPv4("10.0.2.2"), 
		  "", "", 3, PolicyTags());
    if (!register_server.verify_invalidated("foo 1.0.6.0/23 mcast:false"))
	abort();
    if (!register_server.verify_invalidated("foo2 1.0.6.0/23 mcast:false"))
	abort();
    if (!register_server.verify_no_info())
	abort();
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
