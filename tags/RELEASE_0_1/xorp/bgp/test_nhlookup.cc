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

#ident "$XORP: xorp/bgp/test_nhlookup.cc,v 1.8 2002/12/09 18:28:50 hodson Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/selector.hh"
#include "libxorp/xlog.h"

#include "main.hh"
#include "route_table_nhlookup.hh"
#include "route_table_debug.hh"
#include "path_attribute_list.hh"
#include "local_data.hh"

template <class A>
class DummyResolver : public NextHopResolver<A> {
public:
    DummyResolver() : NextHopResolver<A>(0) {
	_ofile = NULL;
	_response = false;
    }
    void set_output_file(FILE *file) { _ofile = file;}
    void set_response(bool response) {_response = response;}
    bool register_nexthop(A nexthop, IPNet<A> net, 
			  BGPNhLookupTable<A> */*requester*/) {
	fprintf(_ofile, "[NH REGISTER]\n");
	fprintf(_ofile, "NextHop: %s\n", nexthop.str().c_str());
	fprintf(_ofile, "Net: %s\n", net.str().c_str());
	return _response;
    }
    void deregister_nexthop(A nexthop, IPNet<A> net, 
			    BGPNhLookupTable<A> */*requester*/) {
	fprintf(_ofile, "[NH DEREGISTER]\n");
	fprintf(_ofile, "NextHop: %s\n", nexthop.str().c_str());
	fprintf(_ofile, "Net: %s\n", net.str().c_str());
    }
private:
    FILE *_ofile;
    bool _response;
};




