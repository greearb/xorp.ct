// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/test_deletion.cc,v 1.19 2004/02/24 03:16:57 atanu Exp $"

#include "bgp_module.h"
#include "config.h"
#include <pwd.h>
#include "libxorp/selector.hh"
#include "libxorp/xlog.h"
#include "libxorp/test_main.hh"

#include "bgp.hh"
#include "route_table_base.hh"
#include "route_table_ribin.hh"
#include "route_table_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"
#include "dump_iterators.hh"

bool
test_deletion(TestInfo& /*info*/)
{
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_deletion.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    LocalData localdata;
    BGPPeer peer1(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);
    BGPPeer peer2(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);

    //trivial plumbing
    RibInTable<IPv4> *ribin 
	= new RibInTable<IPv4>("RIB-IN", SAFI_UNICAST, &handler1);
    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)ribin);
    ribin->set_next_table(debug_table);
    debug_table->set_output_file(filename);
    debug_table->set_canned_response(ADD_USED);

    //create a load of attributes 
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");
    IPNet<IPv4> net3("1.0.3.0/24");
    IPNet<IPv4> net4("1.0.4.0/24");

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);

    IPv4 nexthop2("2.0.0.2");
    NextHopAttribute<IPv4> nhatt2(nexthop2);

    IPv4 nexthop3("2.0.0.3");
    NextHopAttribute<IPv4> nhatt3(nexthop3);

    OriginAttribute igp_origin_att(IGP);

    AsPath aspath1;
    aspath1.prepend_as(AsNum(1));
    aspath1.prepend_as(AsNum(2));
    aspath1.prepend_as(AsNum(3));
    ASPathAttribute aspathatt1(aspath1);

    AsPath aspath2;
    aspath2.prepend_as(AsNum(3));
    aspath2.prepend_as(AsNum(4));
    aspath2.prepend_as(AsNum(5));
    ASPathAttribute aspathatt2(aspath2);

    AsPath aspath3;
    aspath3.prepend_as(AsNum(6));
    aspath3.prepend_as(AsNum(6));
    aspath3.prepend_as(AsNum(7));
    aspath3.prepend_as(AsNum(8));
    ASPathAttribute aspathatt3(aspath3);

    PathAttributeList<IPv4>* palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    PathAttributeList<IPv4>* palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);

    PathAttributeList<IPv4>* palist3 =
	new PathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);

    //create a subnet route
    SubnetRoute<IPv4> *sr1;

    InternalMessage<IPv4>* msg;

    //================================================================
    //Test1: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("TRIVIAL ADD AND PEERING GOES DOWN");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();
    debug_table->write_separator();

    //================================================================
    //Test2: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("3 ADDS, SAME PALIST, PEERING GOES DOWN");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();
    debug_table->write_separator();

    //================================================================
    //Test3: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("3 ADDS, 3 PALISTS, PEERING GOES DOWN");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();
    debug_table->write_separator();

    //================================================================
    //Test4: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 4");
    debug_table->write_comment("4 ADDS, 2 PALISTS, PEERING GOES DOWN");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net4, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();
    debug_table->write_separator();

    //================================================================
    //Test5: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 5");
    debug_table->write_comment("BOUNCE THE PEERING AGAIN");

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }
    debug_table->write_separator();

    //================================================================
    //Test6: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 6");
    debug_table->write_comment("4 ADDS, 2 PALISTS, PEERING GOES DOWN");
    debug_table->write_comment("2 MORE ADDS BEFORE PROCESSING TIMERS");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net4, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net4, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    //================================================================
    //Test7: trivial add and peering goes down
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 7");
    debug_table->write_comment("4 ADDS, 2 PALISTS, PEERING GOES DOWN");
    debug_table->write_comment("2 MORE ADDS BEFORE PROCESSING TIMERS");
    debug_table->write_comment("AS TEST 6, BUT NEW PALISTS ARE DIFFERENT");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net4, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net4, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    //================================================================
    //Test8: 
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 8");
    debug_table->write_comment("4 ADDS, 2 PALISTS, PEERING GOES DOWN");
    debug_table->write_comment("2 MORE ADDS BEFORE PROCESSING TIMERS");
    debug_table->write_comment("AS TEST 6, NEW ROUTES SHOULD DELETE CHAIN");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net4, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net3, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    //================================================================
    //Test9: 
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 9");
    debug_table->write_comment("2 ADDS, 2 PALISTS, PEERING GOES DOWN");
    debug_table->write_comment("SAME 2 ADDS BEFORE PROCESSING TIMERS");
    debug_table->write_comment("ADDS SHOULD REMOVE ALL ROUTE FROM DEL TAB");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    //================================================================
    //Test10: 
    //================================================================
    //add a route
    debug_table->write_comment("****************************************************************");
    debug_table->write_comment("TEST 10");
    debug_table->write_comment("TWO DELETION TABLES");

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("ADD A ROUTE");
    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("PEERING GOES DOWN");
    ribin->ribin_peering_went_down();

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    debug_table->write_separator();
    debug_table->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("PEERING COMES UP");
    ribin->ribin_peering_came_up();

    //================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete ribin;
    delete debug_table;
    delete palist1;
    delete palist2;
    delete palist3;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", filename.c_str());
	fprintf(stderr, "TEST DELETION FAILED\n");
	fclose(file);
	return false;
    }
#define BUFSIZE 20000
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST DELETION FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);

    string ref_filename;
    const char* srcdir = getenv("srcdir");
    if (srcdir) {
	ref_filename = string(srcdir); 
    } else {
	ref_filename = ".";
    }
    ref_filename += "/test_deletion.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST DELETION FAILED\n");
	fclose(file);
	return false;
    }
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST DELETION FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);
    
    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1)!= 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST DELETION FAILED\n");
	return false;
	
    }
    unlink(filename.c_str());
    return true;
}


