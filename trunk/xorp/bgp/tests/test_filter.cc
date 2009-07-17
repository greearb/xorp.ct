// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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
#include "dump_iterators.hh"
#include "dummy_next_hop_resolver.hh"


bool
test_filter(TestInfo& /*info*/)
{
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_filter.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_filter";
    free(tmppath);
#endif
    EventLoop eventloop;
    BGPMain bgpmain(eventloop);
    LocalData localdata(bgpmain.eventloop());
    Iptuple iptuple;
    BGPPeerData *pd1 = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    BGPPeer peer1(&localdata, pd1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);
    BGPPeerData *pd2 = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    BGPPeer peer2(&localdata, pd2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);
    DummyNextHopResolver<IPv4> next_hop_resolver(bgpmain.eventloop(), bgpmain);

    // Trivial plumbing. We're not testing the RibInTable here, so
    // mostly we'll just inject directly into the FilterTable, but we
    // need an RibInTable here to test lookup_route.
    RibInTable<IPv4> *ribin_table
	= new RibInTable<IPv4>("RIB-in", SAFI_UNICAST, &handler1);
    FilterTable<IPv4> *filter_table
	= new FilterTable<IPv4>("FILTER", SAFI_UNICAST, ribin_table,
				next_hop_resolver);
    filter_table->do_versioning();
    ribin_table->set_next_table(filter_table);
    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)filter_table);
    filter_table->set_next_table(debug_table);
    debug_table->set_output_file(filename);
    debug_table->set_canned_response(ADD_USED);

    // Add a filter that drops routes with AS 5 in their path
    filter_table->add_simple_AS_filter(AsNum((5)));

    // create a load of attributes
    IPNet<IPv4> net1("1.0.1.0/24");
    //    IPNet<IPv4> net2("1.0.2.0/24");

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);

    IPv4 nexthop2("2.0.0.2");
    NextHopAttribute<IPv4> nhatt2(nexthop2);

    IPv4 nexthop3("2.0.0.3");
    NextHopAttribute<IPv4> nhatt3(nexthop3);

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
    ASPathAttribute aspathatt2(aspath2);

    ASPath aspath3;
    aspath3.prepend_as(AsNum(7));
    aspath3.prepend_as(AsNum(8));
    aspath3.prepend_as(AsNum(9));
    ASPathAttribute aspathatt3(aspath3);

    FPAList4Ref fpalist1 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    PAListRef<IPv4> palist1 = new PathAttributeList<IPv4>(fpalist1);

    FPAList4Ref fpalist2 =
	new FastPathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);
    PAListRef<IPv4> palist2 = new PathAttributeList<IPv4>(fpalist2);

    FPAList4Ref fpalist3 =
	new FastPathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);
    PAListRef<IPv4> palist3 = new PathAttributeList<IPv4>(fpalist3);

    // create a subnet route
    SubnetRoute<IPv4> *sr1, *sr2;

    InternalMessage<IPv4>* msg;

    PolicyTags pt;

    // ================================================================
    // Test1: trivial add and delete
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE UNFILTERED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);

    debug_table->write_separator();

    // delete the route
    filter_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test1b: trivial add and delete, AS filtered
    // ================================================================
    // add a route
    // create a subnet route
    debug_table->write_comment("TEST 1a");
    debug_table->write_comment("ADD AND DELETE FILTERING");
    debug_table->write_comment("ADD, FILTERED");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);

    debug_table->write_separator();
    debug_table->write_comment("DELETE, FILTERED");

    // delete the route
    filter_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test2: trivial replace
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);

    sr2 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    InternalMessage<IPv4>* msg2 = new InternalMessage<IPv4>(sr2, &handler1, 1);
    debug_table->write_comment("REPLACE ROUTE");
    filter_table->replace_route(*msg, *msg2, ribin_table);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    sr1->unref();
    sr2->unref();

    // ================================================================
    // Test2a: trivial replace, original route filtered
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2a");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);

    sr2 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 1);
    debug_table->write_comment("REPLACE ROUTE, OLD ROUTE FILTERED");
    filter_table->replace_route(*msg, *msg2, ribin_table);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    sr1->unref();
    sr2->unref();

    // ================================================================
    // Test2b: trivial replace, new route filtered
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2b");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);

    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 1);
    debug_table->write_comment("REPLACE ROUTE, NEW ROUTE FILTERED");
    filter_table->replace_route(*msg, *msg2, ribin_table);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    sr1->unref();
    sr2->unref();

    // ================================================================
    // Test2c: trivial replace, both routes filtered
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2c");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);

    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 1);
    debug_table->write_comment("REPLACE ROUTE, BOTH ROUTES FILTERED");
    filter_table->replace_route(*msg, *msg2, ribin_table);


    delete msg;
    delete msg2;
    sr1->unref();
    sr2->unref();
    debug_table->write_separator();

    // ================================================================
    // Test3: route dump
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 3");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);

    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    debug_table->write_comment("ROUTE DUMP");
    filter_table->route_dump(*msg, ribin_table, &handler2);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test3a: route dump, filtered
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 3a");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);

    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    debug_table->write_comment("ROUTE_DUMP, FILTERED");
    filter_table->route_dump(*msg, ribin_table, &handler2);
    sr1->unref();
    delete msg;

    debug_table->write_separator();
    // ================================================================
    // Test4: trivial route lookup
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 4");
    debug_table->write_comment("ADD, LOOKUP AND DELETE UNFILTERED");
    ribin_table->add_route(net1, fpalist1, pt);

    debug_table->write_separator();
    debug_table->write_comment("LOOKUP ROUTE");
    const SubnetRoute<IPv4> *found_route;
    uint32_t genid = 0xffff;
    FPAList4Ref found_fpalist;
    found_route = filter_table->lookup_route(net1, genid, found_fpalist);
    assert(found_route != NULL);
    assert(found_route->net() == net1);
