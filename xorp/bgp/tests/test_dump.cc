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

#include "bgp.hh"
#include "route_table_base.hh"
#include "route_table_filter.hh"
#include "route_table_ribin.hh"
#include "route_table_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"
#include "dummy_next_hop_resolver.hh"


bool
test_dump_create(TestInfo& /*info*/)
{
    const list <const PeerTableInfo<IPv4>*> l;
    DumpTable<IPv4> *dt = new
	DumpTable<IPv4>("dumptable", 0, l, 0, SAFI_UNICAST);
    dt->set_next_table(dt);
    dt->suspend_dump();

    return true;
}

bool
test_dump(TestInfo& /*info*/)
{
    //stuff needed to create an eventloop
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_dump.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_dump";
    free(tmppath);
#endif

    EventLoop eventloop;
    BGPMain bgpmain(eventloop);
    LocalData localdata(bgpmain.eventloop());

    Iptuple iptuple1("eth0", "3.0.0.127", 179, "2.0.0.1", 179);
    BGPPeerData *peer_data1
	= new BGPPeerData(localdata, iptuple1, AsNum(1), IPv4("2.0.0.1"), 30);
    //start off with both being IBGP
    peer_data1->set_id("2.0.0.1");
    BGPPeer peer1(&localdata, peer_data1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);

    Iptuple iptuple2("eth0", "3.0.0.127", 179, "2.0.0.2", 179);
    BGPPeerData *peer_data2
	= new BGPPeerData(localdata, iptuple2, AsNum(1), IPv4("2.0.0.2"), 30);
    //start off with both being IBGP
    peer_data2->set_id("2.0.0.2");
    BGPPeer peer2(&localdata, peer_data2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);

    Iptuple iptuple3("eth0", "3.0.0.127", 179, "2.0.0.3", 179);
    BGPPeerData *peer_data3
	= new BGPPeerData(localdata, iptuple3, AsNum(1), IPv4("2.0.0.3"), 30);
    //start off with both being IBGP
    peer_data3->set_id("2.0.0.3");
    BGPPeer peer3(&localdata, peer_data3, NULL, &bgpmain);
    PeerHandler handler3("test3", &peer3, NULL, NULL);

    DummyNextHopResolver<IPv4> next_hop_resolver(bgpmain.eventloop(), bgpmain);

    DecisionTable<IPv4> *decision_table
	= new DecisionTable<IPv4>("DECISION", SAFI_UNICAST, next_hop_resolver);

    RibInTable<IPv4>* ribin_table1
	= new RibInTable<IPv4>("RIB-IN1", SAFI_UNICAST, &handler1);

    //In principle the cache shouldn't be needed, but ribin can't be
    //directly followed by decision because the dump_table calls
    //set_parent on the next table, and decision table doesn't support
    //set_parent because it is the only table with multiple parents.
    CacheTable<IPv4>* cache_table1
	= new CacheTable<IPv4>("Cache1", SAFI_UNICAST, ribin_table1,
			       &handler1);
    
    NhLookupTable<IPv4>* nhlookup_table1
        = new NhLookupTable<IPv4>("NHLOOKUP", SAFI_UNICAST, &next_hop_resolver,
                                  cache_table1);

    ribin_table1->set_next_table(cache_table1);
    cache_table1->set_next_table(nhlookup_table1);
    nhlookup_table1->set_next_table(decision_table);

    RibInTable<IPv4>* ribin_table2
	= new RibInTable<IPv4>("RIB-IN2", SAFI_UNICAST, &handler2);

    CacheTable<IPv4>* cache_table2
	= new CacheTable<IPv4>("Cache2", SAFI_UNICAST, ribin_table2,
			       &handler2);

    NhLookupTable<IPv4>* nhlookup_table2
        = new NhLookupTable<IPv4>("NHLOOKUP", SAFI_UNICAST, &next_hop_resolver,
                                  cache_table2);

    ribin_table2->set_next_table(cache_table2);
    cache_table2->set_next_table(nhlookup_table2);
    nhlookup_table2->set_next_table(decision_table);

    RibInTable<IPv4>* ribin_table3
	= new RibInTable<IPv4>("RIB-IN3", SAFI_UNICAST, &handler3);

    CacheTable<IPv4>* cache_table3
	= new CacheTable<IPv4>("Cache3", SAFI_UNICAST, ribin_table3,
			       &handler3);

    NhLookupTable<IPv4>* nhlookup_table3
        = new NhLookupTable<IPv4>("NHLOOKUP", SAFI_UNICAST, &next_hop_resolver,
                                  cache_table3);

    ribin_table3->set_next_table(cache_table3);
    cache_table3->set_next_table(nhlookup_table3);
    nhlookup_table3->set_next_table(decision_table);

    decision_table->add_parent(nhlookup_table1, 
			       &handler1, ribin_table1->genid());
    decision_table->add_parent(nhlookup_table2, 
			       &handler2, ribin_table2->genid());
    decision_table->add_parent(nhlookup_table3, 
			       &handler3, ribin_table3->genid());

    FanoutTable<IPv4> *fanout_table
	= new FanoutTable<IPv4>("FANOUT", SAFI_UNICAST, decision_table, NULL, NULL);
    decision_table->set_next_table(fanout_table);

    DebugTable<IPv4>* debug_table1
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());

    DebugTable<IPv4>* debug_table2
	 = new DebugTable<IPv4>("D2", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table2, &handler2, 
				 ribin_table2->genid());

    DebugTable<IPv4>* debug_table3
	 = new DebugTable<IPv4>("D3", (BGPRouteTable<IPv4>*)fanout_table);
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());

    debug_table1->set_output_file(filename);
    debug_table1->set_canned_response(ADD_USED);
    debug_table1->enable_tablename_printing();
    debug_table1->set_get_on_wakeup(true);

    debug_table2->set_output_file(debug_table1->output_file());
    debug_table2->set_canned_response(ADD_USED);
    debug_table2->enable_tablename_printing();
    debug_table2->set_get_on_wakeup(true);

    debug_table3->set_output_file(debug_table1->output_file());
    debug_table3->set_canned_response(ADD_USED);
    debug_table3->enable_tablename_printing();
    debug_table3->set_get_on_wakeup(true);

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

    FPAList4Ref fpalist1 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    fpalist1->add_path_attribute(lpa1);
    PAListRef<IPv4> palist1 = new PathAttributeList<IPv4>(fpalist1);
    palist1.create_attribute_manager();

    FPAList4Ref fpalist2 =
	new FastPathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);
    fpalist2->add_path_attribute(lpa1);
    PAListRef<IPv4> palist2 = new PathAttributeList<IPv4>(fpalist2);

    FPAList4Ref fpalist3 =
	new FastPathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);
    fpalist3->add_path_attribute(lpa1);
    PAListRef<IPv4> palist3 = new PathAttributeList<IPv4>(fpalist3);

    FPAList4Ref fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt4, aspathatt3, igp_origin_att);
    fpalist4->add_path_attribute(lpa1);
    PAListRef<IPv4> palist4 = new PathAttributeList<IPv4>(fpalist4);

    FPAList4Ref fpalist5 =
	new FastPathAttributeList<IPv4>(nhatt5, aspathatt3, igp_origin_att);
    fpalist5->add_path_attribute(lpa1);
    PAListRef<IPv4> palist5 = new PathAttributeList<IPv4>(fpalist5);

    //create a subnet route
    PolicyTags pt;

    //================================================================
    //Test1: trivial add and delete to test plumbing
    //================================================================
    //add a route
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 1");
    debug_table1->write_comment("ADD AND DELETE");
    debug_table1->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();

    //delete the route
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    //================================================================
    //Test2: simple dump
    //================================================================    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 2");
    debug_table1->write_comment("SIMPLE DUMP TO PEER 3");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net1, fpalist1, pt);
    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net2, fpalist2, pt);
    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }
    while (debug_table3->parent()->get_next_message(debug_table3)) {}
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    ribin_table2->delete_route(net2);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net4, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }
    while (debug_table3->parent()->get_next_message(debug_table3)) {}
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    ribin_table1->delete_route(net3);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    ribin_table2->delete_route(net4);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    ribin_table2->add_route(net4, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    debug_table1->write_comment("XXXX");
    ribin_table2->ribin_peering_went_down();
    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    ribin_table1->delete_route(net3);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    ribin_table2->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table2, &handler2, 
				 ribin_table2->genid());
    debug_table2->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table2, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table2->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table2, &handler2, 
				 ribin_table2->genid());
    debug_table2->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table2, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO IMMEDIATE CHANGE");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 1 GOES DOWN");
    debug_table1->write_comment("EXPECT NO IMMEDIATE CHANGE");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table1, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table2->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table2, &handler2, 
				 ribin_table2->genid());
    debug_table2->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table2, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("PEER 2 GOES DOWN");
    fanout_table->remove_next_table(debug_table2);
    ribin_table2->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table2->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table2, &handler2, 
				 ribin_table2->genid());
    debug_table2->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table2, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

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
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 2 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table2->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table2, &handler2, 
				 ribin_table2->genid());
    debug_table2->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table2, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table1, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net4, fpalist4, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 3 WHEN EVENTLOOP RUNS");
    ribin_table2->add_route(net0, fpalist5, pt);


    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net5, fpalist5, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.4.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.5.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    ribin_table1->delete_route(net1);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 3");
    ribin_table1->delete_route(net2);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net3);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net4);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.0.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.0.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net0);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.5.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.5.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net5);

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
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net4, fpalist4, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 2");
    ribin_table1->add_route(net1, fpalist5, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 1");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 3 WHEN EVENTLOOP RUNS");
    ribin_table2->add_route(net3, fpalist5, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT REPLACE AT PEER 1");
    ribin_table2->add_route(net4, fpalist5, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT REPLACE 1.0.3.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.4.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 3");
    ribin_table1->delete_route(net1);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net3);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net4);

    debug_table1->write_separator();


    //================================================================
    // Test12: peer 1 goes down just before peer 3 comes up, peer 1
    // deletion table only runs after peer 3 comes up.
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 12");
    debug_table1->write_comment("PEER 1 GOES DOWN JUST BEFORE P3 COMES UP");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("TAKE PEER 1 DOWN");
    int genid = ribin_table1->genid();
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");
    ((DumpTable<IPv4>*)debug_table3->parent())
	->peering_is_down(&handler1, genid);

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 3");
    ribin_table2->delete_route(net3);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table1, SAFI_UNICAST, "ribname");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();

    //================================================================
    // Test13: adds coming through before we dump the peer
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 13");
    debug_table1->write_comment("NEW ADD ARRIVES BEFORE PEER IS DUMPED");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1 BUT NOT PEER 3");
    ribin_table2->add_route(net3, fpalist3, pt);
    
    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table2->delete_route(net3);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net1);

    debug_table1->write_separator();
    debug_table1->write_separator();

    //================================================================
    // Test14: old delete coming through after we dump the peer
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 14");
    debug_table1->write_comment("OLD DEL AFTER PEER HAS BEEN DUMPED");
    debug_table1->write_comment("TEST OLD GENID DELETE ON CURRENT DUMP PEER");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("TAKE PEER 1 DOWN");
    debug_table1->write_comment("DONT RUN EVENTLOOP, SO DELETION IS DELAYED");
    debug_table1->write_comment("EXPECT NO CHANGE");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();
    uint32_t del_genid = ribin_table1->genid();

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP AGAIN, BUT SKIP DUMPING TO IT");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");
    ((DumpTable<IPv4>*)(debug_table3->parent()))->peering_is_down(&handler1, del_genid);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT NOT RECEIVED BY PEER 2 - PEER NOT YET DUMPED");
    ribin_table1->add_route(net4, fpalist4, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0 **NOT** RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.4.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }
    debug_table1->write_separator();
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net4);

    debug_table1->write_separator();

    //================================================================
    // Test15: new route during dump, after peer has been dumped
    //================================================================
    //take peer3 down
    //    printf("TEST 15\n");
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 15");
    debug_table1->write_comment("ADD AFTER PEER HAS BEEN DUMPED");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT (MOVES TO DUMPING NEXT PEER)");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT NOT RECEIVED BY PEER 3 YET");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net1);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net3);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    //    printf("TEST 15 END\n");

    //================================================================
    // Test16: new route during dump, after peer with no routes has been dumped
    //================================================================
    //take peer3 down
    //    printf("TEST 16\n");
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();
    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 16");
    debug_table1->write_comment("ADD AFTER PEER WITH NO ROUTES HAS BEEN DUMPED");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);


    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("LET EVENT QUEUE DRAIN");
    debug_table1->write_comment("NO ROUTES IN 1st PEER (MOVES TO DUMPING NEXT PEER)");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT NOT RECEIVED BY PEER 3 YET");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT ADD 1.0.3.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net3);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    //    printf("TEST 16 END\n");

    //================================================================
    // Test17: new route during dump on a new peer
    //================================================================
    //    printf("TEST 17\n");
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();

    //take peer1 down
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();

    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 17");
    debug_table1->write_comment("ADD ON NEW PEER DURING DUMP");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY NOONE");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("NO NEED TO DUMP TO THIS PEER FOR THIS TEST");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT NOT RECEIVED BY PEER 3 YET");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net1);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    //    printf("TEST 17 END\n");

    //================================================================
    // Test18: new route during dump on a new peer
    //================================================================
    //    printf("TEST 18\n");
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();


    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 18");
    debug_table1->write_comment("ADD ON NEW PEER DURING DUMP");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT NOT RECEIVED BY PEER 3 YET");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY NOONE");
    ribin_table2->add_route(net2, fpalist2, pt);


    debug_table1->write_separator();
    //take peer1 down
    debug_table1->write_comment("TAKE PEER 1 DOWN");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();
    del_genid = ribin_table1->genid();

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");
    ((DumpTable<IPv4>*)(debug_table3->parent()))->peering_is_down(&handler1, del_genid);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("NO NEED TO DUMP TO THIS PEER FOR THIS TEST");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 NOT RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    //    printf("TEST 18 END\n");

    //================================================================
    // Test19: delete during dump on a peer dump table doesn't know about
    //================================================================
    //    printf("TEST 19\n");
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();


    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 19");
    debug_table1->write_comment("DELETE ON PEER THAT DUMP TABLE DOESNT KNOW ABOUT");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    //take peer1 down
    debug_table1->write_comment("TAKE PEER 1 DOWN");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();
    del_genid = ribin_table1->genid();

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("NO NEED TO DUMP TO THIS PEER FOR THIS TEST");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");
    ((DumpTable<IPv4>*)(debug_table3->parent()))->peering_is_down(&handler1, del_genid);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 NOT RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 NOT RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    //    printf("TEST 19 END\n");

    //================================================================
    // Test20: delete during dump on a peer dump table doesn't know about
    //================================================================
    //    printf("TEST 20\n");
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();


    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 20");
    debug_table1->write_comment("DELETE ON PEER THAT DUMP TABLE DOESNT KNOW ABOUT");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);


    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net4, fpalist4, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    //take peer1 down
    debug_table1->write_comment("TAKE PEER 1 DOWN");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();
    del_genid = ribin_table1->genid();

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("NO NEED TO DUMP TO THIS PEER FOR THIS TEST");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net5, fpalist5, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");
    ((DumpTable<IPv4>*)(debug_table3->parent()))->peering_is_down(&handler1, del_genid);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 NOT RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.3.0/24 NOT RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.4.0/24 NOT RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.5.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.5.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net5);

    debug_table1->write_separator();
    //    printf("TEST 20 END\n");

    //================================================================
    // Test21: add on new rib, after dump went down halfway through 
    // dumping old rib
    //================================================================
    //    printf("TEST 21\n");
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();


    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 21");
    debug_table1->write_comment("ADD ON NEW VERSION OF HALF-DUMPED PEER");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net3, fpalist3, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 1");
    ribin_table2->add_route(net2, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");

    debug_table1->write_separator();
    debug_table1->write_comment("STEP EVENT LOOP ONCE TO DUMP ONE ROUTE");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    //take peer1 down
    debug_table1->write_comment("TAKE PEER 1 DOWN");
    debug_table1->write_comment("EXPECT NO CHANGE UNTIL EVENTLOOP RUNS");
    fanout_table->remove_next_table(debug_table1);
    ribin_table1->ribin_peering_went_down();

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 1 UP");
    debug_table1->write_comment("NO NEED TO DUMP TO THIS PEER FOR THIS TEST");
    ribin_table1->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table1, &handler1, 
				 ribin_table1->genid());
    debug_table1->set_parent(fanout_table);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT ADD 1.0.5.0/24 RECEIVED BY PEER 2");
    ribin_table1->add_route(net5, fpalist5, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("RUN EVENT LOOP TO COMPLETION");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2 & 3");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT ADD 1.0.2.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.5.0/24 RECEIVED BY PEER 3");

    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.2.0/24 RECEIVED BY PEER 1 & 3");
    ribin_table2->delete_route(net2);

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.5.0/24 RECEIVED BY PEER 2 & 3");
    ribin_table1->delete_route(net5);

    debug_table1->write_separator();
    //    printf("TEST 21 END\n");
    //================================================================
    // Test22: two copies of same route - only one should be dumped
    // dumping old rib
    //================================================================
    //take peer3 down
    fanout_table->remove_next_table(debug_table3);
    ribin_table3->ribin_peering_went_down();

    debug_table1->write_comment("******************************************");
    debug_table1->write_comment("TEST 22");
    debug_table1->write_comment("TWO COPIES OF SAME ROUTE");

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 1");
    debug_table1->write_comment("EXPECT RECEIVED BY PEER 2");
    ribin_table1->add_route(net1, fpalist1, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("SENDING FROM PEER 2");
    debug_table1->write_comment("EXPECT NO CHANGE");
    ribin_table2->add_route(net1, fpalist2, pt);

    debug_table1->write_separator();
    debug_table1->write_comment("BRING PEER 3 UP");
    ribin_table3->ribin_peering_came_up();
    fanout_table->add_next_table(debug_table3, &handler3, 
				 ribin_table3->genid());
    debug_table3->set_parent(fanout_table);
    fanout_table->dump_entire_table(debug_table3, SAFI_UNICAST, "ribname");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("**ONLY ONE COPY**");
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 1");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY PEER 2");
    debug_table1->write_comment("EXPECT REP 1.0.1.0/24 RECEIVED BY PEER 3");
    debug_table1->write_comment("EXPECT ADD 1.0.1.0/24 RECEIVED BY PEER 1");
    ribin_table1->delete_route(net1);
    ribin_table1->push(NULL);

    //delete the routes
    debug_table1->write_separator();
    debug_table1->write_comment("DELETING FROM PEER 2");
    debug_table1->write_comment("EXPECT DEL 1.0.1.0/24 RECEIVED BY P1 & 3");
    ribin_table2->delete_route(net1);

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
    delete nhlookup_table1;
    delete nhlookup_table2;
    delete nhlookup_table3;
    palist1.release();
    palist2.release();
    palist3.release();
    palist4.release();
    palist5.release();
    fpalist1 = 0;
    fpalist2 = 0;
    fpalist3 = 0;
    fpalist4 = 0;
    fpalist5 = 0;


    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", filename.c_str());
	fprintf(stderr, "TEST DUMP FAILED\n");
	fclose(file);
	return false;
   }
#define BUFSIZE 100000
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST DUMP FAILED\n");
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
    ref_filename += "/test_dump.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST DUMP FAILED\n");
	fclose(file);
	return false;
    }
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST DUMP FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);
    
    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1)!= 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST DUMP FAILED\n");
	return false;
	
    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}


