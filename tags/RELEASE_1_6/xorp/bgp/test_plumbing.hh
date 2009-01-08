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

// $XORP: xorp/bgp/test_plumbing.hh,v 1.13 2008/11/08 06:14:41 mjh Exp $

#ifndef __BGP_TEST_PLUMBING_HH__
#define __BGP_TEST_PLUMBING_HH__

#include "plumbing.hh"
#include "peer.hh"

class DummyPeer : public BGPPeer {
public:
    DummyPeer(LocalData *ld, BGPPeerData *pd, SocketClient *sock, 
	      BGPMain *m);
    PeerOutputState send_update_message(const UpdatePacket& p);
};

class DummyPeerHandler : public PeerHandler {
public:
    DummyPeerHandler(const string &peername, 
		     BGPPeer *peer, BGPPlumbing *plumbing_unicast,
		     BGPPlumbing *plumbing_multicast);
};

class PlumbingTest : public BGPPlumbing {
public:
    PlumbingTest(NextHopResolver<IPv4>& nhr_ipv4,
		 NextHopResolver<IPv6>& nhr_ipv6,
		 PolicyFilters& pfs,
		 BGPMain& bgp);
    bool run_tests(BGPMain& bgpmain);
    bool test1(BGPMain& bgpmain);
    bool test2(BGPMain& bgpmain);
};

#endif // __BGP_TEST_PLUMBING_HH__