#if 0
    printf("orig attributes: \n%s\n", palist1->str().c_str());
    printf("found attributes1: \n%s\n", found_route->attributes()->str().c_str());
    printf("found attributes2: \n%s\n", found_fpalist->str().c_str());
#endif
    assert(found_route->attributes() == palist1);

    debug_table->write_comment("TEST SUCCESSFUL");
    debug_table->write_separator();

    // delete the route
    ribin_table->delete_route(net1);
    debug_table->write_separator();

    // ================================================================
    // Test4a: trivial route lookup, filtered
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 4a");
    debug_table->write_comment("ADD, LOOKUP AND DELETE FILTERED");
    debug_table->write_comment("ADD, SHOULD BE FILTERED");
    ribin_table->add_route(net1, fpalist2, pt);

    debug_table->write_separator();
    debug_table->write_comment("LOOKUP ROUTE");
    found_route = filter_table->lookup_route(net1, genid, found_fpalist);
    // route should not be found because it was filtered
    assert(found_route == NULL);

    debug_table->write_comment("TEST SUCCESSFUL");
    debug_table->write_separator();

    // delete the route
    debug_table->write_comment("DELETE, SHOULD BE FILTERED");
    ribin_table->delete_route(net1);

    debug_table->write_separator();

    // ================================================================
    // Test5: add and delete with a local_pref_insertion filter
    // ================================================================
    filter_table->add_localpref_insertion_filter(100);
    // add a route
    debug_table->write_comment("TEST 5");
    debug_table->write_comment("ADD AND DELETE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);
    sr1->unref();
    delete msg;

    debug_table->write_separator();

    // delete the route
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->delete_route(*msg, ribin_table);
    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test6: add and delete with a local_pref_insertion filter and a 
    // next_hop_rewrite_filter - both these modify a route
    // ================================================================
    filter_table->add_nexthop_rewrite_filter(IPv4("128.16.64.4"), false,
					     IPNet<IPv4>());
    // add a route
    debug_table->write_comment("TEST 6");
    debug_table->write_comment("ADD AND DELETE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);

    debug_table->write_separator();

    // delete the route
    filter_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test 7a: add and delete with a well known attributes filter
    // This should be passed through, as it has no well-known attributes
    // ================================================================
    filter_table->add_known_community_filter(PEER_TYPE_EBGP);
    // add a route
    debug_table->write_comment("TEST 7");
    debug_table->write_comment("ADD AND DELETE");
    debug_table->write_comment("EXPECT CHANGE TO PROPAGATE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);

    debug_table->write_separator();

    // delete the route
    filter_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test 7b: add and delete with a well known attributes filter
    // This should not be passed through, as it has NO_EXPORT
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 7b");
    debug_table->write_comment("ADD AND DELETE");
    debug_table->write_comment("EXPECT CHANGE NOT TO PROPAGATE");

    FPAList4Ref fpalist4 =
	new FastPathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);
    CommunityAttribute comm_att;
    comm_att.add_community(CommunityAttribute::NO_EXPORT);
    assert(comm_att.contains(CommunityAttribute::NO_EXPORT));
    fpalist4->add_path_attribute(comm_att);
    PAListRef<IPv4> palist4 = new PathAttributeList<IPv4>(fpalist4);

    sr1 = new SubnetRoute<IPv4>(net1, palist4, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);

    debug_table->write_separator();

    // delete the route
    filter_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test 8: test filter versions
    // old filter should give same results as test 6.
    // new filter should not modify route.
    // ================================================================

    // add a route
    debug_table->write_comment("TEST 8a");
    debug_table->write_comment("ADD");
    debug_table->write_comment("EXPECT AS TEST 6");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    filter_table->add_route(*msg, ribin_table);

    debug_table->write_separator();

    // reset filters
    debug_table->write_comment("RECONFIGURE FILTER");
    filter_table->reconfigure_filter();
    

    // add a route
    debug_table->write_comment("TEST 8b");
    debug_table->write_comment("ADD AND DELETE");
    debug_table->write_comment("EXPECT ROUTE UNMODIFIED");
    sr2 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 2/*GenID*/);
    filter_table->add_route(*msg2, ribin_table);

    debug_table->write_separator();

    // delete the route

    filter_table->delete_route(*msg2, ribin_table);
    debug_table->write_separator();
    sr2->unref();
    delete msg2;

    debug_table->write_comment("TEST 8c");
    debug_table->write_comment("DELETE");
    debug_table->write_comment("EXPECT AS TEST 6");
    // delete the route
    filter_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete ribin_table;
    delete filter_table;
    delete debug_table;
    palist1.release();
    palist2.release();
    palist3.release();
    palist4.release();
    fpalist1 = 0;
    fpalist2 = 0;
    fpalist3 = 0;
    fpalist4 = 0;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n",
		filename.c_str());
	fprintf(stderr, "TEST FILTER FAILED\n");
	fclose(file);
	return false;
    }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FILTER FAILED\n");
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
    ref_filename += "/test_filter.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST FILTER FAILED\n");
	fclose(file);
	return false;
    }
    
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST FILTER FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);

    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1) != 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST FILTER FAILED\n");
	return false;

    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}


