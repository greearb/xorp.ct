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

#ident "$XORP: xorp/bgp/test_fanout.cc,v 1.21 2004/05/19 13:23:10 mjh Exp $"

#include "bgp_module.h"
#include "config.h"
#include <pwd.h>
#include "libxorp/selector.hh"
#include "libxorp/xlog.h"
#include "libxorp/test_main.hh"

#include "bgp.hh"
#include "route_table_base.hh"
#include "route_table_filter.hh"
#include "route_table_ribin.hh"
#include "route_table_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"
#include "dump_iterators.hh"

bool
test_fanout(TestInfo& /*info*/)
{
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_fanout.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    //    EventLoop* eventloop = bgpmain.eventloop();
    LocalData localdata;
    Iptuple tuple1("127.0.0.1", 179, "10.0.0.1", 179);
    Iptuple tuple2("127.0.0.1", 179, "10.0.0.2", 179);
    Iptuple tuple3("127.0.0.1", 179, "10.0.0.3", 179);

    BGPPeerData* pd1 
	= new BGPPeerData(tuple1, AsNum(1), IPv4("10.0.0.1"), 30);
    pd1->set_id(IPv4("10.0.0.1"));
    BGPPeerData* pd2
	= new BGPPeerData(tuple2, AsNum(1), IPv4("10.0.0.2"), 30);
    pd2->set_id(IPv4("10.0.0.2"));
    BGPPeerData* pd3
	= new BGPPeerData(tuple3, AsNum(1), IPv4("10.0.0.3"), 30);
    pd3->set_id(IPv4("10.0.0.3"));

    BGPPeer peer1(&localdata, pd1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);
    BGPPeer peer2(&localdata, pd2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);

    FanoutTable<IPv4> *fanout_table
	= new FanoutTable<IPv4>("FANOUT", SAFI_UNICAST, NULL);

    DebugTable<IPv4>* debug_table1
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table1, &handler1, 1);

    DebugTable<IPv4>* debug_table2
	 = new DebugTable<IPv4>("D2", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table2, &handler2, 1);

    debug_table1->set_output_file(filename);
    debug_table1->set_canned_response(ADD_USED);
    debug_table1->enable_tablename_printing();

    debug_table2->set_output_file(debug_table1->output_file());
    debug_table2->set_canned_response(ADD_USED);
    debug_table2->enable_tablename_printing();

    //create a load of attributes 
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");
    //    IPNet<IPv4> net2("1.0.2.0/24");

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
    aspath2.prepend_as(AsNum(4));
    aspath2.prepend_as(AsNum(5));
    aspath2.prepend_as(AsNum(6));
    ASPathAttribute aspathatt2(aspath2);

    AsPath aspath3;
    aspath3.prepend_as(AsNum(7));
    aspath3.prepend_as(AsNum(8));
    aspath3.prepend_as(AsNum(9));
    ASPathAttribute aspathatt3(aspath3);

    PathAttributeList<IPv4>* palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    PathAttributeList<IPv4>* palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);

    PathAttributeList<IPv4>* palist3 =
	new PathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);

    //create a subnet route
    SubnetRoute<IPv4> *sr1, *sr2;
    InternalMessage<IPv4>* msg;

    //================================================================
    //Test1: trivial add and delete
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 1");
    debug_table1->write_comment("ADD AND DELETE");
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    fanout_table->add_route(*msg, NULL);
    fanout_table->push(NULL);

    debug_table1->write_separator();

    //delete the route
    fanout_table->delete_route(*msg, NULL);
    fanout_table->push(NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    fanout_table->add_route(*msg, NULL);
    fanout_table->push(NULL);

    debug_table1->write_separator();

    //delete the route
    fanout_table->delete_route(*msg, NULL);
    fanout_table->push(NULL);

    debug_table1->write_separator();
    sr1->unref();
    delete msg;

    //================================================================
    //Test2: trivial flow control
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 2");
    debug_table1->write_comment("SIMPLE FLOW CONTROL - PEER 2 BUSY");
    debug_table1->write_comment("SENDING FROM PEER 1");

    //set output state on peer 2 to be busy
    fanout_table->output_state(true, debug_table2);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    fanout_table->add_route(*msg, NULL);
    fanout_table->push(NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);
    debug_table1->write_separator();
    sr1->unref();

    //================================================================
    //Test2b: trivial flow control
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 2B");
    debug_table1->write_comment("SIMPLE FLOW CONTROL - PEER 2 BUSY");
    debug_table1->write_comment("SENDING FROM PEER 1");

    //set output state on peer 2 to be busy
    fanout_table->output_state(true, debug_table2);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    //use the implicit push bit rather than an explicit push
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);
    debug_table1->write_separator();
    sr1->unref();

    //================================================================
    //Test3: more complex flow control
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 3");
    debug_table1->write_comment("INTERLEAVED FLOW CONTROL - BOTH PEERS BUSY");
    debug_table1->write_comment("SENDING FROM PEER 1");

    //set output state on both peers to be busy
    fanout_table->output_state(true, debug_table1);
    fanout_table->output_state(true, debug_table2);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    fanout_table->add_route(*msg, NULL);
    fanout_table->delete_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    sr2->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr2, &handler2, 0);
    fanout_table->add_route(*msg, NULL);
    fanout_table->delete_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);
    debug_table1->write_separator();
    sr1->unref();
    sr2->unref();

    //================================================================
    //Test4: more complex flow control
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 4");
    debug_table1->write_comment("INTERLEAVED FLOW CONTROL - BOTH PEERS BUSY");
    debug_table1->write_comment("SENDING FROM PEER 1");

    //set output state on both peers to be busy
    fanout_table->output_state(true, debug_table1);
    fanout_table->output_state(true, debug_table2);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    sr2->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr2, &handler2, 0);
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;
    fanout_table->print_queue();

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    sr1->unref();
    sr2->unref();

    //================================================================
    //Test5: more complex flow control, three peers
    //================================================================
    //add a route
    //set output state on both peers to be busy
    BGPPeer peer3(&localdata, pd3, NULL, &bgpmain);
    PeerHandler handler3("test3", &peer3, NULL, NULL);

    DebugTable<IPv4>* debug_table3
	 = new DebugTable<IPv4>("D3", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table3, &handler3, 1);

    debug_table3->set_output_file(debug_table1->output_file());
    debug_table3->set_canned_response(ADD_USED);
    debug_table3->enable_tablename_printing();

    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 5");
    debug_table1->write_comment("INTERLEAVED FLOW CONTROL - THREE PEERS BUSY");
    debug_table1->write_comment("SENDING FROM PEER 1");

    fanout_table->output_state(true, debug_table1);
    fanout_table->output_state(true, debug_table2);
    fanout_table->output_state(true, debug_table3);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    sr2->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr2, &handler2, 0);
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;
    fanout_table->print_queue();

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table1);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table2);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 3 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 3 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 3 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 3 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 3 GET_NEXT_MESSAGE");
    fanout_table->get_next_message(debug_table3);

    debug_table1->write_separator();
    sr1->unref();
    sr2->unref();

    //================================================================

    debug_table1->write_comment("SHUTDOWN AND CLEAN UP");
    delete fanout_table;
    delete debug_table1;
    delete debug_table2;
    delete debug_table3;
    delete palist1;
    delete palist2;
    delete palist3;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", filename.c_str());
	fprintf(stderr, "TEST FANOUT FAILED\n");
	fclose(file);
	return false;
   }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FANOUT FAILED\n");
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
    ref_filename += "/test_fanout.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST FANOUT FAILED\n");
	fclose(file);
	return false;
    }
    
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FANOUT FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);
    
    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1)!= 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST FANOUT FAILED\n");
	return false;
	
    }
    unlink(filename.c_str());
    return true;
}


