// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/peer_handler_debug.cc,v 1.3 2003/10/03 00:26:59 atanu Exp $"

//#define DEBUG_LOGGING
#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "peer_handler_debug.hh"

DebugPeerHandler::DebugPeerHandler(BGPPeer *peer) 
    : PeerHandler("DebugPeerHandler", peer, NULL, NULL)
{
}

DebugPeerHandler::~DebugPeerHandler() 
{
}

int 
DebugPeerHandler::start_packet(bool ibgp) 
{
    _ibgp = ibgp;
    debug_msg("DebugPeerHandler::start packet\n");
    if (ibgp)
	fprintf(_ofile, "[PEER: START_PACKET, IBGP]\n");
    else
	fprintf(_ofile, "[PEER: START_PACKET, EBGP]\n");
    return 0;
}

int 
DebugPeerHandler::add_route(const SubnetRoute<IPv4> &rt) 
{
    debug_msg("DebugPeerHandler::add_route(IPv4) %x\n", (u_int)(&rt));
    fprintf(_ofile, "[PEER: ADD_ROUTE]\n");
    fprintf(_ofile, "%s\n", rt.str().c_str());
    return 0;
}

int 
DebugPeerHandler::add_route(const SubnetRoute<IPv6>& rt) {
    debug_msg("DebugPeerHandler::add_route(IPv6) %p\n", &rt);
    fprintf(_ofile, "[PEER: ADD_ROUTE]\n");
    fprintf(_ofile, "%s\n", rt.str().c_str());
    return 0;
}

int 
DebugPeerHandler::replace_route(const SubnetRoute<IPv4> &old_rt,
			     const SubnetRoute<IPv4> &new_rt) {
    debug_msg("DebugPeerHandler::replace_route(IPv4) %p %p\n", &old_rt, &new_rt);
    fprintf(_ofile, "[PEER: REPLACE_ROUTE]\n");
    fprintf(_ofile, "[PEER: OLD]\n");
    fprintf(_ofile, "%s\n", old_rt.str().c_str());
    fprintf(_ofile, "[PEER: NEW]\n");
    fprintf(_ofile, "%s\n", old_rt.str().c_str());
    return 0;
}

int 
DebugPeerHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
			     const SubnetRoute<IPv6> &new_rt) {
    debug_msg("DebugPeerHandler::replace_route(IPv6) %p %p\n", &old_rt, &new_rt);
    fprintf(_ofile, "[PEER: REPLACE_ROUTE]\n");
    fprintf(_ofile, "[PEER: OLD]\n");
    fprintf(_ofile, "%s\n", old_rt.str().c_str());
    fprintf(_ofile, "[PEER: NEW]\n");
    fprintf(_ofile, "%s\n", old_rt.str().c_str());
    return 0;
}

int 
DebugPeerHandler::delete_route(const SubnetRoute<IPv4> &rt)
{
    debug_msg("DebugPeerHandler::delete_route(IPv4) %x\n", (u_int)(&rt));
    fprintf(_ofile, "[PEER: DELETE_ROUTE]\n");
    fprintf(_ofile, "%s\n", rt.str().c_str());
    return 0;
}

int 
DebugPeerHandler::delete_route(const SubnetRoute<IPv6>& rt)
{
    debug_msg("DebugPeerHandler::delete_route(IPv6) %p\n", &rt);
    fprintf(_ofile, "[PEER: DELETE_ROUTE]\n");
    fprintf(_ofile, "%s\n", rt.str().c_str());
    return 0;
}

PeerOutputState
DebugPeerHandler::push_packet()
{
    debug_msg("DebugPeerHandler::push packet\n");
    fprintf(_ofile, "[PEER: PUSH_PACKET]\n");
    return  _canned_response;
}
