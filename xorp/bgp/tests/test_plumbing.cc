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

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "bgp.hh"
#include "test_plumbing.hh"

PlumbingTest::PlumbingTest(NextHopResolver<IPv4>& nhr_ipv4,
			   NextHopResolver<IPv6>& nhr_ipv6,
			   PolicyFilters& pfs,
			   BGPMain& bgp)
    : BGPPlumbing(SAFI_UNICAST, NULL, NULL, nhr_ipv4, nhr_ipv6, pfs, bgp)
{
}

bool
PlumbingTest::run_tests(BGPMain& bgpmain)
{
    typedef bool (PlumbingTest::*test)(BGPMain&);

    test tests[] = {&PlumbingTest::test1,
		    &PlumbingTest::test2};

    for(unsigned int i = 0; i < sizeof(tests)/sizeof(test); i++)
	if(!(this->*tests[i])(bgpmain))
	    return false;

    return true;
}

bool
PlumbingTest::test1(BGPMain& bgpmain) 
{
    Iptuple iptuple1("127.0.0.1", 179, "1.0.0.1", 179);
    Iptuple iptuple2("127.0.0.1", 179, "1.0.0.2", 179);
    IPv4 nh;

    //    EventLoop eventloop;
    LocalData local_data(bgpmain.eventloop());
    local_data.set_as(AsNum(1));

    BGPPeerData *peer_data1 = new BGPPeerData(local_data, iptuple1, AsNum(666),
					      nh, 0);
//     peer_data1->set_as(AsNum(666));
    peer_data1->set_id("1.0.0.1");
    DummyPeer dummy_peer1(&local_data, peer_data1, 0, (BGPMain *)NULL);

    printf("Adding Peering 1\n");
    //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler1("peer1", &dummy_peer1, this, 0);
    //add_peering(&dummy_peerhandler1);
    printf("Peering Added.\n");

    BGPPeerData *peer_data2 = new BGPPeerData(local_data, iptuple2, AsNum(667),
					      nh, 0);
//     peer_data2->set_as(AsNum(667));
    peer_data2->set_id("1.0.0.2");
    DummyPeer dummy_peer2(&local_data, peer_data2, 0, (BGPMain *)NULL);
 
    printf("Adding Peering 2\n");
   //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler2("peer2", &dummy_peer2, this, 0);
    //add_peering(&dummy_peerhandler2);
    printf("Peering Added.\n");

    PolicyTags pt;
    printf("------------------------------------------------------\n");
    printf("ADD A ROUTE\n");
    IPv4 nhaddr("20.20.20.1");
    NextHopAttribute<IPv4> nexthop(nhaddr);
    ASPath as_path;
    ASSegment as_seq;
    as_seq.set_type(AS_SEQUENCE);
    as_seq.add_as(AsNum(666));
    as_path.add_segment(as_seq);
    ASPathAttribute aspathatt(as_path);
    printf("****>%s<****\n", aspathatt.str().c_str());
    OriginAttribute origin(EGP);
    FPAList4Ref fpalist1 
	= new FastPathAttributeList<IPv4>(nexthop, aspathatt, origin);
    PAListRef<IPv4> pathattr1 = new PathAttributeList<IPv4>(fpalist1);
    IPv4 rtaddr1("10.10.10.0");
    IPv4Net net1(rtaddr1, 24);
    SubnetRoute<IPv4> *route1;

    InternalMessage<IPv4> *rtm1;
    printf("Adding Route 1 from peerhandler %p\n", &dummy_peerhandler1);
    add_route(net1, fpalist1, pt, &dummy_peerhandler1);
    printf("4 ****>%s<****\n", aspathatt.str().c_str());
    printf("Add done\n");

    printf("------------------------------------------------------\n");
    printf("PUSH A ROUTE\n");
    printf("Pushing Route 1\n");
    push<IPv4>(&dummy_peerhandler1);
    printf("Push done\n");

    printf("------------------------------------------------------\n");
    printf("DELETE THE ROUTE\n");
    printf("Deleting Route 1\n");
    route1 = new SubnetRoute<IPv4>(net1, pathattr1, NULL);
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    rtm1->set_push();
    delete_route(*rtm1, &dummy_peerhandler1);
    printf("Delete done\n");

    delete rtm1;
    route1->unref();

    printf("------------------------------------------------------\n");
    IPv4 rtaddr2("10.10.20.0");
    IPv4Net net2(rtaddr2, 24);
    SubnetRoute<IPv4> *route2;

    printf("Adding Route 1\n");
    add_route(net1, fpalist1, pt, &dummy_peerhandler1);
    printf("Add done\n");

    InternalMessage<IPv4> *rtm2;
    printf("Adding Route 2\n");
    add_route(net2, fpalist1, pt, &dummy_peerhandler1);
    printf("Add done\n");

    printf("Pushing Routes\n");
    push<IPv4>(&dummy_peerhandler1);
    printf("Push done\n");

    printf("------------------------------------------------------\n");
    printf("Deleting Route 2\n");
    route2 = new SubnetRoute<IPv4>(net2,
				   pathattr1, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler1, GENID_UNKNOWN);
    rtm2->set_push();
    delete_route(*rtm2, &dummy_peerhandler1);
    delete rtm2;
    route2->unref();
    printf("Delete done\n");
    printf("------------------------------------------------------\n");
    printf("Deleting Route 1\n");
    route1 = new SubnetRoute<IPv4>(net1,
				   pathattr1, NULL);
    rtm1 = new InternalMessage<IPv4>(route1, &dummy_peerhandler1, GENID_UNKNOWN);
    rtm1->set_push();
    delete_route(*rtm1, &dummy_peerhandler1);
    printf("Delete done\n");
    delete rtm1;
    route1->unref();
    printf("------------------------------------------------------\n");
    printf("Test1 of decision\n");
    printf("Adding Route 1\n");
    add_route(net1, fpalist1, pt, &dummy_peerhandler1);
    printf("Add done\n");

    printf("Pushing Routes\n");
    push<IPv4>(&dummy_peerhandler1);
    printf("Push done\n");

    IPv4 nhaddr2("20.20.20.2");
    NextHopAttribute<IPv4> nexthop2(nhaddr2);
    FPAList4Ref fpalist2 
	= new FastPathAttributeList<IPv4>(nexthop2, aspathatt, origin);
    PAListRef<IPv4> pathattr2 = new PathAttributeList<IPv4>(fpalist2);
    printf("\n\nAdding Route 2 - this should lose to route 1\n");
    add_route(net1, fpalist2, pt, &dummy_peerhandler2);
    printf("Add done\n");

    printf("Pushing Routes\n");
    push<IPv4>(&dummy_peerhandler2);
    printf("Push done\n");

    printf("\n\nDeleting Route 2 - shouldn't affect RibOut\n");
    route2 = new SubnetRoute<IPv4>(IPv4Net(rtaddr1, 24),
				   pathattr2, NULL);
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler2, GENID_UNKNOWN);
    delete_route(*rtm2, &dummy_peerhandler2);
    printf("Delete done\n");
    delete rtm2;
    route2->unref();

    printf("Pushing Routes\n");
    push<IPv4>(&dummy_peerhandler2);
    printf("Push done\n");

    IPv4 nhaddr3("20.20.19.1");
    NextHopAttribute<IPv4> nexthop3(nhaddr3);
    FPAList4Ref fpalist3
	= new FastPathAttributeList<IPv4>(nexthop3, aspathatt, origin);
    PAListRef<IPv4> pathattr3 
	= new PathAttributeList<IPv4>(fpalist3);
    printf("\n\nAdding Route 3 - this should beat route 1\n");
    add_route(net1, fpalist3, pt, &dummy_peerhandler2);
    printf("Add done\n");

    printf("Pushing Routes\n");
    push<IPv4>(&dummy_peerhandler2);
    printf("Push done\n");

    printf("\n\nDeleting Route 3 - this should cause route 1 to win again\n");
    rtm2 = new InternalMessage<IPv4>(route2, &dummy_peerhandler2, GENID_UNKNOWN);
    delete_route(*rtm2, &dummy_peerhandler2);
    printf("Delete done\n");
    delete rtm2;
    route2->unref();

    printf("Pushing Routes\n");
    push<IPv4>(&dummy_peerhandler2);
    printf("Push done\n");

    printf("------------------------------------------------------\n");

    printf("ALL TESTS DONE\n\n");
    printf("****>%s<****\n", aspathatt.str().c_str());

    return true;
}

