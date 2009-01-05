// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

#ident "$XORP: xorp/contrib/olsr/test_faces.cc,v 1.3 2008/10/02 21:56:36 bms Exp $"

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/tokenize.hh"
#include "libxorp/test_main.hh"

#include "olsr.hh"
#include "debug_io.hh"
#include "emulate_net.hh"

// Reduce the hello interval from 10 to 1 second to speed up the test.
uint16_t hello_interval = 1;
uint16_t refresh_interval = 1;

// Run on a different port.
static const uint16_t MY_OLSR_PORT = 6698;

// Do not stop a tests allow it to run forever to observe timers.
bool forever = false;

/**
 * Configure a single face. Nothing is really expected to go wrong
 * but the test is useful to verify the normal path through the code.
 */
bool
single_face(TestInfo& info)
{
    EventLoop eventloop;

    DebugIO io(info, eventloop);
    io.startup();

    Olsr olsr(eventloop, &io);

    FaceManager& fm = olsr.face_manager();

    // Create a Face.
    const string interface = "eth0";
    const string vif = "vif0";
    const uint16_t interface_cost = 10;

    OlsrTypes::FaceID faceid = fm.create_face(interface, vif);

    olsr.set_main_addr("192.0.2.1");
    fm.set_local_addr(faceid, IPv4("192.0.2.1"));
    fm.set_local_port(faceid, MY_OLSR_PORT);
    fm.set_all_nodes_addr(faceid, IPv4("255.255.255.255"));
    fm.set_all_nodes_port(faceid, MY_OLSR_PORT);

    fm.activate_face(interface, vif);

    if (!olsr.set_hello_interval(TimeVal(hello_interval, 0))) {
	DOUT(info) << "Failed to set hello interval\n";
	return false;
    }

    if (!olsr.set_refresh_interval(TimeVal(refresh_interval, 0))) {
	DOUT(info) << "Failed to set refresh interval\n";
	return false;
    }

    if (!olsr.set_interface_cost(interface, vif, interface_cost)) {
	DOUT(info) << "Failed to set face cost\n";
	return false;
    }

    // Bring the OLSR interface binding up.
    if (!fm.set_face_enabled(faceid, true)) {
	DOUT(info) << "Failed enable face\n";
	return false;
    }

    if (forever)
	while (olsr.running())
	    eventloop.run();

    bool timeout = false;
    XorpTimer t = eventloop.set_flag_after(TimeVal(4 * hello_interval ,0),
					   &timeout);
    while (olsr.running() && !timeout) {
	eventloop.run();
	if (2 == io.packets())
	    break;
    }
    if (timeout) {
	DOUT(info) << "No packets sent, test timed out\n";
	return false;
    }

    // Take the OLSR interface binding down
    if (!fm.set_face_enabled(faceid, false)) {
	DOUT(info) << "Failed to disable face\n";
	return false;
    }

    // Delete the face.
    if (!fm.delete_face(faceid)) {
	DOUT(info) << "Failed to delete face\n";
	return false;
    }

    return true;
}

enum Stagger { NOSTAGGER, STAGGER1, STAGGER2};

string suppress;

/**
 * Configure two faces. Nothing is really expected to go wrong
 * but the test is useful to verify the normal path through the code.
 */
