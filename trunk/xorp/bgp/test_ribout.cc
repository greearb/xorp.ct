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
#include "libxorp/asnum.hh"
#include "libxorp/test_main.hh"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "bgp.hh"
#include "route_table_base.hh"
#include "route_table_ribout.hh"
#include "route_table_debug.hh"
#include "peer_handler_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"
#include "dump_iterators.hh"


bool
test_ribout(TestInfo& /*info*/)
{
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_ribout.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_ribout";
    free(tmppath);
#endif

    EventLoop eventloop;
    BGPMain bgpmain(eventloop);
    LocalData localdata(bgpmain.eventloop());
    localdata.set_as(AsNum(1));
    Iptuple iptuple;
    BGPPeerData *peer_data1
	= new BGPPeerData(localdata, iptuple, AsNum(1), IPv4("2.0.0.1"), 30);
    peer_data1->compute_peer_type();
    BGPPeer peer1(&localdata, peer_data1, NULL, &bgpmain);
    DebugPeerHandler handler(&peer1);

    BGPPeerData *peer_data2
	= new BGPPeerData(localdata, iptuple, AsNum(2), IPv4("2.0.0.2"), 30);
    peer_data2->compute_peer_type();
    BGPPeer peer2(&localdata, peer_data2, NULL, &bgpmain);
    DebugPeerHandler ebgp_handler(&peer2);

    //trivial plumbing
    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", NULL);
    RibOutTable<IPv4> *ribout_table
	= new RibOutTable<IPv4>("RibOut", SAFI_UNICAST, debug_table, &handler);
    ribout_table->peering_came_up(&handler, 0, debug_table);

    debug_table->set_output_file(filename);
    debug_table->set_canned_response(ADD_USED);
    handler.set_output_file(debug_table->output_file());

    //create a load of attributes 
    IPNet<IPv4> net1("1.0.1.0/24");
    IPNet<IPv4> net2("1.0.2.0/24");
    IPNet<IPv4> net3("1.0.3.0/24");

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

    //create a subnet route
    SubnetRoute<IPv4> *sr1;

    InternalMessage<IPv4>* msg;

    UNUSED(palist3);
    //================================================================
    //Test1: trivial add and delete
    //================================================================
    //add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    msg->set_push();
    ribout_table->add_route(*msg, debug_table);

    debug_table->write_separator();

    //delete the route
    ribout_table->delete_route(*msg, debug_table);
    msg->set_push();

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    //================================================================
    //Test2: three adds, two with the same PAlist.
    //================================================================
    //add a route
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("THREE ADDS ON TWO PA_LISTS");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    ribout_table->add_route(*msg, debug_table);
    sr1->unref();
    delete msg;

    sr1 = new SubnetRoute<IPv4>(net2, palist2, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    ribout_table->add_route(*msg, debug_table);
    sr1->unref();
    delete msg;

    sr1 = new SubnetRoute<IPv4>(net3, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    ribout_table->add_route(*msg, debug_table);
    sr1->unref();
    delete msg;

    ribout_table->push(debug_table);

    debug_table->write_separator();

    //================================================================
    //Test3: delete then add
    //================================================================
    //add a route
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("DELETE THEN ADD");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    ribout_table->delete_route(*msg, debug_table);
    sr1->unref();
    delete msg;

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    ribout_table->add_route(*msg, debug_table);
    sr1->unref();
    delete msg;

    ribout_table->push(debug_table);

    debug_table->write_separator();

    //================================================================
    //Test4: add when peer is busy
    //================================================================
    //add a route
    debug_table->write_comment("TEST 4");
    debug_table->write_comment("ADD WHEN PEER BUSY");
    handler.set_canned_response(PEER_OUTPUT_BUSY);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    ribout_table->add_route(*msg, debug_table);
    sr1->unref();
    delete msg;

    ribout_table->push(debug_table);

    debug_table->write_separator();

    debug_table->write_comment("PEER_GOES IDLE");
    handler.set_canned_response(PEER_OUTPUT_OK);
    debug_table->set_next_messages(2);
    ribout_table->output_no_longer_busy();

    //================================================================
    //Test5: replace from IBGP peer to EBGP peer
    //================================================================
    //add a route
    debug_table->write_comment("TEST 5");
    debug_table->write_comment("SWITCH FROM IBGP PEER TO EBGP PEER");
    handler.set_canned_response(PEER_OUTPUT_BUSY);

    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler, 0);
    SubnetRoute<IPv4> *sr2;
    InternalMessage<IPv4>* msg2;
    sr2 = new SubnetRoute<IPv4>(net1, palist2, NULL);
    msg2 = new InternalMessage<IPv4>(sr2, &ebgp_handler, 0);
    ribout_table->replace_route(*msg, *msg2, debug_table);
    sr1->unref();
    delete msg;
    sr2->unref();
    delete msg2;

    ribout_table->push(debug_table);

    debug_table->write_separator();

    debug_table->write_comment("PEER_GOES IDLE");
    handler.set_canned_response(PEER_OUTPUT_OK);
    debug_table->set_next_messages(2);
    ribout_table->output_no_longer_busy();

    //================================================================
    //Check debug output against reference
    //================================================================

    debug_table->write_separator();
    debug_table->write_comment("SHUTDOWN AND CLEAN UP");

    delete ribout_table;
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
	fprintf(stderr, "TEST RIBOUT FAILED\n");
	fclose(file);
	return false;
    }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST RIBOUT FAILED\n");
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
    ref_filename += "/test_ribout.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST RIBOUT FAILED\n");
	fclose(file);
	return false;
    }

    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST RIBOUT FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);
    
    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1)!= 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST RIBOUT FAILED\n");
	return false;
	
    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}