bool
PlumbingTest::test2(BGPMain& bgpmain)
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
    Iptuple iptuple1("127.0.0.1", 179, "1.0.0.1", 179);
    Iptuple iptuple2("127.0.0.1", 179, "1.0.0.2", 179);
    IPv4 nh;

    //    EventLoop eventloop;
    LocalData local_data(bgpmain.eventloop());
    local_data.set_as(AsNum(1));

    BGPPeerData *peer_data1 = new BGPPeerData(local_data, iptuple1, AsNum(666),
					      nh, 0);
//     peer_data1->set_as(AsNum(666));
    peer_data1->set_id("1.0.0.1");
    DummyPeer dummy_peer1(&local_data, peer_data1, 0, (BGPMain *)NULL);

    printf("Adding Peering 1\n");
    //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler1("peer1", &dummy_peer1, this, 0);
    //add_peering(&dummy_peerhandler1);
    printf("Peering Added.\n");

    /*
    ** 2. Inject one route.
    */
    printf("------------------------------------------------------\n");
    printf("ADD A ROUTE\n");
    IPv4 nhaddr("20.20.20.1");
    NextHopAttribute<IPv4> nexthop(nhaddr);
    ASPath as_path;
    ASSegment as_seq;
    as_seq.set_type(AS_SEQUENCE);
    as_seq.add_as(AsNum(666));
    as_path.add_segment(as_seq);
    ASPathAttribute aspathatt(as_path);
    printf("****>%s<****\n", aspathatt.str().c_str());
    OriginAttribute origin(EGP);
    FPAList4Ref fpalist1 
	= new FastPathAttributeList<IPv4>(nexthop, aspathatt, origin);
    //    PAListRef<IPv4> pathattr1 = new PathAttributeList<IPv4>(fpalist1);
    IPv4 rtaddr1("10.10.10.0");
    IPv4Net net1(rtaddr1, 24);
    PolicyTags pt;

    printf("Adding Route 1 from peerhandler %p\n", &dummy_peerhandler1);
    add_route(net1, fpalist1, pt, &dummy_peerhandler1);
    printf("4 ****>%s<****\n", aspathatt.str().c_str());
    printf("Add done\n");

    printf("------------------------------------------------------\n");
    printf("PUSH A ROUTE\n");
    printf("Pushing Route 1\n");
    push<IPv4>(&dummy_peerhandler1);
    printf("Push done\n");
    
    /*
    ** 3. Add another peer (peer2).
    */
    BGPPeerData *peer_data2 = new BGPPeerData(local_data, iptuple2, AsNum(667),
					      nh, 0);
    peer_data2->set_id("1.0.0.2");
