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
#include "route_table_cache.hh"
#include "route_table_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"
#include "dump_iterators.hh"


bool
test_cache(TestInfo& /*info*/)
{
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_cache.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_cache";
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

#if 0
    PAListRef<IPv4> dummy_palist;
    dummy_palist.create_attribute_manager();   
#endif

    // trivial plumbing
    CacheTable<IPv4> *cache_table
	= new CacheTable<IPv4>("CACHE", SAFI_UNICAST, NULL, &handler1);
    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)cache_table);
    cache_table->set_next_table(debug_table);
    assert(cache_table->route_count() == 0);

    debug_table->set_output_file(filename);
    debug_table->set_canned_response(ADD_USED);

    // create a load of attributes
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");

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
    InternalMessage<IPv4>* msg, *msg2;

    // ================================================================
    // Test1: trivial add and delete
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE, UNCACHED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    cache_table->add_route(*msg, NULL);

    // we even cache unchanged routes now
    assert(cache_table->route_count() == 1);

    debug_table->write_separator();

    // delete the route
    cache_table->delete_route(*msg, NULL);
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test1b: trivial add and delete, route cached
    // ================================================================
    // add a route
    // create a subnet route
    debug_table->write_comment("TEST 1a");
    debug_table->write_comment("ADD AND DELETE WITH CACHING.");
    debug_table->write_comment("ADD, CACHED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();
    msg->set_push();
    cache_table->add_route(*msg, NULL);
    // note that as the route has changed, the cache table is
    // responsible for deleting the route in the message
    delete msg;
    sr1->unref();

    // verify that this route was now cached
    assert(cache_table->route_count() == 1);

    debug_table->write_separator();
    debug_table->write_comment("DELETE, CACHED");

    // delete the route
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    cache_table->delete_route(*msg, NULL);

    // note that as the route has changed, the cache table is
    // responsible for deleting the route in the message
    delete msg;
    sr1->unref();

    // verify that this route was now deleted from the cache
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();

#if 0
Test no longer makes sense now we cache all data
    // ================================================================
    // Test2: trivial replace
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("REPLACE ROUTE, NOT CACHED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);

    sr2 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg2->set_push();
    cache_table->replace_route(*msg, *msg2, NULL);
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    sr1->unref();
    sr2->unref();
    // ================================================================
    // Test2a: trivial replace, original route cacheed
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2a");
    debug_table->write_comment("REPLACE ROUTE, OLD ROUTE CACHEED");
    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();
    cache_table->add_route(*msg, NULL);
    // note that as the route has changed, the cache table is
    // responsible for deleting the route in the message
    delete msg;
    assert(cache_table->route_count() == 1);

    sr1 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();

    sr2 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg2->set_push();
    cache_table->replace_route(*msg, *msg2, NULL);
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    sr2->unref();

    // ================================================================
    // Test2b: trivial replace, new route to be cached
    // ================================================================
    assert(cache_table->route_count() == 0);
    debug_table->write_comment("TEST 2b");
    debug_table->write_comment("REPLACE ROUTE, NEW ROUTE TO BE CACHED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);

    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg2->set_changed();
    msg2->set_push();
    cache_table->replace_route(*msg, *msg2, NULL);

    // verify that the new route was cached
    assert(cache_table->route_count() == 1);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    sr1->unref();

    // delete the route
    debug_table->write_comment("CLEANUP");
    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg->set_changed();
    cache_table->delete_route(*msg, NULL);
    delete msg;
    assert(cache_table->route_count() == 0);
    debug_table->write_separator();
#endif

    // ================================================================
    // Test2c: trivial replace, both routes cacheed
    // ================================================================
    debug_table->write_comment("TEST 2c");
    debug_table->write_comment("REPLACE ROUTE, BOTH ROUTES CACHED");
    assert(cache_table->route_count() == 0);
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();
    debug_table->write_comment("ADD ROUTE TO CACHE");
    cache_table->add_route(*msg, NULL);
    //    assert(msg->route() == NULL);
    sr1->unref();
    delete msg;

    assert(cache_table->route_count() == 1);
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();

    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg2->set_changed();
    msg2->set_push();
    debug_table->write_comment("REPLACE ROUTE");
    cache_table->replace_route(*msg, *msg2, NULL);

    // verify that the new route was cached
    assert(cache_table->route_count() == 1);

    debug_table->write_separator();

    //    assert(msg->route() == NULL);
    //    assert(msg2->route() == NULL);
    sr1->unref();
    sr2->unref();
    delete msg;
    delete msg2;

    // delete the route
    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg->set_changed();
    debug_table->write_comment("CLEANUP");
    debug_table->write_comment("DELETE ROUTE FROM CACHE");
    cache_table->delete_route(*msg, NULL);
    delete msg;
    sr2->unref();
    assert(cache_table->route_count() == 0);
    debug_table->write_separator();

    // ================================================================
    // Test3: flush cache
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("FLUSH_CACHE");
    assert(cache_table->route_count() == 0);
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();
    cache_table->add_route(*msg, NULL);
    //    assert(msg->route() == NULL);
    sr1->unref();
    delete msg;

    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_changed();
    cache_table->add_route(*msg, NULL);
    //    assert(msg->route() == NULL);
    sr1->unref();
    delete msg;

    assert(cache_table->route_count() == 2);
    cache_table->flush_cache();
    while (bgpmain.eventloop().events_pending()) {
	bgpmain.eventloop().run();
    }
    assert(cache_table->route_count() == 0);

    // ================================================================
    // Check debug output against reference
    // ================================================================

    debug_table->write_separator();
    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete cache_table;
    delete debug_table;
    palist1.release();
    palist2.release();
    palist3.release();
    fpalist1 = 0;
    fpalist2 = 0;
    fpalist3 = 0;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", filename.c_str());
	fprintf(stderr, "TEST CACHE FAILED\n");
	fclose(file);
	return false;
    }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST CACHE FAILED\n");
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
    ref_filename += "/test_cache.reference";
    
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST CACHE FAILED\n");
	fclose(file);
	return false;
    }
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST CACHE FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);

    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1) != 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST CACHE FAILED\n");
	return false;

    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}