bool
two_faces(TestInfo& info, Stagger stagger)
{
    EventLoop eventloop;

    bool verbose[2];
    verbose[0] = info.verbose();
    verbose[1] = info.verbose();

    if (suppress == "")
	;
    else if (suppress == "olsr1")
	verbose[0] = false;
    else if (suppress == "olsr2")
	verbose[1] = false;
    else {
	info.out() << "illegal value for suppress" << suppress << endl;
	return false;
    }
    
    TestInfo info1(info.test_name() + "(olsr1)" , verbose[0],
		   info.verbose_level(), info.out());
    TestInfo info2(info.test_name() + "(olsr2)" , verbose[1],
		   info.verbose_level(), info.out());

    DebugIO io_1(info1, eventloop);
    io_1.startup();
    DebugIO io_2(info2, eventloop);
    io_2.startup();
    
    Olsr olsr_1(eventloop, &io_1);
    Olsr olsr_2(eventloop, &io_2);

    olsr_1.set_main_addr("192.0.2.1");
    olsr_2.set_main_addr("192.0.2.2");

    FaceManager& fm_1 = olsr_1.face_manager();
    FaceManager& fm_2 = olsr_2.face_manager();

    const string interface_1 = "eth1";
    const string interface_2 = "eth2";
    const string vif_1 = "vif1";
    const string vif_2 = "vif2";
    IPv4 addr_1("192.0.2.1");
    IPv4 addr_2("192.0.2.2");
    const uint16_t interface_cost = 10;

    OlsrTypes::FaceID faceid_1 = fm_1.
	create_face(interface_1, vif_1);
    OlsrTypes::FaceID faceid_2 = fm_2.
	create_face(interface_2, vif_2);

    // TODO: push to DebugIo and use getaddresses() interface from
    // recompute_addresses to deal with the link's addresses.
    fm_1.set_local_addr(faceid_1, addr_1);
    fm_1.set_local_port(faceid_1, MY_OLSR_PORT);
    fm_1.set_all_nodes_addr(faceid_1, IPv4("255.255.255.255"));
    fm_1.set_all_nodes_port(faceid_1, MY_OLSR_PORT);

    fm_2.set_local_addr(faceid_2, addr_2);
    fm_2.set_local_port(faceid_2, MY_OLSR_PORT);
    fm_2.set_all_nodes_addr(faceid_2, IPv4("255.255.255.255"));
    fm_2.set_all_nodes_port(faceid_2, MY_OLSR_PORT);

    fm_1.activate_face(interface_1, vif_1);
    fm_2.activate_face(interface_2, vif_2);

    olsr_1.set_hello_interval(TimeVal(hello_interval, 0));
    olsr_1.set_refresh_interval(TimeVal(refresh_interval, 0));
    olsr_1.set_interface_cost(interface_1, vif_1, interface_cost);

    olsr_2.set_hello_interval(TimeVal(hello_interval, 0));
    olsr_2.set_refresh_interval(TimeVal(refresh_interval, 0));
    olsr_2.set_interface_cost(interface_2, vif_2, interface_cost);

    EmulateSubnet emu(info, eventloop);

    emu.bind_interface("olsr1", interface_1, vif_1, addr_1, MY_OLSR_PORT, io_1);
    emu.bind_interface("olsr2", interface_2, vif_2, addr_2, MY_OLSR_PORT, io_2);

    if (STAGGER1 != stagger)
	fm_1.set_face_enabled(faceid_1, true);
    if (STAGGER2 != stagger)
	fm_2.set_face_enabled(faceid_2, true);

    Neighborhood& nh_1 = olsr_1.neighborhood();
    Neighborhood& nh_2 = olsr_2.neighborhood();

    nh_1.set_willingness(OlsrTypes::WILL_HIGH);
    nh_2.set_willingness(OlsrTypes::WILL_HIGH);

    if (forever)
	while (olsr_1.running() && olsr_2.running())
	    eventloop.run();

    bool timeout = false;
    XorpTimer t = eventloop.set_flag_after(TimeVal(20 * hello_interval, 0),
					   &timeout);

    const int expected = 16;
    while (olsr_1.running() && olsr_2.running() && !timeout) {
	eventloop.run();
	if (expected <= io_1.packets() + io_2.packets())
	    break;
	if (STAGGER1 == stagger && 1 == io_2.packets())
	    fm_1.set_face_enabled(faceid_1, true);
	if (STAGGER2 == stagger && 1 == io_1.packets())
	    fm_2.set_face_enabled(faceid_2, true);
    }
    if (timeout) {
	DOUT(info) << io_1.packets() << " packets sent, at most " << expected <<
	    " expected; test timed out\n";
	return false;
    }

    // Assert that OLSR1 sees OLSR2 correctly.
    try {
	// Assert that OLSR2's interface on the emulated subnet is present in
	// OLSR1's link database, and that the link is symmetric.
	OlsrTypes::LogicalLinkID lid = nh_1.get_linkid(addr_2, addr_1);
	const LogicalLink* l = nh_1.get_logical_link(lid);
	if (l->is_sym() == false) {
	    DOUT(info) << "Failed to establish a symmetric link between "
		       << "remote interface " << addr_2.str() << "\n"
		       << "and local interface " << addr_1.str() << ".\n";
	    return false;
	}

	// Assert that OLSR2 is present in OLSR1's neighbor database,
	// and that the neighbor is symmetric.
	OlsrTypes::NeighborID nid = nh_1.get_neighborid_by_main_addr(addr_2);
	const Neighbor* n = nh_1.get_neighbor(nid);
	if (n->is_sym() == false) {
	    DOUT(info) << addr_1.str() << " failed to see " << addr_2.str() <<
	        " as a symmetric neighbor.\n";
	    return false;
	}

	XLOG_ASSERT(n->willingness() == OlsrTypes::WILL_HIGH);

    } catch (XorpException& e) {
	DOUT(info) << " threw exception " << cstring(e) << "\n";
	return false;
    }

    // Assert that OLSR2 sees OLSR1 correctly.
    try {
	// Assert that OLSR1's interface on the emulated subnet is present in
	// OLSR2's link database, and that the link is symmetric.
	OlsrTypes::LogicalLinkID lid = nh_2.get_linkid(addr_1, addr_2);
	const LogicalLink* l = nh_2.get_logical_link(lid);
	if (l->is_sym() == false) {
	    DOUT(info) << "Failed to establish a symmetric link between "
		       << "remote interface " << addr_1.str() << "\n"
		       << "and local interface " << addr_2.str() << ".\n";
	    return false;
	}

	// Assert that OLSR1 is present in OLSR2's neighbor database,
	// and that the neighbor is symmetric.
	OlsrTypes::NeighborID nid = nh_2.get_neighborid_by_main_addr(addr_1);
	const Neighbor* n = nh_2.get_neighbor(nid);
	if (n->is_sym() == false) {
	    DOUT(info) << addr_2.str() << " failed to see " << addr_1.str() <<
	        " as a symmetric neighbor.\n";
	    return false;
	}

	XLOG_ASSERT(n->willingness() == OlsrTypes::WILL_HIGH);

    } catch (XorpException& e) {
	DOUT(info) << " threw exception " << cstring(e) << "\n";
	return false;
    }

    // Simulate neighbor loss by alternately disconnecting both OLSR
    // instances from the EmulateSubnet. This is similar to pulling the
    // Ethernet jack out or turning the 802.11 radios off below MAC layer.
    //
    // Run the event loop until 3 * NEIGH_HOLD_TIME expires.
    // Verify that both neighbors expire their links.
    //
    // TODO: Simulate the separate use case of interface withdrawal,
    // i.e. OLSR is unconfigured from an interface *or* it is marked
    // administratively down for OLSR.

    int expected_1 = io_1.packets() + 1;
    int expected_2 = io_2.packets() + 1;

    bool pulled_1 = false;
    bool pulled_2 = false;

    if (NOSTAGGER == stagger) {
	debug_msg("Pulling both ports.\n");
	emu.unbind_interface("olsr1", interface_1, vif_1, addr_1,
			     MY_OLSR_PORT, io_1);
	emu.unbind_interface("olsr2", interface_2, vif_2, addr_2,
			     MY_OLSR_PORT, io_2);
	pulled_1 = pulled_2 = true;
    }

    timeout = false;
    t.clear();
    t = eventloop.set_flag_after(3 * nh_1.get_neighbor_hold_time(), &timeout);
    while (!timeout) {
	eventloop.run();
	if (!pulled_1 && STAGGER1 == stagger && expected_1 == io_1.packets()) {
	    debug_msg("Pulling OLSR1.\n");
	    emu.unbind_interface("olsr1", interface_1, vif_1, addr_1,
				 MY_OLSR_PORT, io_1);
	    pulled_1 = true;
	}
	if (!pulled_2 && STAGGER2 == stagger && expected_2 == io_2.packets()) {
	    debug_msg("Pulling OLSR2.\n");
	    emu.unbind_interface("olsr2", interface_2, vif_2, addr_2,
				 MY_OLSR_PORT, io_2);
	    pulled_2 = true;
	}
    }

    if (io_1.packets() + io_2.packets() < expected_1 + expected_2) {
	DOUT(info) << io_1.packets() + io_2.packets()
	   << " packets sent, at least " << expected_1 + expected_2
	   << " expected\n";
	return false;
    }

    // Assert that both neighbors no longer contain each other
    // in their link databases.
    bool thrown;

    // Assert that OLSR1 no longer sees OLSR2 as a neighbor.
    try {
	thrown = false;
	OlsrTypes::NeighborID nid = nh_1.get_neighborid_by_main_addr(addr_2);
	UNUSED(nid);
    } catch (XorpException& e) {
	thrown = true;
    }
    XLOG_ASSERT(thrown == true);

    // Assert that OLSR1 no longer sees a link between OLSR2's and OLSR1's
    // interface addresses.
    try {
	thrown = false;
	OlsrTypes::LogicalLinkID lid = nh_1.get_linkid(addr_2, addr_1);
	UNUSED(lid);
    } catch (XorpException& e) {
	thrown = true;
    }
    XLOG_ASSERT(thrown == true);

    // Assert that OLSR2 no longer sees OLSR1 as a neighbor.
    try {
	thrown = false;
	OlsrTypes::NeighborID nid = nh_2.get_neighborid_by_main_addr(addr_1);
	UNUSED(nid);
    } catch (XorpException& e) {
	thrown = true;
    }
    XLOG_ASSERT(thrown == true);

    // Assert that OLSR2 no longer sees a link between OLSR1's and OLSR2's
    // interface addresses.
    try {
	thrown = false;
	OlsrTypes::LogicalLinkID lid = nh_2.get_linkid(addr_1, addr_2);
	UNUSED(lid);
    } catch (XorpException& e) {
	thrown = true;
    }
    XLOG_ASSERT(thrown == true);

    // Delete the faces.
    if (!fm_1.delete_face(faceid_1)) {
	DOUT(info) << "Failed to delete face\n";
	return false;
    }

    if (!fm_2.delete_face(faceid_2)) {
	DOUT(info) << "Failed to delete face\n";
	return false;
    }

    // TODO: Assert that no leftover state remains.

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

    typedef XorpCallback1<bool, TestInfo&>::RefPtr TestCB;

    struct test {
	string test_name;
	TestCB cb;
    } tests[] = {
	{"single_face", callback(single_face)},
	{"two_faces", callback(two_faces, NOSTAGGER)},
	{"two_faces_s1", callback(two_faces, STAGGER1)},
	{"two_faces_s2", callback(two_faces, STAGGER2)},
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
