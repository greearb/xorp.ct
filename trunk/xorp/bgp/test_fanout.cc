// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/test_fanout.cc,v 1.4 2002/12/17 22:06:07 mjh Exp $"

#include "bgp_module.h"
#include "config.h"
#include <pwd.h>
#include "libxorp/selector.hh"
#include "libxorp/xlog.h"

#include "main.hh"
#include "route_table_base.hh"
#include "route_table_filter.hh"
#include "route_table_ribin.hh"
#include "route_table_debug.hh"
#include "path_attribute_list.hh"
#include "local_data.hh"
#include "dump_iterators.hh"


int main(int, char** argv) {
    //stuff needed to create an eventloop
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_add_default_output();
    xlog_start();
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_fanout.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    //    EventLoop* eventloop = bgpmain.get_eventloop();
    LocalData localdata;
    BGPPeer peer1(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL);
    BGPPeer peer2(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL);

    FanoutTable<IPv4> *fanout_table
	= new FanoutTable<IPv4>("FANOUT", NULL);

    DebugTable<IPv4>* debug_table1
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table1, &handler1);

    DebugTable<IPv4>* debug_table2
	 = new DebugTable<IPv4>("D2", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table2, &handler2);

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
    aspath1.add_AS_in_sequence(AsNum(1));
    aspath1.add_AS_in_sequence(AsNum(2));
    aspath1.add_AS_in_sequence(AsNum(3));
    ASPathAttribute aspathatt1(aspath1);

    AsPath aspath2;
    aspath2.add_AS_in_sequence(AsNum(4));
    aspath2.add_AS_in_sequence(AsNum(5));
    aspath2.add_AS_in_sequence(AsNum(6));
    ASPathAttribute aspathatt2(aspath2);

    AsPath aspath3;
    aspath3.add_AS_in_sequence(AsNum(7));
    aspath3.add_AS_in_sequence(AsNum(8));
    aspath3.add_AS_in_sequence(AsNum(9));
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
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
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
    delete sr1;
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

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
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
    delete sr1;

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

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
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
    delete sr1;

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

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    fanout_table->add_route(*msg, NULL);
    fanout_table->delete_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr2 = new SubnetRoute<IPv4>(net2, palist2);
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
    delete sr1;
    delete sr2;

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

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr2 = new SubnetRoute<IPv4>(net2, palist2);
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
    delete sr1;
    delete sr2;

    //================================================================
    //Test5: more complex flow control, three peers
    //================================================================
    //add a route
    //set output state on both peers to be busy
    BGPPeer peer3(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler3("test3", &peer3, NULL);

    DebugTable<IPv4>* debug_table3
	 = new DebugTable<IPv4>("D3", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table3, &handler3);

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

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    fanout_table->add_route(*msg, NULL);
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr2 = new SubnetRoute<IPv4>(net2, palist2);
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
    delete sr1;
    delete sr2;

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
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
   }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
    }
    fclose(file);
    
    file = fopen("test_fanout.reference", "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read test_fanout.reference\n");
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
    }
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
    }
    fclose(file);
    
    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1)!= 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
	
    }
    unlink(filename.c_str());
}


