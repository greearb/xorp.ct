// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/test_policy.cc,v 1.2 2006/09/08 22:55:24 mjh Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/test_main.hh"

#include "bgp.hh"
#include "route_table_base.hh"
#include "route_table_policy.hh"
#include "route_table_ribin.hh"
#include "route_table_debug.hh"
#include "path_attribute.hh"
#include "local_data.hh"

#include "policy/backend/version_filters.hh" 

#ifndef HOST_OS_WINDOWS
#include <pwd.h>
#endif

bool
test_policy_export(TestInfo& /*info*/)
{
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_policy_export.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_policy_export";
    free(tmppath);
#endif
    BGPMain bgpmain;
    LocalData localdata(bgpmain.eventloop());
    Iptuple iptuple;
    BGPPeerData *pd1 = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    BGPPeer peer1(&localdata, pd1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);
    BGPPeerData *pd2 = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    BGPPeer peer2(&localdata, pd2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);

    VersionFilters policy_filters;

    // Trivial plumbing. We're not testing the RibInTable here, so
    // mostly we'll just inject directly into the PolicyTable, but we
    // need an RibInTable here to test lookup_route.
    RibInTable<IPv4> *ribin_table
	= new RibInTable<IPv4>("RIB-in", SAFI_UNICAST, &handler1);
    PolicyTableExport<IPv4> *policy_table
	= new PolicyTableExport<IPv4>("POLICY", SAFI_UNICAST, ribin_table,
				      policy_filters, "test_neighbour");

    ribin_table->set_next_table(policy_table);
    DebugTable<IPv4>* debug_table
	 = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)policy_table);
    policy_table->set_next_table(debug_table);
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
    UNUSED(sr2);

    InternalMessage<IPv4>* msg;

    // ================================================================
    // Test1: trivial add and delete before config
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE UNFILTERED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    policy_table->add_route(*msg, ribin_table);


    // delete the route
    policy_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test2: trivial add then configure then delete 
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("ADD, CONFIG, DELETE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    policy_table->add_route(*msg, ribin_table);

    debug_table->write_comment("CONFIGURE FILTER");

    // Configure filter
    string filter_conf = "POLICY_START foo\n\
TERM_START foo2\n\
PUSH u32 200\n\
STORE 17\n\
ACCEPT\n\
TERM_END\n\
POLICY_END\n";

    policy_filters.configure(filter::EXPORT, filter_conf);
    
    debug_table->write_comment("EXPECT DELETE TO NOT HAVE LOCALPREF");
    // delete the route
    policy_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test2: add and delete with configured policy
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("ADD AND DELETE FILTERED");
    debug_table->write_comment("EXPECT ROUTES TO HAVE LOCALPREF");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    policy_table->add_route(*msg, ribin_table);


    // delete the route
    policy_table->delete_route(*msg, ribin_table);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete ribin_table;
    delete policy_table;
    delete debug_table;
    delete palist1;
    delete palist2;
    delete palist3;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n",
		filename.c_str());
	fprintf(stderr, "TEST POLICY EXPORT FAILED\n");
	fclose(file);
	return false;
    }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST POLICY EXPORT FAILED\n");
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
    ref_filename += "/test_policy_export.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST POLICY EXPORT FAILED\n");
	fclose(file);
	return false;
    }
    
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST POLICY EXPORT FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);

    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1) != 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST POLICY EXPORT FAILED\n");
	return false;

    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}