//     peer_data2->set_as(AsNum(667));
    DummyPeer dummy_peer2(&local_data, peer_data2, 0, (BGPMain *)NULL);
 
    printf("Adding Peering 2\n");
   //note: creating the peerhandler adds the peering.
    DummyPeerHandler dummy_peerhandler2("peer2", &dummy_peer2, this, 0);
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
				   BGPPeer *peer,
				   BGPPlumbing *plumbing_unicast,
				   BGPPlumbing *plumbing_multicast) 
    : PeerHandler(init_peername, peer, plumbing_unicast, plumbing_multicast)
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
	// The BGP constructor expects to use the finder, so start one.
	pid_t pid;

	switch(pid = fork()) {
	case 0:
	    execlp("../libxipc/xorp_finder", "xorp_finder", static_cast<char *>(NULL));
	    exit(0);
	case -1:
	    XLOG_FATAL("unable to exec xorp_finder");
	default:
	    break;
	}

	EventLoop eventloop;
	BGPMain bgpm(eventloop);

	PlumbingTest *tester;
 	tester = (PlumbingTest*)(bgpm.plumbing_unicast());
	if (!tester->run_tests(bgpm)) {
	    fprintf(stderr, "Test failed\n");
	    exit(1);
	}

	// Remember to kill the finder.
 	kill(pid, SIGTERM);

	printf("Tests successful\n");
    } catch(...) {
	xorp_catch_standard_exceptions();
    }
    xlog_stop();
    xlog_exit();

    return 0;
}

