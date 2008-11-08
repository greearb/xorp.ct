// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/peer_handler_debug.cc,v 1.15 2008/10/02 21:56:17 bms Exp $"

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


void
DebugPeerHandler::print_route(const SubnetRoute<IPv4>& route, FPAList4Ref palist) const
{
    palist->canonicalize();
    string s;
    s = "SubnetRoute:\n";
    s += "  Net: " + route.net().str() + "\n";
    s += "  PAList: " + palist->str() + "\n";
    fprintf(_ofile, s.c_str());
}

void
DebugPeerHandler::print_route(const SubnetRoute<IPv6>& route, FPAList6Ref palist) const
{
    palist->canonicalize();
    string s;
    s = "SubnetRoute:\n";
    s += "  Net: " + route.net().str() + "\n";
    s += "  PAList: " + palist->str() + "\n";
    fprintf(_ofile, s.c_str());
}

int 
DebugPeerHandler::start_packet() 
{
    debug_msg("DebugPeerHandler::start packet\n");
    fprintf(_ofile, "[PEER: START_PACKET]\n");
    return 0;
}

int 
DebugPeerHandler::add_route(const SubnetRoute<IPv4> &rt, 
			    FPAList4Ref& pa_list,
			    bool ibgp, Safi) 
{
    debug_msg("DebugPeerHandler::add_route(IPv4) %p\n", &rt);
    fprintf(_ofile, "[PEER: ADD_ROUTE, ");
    if (ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    //    fprintf(_ofile, "%s\n", rt.str().c_str());
    print_route(rt, pa_list);
    return 0;
}

int 
DebugPeerHandler::add_route(const SubnetRoute<IPv6>& rt, 
			    FPAList6Ref& pa_list,
			    bool ibgp, Safi) {
    debug_msg("DebugPeerHandler::add_route(IPv6) %p\n", &rt);
    fprintf(_ofile, "[PEER: ADD_ROUTE, ");
    if (ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    //fprintf(_ofile, "%s\n", rt.str().c_str());
    print_route(rt, pa_list);
    return 0;
}

int 
DebugPeerHandler::replace_route(const SubnetRoute<IPv4> &old_rt, 
				bool old_ibgp, 
				const SubnetRoute<IPv4> &new_rt, 
				bool new_ibgp, 
				FPAList4Ref& pa_list,
				Safi) {
    debug_msg("DebugPeerHandler::replace_route(IPv4) %p %p\n", &old_rt, &new_rt);
    UNUSED(new_rt);

    fprintf(_ofile, "[PEER: REPLACE_ROUTE]\n");
    fprintf(_ofile, "[PEER: OLD, ");
    if (old_ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    fprintf(_ofile, "%s\n", old_rt.str().c_str());
    fprintf(_ofile, "[PEER: NEW, ");
    if (new_ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    //    fprintf(_ofile, "%s\n", new_rt.str().c_str());
    print_route(new_rt, pa_list);
    return 0;
}

int 
DebugPeerHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
				bool old_ibgp, 
				const SubnetRoute<IPv6> &new_rt, 
				bool new_ibgp, 
				FPAList6Ref& pa_list,
				Safi) {
    debug_msg("DebugPeerHandler::replace_route(IPv6) %p %p\n", &old_rt, &new_rt);
    UNUSED(new_rt);

    fprintf(_ofile, "[PEER: REPLACE_ROUTE]\n");
    fprintf(_ofile, "[PEER: OLD, ");
    if (old_ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    fprintf(_ofile, "%s\n", old_rt.str().c_str());
    fprintf(_ofile, "[PEER: NEW, ");
    if (new_ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    //fprintf(_ofile, "%s\n", new_rt.str().c_str());
    print_route(new_rt, pa_list);
    return 0;
}

int 
DebugPeerHandler::delete_route(const SubnetRoute<IPv4> &rt, 
			       FPAList4Ref& pa_list,
			       bool ibgp, Safi)
{
    debug_msg("DebugPeerHandler::delete_route(IPv4) %p\n", &rt);
    fprintf(_ofile, "[PEER: DELETE_ROUTE, ");
    if (ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    //fprintf(_ofile, "%s\n", rt.str().c_str());
    print_route(rt, pa_list);
    return 0;
}

int 
DebugPeerHandler::delete_route(const SubnetRoute<IPv6>& rt,
			       FPAList6Ref& pa_list,
			       bool ibgp, Safi)
{
    debug_msg("DebugPeerHandler::delete_route(IPv6) %p\n", &rt);
    fprintf(_ofile, "[PEER: DELETE_ROUTE, ");
    if (ibgp)
	fprintf(_ofile, "IBGP]\n");
    else
	fprintf(_ofile, "EBGP]\n");
    //fprintf(_ofile, "%s\n", rt.str().c_str());
    print_route(rt, pa_list);
    return 0;
}

PeerOutputState
DebugPeerHandler::push_packet()
{
    debug_msg("DebugPeerHandler::push packet\n");
    fprintf(_ofile, "[PEER: PUSH_PACKET]\n");
    return  _canned_response;
}