int main(int, char** argv) {
    //stuff needed to create an eventloop
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_add_default_output();
    xlog_start();
    BGPMain bgpmain;
    //    EventLoop* eventloop = bgpmain.get_eventloop();
    LocalData localdata;
    BGPPeer peer1(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL);
    BGPPeer peer2(&localdata, NULL, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL);

    DummyResolver<IPv4> nh_resolver;

    //trivial plumbing
    BGPNhLookupTable<IPv4> *nhlookup_table
	= new BGPNhLookupTable<IPv4>("NHLOOKUP", &nh_resolver, NULL);
    BGPDebugTable<IPv4>* debug_table
	 = new BGPDebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)nhlookup_table);
    nhlookup_table->set_next_table(debug_table);

    debug_table->set_output_file("/tmp/test_nhlookup");
    debug_table->set_canned_response(ADD_USED);

    //use the same output file as debug table
    nh_resolver.set_output_file(debug_table->output_file());

    //create a load of attributes 
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");

    IPv4 nexthop1("2.0.0.1");
    NextHopAttribute<IPv4> nhatt1(nexthop1);

    IPv4 nexthop2("2.0.0.2");
    NextHopAttribute<IPv4> nhatt2(nexthop2);

    IPv4 nexthop3("2.0.0.3");
    NextHopAttribute<IPv4> nhatt3(nexthop3);

    OriginAttribute igp_origin_att(IGP);

    AsPath aspath1;
    aspath1.add_AS_in_sequence(AsNum((uint16_t)1));
    aspath1.add_AS_in_sequence(AsNum((uint16_t)2));
    aspath1.add_AS_in_sequence(AsNum((uint16_t)3));
    ASPathAttribute aspathatt1(aspath1);

    AsPath aspath2;
    aspath2.add_AS_in_sequence(AsNum((uint16_t)4));
    aspath2.add_AS_in_sequence(AsNum((uint16_t)5));
    aspath2.add_AS_in_sequence(AsNum((uint16_t)6));
    ASPathAttribute aspathatt2(aspath2);

    AsPath aspath3;
    aspath3.add_AS_in_sequence(AsNum((uint16_t)7));
    aspath3.add_AS_in_sequence(AsNum((uint16_t)8));
    aspath3.add_AS_in_sequence(AsNum((uint16_t)9));
    ASPathAttribute aspathatt3(aspath3);

    PathAttributeList<IPv4>* palist1 =
	new PathAttributeList<IPv4>(nhatt1, aspathatt1, igp_origin_att);

    PathAttributeList<IPv4>* palist2 =
	new PathAttributeList<IPv4>(nhatt2, aspathatt2, igp_origin_att);

    PathAttributeList<IPv4>* palist3 =
	new PathAttributeList<IPv4>(nhatt3, aspathatt3, igp_origin_att);

    SubnetRoute<IPv4> *sr1, *sr2, *sr3;
    InternalMessage<IPv4> *msg, *msg2;

    //================================================================
    //Test1: trivial add and delete, nexthop resolves
    //================================================================
    //add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE, NEXTHOP RESOLVES");

    //pretend we have already resolved the nexthop
    nh_resolver.set_response(true);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->add_route(*msg, NULL);
    delete sr1;
    delete msg;

    debug_table->write_separator();

    //delete the route
    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);

    debug_table->write_separator();
    delete sr1;
    delete msg;


    //================================================================
    //Test1a: trivial add and delete, nexthop doesn't yet resolve
    //================================================================
    //add a route
    debug_table->write_comment("TEST 1a");
    debug_table->write_comment("ADD AND DELETE, NEXTHOP NOT YET RESOLVED");

    //pretend we have already resolved the nexthop
    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->add_route(*msg, NULL);
    delete msg;

    debug_table->write_separator();

    //delete the route
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);

    debug_table->write_separator();
    delete sr1;
    delete msg;

    //================================================================
    //Test1b: trivial add and delete, nexthop doesn't yet resolve
    //================================================================
    //add a route
    debug_table->write_comment("TEST 1b");
    debug_table->write_comment("ADD AND DELETE, NEXTHOP NOT YET RESOLVED");

    //pretend we have not yet resolved the nexthop
    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr1 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("THEN NEXTHOP RESOLVES");

    set <IPNet<IPv4> > nets;
    nets.insert(net1);
    nhlookup_table->RIB_lookup_done(nexthop1, nets, true);
    nets.erase(nets.begin());
    assert(nets.empty());
    
    debug_table->write_separator();

    debug_table->write_comment("NOW DELETE THE ROUTE");
    //delete the route
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);

    debug_table->write_separator();
    delete sr1;
    delete msg;

    //================================================================
    //Test1c: trivial add and delete, nexthop doesn't yet resolve.  
    // Resolve two routes that use the same nexthop
    //================================================================
    debug_table->write_comment("TEST 1c");
    debug_table->write_comment("ADDx2 AND DELETE, NEXTHOP NOT YET RESOLVED");

    //pretend we have not yet resolved the nexthop
    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr1 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    sr2 = new SubnetRoute<IPv4>(net2, palist1);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr2 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("THEN NEXTHOP RESOLVES");

    //tell NhLookupTable the nexthop now resolves
    nets.insert(net1);
    nets.insert(net2);
    nhlookup_table->RIB_lookup_done(nexthop1, nets, true);
    while (!nets.empty())
	nets.erase(nets.begin());
    
    debug_table->write_separator();

    debug_table->write_comment("NOW DELETE THE ROUTES");
    //delete the routes
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);
    delete sr2;
    delete msg;

    debug_table->write_separator();

    //================================================================
    //Test2: add, nexthop doesn't yet resolve, replace, resolve, delete
    //================================================================
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("ADD, REPLACE, RESOLVE, DELETE");

    //pretend we have not yet resolved the nexthop
    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("ADD");
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr1 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    debug_table->write_separator();
 
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    sr2 = new SubnetRoute<IPv4>(net1, palist2);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0); 
    debug_table->write_comment("REPLACE");
    nhlookup_table->replace_route(*msg, *msg2, NULL);
    //Note: don't delete sr2 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete sr1;
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("THEN NEXTHOP RESOLVES");

    nets.insert(net1);
    nhlookup_table->RIB_lookup_done(nexthop2, nets, true);
    nets.erase(nets.begin());
    assert(nets.empty());
    
    debug_table->write_separator();

    debug_table->write_comment("NOW DELETE THE ROUTE");
    //delete the route
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);

    debug_table->write_separator();
    delete sr2;
    delete msg;

    //================================================================
    //Test2b: add, nexthop doesn't yet resolve, replace, resolve, delete
    //================================================================
    debug_table->write_comment("TEST 2b");
    debug_table->write_comment("ADD, REPLACE (OLD IS DELETED), RESOLVE, DELETE");

    //pretend we have not yet resolved the nexthop
    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    debug_table->write_comment("ADD");
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr1 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    debug_table->write_separator();
 
    sr3 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr3, &handler1, 0);
    //delete the old route, as might happen with a replace route on RibIn
    delete sr1;
    sr2 = new SubnetRoute<IPv4>(net1, palist2);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0); 
    debug_table->write_comment("REPLACE");
    nhlookup_table->replace_route(*msg, *msg2, NULL);
    //Note: don't delete sr2 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete sr3;
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("THEN NEXTHOP RESOLVES");

    nets.insert(net1);
    nhlookup_table->RIB_lookup_done(nexthop2, nets, true);
    nets.erase(nets.begin());
    assert(nets.empty());
    
    debug_table->write_separator();

    debug_table->write_comment("NOW DELETE THE ROUTE");
    //delete the route
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);

    debug_table->write_separator();
    delete sr2;
    delete msg;

    //================================================================
    //Test3: replace, nexthop doesn't yet resolve, replace, resolve, delete
    //================================================================
    //add a route
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("REPLACE, REPLACE, RESOLVE, DELETE");

    //We'll start off by sending a replace.  This is so we can get the
    //replace into the queue, and then send a replace to replace the
    //replace.  But it also tests the case where the previous nexthop
    //did resolve and now the new one doesn't.

    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    sr2 = new SubnetRoute<IPv4>(net1, palist2);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg2 = new InternalMessage<IPv4>(sr2, &handler1, 0);
    debug_table->write_comment("FIRST REPLACE");
    nhlookup_table->replace_route(*msg, *msg2, NULL);
    //Note: don't delete sr2 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;
    delete msg2;
    delete sr1;

    debug_table->write_separator();
 
    sr1 = new SubnetRoute<IPv4>(net1, palist2);
    sr3 = new SubnetRoute<IPv4>(net1, palist3);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg2 = new InternalMessage<IPv4>(sr3, &handler1, 0); 
    debug_table->write_comment("SECOND REPLACE");
    nhlookup_table->replace_route(*msg, *msg2, NULL);
    //Note: don't delete sr2 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete sr1;
    delete msg;

    debug_table->write_separator();

    debug_table->write_comment("THEN NEXTHOP RESOLVES");

    nets.insert(net1);
    nhlookup_table->RIB_lookup_done(nexthop3, nets, true);
    nets.erase(nets.begin());
    assert(nets.empty());
    
    debug_table->write_separator();

    debug_table->write_comment("NOW DELETE THE ROUTE");
    //delete the route
    msg = new InternalMessage<IPv4>(sr3, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);

    debug_table->write_separator();
    delete sr3;
    delete msg;

    //================================================================
    //Test4: checking push is propagated properly.
    // trivial add and delete, nexthop doesn't yet resolve.  
    // Resolve two routes that use the same nexthop
    //================================================================
    debug_table->write_comment("TEST 4");
    debug_table->write_comment("TESTING PUSH");
    debug_table->write_comment("ADDx2 AND DELETE, NEXTHOP NOT YET RESOLVED");

    //pretend we have not yet resolved the nexthop
    nh_resolver.set_response(false);

    sr1 = new SubnetRoute<IPv4>(net1, palist1);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    msg->set_push();
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr1 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    sr2 = new SubnetRoute<IPv4>(net2, palist1);
    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    nhlookup_table->add_route(*msg, NULL);
    //Note: don't delete sr2 here - the NhLookupTable assumes the
    //subnet route is still held in the RIB-In.
    delete msg;

    debug_table->write_comment("SEND PUSH");
    nhlookup_table->push(NULL);

    debug_table->write_separator();

    debug_table->write_comment("THEN NEXTHOP RESOLVES");

    //tell NhLookupTable the nexthop now resolves
    nets.insert(net1);
    nets.insert(net2);
    nhlookup_table->RIB_lookup_done(nexthop1, nets, true);
    while (!nets.empty())
	nets.erase(nets.begin());
    
    debug_table->write_separator();

    debug_table->write_comment("NOW DELETE THE ROUTES");
    //delete the routes
    msg = new InternalMessage<IPv4>(sr1, &handler1, 0);
    nhlookup_table->delete_route(*msg, NULL);
    delete sr1;
    delete msg;

    msg = new InternalMessage<IPv4>(sr2, &handler1, 0);
    msg->set_push();
    nhlookup_table->delete_route(*msg, NULL);
    delete sr2;
    delete msg;

    debug_table->write_comment("SEND PUSH AGAIN");
    nhlookup_table->push(NULL);

    debug_table->write_separator();

    //================================================================
    //Check debug output against reference
    //================================================================

    debug_table->write_separator();
    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete nhlookup_table;
    delete debug_table;


    FILE *file = fopen("/tmp/test_nhlookup", "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read /tmp/test_nhlookup\n");
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
    
    file = fopen("test_nhlookup.reference", "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read test_nhlookup.reference\n");
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
	fprintf(stderr, "Output in /tmp/test_nhlookup doesn't match reference output\n");
	fprintf(stderr, "TEST FAILED\n");
	exit(1);
	
    }
    unlink("/tmp/test_nhlookup");
    exit(0);
}