bool
test_policy(TestInfo& /*info*/)
{
#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/test_policy.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "test_policy";
    free(tmppath);
#endif
    BGPMain bgpmain;
    LocalData localdata(bgpmain.eventloop());
    Iptuple iptuple;
    BGPPeerData *pd1 = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    BGPPeer peer1(&localdata, pd1, NULL, &bgpmain);
    PeerHandler handler1("test1", &peer1, NULL, NULL);
    BGPPeerData *pd2 = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    BGPPeer peer2(&localdata, pd2, NULL, &bgpmain);
    PeerHandler handler2("test2", &peer2, NULL, NULL);

    VersionFilters policy_filters;

    // Trivial plumbing. We're not testing the RibInTable here, so
    // mostly we'll just inject directly into the PolicyTable, but we
    // need an RibInTable here to test lookup_route.
    RibInTable<IPv4> *ribin_table
	= new RibInTable<IPv4>("RIB-in", SAFI_UNICAST, &handler1);
    PolicyTableImport<IPv4> *policy_table_import
	= new PolicyTableImport<IPv4>("POLICY", SAFI_UNICAST, ribin_table,
				      policy_filters);
    ribin_table->set_next_table(policy_table_import);

    FanoutTable<IPv4> *fanout_table
	= new FanoutTable<IPv4>("FANOUT", SAFI_UNICAST, policy_table_import, 
				NULL, NULL);
    policy_table_import->set_next_table(fanout_table);

    PolicyTableExport<IPv4> *policy_table_export
	= new PolicyTableExport<IPv4>("POLICY", SAFI_UNICAST, 
				      fanout_table,
				      policy_filters, "test_neighbour");

    DebugTable<IPv4>* debug_table
       = new DebugTable<IPv4>("D1", (BGPRouteTable<IPv4>*)policy_table_export);
    policy_table_export->set_next_table(debug_table);
    fanout_table->add_next_table(policy_table_export, &handler2, 1);

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
    UNUSED(sr2);

    InternalMessage<IPv4>* msg;

    // ================================================================
    // Test1: trivial add and delete before config
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 1");
    debug_table->write_comment("ADD AND DELETE UNFILTERED");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    policy_table_import->add_route(*msg, ribin_table);
    fanout_table->get_next_message(policy_table_export);


    // delete the route
    policy_table_import->delete_route(*msg, ribin_table);
    fanout_table->get_next_message(policy_table_export);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test2: trivial add then configure then dump
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 2");
    debug_table->write_comment("ADD, CONFIG, DELETE");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);
    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    policy_table_import->add_route(*msg, ribin_table);
    fanout_table->get_next_message(policy_table_export);

    debug_table->write_comment("CONFIGURE EXPORT FILTER");

    // Configure export filter
    string filter_export_conf = "POLICY_START foo\n\
TERM_START foo2\n\
PUSH u32 200\n\
STORE 17\n\
ACCEPT\n\
TERM_END\n\
POLICY_END\n";

    policy_filters.configure(filter::EXPORT, filter_export_conf);
    
    // Configure import filter
    string filter_import_conf = "POLICY_START foo\n\
TERM_START foo2\n\
PUSH u32 100\n\
STORE 17\n\
ACCEPT\n\
TERM_END\n\
POLICY_END\n";

    policy_filters.configure(filter::IMPORT, filter_import_conf);
    
    // dump the route
    debug_table->write_comment("DO DUMP");
    debug_table->write_comment("EXPECT DELETE TO *NOT* HAVE LOCALPREF");
    debug_table->write_comment("EXPECT ADD TO HAVE LOCALPREF OF 200");
    policy_table_import->route_dump(*msg, ribin_table, NULL);
    fanout_table->get_next_message(policy_table_export);
    fanout_table->get_next_message(policy_table_export);

    // delete the route
    debug_table->write_comment("EXPECT DELETE TO HAVE LOCALPREF OF 200");
    policy_table_import->delete_route(*msg, ribin_table);
    fanout_table->get_next_message(policy_table_export);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================
    // Test2: add and delete with configured policy
    // ================================================================
    // add a route
    debug_table->write_comment("TEST 3");
    debug_table->write_comment("ADD AND DELETE FILTERED");
    debug_table->write_comment("EXPECT ROUTES TO HAVE LOCALPREF");
    sr1 = new SubnetRoute<IPv4>(net1, palist1, NULL);
    sr1->set_nexthop_resolved(true);

    msg = new InternalMessage<IPv4>(sr1, &handler1, 1);
    policy_table_import->add_route(*msg, ribin_table);
    fanout_table->get_next_message(policy_table_export);


    // delete the route
    policy_table_import->delete_route(*msg, ribin_table);
    fanout_table->get_next_message(policy_table_export);

    debug_table->write_separator();
    sr1->unref();
    delete msg;

    // ================================================================

    debug_table->write_comment("SHUTDOWN AND CLEAN UP");
    delete ribin_table;
    delete policy_table_import;
    delete policy_table_export;
    delete debug_table;
    delete palist1;
    delete palist2;
    delete palist3;

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n",
		filename.c_str());
	fprintf(stderr, "TEST POLICY FAILED\n");
	fclose(file);
	return false;
    }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST POLICY FAILED\n");
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
    ref_filename += "/test_policy.reference";
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST POLICY FAILED\n");
	fclose(file);
	return false;
    }
    
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST POLICY FAILED\n");
	fclose(file);
	return false;
    }
    fclose(file);

    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1) != 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		filename.c_str());
	fprintf(stderr, "TEST POLICY FAILED\n");
	return false;

    }
#ifndef HOST_OS_WINDOWS
    unlink(filename.c_str());
#else
    DeleteFileA(filename.c_str());
#endif
    return true;
}


