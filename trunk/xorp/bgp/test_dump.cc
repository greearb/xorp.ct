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

#ident "$XORP: xorp/bgp/test_dump.cc,v 1.5 2003/01/17 03:50:48 mjh Exp $"

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
#include "dummy_next_hop_resolver.hh"


int main(int, char** argv) {
    //stuff needed to create an eventloop
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_add_default_output();
    xlog_start();
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_dump.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    EventLoop* eventloop = bgpmain.get_eventloop();
    LocalData localdata;

    Iptuple iptuple1("3.0.0.127", 179, "2.0.0.1", 179);
    BGPPeerData *peer_data1
	= new BGPPeerData(iptuple1, AsNum(1), IPv4("2.0.0.1"), 30);
    //start off with both being IBGP
    peer_data1->set_internal_peer(true);
    peer_data1->set_id("2.0.0.0");
    BGPPeer peer1(&localdata, peer_data1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL);

    Iptuple iptuple2("3.0.0.127", 179, "2.0.0.2", 179);
    BGPPeerData *peer_data2
	= new BGPPeerData(iptuple2, AsNum(1), IPv4("2.0.0.2"), 30);
    //start off with both being IBGP
    peer_data2->set_internal_peer(true);
    peer_data2->set_id("2.0.0.0");
    BGPPeer peer2(&localdata, peer_data2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL);

    Iptuple iptuple3("3.0.0.127", 179, "2.0.0.3", 179);
    BGPPeerData *peer_data3
	= new BGPPeerData(iptuple2, AsNum(1), IPv4("2.0.0.3"), 30);
    //start off with both being IBGP
    peer_data3->set_internal_peer(true);
    peer_data3->set_id("2.0.0.0");
    BGPPeer peer3(&localdata, peer_data3, NULL, &bgpmain);
    PeerHandler handler3("test3", &peer3, NULL);

    DummyNextHopResolver<IPv4> next_hop_resolver;

    DecisionTable<IPv4> *decision_table
	= new DecisionTable<IPv4>("DECISION", next_hop_resolver);

    RibInTable<IPv4>* ribin_table1
	= new RibInTable<IPv4>("RIB-IN1", &handler1);

    //In principle the cache shouldn't be needed, but ribin can't be
    //directly dollowed by decision because the dump_table calls
    //set_parent on the next table, and decision table doesn't support
    //set_parent because it is the only table with multiple parents.
    CacheTable<IPv4>* cache_table1
	= new CacheTable<IPv4>("Cache1", ribin_table1);
    ribin_table1->set_next_table(cache_table1);
    cache_table1->set_next_table(decision_table);

    RibInTable<IPv4>* ribin_table2
	= new RibInTable<IPv4>("RIB-IN2", &handler2);

    CacheTable<IPv4>* cache_table2
	= new CacheTable<IPv4>("Cache2", ribin_table2);
    ribin_table2->set_next_table(cache_table2);
    cache_table2->set_next_table(decision_table);

    RibInTable<IPv4>* ribin_table3
	= new RibInTable<IPv4>("RIB-IN3", &handler3);

    CacheTable<IPv4>* cache_table3
	= new CacheTable<IPv4>("Cache3", ribin_table3);
    ribin_table3->set_next_table(cache_table3);
    cache_table3->set_next_table(decision_table);

    decision_table->add_parent(cache_table1, &handler1);
    decision_table->add_parent(cache_table2, &handler2);
    decision_table->add_parent(cache_table3, &handler3);

    FanoutTable<IPv4> *fanout_table
	= new FanoutTable<IPv4>("FANOUT", decision_table);
    decision_table->set_next_table(fanout_table);

    DebugTable<IPv4>* debug_table1
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table1, &handler1);

    DebugTable<IPv4>* debug_table2
	 = new DebugTable<IPv4>("D2", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table2, &handler2);

    DebugTable<IPv4>* debug_table3
	 = new DebugTable<IPv4>("D3", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table3, &handler3);

    debug_table1->set_output_file(filename);
    debug_table1->set_canned_response(ADD_USED);
    debug_table1->enable_tablename_printing();

    debug_table2->set_output_file(debug_table1->output_file());
    debug_table2->set_canned_response(ADD_USED);
    debug_table2->enable_tablename_printing();

    debug_table3->set_output_file(debug_table1->output_file());
    debug_table3->set_canned_response(ADD_USED);
    debug_table3->enable_tablename_printing();

    //create a load of attributes 
    IPNet<IPv4> net0("1.0.0.0/24");
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");
    IPNet<IPv4> net3("1.0.3.0/24");
    IPNet<IPv4> net4("1.0.4.0/24");
    IPNet<IPv4> net5("1.0.5.0/24");

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);

    IPv4 nexthop2("2.0.0.2");
    NextHopAttribute<IPv4> nhatt2(nexthop2);

    IPv4 nexthop3("2.0.0.3");
    NextHopAttribute<IPv4> nhatt3(nexthop3);

    IPv4 nexthop4("2.0.0.4");
    NextHopAttribute<IPv4> nhatt4(nexthop4);

    IPv4 nexthop5("2.0.0.5");
    NextHopAttribute<IPv4> nhatt5(nexthop5);

    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);
    next_hop_resolver.set_nexthop_metric(nexthop4, 27);
    next_hop_resolver.set_nexthop_metric(nexthop5, 27);

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
    aspath2.add_AS_in_sequence(AsNum(6));
    ASPathAttribute aspathatt2(aspath2);

    AsPath aspath3;
    aspath3.add_AS_in_sequence(AsNum(7));
    aspath3.add_AS_in_sequence(AsNum(8));
    aspath3.add_AS_in_sequence(AsNum(9));
    ASPathAttribute aspathatt3(aspath3);

    PathAttributeList<IPv4>* palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist1->add_path_attribute(LocalPrefAttribute(100));
    palist1->rehash();

    PathAttributeList<IPv4>* palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);
    palist2->add_path_attribute(LocalPrefAttribute(100));
    palist2->rehash();

    PathAttributeList<IPv4>* palist3 =
	new PathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);
    palist3->add_path_attribute(LocalPrefAttribute(100));
    palist3->rehash();

    PathAttributeList<IPv4>* palist4 =
	new PathAttributeList<IPv4>(nhatt4, aspathatt3, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->rehash();

    PathAttributeList<IPv4>* palist5 =
	new PathAttributeList<IPv4>(nhatt5, aspathatt3, igp_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(100));
    palist5->rehash();

    //create a subnet route
    SubnetRoute<IPv4> *sr1;
    InternalMessage<IPv4>* msg;

    //================================================================
    //Test1: trivial add and delete to test plumbing
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 1");
    debug_table1->write_comment("ADD AND DELETE");
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();

    //delete the route
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1,  0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    //================================================================
    //Test2: simple dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 2");
    debug_table1->write_comment("SIMPLE DUMP TO PEER 3");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();

    //================================================================
    //Test3: simple dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 3");
    debug_table1->write_comment("SIMPLE DUMP TO PEER 3");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net4, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net4, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();

    //================================================================
    //Test4: dump, peer 2 goes down during dump, before anything has 
    //       been dumped
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 4");
    debug_table1->write_comment("SIMPLE DUMP TO PEER 3, PEER 2 GOES DOWN");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net4, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    fanout_table->add_next_table(debug_table2, &handler2);
    debug_table2->set_parent(fanout_table);
    ribin_table2->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }
    debug_table1->write_separator();

    //================================================================
    //Test5: dump, peer 2 goes down during dump, half way through peer 2 dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 5");
    debug_table1->write_comment("DUMP TO PEER 3, PEER 2 GOES DOWN DURING P2 DUMP");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->add_next_table(debug_table2, &handler2);
    debug_table2->set_parent(fanout_table);
    ribin_table2->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();

    //================================================================
    //Test6: dump, peer 1 goes down before any dump processing can happen
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 6");
    debug_table1->write_comment("DUMP TO PEER 3, PEER 1 GOES DOWN");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO IMMEDIATE CHANGE");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GOES DOWN");
    debug_table1->write_comment("EXPECT NO IMMEDIATE CHANGE");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->add_next_table(debug_table1, &handler1);
    debug_table1->set_parent(fanout_table);
    ribin_table1->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table1);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();

    //================================================================
    //Test7: dump, peer 2 goes down during dump, 
    //       between peer 1 and peer 2 dumps
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 7");
    debug_table1->write_comment("DUMP TO PEER 3, PEER 2 GOES DOWN BETWEEN P1 AND P2 DUMPS");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->add_next_table(debug_table2, &handler2);
    debug_table2->set_parent(fanout_table);
    ribin_table2->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();

    //================================================================
    //Test8: dump, peer 2 goes down during dump, 
    //       before peer 1 dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 8");
    debug_table1->write_comment("DUMP TO PEER 3, PEER 2 GOES DOWN BEFORE P1 DUMP");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->add_next_table(debug_table2, &handler2);
    debug_table2->set_parent(fanout_table);
    ribin_table2->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();

    //================================================================
    //Test9: dump, both peers go down before first dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 9");
    debug_table1->write_comment("DUMP TO PEER 3, P1 AND P2 GO DOWN BEFORE FIRST DUMP");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GOES DOWN");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT NO CHANGE");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->add_next_table(debug_table2, &handler2);
    debug_table2->set_parent(fanout_table);
    ribin_table2->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->add_next_table(debug_table1, &handler1);
    debug_table1->set_parent(fanout_table);
    ribin_table1->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table1);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table1->write_separator();

    //================================================================
    //Test10: dump.  Send new routes during dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 10");
    debug_table1->write_comment("DUMP TO PEER 3, SEND NEW ROUTES");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net4, palist4);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 3 WHEN EVENTLOOP RUNS");
    sr1 = new SubnetRoute<IPv4>(net0, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net5, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.4.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.5.0/24 RECEIVED BY PEER 3");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net2, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net4, palist4);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.0.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.0.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net0, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.5.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.5.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net5, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();

    //================================================================
    //Test11: dump.  Replace routes during dump
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 11");
    debug_table1->write_comment("DUMP TO PEER 3, REPLACE ROUTES");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    sr1 = new SubnetRoute<IPv4>(net4, palist4);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    fanout_table->add_next_table(debug_table3, &handler3);
    debug_table3->set_parent(fanout_table);
    ribin_table3->ribin_peering_came_up();
    fanout_table->dump_entire_table(debug_table3);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("ONE EVENT RUN");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    eventloop->run();

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 1");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 3 WHEN EVENTLOOP RUNS");
    sr1 = new SubnetRoute<IPv4>(net3, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 1");
    sr1 = new SubnetRoute<IPv4>(net4, palist5);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT REPLACE 1.0.3.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.4.0/24 RECEIVED BY PEER 3");
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net3, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 3");
    sr1 = new SubnetRoute<IPv4>(net4, palist4);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table1->write_separator();

    //================================================================

    debug_table1->write_comment("SHUTDOWN AND CLEAN UP");
    delete decision_table;
    delete debug_table1;
    delete debug_table2;
    delete debug_table3;
    delete ribin_table1;
    delete ribin_table2;
    delete ribin_table3;
    delete fanout_table;
    delete cache_table1;
    delete cache_table2;
    delete cache_table3;
    delete palist1;
    delete palist2;
    delete palist3;
    delete palist4;
    delete palist5;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", filename.c_str());
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
   }
#define BUFSIZE 60000
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
    }
    fclose(file);
    
    file = fopen("test_dump.reference", "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read test_dump.reference\n");
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


