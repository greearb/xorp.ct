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

#ident "$XORP: xorp/bgp/plumbing_test.cc,v 1.8 2003/01/29 23:38:13 rizzo Exp $"

#include "fcntl.h"

#include "bgp_module.h"
#include "main.hh"
#include "plumbing_test.hh"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"

PlumbingTest::PlumbingTest() 
    //dummy args to BGPPlumbing because we'll not use this constructor
    : BGPPlumbing(NULL, NULL) 
{
}

bool
PlumbingTest::run_tests()
{
    typedef bool (PlumbingTest::*test)();

    test tests[] = {&PlumbingTest::test1,
		    &PlumbingTest::test2};

    for(unsigned int i = 0; i < sizeof(tests)/sizeof(test); i++)
	if(!(this->*tests[i])())
	    return false;

    return true;
}

bool
PlumbingTest::test1() 
{
    LocalData local_data;
    local_data.set_as(my_AS_number());

    BGPPeerData *peer_data1 = new BGPPeerData();
    peer_data1->set_as(AsNum(666));
    peer_data1->set_internal_peer(true);
    DummyPeer dummy_peer1(&local_data, peer_data1, 0, (BGPMain *)NULL);

    printf("Adding Peering 1\n");
    //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler1("peer1", &dummy_peer1, this);
    //add_peering(&dummy_peerhandler1);
    printf("Peering Added.\n");

    BGPPeerData *peer_data2 = new BGPPeerData();;
    peer_data2->set_as(AsNum(667));
    peer_data1->set_internal_peer(true);
    DummyPeer dummy_peer2(&local_data, peer_data2, 0, (BGPMain *)NULL);
 
    printf("Adding Peering 2\n");
   //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler2("peer2", &dummy_peer2, this);
    //add_peering(&dummy_peerhandler2);
    printf("Peering Added.\n");

    printf("------------------------------------------------------\n");
    printf("ADD A ROUTE\n");
    IPv4 nhaddr("20.20.20.1");
    NextHopAttribute<IPv4> nexthop(nhaddr);
    AsPath as_path;
    AsSegment as_seq;
    as_seq.set_type(AS_SEQUENCE);
    as_seq.add_as(AsNum(666));
    as_path.add_segment(as_seq);
    ASPathAttribute aspathatt(as_path);
    printf("****>%s<****\n", aspathatt.str().c_str());
    OriginAttribute origin(EGP);
    PathAttributeList<IPv4> pathattr1(nexthop, aspathatt, origin);
    IPv4 rtaddr1("10.10.10.0");
    SubnetRoute<IPv4> *route1;
    route1 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				      &pathattr1, NULL);

    InternalMessage<IPv4> *rtm1;
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    printf("Adding Route 1 from peerhandler %x\n", 
	   (u_int)(&dummy_peerhandler1));
    add_route(*rtm1, &dummy_peerhandler1);
    printf("4 ****>%s<****\n", aspathatt.str().c_str());
    printf("Add done\n");

    printf("------------------------------------------------------\n");
    printf("PUSH A ROUTE\n");
    printf("Pushing Route 1\n");
    push(&dummy_peerhandler1);
    printf("Push done\n");

    printf("------------------------------------------------------\n");
    printf("DELETE THE ROUTE\n");
    printf("Deleting Route 1\n");
    rtm1->set_push();
    delete_route(*rtm1, &dummy_peerhandler1);
    printf("Delete done\n");

    delete rtm1;
    delete route1;

    printf("------------------------------------------------------\n");
    IPv4 rtaddr2("10.10.20.0");
    SubnetRoute<IPv4> *route2;

    printf("Adding Route 1\n");
    route1 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   &pathattr1, NULL);
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    add_route(*rtm1, &dummy_peerhandler1);
    printf("Add done\n");
    delete rtm1;
    delete route1;

    InternalMessage<IPv4> *rtm2;
    printf("Adding Route 2\n");
    route2 = new SubnetRoute<IPv4>(IPv4Net(rtaddr2, 24),
				   &pathattr1, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler1, GENID_UNKNOWN);
    add_route(*rtm2, &dummy_peerhandler1);
    printf("Add done\n");
    delete rtm2;
    delete route2;

    printf("Pushing Routes\n");
    push(&dummy_peerhandler1);
    printf("Push done\n");

    printf("------------------------------------------------------\n");
    printf("Deleting Route 2\n");
    route2 = new SubnetRoute<IPv4>(IPv4Net(rtaddr2, 24),
				   &pathattr1, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler1, GENID_UNKNOWN);
    rtm2->set_push();
    delete_route(*rtm2, &dummy_peerhandler1);
    delete rtm2;
    delete route2;
    printf("Delete done\n");
    printf("------------------------------------------------------\n");
    printf("Deleting Route 1\n");
    route1 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   &pathattr1, NULL);
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    rtm1->set_push();
    delete_route(*rtm1, &dummy_peerhandler1);
    printf("Delete done\n");
    delete rtm1;
    delete route1;
    printf("------------------------------------------------------\n");
    printf("Test1 of decision\n");
    printf("Adding Route 1\n");
    route1 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				      &pathattr1, NULL);
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    add_route(*rtm1, &dummy_peerhandler1);
    printf("Add done\n");
    delete rtm1;
    delete route1;

    printf("Pushing Routes\n");
    push(&dummy_peerhandler1);
    printf("Push done\n");

    IPv4 nhaddr2("20.20.20.2");
    NextHopAttribute<IPv4> nexthop2(nhaddr2);
    PathAttributeList<IPv4> pathattr2(nexthop2, aspathatt, origin);
    printf("\n\nAdding Route 2 - this should lose to route 1\n");
    route2 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   &pathattr2, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler2, GENID_UNKNOWN);
    add_route(*rtm2, &dummy_peerhandler2);
    printf("Add done\n");
    delete rtm2;
    delete route2;

    printf("Pushing Routes\n");
    push(&dummy_peerhandler2);
    printf("Push done\n");

    printf("\n\nDeleting Route 2 - shouldn't affect RibOut\n");
    route2 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   &pathattr2, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler2, GENID_UNKNOWN);
    delete_route(*rtm2, &dummy_peerhandler2);
    printf("Delete done\n");
    delete rtm2;
    delete route2;

    printf("Pushing Routes\n");
    push(&dummy_peerhandler2);
    printf("Push done\n");

    IPv4 nhaddr3("20.20.19.1");
    NextHopAttribute<IPv4> nexthop3(nhaddr3);
    PathAttributeList<IPv4> pathattr3(nexthop3, aspathatt, origin);
    printf("\n\nAdding Route 3 - this should beat route 1\n");
    route2 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   &pathattr3, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler2, GENID_UNKNOWN);
    add_route(*rtm2, &dummy_peerhandler2);
    printf("Add done\n");
    delete rtm2;

    printf("Pushing Routes\n");
    push(&dummy_peerhandler2);
    printf("Push done\n");

    printf("\n\nDeleting Route 3 - this should cause route 1 to win again\n");
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler2, GENID_UNKNOWN);
    delete_route(*rtm2, &dummy_peerhandler2);
    printf("Delete done\n");
    delete rtm2;
    delete route2;

    printf("Pushing Routes\n");
    push(&dummy_peerhandler2);
    printf("Push done\n");

    printf("------------------------------------------------------\n");

    printf("ALL TESTS DONE\n\n");
    printf("****>%s<****\n", aspathatt.str().c_str());

    return true;
}

