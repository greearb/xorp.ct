// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/test_main.hh"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "libcomm/comm_api.h"

#include "bgp.hh"
#include "route_table_base.hh"
#include "route_table_filter.hh"
#include "route_table_ribin.hh"
#include "route_table_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"
#include "dummy_next_hop_resolver.hh"


bool
test_decision(TestInfo& /*info*/)
{
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_decision.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_decision";
    free(tmppath);
#endif

    EventLoop eventloop;
    BGPMain bgpmain(eventloop);
    LocalData localdata(bgpmain.eventloop());
    localdata.set_as(AsNum(1));

    comm_init();

    Iptuple iptuple1("eth0", "3.0.0.127", 179, "2.0.0.1", 179);
    BGPPeerData *peer_data1 =
	new BGPPeerData(localdata, iptuple1, AsNum(1), IPv4("2.0.0.1"), 30);
    peer_data1->compute_peer_type();
    // start off with both being IBGP
    peer_data1->set_id("2.0.0.0");
    BGPPeer peer1(&localdata, peer_data1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);

    Iptuple iptuple2("eth0", "3.0.0.127", 179, "2.0.0.2", 179);
    BGPPeerData *peer_data2 =
	new BGPPeerData(localdata, iptuple2, AsNum(1), IPv4("2.0.0.2"), 30);
    peer_data2->compute_peer_type();
    // start off with both being IBGP
    peer_data2->set_id("2.0.0.1");
    BGPPeer peer2(&localdata, peer_data2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);

    Iptuple iptuple3("eth0", "3.0.0.127", 179, "2.0.0.3", 179);
    BGPPeerData *peer_data3 =
	new BGPPeerData(localdata, iptuple2, AsNum(1), IPv4("2.0.0.3"), 30);
    peer_data3->compute_peer_type();
    // start off with both being IBGP
    peer_data3->set_id("2.0.0.3");
    BGPPeer peer3(&localdata, peer_data3, NULL, &bgpmain);
    PeerHandler handler3("test3", &peer3, NULL, NULL);

    DummyNextHopResolver<IPv4> next_hop_resolver(bgpmain.eventloop(), bgpmain);

    DecisionTable<IPv4> *decision_table
	= new DecisionTable<IPv4>("DECISION", SAFI_UNICAST, next_hop_resolver);

    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)decision_table);
    decision_table->set_next_table(debug_table);

    RibInTable<IPv4>* ribin_table1
	= new RibInTable<IPv4>("RIB-IN1", SAFI_UNICAST, &handler1);

    NhLookupTable<IPv4>* nhlookup_table1
	= new NhLookupTable<IPv4>("NHL-IN1", SAFI_UNICAST, &next_hop_resolver,
				  ribin_table1);
    ribin_table1->set_next_table(nhlookup_table1);
    nhlookup_table1->set_next_table(decision_table);
    decision_table->add_parent(nhlookup_table1, &handler1,
			       ribin_table1->genid());

    RibInTable<IPv4>* ribin_table2
	= new RibInTable<IPv4>("RIB-IN2", SAFI_UNICAST, &handler2);

    NhLookupTable<IPv4>* nhlookup_table2
	= new NhLookupTable<IPv4>("NHL-IN2", SAFI_UNICAST, &next_hop_resolver,
				  ribin_table2);
    ribin_table2->set_next_table(nhlookup_table2);
    nhlookup_table2->set_next_table(decision_table);
    decision_table->add_parent(nhlookup_table2, &handler2,
			       ribin_table2->genid());

    RibInTable<IPv4>* ribin_table3
	= new RibInTable<IPv4>("RIB-IN3", SAFI_UNICAST, &handler3);

    NhLookupTable<IPv4>* nhlookup_table3
	= new NhLookupTable<IPv4>("NHL-IN3", SAFI_UNICAST, &next_hop_resolver,
				  ribin_table3);
    ribin_table3->set_next_table(nhlookup_table3);
    nhlookup_table3->set_next_table(decision_table);
    decision_table->add_parent(nhlookup_table3, &handler3,
			       ribin_table3->genid());

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

    ASPath aspath1;
    aspath1.prepend_as(AsNum(1));
    aspath1.prepend_as(AsNum(2));
    aspath1.prepend_as(AsNum(3));
    ASPathAttribute aspathatt1(aspath1);

    ASPath aspath2;
    aspath2.prepend_as(AsNum(4));
    aspath2.prepend_as(AsNum(5));
    aspath2.prepend_as(AsNum(6));
    aspath2.prepend_as(AsNum(6));
    ASPathAttribute aspathatt2(aspath2);

    ASPath aspath3;
    aspath3.prepend_as(AsNum(7));
    aspath3.prepend_as(AsNum(8));
    aspath3.prepend_as(AsNum(9));
    ASPathAttribute aspathatt3(aspath3);

    LocalPrefAttribute lpa1(100);
    LocalPrefAttribute lpa2(200);
    LocalPrefAttribute lpa3(300);
    FPAList4Ref fpalist1 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist1->add_path_attribute(lpa1);
    PAListRef<IPv4> palist1 = new PathAttributeList<IPv4>(fpalist1);

    FPAList4Ref fpalist2 =
	new FastPathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);
    fpalist2->add_path_attribute(lpa1);
    PAListRef<IPv4> palist2 = new PathAttributeList<IPv4>(fpalist2);

    FPAList4Ref fpalist3 =
	new FastPathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);
    fpalist3->add_path_attribute(lpa1);
    PAListRef<IPv4> palist3 = new PathAttributeList<IPv4>(fpalist3);

    FPAList4Ref fpalist4;
    FPAList4Ref fpalist5;
    FPAList4Ref fpalist6;
    PAListRef<IPv4> palist4;
    PAListRef<IPv4> palist5;
    PAListRef<IPv4> palist6;

    // create a subnet route
    SubnetRoute<IPv4> *sr1;
    InternalMessage<IPv4>* msg;
    PolicyTags pt;
    UNUSED(net2);
    // ================================================================
    // Test1: trivial add and delete
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    ribin_table1->push(NULL);
    debug_table->write_separator();

    // delete the route
    ribin_table1->delete_route(net1);
    ribin_table1->push(NULL);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING PUSH FROM PEER 1");
    ribin_table1->push(NULL);
    debug_table->write_separator();

    // delete the route
    debug_table->write_comment("SENDING DELETE FROM PEER 1");
    ribin_table1->delete_route(net1);
    ribin_table1->push(NULL);

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
    fpalist1 = new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, 
					       igp_origin_att);
    fpalist1->add_path_attribute(lpa1);

    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table2->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table2->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    // ================================================================
    // Test3: decision by AS path length
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("TEST OF DIFFERENT AS PATH LENGTHS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    // ================================================================
    // Test3b: decision by AS path length
    // ================================================================
    // add a route
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 3B");
    debug_table->write_comment("TEST OF DIFFERENT AS PATH LENGTHS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    // ================================================================
    // Test4: decision by LocalPref
    // ================================================================
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa2);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 4");
    debug_table->write_comment("TEST OF LOCALPREF");
    debug_table->write_comment("HIGHEST LOCALPREF WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    // ================================================================
    // Test4b: decision by LocalPref
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 4B");
    debug_table->write_comment("TEST OF LOCALPREF");
    debug_table->write_comment("HIGHEST LOCALPREF WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);
    palist4.release();

    debug_table->write_separator();
    // ================================================================
    // Test4c: Test for incorrectly allowing routes with second-highest
    // local pref to pass through to ASPATH length selection
    // ================================================================

    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt2, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist5->add_path_attribute(lpa1);
    palist5 = new PathAttributeList<IPv4>(fpalist5);

    fpalist6 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt2, igp_origin_att);
    fpalist6->add_path_attribute(lpa2);
    palist6 = new PathAttributeList<IPv4>(fpalist6);

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 4C");
    debug_table->write_comment("TEST OF LOCALPREF");
    debug_table->write_comment("HIGHEST LOCALPREF WINS");

    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    ribin_table3->add_route(net1, fpalist6, pt);


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    ribin_table3->delete_route(net1);

    palist4.release(); 
    palist5.release(); 
    palist6.release(); 

    debug_table->write_separator();
    // ================================================================
    // Test5: decision by MED
    // ================================================================
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    MEDAttribute med1(100);
    fpalist4->add_path_attribute(med1);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist5->add_path_attribute(lpa1);
    MEDAttribute med2(200);
    fpalist5->add_path_attribute(med2);


    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("LOWEST MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    // ================================================================
    // Test5B: decision by MED
    // ================================================================
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5B");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("LOWEST MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();

    // ================================================================
    // Test5C: decision by MED
    // ================================================================
    fpalist5->remove_attribute_by_type(MED);
    palist5 = new PathAttributeList<IPv4>(fpalist5);

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5C");
    debug_table->write_comment("TEST OF MED - ONLY ONE HAS MED");
    debug_table->write_comment("ROUTE WITHOUT MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();

    // ================================================================
    // Test5D: decision by MED
    // ================================================================
    fpalist5->remove_attribute_by_type(MED);
    palist5 = new PathAttributeList<IPv4>(fpalist5);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5D");
    debug_table->write_comment("TEST OF MED - ONLY ONE HAS MED");
    debug_table->write_comment("ROUTE WITHOUT MED WINS");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);
    palist4.release(); 
    palist5.release(); 

    debug_table->write_separator();

    // ================================================================
    // Test5E: decision by MED
    // ================================================================

    // AS paths are the same length, but different paths.
    // MEDs are different (peer 1 has better MED), but shouldn't be compared.
    // peer 2 should win on BGP ID test.
    peer_data1->set_id("101.0.0.0");
    peer_data2->set_id("100.0.0.0");
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    fpalist4->add_path_attribute(med1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt3, igp_origin_att);
    fpalist5->add_path_attribute(lpa1);
    fpalist5->add_path_attribute(med2);
    palist5 = new PathAttributeList<IPv4>(fpalist5);


    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5E");
    debug_table->write_comment("TEST OF MED - DIFFERENT MEDS FROM DIFFERENCE AS'S");
    debug_table->write_comment("");
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;
    palist4.release(); 
    palist5.release(); 
    debug_table->write_separator();

    // ================================================================
    // Test5F: decision by MED
    // ================================================================

    peer_data1->set_id("101.0.0.0");
    peer_data2->set_id("100.0.0.0");
    peer_data3->set_id("102.0.0.0");

    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    fpalist4->add_path_attribute(med1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt3, igp_origin_att);
    fpalist5->add_path_attribute(lpa1);
    fpalist5->add_path_attribute(med2);
    palist5 = new PathAttributeList<IPv4>(fpalist5);

    fpalist6 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt3, igp_origin_att);
    fpalist6->add_path_attribute(lpa1);
    MEDAttribute med50(50);
    fpalist6->add_path_attribute(med50);
    palist6 = new PathAttributeList<IPv4>(fpalist6);


    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5F1");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("");
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("NEW ROUTE WINS ON BGP ID");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    debug_table->write_comment("ROUTE 1 RE-INSTATED");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(net1, fpalist6, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    debug_table->write_comment("ROUTE 2 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("ROUTE 1 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5F2");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(net1, fpalist6, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("WINS ON BGP ID");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    debug_table->write_comment("ROUTE 3 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(net1);
    sr1->unref();
    delete msg;
    debug_table->write_separator();



    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5F3");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    debug_table->write_comment("ROUTE 1 NOW WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(net1, fpalist6, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    debug_table->write_comment("ROUTE 2 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5F4");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(net1, fpalist6, pt);
    sr1->unref();
    delete msg;
 
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;


    debug_table->write_separator();
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5F5");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("");

    debug_table->write_comment("SENDING FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    debug_table->write_comment("ROUTE 3 NOW WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(net1, fpalist6, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("ROUTE 1 NOW WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    debug_table->write_comment("ROUTE 3 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    debug_table->write_comment("ROUTE 2 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 5F6");
    debug_table->write_comment("TEST OF MED");
    debug_table->write_comment("");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->add_route(net1, fpalist6, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("ROUTE 1 WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    debug_table->write_comment("ROUTE 3 TAKES OVER");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist6, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler3, 0);
    msg->set_push();
    ribin_table3->delete_route(net1);
    sr1->unref();
    delete msg;

    palist4.release(); 
    palist5.release(); 
    palist6.release(); 
    debug_table->write_separator();

    // ================================================================
    // Test6: decision by Origin
    // ================================================================
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, egp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, 
					incomplete_origin_att);
    fpalist5->add_path_attribute(lpa1);
    palist5 = new PathAttributeList<IPv4>(fpalist5);


    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 6");
    debug_table->write_comment("TEST OF ORIGIN");
    debug_table->write_comment("IGP beats EGP beats INCOMPLETE");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: WINS BY DEFAULT");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist1, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: LOSES");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    debug_table->write_comment("EXPECT: ALLOWS EGP ROUTE TO WIN");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: LOSES");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("EXPECT: ALLOWS INCOMPLETE ROUTE TO WIN");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;
    palist4.release(); 
    palist5.release(); 

    debug_table->write_separator();

    // ================================================================
    // Test6B: decision by Origin
    // ================================================================
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, egp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, 
					incomplete_origin_att);
    fpalist5->add_path_attribute(lpa1);
    palist5 = new PathAttributeList<IPv4>(fpalist5);


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
    ribin_table2->add_route(net1, fpalist5, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->add_route(net1, fpalist4, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    debug_table->write_comment("EXPECT: NO CHANGE");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: WINS");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->add_route(net1, fpalist1, pt);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler2, 0);
    msg->set_push();
    ribin_table2->delete_route(net1);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    sr1 = new SubnetRoute<IPv4>(net1, palist5, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    ribin_table1->delete_route(net1);
    sr1->unref();
    delete msg;
    palist4.release();
    palist5.release(); 

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist3, pt);


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();

    // ================================================================
    // Test9A: test of replace_route
    // ================================================================
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);
    fpalist5->add_path_attribute(lpa2);
    palist5 = new PathAttributeList<IPv4>(fpalist5);

    fpalist6 =
	new FastPathAttributeList<IPv4>(nhatt3, aspathatt1, igp_origin_att);
    fpalist6->add_path_attribute(lpa3);
    palist6 = new PathAttributeList<IPv4>(fpalist6);

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 9A");
    debug_table->write_comment("TEST OF REPLACE_ROUTE");
    debug_table->write_comment("PEER 1 WINS, PEER 2 LOSES, PEER2 WINS");
    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop3.str() +
				" BECOMES UNRESOLVABLE").c_str());
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    decision_table->igp_nexthop_changed(nexthop3);
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop2.str() +
				" BECOMES UNRESOLVABLE").c_str());
    next_hop_resolver.unset_nexthop_metric(nexthop2);
    decision_table->igp_nexthop_changed(nexthop2);
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop3.str() +
				" BECOMES RESOLVABLE").c_str());
    next_hop_resolver.set_nexthop_metric(nexthop3, 27);
    decision_table->igp_nexthop_changed(nexthop3);
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    palist4.release(); 
    palist5.release(); 
    palist6.release(); 

    // ================================================================
    // Test 11A: test of routes becoming resolvable
    // ================================================================
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);
    fpalist5->add_path_attribute(lpa2);
    palist5 = new PathAttributeList<IPv4>(fpalist5);

    fpalist6 =
	new FastPathAttributeList<IPv4>(nhatt3, aspathatt1, igp_origin_att);
    fpalist6->add_path_attribute(lpa3);
    palist6 = new PathAttributeList<IPv4>(fpalist6);

    next_hop_resolver.unset_nexthop_metric(nexthop2);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 11A");
    debug_table->write_comment("TEST OF ROUTES BECOMING RESOLVABLE");
    debug_table->write_comment("PEER 1, 2 NOT RESOLVABLE, PEER1 BECOMES RESOLVABLE");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop2.str() +
				" BECOMES RESOLVABLE").c_str());
    next_hop_resolver.set_nexthop_metric(nexthop2, 27);
    decision_table->igp_nexthop_changed(nexthop2);
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();

    palist4.release(); 
    palist5.release(); 
    palist6.release(); 

    // ================================================================
    // Test 11B: test of routes becoming resolvable
    // ================================================================
    fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    palist4 = new PathAttributeList<IPv4>(fpalist4);

    fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist5->add_path_attribute(lpa2);
    palist5 = new PathAttributeList<IPv4>(fpalist5);

    fpalist6 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist6->add_path_attribute(lpa3);
    palist6 = new PathAttributeList<IPv4>(fpalist6);

    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop2);

    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 11B");
    debug_table->write_comment("TEST OF ROUTES BECOMING RESOLVABLE");
    debug_table->write_comment("PEER 1, 2 NOT RESOLVABLE, BOTH BECOME RESOLVABLE SIMULTANEOUSLY");

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop1.str() +
				" BECOMES RESOLVABLE").c_str());
    next_hop_resolver.set_nexthop_metric(nexthop1, 27);
    decision_table->igp_nexthop_changed(nexthop1);
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment(("NEXTHOP " + nexthop1.str() +
				" BECOMES UNRESOLVABLE").c_str());
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    decision_table->igp_nexthop_changed(nexthop1);
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);


    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

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
    ribin_table1->add_route(net1, fpalist4, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net1, fpalist5, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 3");
    ribin_table3->add_route(net1, fpalist6, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 3");
    ribin_table3->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();

    // ================================================================
    // Test 13: decision of IBGP vs EBGP
    // ================================================================
    next_hop_resolver.unset_nexthop_metric(nexthop1);
    next_hop_resolver.unset_nexthop_metric(nexthop3);
    next_hop_resolver.set_nexthop_metric(nexthop1, 200);
    next_hop_resolver.set_nexthop_metric(nexthop3, 100);
    peer_data2->set_as(AsNum(9));
    peer_data2->compute_peer_type();
    debug_table->write_comment("******************************************");
    debug_table->write_comment("TEST 13");
    debug_table->write_comment("TEST OF IBGP vs EBGP");
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("PEER 1 HAS BETTER IGP DISTANCE");
    debug_table->write_comment("PEER 2 IS EBGP");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 2");
    debug_table->write_comment("EXPECT: WINS");
    ribin_table2->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

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
    ribin_table2->add_route(net1, fpalist3, pt);

    debug_table->write_separator();
    debug_table->write_comment("SENDING FROM PEER 1");
    debug_table->write_comment("EXPECT: LOSES");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 2");
    ribin_table2->delete_route(net1);

    debug_table->write_separator();
    debug_table->write_comment("DELETION FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table->write_separator();

    // ================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete decision_table;
    delete debug_table;
    delete ribin_table1;
    delete ribin_table2;
    delete ribin_table3;
    delete nhlookup_table1;
    delete nhlookup_table2;
    delete nhlookup_table3;
    palist1.release(); 
    palist2.release(); 
    palist3.release(); 
    palist4.release(); 
    palist5.release(); 
    palist6.release(); 


    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", filename.c_str());
	fprintf(stderr, "TEST DECISION FAILED\n");
	fclose(file);
	return false;
   }
#define BUFSIZE 80000
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST DECISION FAILED\n");
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
    ref_filename += "/test_decision.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST DECISION FAILED\n");
	fclose(file);
	return false;
    }

    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST DECISION FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);

    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1) != 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST DECISION FAILED\n");
	return false;

    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}



