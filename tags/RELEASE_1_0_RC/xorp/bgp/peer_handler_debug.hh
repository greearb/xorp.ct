// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/bgp/peer_handler_debug.hh,v 1.3 2003/10/23 03:10:05 atanu Exp $

#ifndef __BGP_PEER_HANDLER_DEBUG_HH__
#define __BGP_PEER_HANDLER_DEBUG_HH__

#include <queue>

#include "peer_handler.hh"

class DebugPeerHandler : public PeerHandler {
public:
    DebugPeerHandler(BGPPeer *peer);

    ~DebugPeerHandler();

    int start_packet(bool ibgp);
    /* add_route and delete_route are called to propagate a route *to*
       the RIB. */
    int add_route(const SubnetRoute<IPv4> &rt, Safi safi);
    int add_route(const SubnetRoute<IPv6> &rt, Safi safi);
    int replace_route(const SubnetRoute<IPv4> &old_rt,
		      const SubnetRoute<IPv4> &new_rt, Safi safi);
    int replace_route(const SubnetRoute<IPv6> &old_rt,
		      const SubnetRoute<IPv6> &new_rt, Safi safi);
    int delete_route(const SubnetRoute<IPv4> &rt, Safi safi);
    int delete_route(const SubnetRoute<IPv6> &rt, Safi safi);
    PeerOutputState push_packet();

    void set_output_file(FILE *file) {_ofile = file;}
    void set_canned_response(PeerOutputState state) {
	_canned_response = state;
    }
private:
    bool _ibgp; //did the current update message originate in IBGP?
    FILE *_ofile;
    PeerOutputState _canned_response;
};

#endif // __BGP_PEER_HANDLER_DEBUG_HH__
