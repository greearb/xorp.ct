// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/bgp/peer_handler_debug.hh,v 1.12 2008/11/08 06:14:37 mjh Exp $

#ifndef __BGP_PEER_HANDLER_DEBUG_HH__
#define __BGP_PEER_HANDLER_DEBUG_HH__



#include "peer_handler.hh"

class DebugPeerHandler : public PeerHandler {
public:
    DebugPeerHandler(BGPPeer *peer);

    ~DebugPeerHandler();

    int start_packet();
    /* add_route and delete_route are called to propagate a route *to*
       the RIB. */
    int add_route(const SubnetRoute<IPv4> &rt,
                          FPAList4Ref& pa_list,
                          bool ibgp, Safi safi);
    int add_route(const SubnetRoute<IPv6> &rt,
                          FPAList6Ref& pa_list,
                          bool ibgp, Safi safi);
    int replace_route(const SubnetRoute<IPv4> &old_rt, bool old_ibgp,
                              const SubnetRoute<IPv4> &new_rt, bool new_ibgp,
                              FPAList4Ref& pa_list,
                              Safi safi);
    int replace_route(const SubnetRoute<IPv6> &old_rt, bool old_ibgp,
                              const SubnetRoute<IPv6> &new_rt, bool new_ibgp,
                              FPAList6Ref& pa_list,
                              Safi safi);
    int delete_route(const SubnetRoute<IPv4> &rt,
                             FPAList4Ref& pa_list,
                             bool new_ibgp,
                             Safi safi);
    int delete_route(const SubnetRoute<IPv6> &rt,
                             FPAList6Ref& pa_list,
                             bool new_ibgp,
                             Safi safi);
    PeerOutputState push_packet();

    void set_output_file(FILE *file) {_ofile = file;}
    void set_canned_response(PeerOutputState state) {
	_canned_response = state;
    }
private:
    void print_route(const SubnetRoute<IPv4>& route, FPAList4Ref palist) const;
    void print_route(const SubnetRoute<IPv6>& route, FPAList6Ref palist) const;
    FILE *_ofile;
    PeerOutputState _canned_response;
};

#endif // __BGP_PEER_HANDLER_DEBUG_HH__
