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

#ident "$XORP: xorp/bgp/test_decision.cc,v 1.10 2003/02/07 05:35:38 mjh Exp $"

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
#include "path_attribute.hh"
#include "local_data.hh"
#include "dummy_next_hop_resolver.hh"


int main(int, char** argv) {
    // stuff needed to create an eventloop
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_add_default_output();
    xlog_start();
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_decision.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    EventLoop* eventloop = bgpmain.get_eventloop();
    LocalData localdata;

    Iptuple iptuple1("3.0.0.127", 179, "2.0.0.1", 179);
    BGPPeerData *peer_data1 =
	new BGPPeerData(iptuple1, AsNum(1), IPv4("2.0.0.1"), 30);
    // start off with both being IBGP
    peer_data1->set_internal_peer(true);
    peer_data1->set_id("2.0.0.0");
    BGPPeer peer1(&localdata, peer_data1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL);

    Iptuple iptuple2("3.0.0.127", 179, "2.0.0.2", 179);
    BGPPeerData *peer_data2 =
	new BGPPeerData(iptuple2, AsNum(1), IPv4("2.0.0.2"), 30);
    // start off with both being IBGP
    peer_data2->set_internal_peer(true);
    peer_data2->set_id("2.0.0.0");
    BGPPeer peer2(&localdata, peer_data2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL);

    Iptuple iptuple3("3.0.0.127", 179, "2.0.0.3", 179);
    BGPPeerData *peer_data3 =
	new BGPPeerData(iptuple2, AsNum(1), IPv4("2.0.0.3"), 30);
    // start off with both being IBGP
    peer_data3->set_internal_peer(true);
    peer_data3->set_id("2.0.0.0");
    BGPPeer peer3(&localdata, peer_data3, NULL, &bgpmain);
    PeerHandler handler3("test3", &peer3, NULL);

    DummyNextHopResolver<IPv4> next_hop_resolver;

    DecisionTable<IPv4> *decision_table
	= new DecisionTable<IPv4>("DECISION", next_hop_resolver);

    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)decision_table);
    decision_table->set_next_table(debug_table);

    RibInTable<IPv4>* ribin_table1
	= new RibInTable<IPv4>("RIB-IN1", &handler1);
    ribin_table1->set_next_table(decision_table);
    decision_table->add_parent(ribin_table1, &handler1);

    RibInTable<IPv4>* ribin_table2
	= new RibInTable<IPv4>("RIB-IN2", &handler2);
    ribin_table2->set_next_table(decision_table);
    decision_table->add_parent(ribin_table2, &handler2);

    RibInTable<IPv4>* ribin_table3
	= new RibInTable<IPv4>("RIB-IN3", &handler3);
    ribin_table3->set_next_table(decision_table);
    decision_table->add_parent(ribin_table3, &handler3);

    debug_table->set_output_file(filename);
    debug_table->set_canned_response(ADD_USED);

    // create a load of attributes
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");
    //    IPNet<IPv4> net2("1.0.2.0/24");

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);

    IPv4 nexthop2("2.0.0.2");
    NextHopAttribute<IPv4> nhatt2(nexthop2);

    IPv4 nexthop3("2.0.0.3");
    NextHopAttribute<IPv4> nhatt3(nexthop3);

    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);

    OriginAttribute igp_origin_att(IGP);
    OriginAttribute egp_origin_att(EGP);
    OriginAttribute incomplete_origin_att(INCOMPLETE);

    AsPath aspath1;
    aspath1.prepend_as(AsNum(1));
    aspath1.prepend_as(AsNum(2));
    aspath1.prepend_as(AsNum(3));
    ASPathAttribute aspathatt1(aspath1);

    AsPath aspath2;
    aspath2.prepend_as(AsNum(4));
    aspath2.prepend_as(AsNum(5));
    aspath2.prepend_as(AsNum(6));
    aspath2.prepend_as(AsNum(6));
    ASPathAttribute aspathatt2(aspath2);

    AsPath aspath3;
    aspath3.prepend_as(AsNum(7));
    aspath3.prepend_as(AsNum(8));
    aspath3.prepend_as(AsNum(9));
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

    PathAttributeList<IPv4>* palist4;
    PathAttributeList<IPv4>* palist5;
    PathAttributeList<IPv4>* palist6;

    // create a subnet route
    SubnetRoute<IPv4> *sr1;
    InternalMessage<IPv4>* msg;
    UNUSED(net2);
    // ================================================================
    // Test1: trivial add and delete
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    ribin_table1->push(NULL);
    debug_table->write_separator();

    // delete the route
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin_table1->delete_route(*msg, NULL);
    ribin_table1->push(NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test1b: trivial add and delete
    // ================================================================
    // add a route - nexthop is unresolvable
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 1b");
    debug_table->write_comment("ADD AND DELETE, NEXTHOP UNRESOLVABLE");
    debug_table->write_comment("SENDING ADD FROM PEER 1");
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING PUSH FROM PEER 1");
    ribin_table1->push(NULL);
    debug_table->write_separator();

    // delete the route
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("SENDING DELETE FROM PEER 1");
    ribin_table1->delete_route(*msg, NULL);
    ribin_table1->push(NULL);
    sr1->unref();
    delete msg;

    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    debug_table->write_separator();

    // ================================================================
    // Test2: identical add from two peers
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("IDENTICAL ADD FROM TWO PEERS");
    debug_table->write_comment("PEER1 HAS LOWEST NEIGHBOR ADDRESS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test2B: identical add from two peers
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 2B");
    debug_table->write_comment("IDENTICAL ADD FROM TWO PEERS");
    debug_table->write_comment("PEER1 HAS LOWEST NEIGHBOR ADDRESS");
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test2C: identical add from two peers
    // ================================================================
    // add a route
    peer_data1->set_id("101.0.0.0");
    peer_data2->set_id("100.0.0.0");
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 2C");
    debug_table->write_comment("IDENTICAL ADD FROM TWO PEERS");
    debug_table->write_comment("PEER1 HAS LOWEST NEIGHBOR ADDRESS");
    debug_table->write_comment("PEER2 HAS LOWEST BGP ID");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test2D: identical add from two peers
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 2D");
    debug_table->write_comment("IDENTICAL ADD FROM TWO PEERS");
    debug_table->write_comment("PEER1 HAS LOWEST NEIGHBOR ADDRESS");
    debug_table->write_comment("PEER2 HAS LOWEST BGP ID");
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test3: decision by AS path length
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("TEST OF DIFFERENT AS PATH LENGTHS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test3b: decision by AS path length
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 3B");
    debug_table->write_comment("TEST OF DIFFERENT AS PATH LENGTHS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test4: decision by LocalPref
    // ================================================================
    palist4 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(200));
    palist4->rehash();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 4");
    debug_table->write_comment("TEST OF LOCALPREF");
    debug_table->write_comment("HIGHEST LOCALPREF WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test4b: decision by LocalPref
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 4B");
    debug_table->write_comment("TEST OF LOCALPREF");
    debug_table->write_comment("HIGHEST LOCALPREF WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;
    delete palist4;

    debug_table->write_separator();
    // ================================================================
    // Test5: decision by MED
    // ================================================================
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->add_path_attribute(MEDAttribute(100));
    palist4->rehash();

    palist5 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(100));
    palist5->add_path_attribute(MEDAttribute(200));
    palist5->rehash();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("LOWEST MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test5B: decision by MED
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5B");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("LOWEST MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test5C: decision by MED
    // ================================================================
    palist5->remove_attribute_by_type(MED);
    palist5->rehash();
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5C");
    debug_table->write_comment("TEST OF MED - ONLY ONE HAS MED");
    debug_table->write_comment("ROUTE WITHOUT MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test5D: decision by MED
    // ================================================================
    palist5->remove_attribute_by_type(MED);
    palist5->rehash();
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5D");
    debug_table->write_comment("TEST OF MED - ONLY ONE HAS MED");
    debug_table->write_comment("ROUTE WITHOUT MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;
    delete palist4;
    delete palist5;

    debug_table->write_separator();

    // ================================================================
    // Test5E: decision by MED
    // ================================================================

    // AS paths are the same length, but different paths.
    // MEDs are different (peer 1 has better MED), but shouldn't be compared.
    // peer 2 should win on BGP ID test.
    peer_data1->set_id("101.0.0.0");
    peer_data2->set_id("100.0.0.0");
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->add_path_attribute(MEDAttribute(100));
    palist4->rehash();

    palist5 = new PathAttributeList<IPv4>(nhatt1, aspathatt3, igp_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(100));
    palist5->add_path_attribute(MEDAttribute(200));
    palist5->rehash();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5E");
    debug_table->write_comment("TEST OF MED - DIFFERENT MEDS FROM DIFFERENCE AS'S");
    debug_table->write_comment("");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;
    delete palist4;
    delete palist5;
    debug_table->write_separator();

    // ================================================================
    // Test6: decision by Origin
    // ================================================================
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, egp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->rehash();

    palist5 = new PathAttributeList<IPv4>(nhatt1, aspathatt1,
					  incomplete_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(100));
    palist5->rehash();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 6");
    debug_table->write_comment("TEST OF ORIGIN");
    debug_table->write_comment("IGP beats EGP beats INCOMPLETE");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: WINS BY DEFAULT");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: LOSES");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    debug_table->write_comment("EXPECT: ALLOWS EGP ROUTE TO WIN");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: LOSES");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("EXPECT: ALLOWS INCOMPLETE ROUTE TO WIN");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;
    delete palist4;
    delete palist5;

    debug_table->write_separator();

    // ================================================================
    // Test6B: decision by Origin
    // ================================================================
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, egp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->rehash();

    palist5 = new PathAttributeList<IPv4>(nhatt1, aspathatt1,
					  incomplete_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(100));
    palist5->rehash();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 6B");
    debug_table->write_comment("TEST OF ORIGIN");
    debug_table->write_comment("IGP beats EGP beats INCOMPLETE");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: WINS BY DEFAULT");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("EXPECT: NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;
    delete palist4;
    delete palist5;

    debug_table->write_separator();

    // ================================================================
    // Test7: decision by IGP distance
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    next_hop_resolver.set_nexthop_metric(nexthop1, 100);
    next_hop_resolver.set_nexthop_metric(nexthop3, 200);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 7");
    debug_table->write_comment("TEST OF IGP DISTANCE");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("PEER 1 HAS BETTER IGP DISTANCE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test7B: decision by IGP distance
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    next_hop_resolver.set_nexthop_metric(nexthop1, 200);
    next_hop_resolver.set_nexthop_metric(nexthop3, 100);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 7B");
    debug_table->write_comment("TEST OF IGP DISTANCE");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("PEER 2 HAS BETTER IGP DISTANCE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test7C: decision by IGP distance
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    next_hop_resolver.set_nexthop_metric(nexthop1, 100);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 7C");
    debug_table->write_comment("TEST OF IGP DISTANCE");
    debug_table->write_comment("PEER 2 NOT RESOLVABLE");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test7D: decision by IGP distance
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.set_nexthop_metric(nexthop3, 100);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 7D");
    debug_table->write_comment("TEST OF IGP DISTANCE");
    debug_table->write_comment("PEER 1 NOT RESOLVABLE");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test7E: decision by IGP distance
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 7E");
    debug_table->write_comment("TEST OF IGP DISTANCE");
    debug_table->write_comment("NEITHER PEER IS RESOLVABLE");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test8A: test of replace_route
    // ================================================================
    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 8A");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("REPLACING FROM A SINGLE PEER");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test8B: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 8B");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("REPLACING FROM A SINGLE PEER, FIRST NOT RESOLVABLE");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test8C: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 8C");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("REPLACING FROM A SINGLE PEER, SECOND NOT RESOLVABLE");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test8D: test of replace_route
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop2);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 8D");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("REPLACING FROM A SINGLE PEER, NEITHER RESOLVABLE");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test9A: test of replace_route
    // ================================================================
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->rehash();
    palist5 = new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(200));
    palist5->rehash();
    palist6 = new PathAttributeList<IPv4>(nhatt3, aspathatt1, igp_origin_att);
    palist6->add_path_attribute(LocalPrefAttribute(300));
    palist6->rehash();
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 9A");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 LOSES, PEER2 WINS");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test9B: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 9B");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 2 WINS, PEER 1 WINS, PEER2 WINS");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test9C: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 9C");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 LOSES, PEER2 LOSES");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test9D: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 9D");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 WINS, PEER2 WINS");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test9E: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 9E");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 WINS, PEER2 LOSES");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test 10A: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 10A");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 WINS, PEER2 BECOMES UNRESOLVABLE");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop3.str() +
				" BECOMES UNRESOLVABLE").c_str());
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    decision_table->igp_nexthop_changed(nexthop3);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);

    // ================================================================
    // Test 10B: test of replace_route
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 10B");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 WINS, PEER1 BECOMES UNRESOLVABLE");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop2.str() +
				" BECOMES UNRESOLVABLE").c_str());
    next_hop_resolver.unset_nexthop_metric(nexthop2);
    decision_table->igp_nexthop_changed(nexthop2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test 10C: test of replace_route
    // ================================================================
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 10C");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 NOT RESOLVABLE, PEER2 BECOMES RESOLVABLE AND WINS");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop3.str() +
				" BECOMES RESOLVABLE").c_str());
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);
    decision_table->igp_nexthop_changed(nexthop3);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    delete palist4;
    delete palist5;
    delete palist6;

    // ================================================================
    // Test 11A: test of routes becoming resolvable
    // ================================================================
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->rehash();
    palist5 = new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(200));
    palist5->rehash();
    palist6 = new PathAttributeList<IPv4>(nhatt3, aspathatt1, igp_origin_att);
    palist6->add_path_attribute(LocalPrefAttribute(300));
    palist6->rehash();
    next_hop_resolver.unset_nexthop_metric(nexthop2);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 11A");
    debug_table->write_comment("TEST OF ROUTES BECOMING RESOLVABLE");
    debug_table->write_comment("PEER 1, 2 NOT RESOLVABLE, PEER1 BECOMES RESOLVABLE");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop2.str() +
				" BECOMES RESOLVABLE").c_str());
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    decision_table->igp_nexthop_changed(nexthop2);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    delete palist4;
    delete palist5;
    delete palist6;
    // ================================================================
    // Test 11B: test of routes becoming resolvable
    // ================================================================
    palist4 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist4->add_path_attribute(LocalPrefAttribute(100));
    palist4->rehash();
    palist5 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist5->add_path_attribute(LocalPrefAttribute(200));
    palist5->rehash();
    palist6 = new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    palist6->add_path_attribute(LocalPrefAttribute(300));
    palist6->rehash();
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop2);

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 11B");
    debug_table->write_comment("TEST OF ROUTES BECOMING RESOLVABLE");
    debug_table->write_comment("PEER 1, 2 NOT RESOLVABLE, BOTH BECOME RESOLVABLE SIMULTANEOUSLY");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop1.str() +
				" BECOMES RESOLVABLE").c_str());
    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    decision_table->igp_nexthop_changed(nexthop1);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test 11C: test of routes becoming unresolvable
    // ================================================================

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 11C");
    debug_table->write_comment("TEST OF ROUTES BECOMING UNRESOLVABLE");
    debug_table->write_comment("PEER 1, 2 RESOLVABLE, BOTH BECOME UNRESOLVABLE SIMULTANEOUSLY");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop1.str() +
				" BECOMES UNRESOLVABLE").c_str());
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    decision_table->igp_nexthop_changed(nexthop1);
    while (eventloop->timers_pending()) {
	eventloop->run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test 12: test with three peers
    // ================================================================
    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 12");
    debug_table->write_comment("TEST WITH THREE PEERS");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test 13: decision of IBGP vs EBGP
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    next_hop_resolver.set_nexthop_metric(nexthop1, 200);
    next_hop_resolver.set_nexthop_metric(nexthop3, 100);
    peer_data2->set_internal_peer(false);
    peer_data2->set_as(AsNum(9));
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 13");
    debug_table->write_comment("TEST OF IBGP vs EBGP");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("PEER 1 HAS BETTER IGP DISTANCE");
    debug_table->write_comment("PEER 2 IS EBGP");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================
    // Test 13B: decision of IBGP vs EBGP
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 13B");
    debug_table->write_comment("TEST OF IBGP vs EBGP");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("PEER 1 HAS BETTER IGP DISTANCE");
    debug_table->write_comment("PEER 2 IS EBGP");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: LOSES");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(*msg, NULL);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // ================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete decision_table;
    delete debug_table;
    delete ribin_table1;
    delete ribin_table2;
    delete ribin_table3;
    delete palist1;
    delete palist2;
    delete palist3;
    delete palist4;
    delete palist5;
    delete palist6;

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

    file = fopen("test_decision.reference", "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read test_decision.reference\n");
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

    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1) != 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST FAILED\n");
	exit(1);

    }
    unlink(filename.c_str());
}



