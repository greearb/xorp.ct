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

#ident "$XORP: xorp/bgp/test_cache.cc,v 1.6 2003/01/29 00:38:57 rizzo Exp $"

#include "bgp_module.h"
#include "config.h"
#include <pwd.h>
#include "libxorp/selector.hh"
#include "libxorp/xlog.h"

#include "main.hh"
#include "route_table_base.hh"
#include "route_table_cache.hh"
#include "route_table_debug.hh"
#include "path_attribute_list.hh"
#include "local_data.hh"
#include "dump_iterators.hh"


int main(int, char** argv) {
    // stuff needed to create an eventloop
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_add_default_output();
    xlog_start();
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_cache.";
    filename += pwd->pw_name;
    BGPMain bgpmain;
    // EventLoop* eventloop = bgpmain.get_eventloop();
    LocalData localdata;
    BGPPeer peer1(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL);
    BGPPeer peer2(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL);

    // trivial plumbing
    CacheTable<IPv4> *cache_table
	= new CacheTable<IPv4>("CACHE", NULL);
    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)cache_table);
    cache_table->set_next_table(debug_table);
    assert(cache_table->route_count() == 0);

    debug_table->set_output_file(filename);
    debug_table->set_canned_response(ADD_USED);

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

    // create a subnet route
    SubnetRoute<IPv4> *sr1, *sr2;
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);

    // ================================================================
    // Test1: trivial add and delete
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE, UNCACHED");
    InternalMessage<IPv4>* msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    cache_table->add_route(*msg, NULL);

    // route shouldn't have been cached.
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();

    // delete the route
    cache_table->delete_route(*msg, NULL);
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();
    delete sr1;
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
    delete sr1;

    // verify that this route was now deleted from the cache
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();

    // ================================================================
    // Test2: trivial replace
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("REPLACE ROUTE, NOT CACHED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);

    sr2 = new SubnetRoute<IPv4>(net1, palist3, NULL);
    InternalMessage<IPv4>* msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg2->set_push();
    cache_table->replace_route(*msg, *msg2, NULL);
    assert(cache_table->route_count() == 0);

    debug_table->write_separator();

    delete msg;
    delete msg2;
    delete sr1;
    delete sr2;

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
    delete sr2;

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
    delete sr1;

    // delete the route
    debug_table->write_comment("CLEANUP");
    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg->set_changed();
    cache_table->delete_route(*msg, NULL);
    delete msg;
    assert(cache_table->route_count() == 0);
    debug_table->write_separator();

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
    assert(msg->route() == NULL);
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

    assert(msg->route() == NULL);
    assert(msg2->route() == NULL);
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
    assert(msg->route() == NULL);
    delete msg;

    assert(cache_table->route_count() == 1);
    cache_table->flush_cache();
    assert(cache_table->route_count() == 0);


    // ================================================================
    // Check debug output against reference
    // ================================================================

    debug_table->write_separator();
    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete cache_table;
    delete debug_table;
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

    file = fopen("test_cache.reference", "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read test_cache.reference\n");
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


