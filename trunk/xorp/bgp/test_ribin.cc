// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/test_ribin.cc,v 1.23 2004/05/13 20:31:46 mjh Exp $"

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
validate_reference_file(string reference_file, string output_file,
			string testname);

bool
test_ribin_dump(TestInfo& /*info*/)
{
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_ribin_dump.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    LocalData localdata;
    BGPPeer peer1(&localdata, NULL, NULL, &bgpmain);
    BGPPeer peer2(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);
    PeerHandler handler2("test1", &peer2, NULL, NULL);

    RibInTable<IPv4> *ribin 
	= new RibInTable<IPv4>("RIB-IN", SAFI_UNICAST, &handler1);
    DebugTable<IPv4>* debug_table
	= new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)ribin);
    ribin->set_next_table(debug_table);
    debug_table->set_output_file(filename);

    IPNet<IPv4> net1("10.0.0.0/8");
    IPNet<IPv4> net2("30.0.0.0/8");
    IPNet<IPv4> net3("20.0.0.0/8");

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);
    OriginAttribute igp_origin_att(IGP);
    AsPath aspath1("3,2,1");
    ASPathAttribute aspathatt1(aspath1);
    PathAttributeList<IPv4>* palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    SubnetRoute<IPv4> *sr1, *sr2, *sr3;

    debug_table->write_comment("TEST 1");

    InternalMessage<IPv4>* msg;

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    //routes will only be dumped if they won
    sr1->set_is_winner(1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);
    delete msg;
    sr1->unref();

    sr2 = new SubnetRoute<IPv4>(net2, palist1, NULL);
    sr2->set_is_winner(1);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    ribin->add_route(*msg, NULL);
    delete msg;
    sr2->unref();

    list <const PeerTableInfo<IPv4>*> peers_to_dump;
    PeerTableInfo<IPv4>* pti = new PeerTableInfo<IPv4>(NULL, &handler1, 
						       ribin->genid());
    peers_to_dump.push_back(pti);
    DumpIterator<IPv4>* dump_iter 
	= new DumpIterator<IPv4>(&handler2, peers_to_dump);

    ribin->dump_next_route(*dump_iter);

    debug_table->write_comment("The dump has started now add a new route"
			       " it should appear once we start "
			       "dumping again");

    sr3 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    sr3->set_is_winner(1);
    msg = new InternalMessage<IPv4>(sr3, &handler1, 0);
    ribin->add_route(*msg, NULL);
    delete msg;
    sr3->unref();

    while (ribin->dump_next_route(*dump_iter)) {
	while (bgpmain.eventloop().timers_pending()) {
	    bgpmain.eventloop().run();
	}
    }

    delete dump_iter;

    debug_table->
	write_comment(c_format("USE A NEW ITERATOR to prove %s is present",
			       net3.str().c_str()).c_str());

    dump_iter = new DumpIterator<IPv4>(&handler2, peers_to_dump);
    ribin->dump_next_route(*dump_iter);
    ribin->dump_next_route(*dump_iter);
    ribin->dump_next_route(*dump_iter);

    // Notify the dump iterator that the peering went down. Otherwise
    // it will still have an iterator pointing at the RIB-IN trie.
    dump_iter->peering_went_down(&handler1, 1);

    // Delete all the allocated memory
    // Delete the routes from the ribin.
    debug_table->write_comment("Delete all the routes from the RIB-IN "
			       "so the memory can be free'd");

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_is_winner(1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    sr1->unref();

    sr2 = new SubnetRoute<IPv4>(net2, palist1, NULL);
    sr2->set_is_winner(1);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    sr2->unref();

    sr3 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    sr3->set_is_winner(1);
    msg = new InternalMessage<IPv4>(sr3, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    sr3->unref();

    delete palist1;

    delete ribin;
    delete debug_table;

    // Delete the dump iterator last to verify it no longer has any
    // references to the RIB-IN.
    delete dump_iter;
    delete pti;

    return validate_reference_file("/test_ribin_dump.reference", filename,
				   "RIBIN DUMP");
}

bool
test_ribin(TestInfo& /*info*/)
{
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_ribin.";
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

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);

    IPv4 nexthop2("2.0.0.2");
    NextHopAttribute<IPv4> nhatt2(nexthop2);

    OriginAttribute igp_origin_att(IGP);

    AsPath aspath1;
    aspath1.prepend_as(AsNum(1));
    aspath1.prepend_as(AsNum(2));
    aspath1.prepend_as(AsNum(3));
    AsPath foo("3,2,1");
    assert (foo == aspath1);
    ASPathAttribute aspathatt1(aspath1);

    AsPath aspath2;
    aspath2.prepend_as(AsNum(3));
    aspath2.prepend_as(AsNum(4));
    aspath2.prepend_as(AsNum(5));
    ASPathAttribute aspathatt2(aspath2);

    PathAttributeList<IPv4>* palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    PathAttributeList<IPv4>* palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);

    PathAttributeList<IPv4>* palist3, *palist4;

    //create a subnet route
    SubnetRoute<IPv4> *sr1, *sr2, *sr3, *sr4;
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);

    //================================================================
    //Test1: trivial add and delete
    //================================================================
    //add a route
    debug_table->write_comment("TEST 1");
    InternalMessage<IPv4>* msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->add_route(*msg, NULL);

    //check we only ended up with one copy of the PA list
    assert(sr1->number_of_managed_atts() == 1);

    //check there's one route in the RIB-IN
    assert(ribin->route_count() == 1);

    debug_table->write_separator();

    //delete the route
    ribin->delete_route(*msg, NULL);

    //check there's still one copy of the PA list
    assert(sr1->number_of_managed_atts() == 1);

    //check there are no routes in the RIB-IN
    assert(ribin->route_count() == 0);

    debug_table->write_separator();
    delete msg;

    //================================================================
    //Test2: trivial add , replace and delete
    //================================================================
    //add a route
    debug_table->write_comment("TEST 2");
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("ADD FIRST ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist1;

    //check there's still one route in the RIB-IN
    assert(ribin->route_count() == 1);

    //check we only ended up with one copy of the PA list
    assert(sr1->number_of_managed_atts() == 1);
    sr1->unref();

    debug_table->write_separator();

    //create a subnet route
    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);

    //temporarily, we should have two managed palists
    assert(sr2->number_of_managed_atts() == 2);

    //add the same route again - should be treated as an update, and
    //trigger a replace operation
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    debug_table->write_comment("ADD MODIFIED FIRST ROUTE");
    ribin->add_route(*msg, NULL);

    //check we only ended up with one copy of the PA list
    assert(sr2->number_of_managed_atts() == 1);

    //check there's still one route in the RIB-IN
    assert(ribin->route_count() == 1);

    debug_table->write_separator();

    //delete the route
    debug_table->write_comment("DELETE ROUTE");
    ribin->delete_route(*msg, NULL);

    //check there's still one copy of the PA list
    assert(sr2->number_of_managed_atts() == 1);

    //check there are no routes in the RIB-IN
    assert(ribin->route_count() == 0);

    debug_table->write_separator();

    delete msg;
    sr2->unref();
    delete palist2;
    
    //================================================================
    //Test3: trivial add two routes, then drop peering
    //================================================================
    //add a route
    debug_table->write_comment("TEST 3");
    palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);

    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("ADD FIRST ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist1;

    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    debug_table->write_comment("ADD SECOND ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist2;
    sr2->unref();

    //check there are two routes in the RIB-IN
    assert(ribin->route_count() == 2);

    //check we only ended up with two PA lists
    assert(sr1->number_of_managed_atts() == 2);
    sr1->unref();

    debug_table->write_separator();

    debug_table->write_comment("NOW DROP THE PEERING");

    ribin->ribin_peering_went_down();
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();

    //================================================================
    //Test4: trivial add two routes, then do a route dump
    //================================================================
    ribin->ribin_peering_came_up();
    //add a route
    debug_table->write_comment("TEST 4");
    palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    //routes will only be dumped if they won at decision
    sr1->set_is_winner(1);

    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    sr2->set_is_winner(1);

    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("ADD FIRST ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;

    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    debug_table->write_comment("ADD SECOND ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    sr2->unref();
    delete palist1;

    //check there are two routes in the RIB-IN
    assert(ribin->route_count() == 2);

    //check we only ended up with two PA lists
    assert(sr1->number_of_managed_atts() == 2);
    sr1->unref();
    delete palist2;

    debug_table->write_separator();

    debug_table->write_comment("NOW DO THE ROUTE DUMP");

    list <const PeerTableInfo<IPv4>*> peers_to_dump;
    PeerTableInfo<IPv4>* pti = new PeerTableInfo<IPv4>(NULL, &handler1, 
						       ribin->genid());
    peers_to_dump.push_back(pti);
    DumpIterator<IPv4>* dump_iter 
	= new DumpIterator<IPv4>(&handler2, peers_to_dump);
    while (ribin->dump_next_route(*dump_iter)) {
	while (bgpmain.eventloop().timers_pending()) {
	    bgpmain.eventloop().run();
	}
    }
    delete dump_iter;
    delete pti;

    debug_table->write_separator();

    debug_table->write_comment("DELETE THE ROUTES AND CLEAN UP");

    palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    sr1->unref();
    delete palist1;

    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    sr2->unref();
    delete palist2;


    debug_table->write_separator();
    //================================================================
    //Test5: IGP nexthop changes
    //================================================================

    debug_table->write_comment("TEST 5");
    debug_table->write_comment("IGP NEXTHOP CHANGES");
    palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);

    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("ADD FIRST ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist1;
    sr1->unref();

    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    debug_table->write_comment("ADD SECOND ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist2;
    sr2->unref();

    //check there are two routes in the RIB-IN
    assert(ribin->route_count() == 2);

    debug_table->write_separator();
    debug_table->write_comment("NEXTHOP 2.0.0.2 CHANGES");
    //this should trigger a replace
    ribin->igp_nexthop_changed(nexthop2);
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("NEXTHOP 2.0.0.1 CHANGES");
    //this should trigger a replace
    ribin->igp_nexthop_changed(nexthop1);
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    IPv4 nexthop3("1.0.0.1");
    debug_table->write_separator();
    debug_table->write_comment("NEXTHOP 1.0.0.1 CHANGES");
    //this should have no effect
    ribin->igp_nexthop_changed(nexthop3);
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    IPv4 nexthop4("3.0.0.1");
    debug_table->write_separator();
    //this should have no effect
    debug_table->write_comment("NEXTHOP 3.0.0.1 CHANGES");
    ribin->igp_nexthop_changed(nexthop4);
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();

    //Add two more routes with same nexthop as net1

    IPNet<IPv4> net3("1.0.3.0/24");
    IPNet<IPv4> net4("1.0.4.0/24");
    //palist1 has the same palist as the route for net1
    palist3 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    //palist1 has the same nexthop, but different palist as the route for net1
    palist4 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt2, igp_origin_att);
    sr3 = new SubnetRoute<IPv4>(net3, palist3, NULL);
    sr4 = new SubnetRoute<IPv4>(net4, palist4, NULL);

    msg = new InternalMessage<IPv4>(sr3, &handler1, 0);
    debug_table->write_comment("ADD THIRD ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist3;
    sr3->unref();

    //check there are 3 routes in the RIB-IN
    assert(ribin->route_count() == 3);

    msg = new InternalMessage<IPv4>(sr4, &handler1, 0);
    debug_table->write_comment("ADD FOURTH ROUTE");
    ribin->add_route(*msg, NULL);
    delete msg;
    delete palist4;
    sr4->unref();

    //check there are 4 routes in the RIB-IN
    assert(ribin->route_count() == 4);

    debug_table->write_separator();
    debug_table->write_separator();
    debug_table->write_comment("NEXTHOP 2.0.0.1 CHANGES");
    //this should trigger three a replaces
    ribin->igp_nexthop_changed(nexthop1);
    while (bgpmain.eventloop().timers_pending()) {
	bgpmain.eventloop().run();
    }

    debug_table->write_separator();
    debug_table->write_comment("DELETE THE ROUTES AND CLEAN UP");

    palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt1, igp_origin_att);

    palist3 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt2, igp_origin_att);

    palist4 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    delete palist1;
    sr1->unref();

    sr2 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    delete palist2;
    sr2->unref();

    sr3 = new SubnetRoute<IPv4>(net3, palist3, NULL);
    msg = new InternalMessage<IPv4>(sr3, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    delete palist3;
    sr3->unref();

    sr4 = new SubnetRoute<IPv4>(net4, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr4, &handler1, 0);
    ribin->delete_route(*msg, NULL);
    delete msg;
    delete palist4;
    sr4->unref();

    debug_table->write_separator();

    //================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete ribin;
    delete debug_table;

    return validate_reference_file("/test_ribin.reference", filename, "RIBIN");
}