bool
PlumbingTest::test2()
{
    /*
    ** 1. Create a single peer (peer1).
    ** 2. Inject one route.
    ** 3. Add another peer (peer2).
    ** 4. Verify that the injected route emerges by inspection.
    */

    /*
    ** 1. Create a single peer (peer1).
    */
    LocalData local_data;
    local_data.set_as(my_AS_number());

    BGPPeerData *peer_data1 = new BGPPeerData();
    peer_data1->set_as(AsNum(666));
    DummyPeer dummy_peer1(&local_data, peer_data1, 0, (BGPMain *)NULL);

    printf("Adding Peering 1\n");
    //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler1("peer1", &dummy_peer1, this);
    //add_peering(&dummy_peerhandler1);
    printf("Peering Added.\n");

    /*
    ** 2. Inject one route.
    */
    printf("------------------------------------------------------\n");
    printf("ADD A ROUTE\n");
    IPv4 nhaddr("20.20.20.1");
    NextHopAttribute<IPv4> nexthop(nhaddr);
    AsPath as_path;
    AsSegment as_seq;
    as_seq.set_type(AS_SEQUENCE);
    as_seq.add_as(AsNum(666));
    as_path.add_segment(as_seq);
    ASPathAttribute aspathatt(as_path);
    printf("****>%s<****\n", aspathatt.str().c_str());
    OriginAttribute origin(EGP);
    PathAttributeList<IPv4> pathattr1(nexthop, aspathatt, origin);
    IPv4 rtaddr1("10.10.10.0");
    SubnetRoute<IPv4> *route1;
    route1 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   &pathattr1, NULL);

    InternalMessage<IPv4> *rtm1;
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    printf("Adding Route 1 from peerhandler %x\n", 
	   (u_int)(&dummy_peerhandler1));
    add_route(*rtm1, &dummy_peerhandler1);
    delete rtm1;
    delete route1;
    printf("4 ****>%s<****\n", aspathatt.str().c_str());
    printf("Add done\n");

    printf("------------------------------------------------------\n");
    printf("PUSH A ROUTE\n");
    printf("Pushing Route 1\n");
    push(&dummy_peerhandler1);
    printf("Push done\n");
    
    /*
    ** 3. Add another peer (peer2).
    */
    BGPPeerData *peer_data2 = new BGPPeerData();;
    peer_data2->set_as(AsNum(667));
    DummyPeer dummy_peer2(&local_data, peer_data2, 0, (BGPMain *)NULL);
 
    printf("Adding Peering 2\n");
   //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler2("peer2", &dummy_peer2, this);
    //add_peering(&dummy_peerhandler2);
    printf("Peering Added.\n");

    return true;
}

DummyPeer::DummyPeer(LocalData *ld, BGPPeerData *pd, SocketClient *sock, 
		     BGPMain *m) : BGPPeer(ld, pd, sock, m)
{
}

PeerOutputState
DummyPeer::send_update_message(const UpdatePacket& p)
{
    printf("send_update_message %s\n", p.str().c_str());
    return PEER_OUTPUT_OK;
}

DummyPeerHandler::DummyPeerHandler(const string &init_peername,
				   BGPPeer *peer, BGPPlumbing *plumbing) 
    : PeerHandler(init_peername, peer, plumbing)
{
}


/* **************** main *********************** */

int main(int /* argc */, char *argv[])
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try { 
	BGPMain bgpm;

	PlumbingTest *tester;
	tester = (PlumbingTest*)(bgpm.plumbing());
	if (!tester->run_tests()) {
	    fprintf(stderr, "Test failed\n");
	    exit(1);
	}
	printf("Tests successful\n");
    } catch(...) {
	xorp_catch_standard_exceptions();
    }
    xlog_stop();
    xlog_exit();

    return 0;
}
