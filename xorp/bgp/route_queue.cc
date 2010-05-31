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
#include "libxorp/xlog.h"

#include "route_queue.hh"
#include "peer_handler.hh"


template<class A>
string
RouteQueueEntry<A>::str() const {
    string s;
    switch(_op) {
    case RTQUEUE_OP_ADD:
	s = "RTQUEUE_OP_ADD";
	break;
    case RTQUEUE_OP_DELETE:
	s = "RTQUEUE_OP_DELETE";
	break;
    case RTQUEUE_OP_REPLACE_OLD:
	s = "RTQUEUE_OP_REPLACE_OLD";
	break;
    case RTQUEUE_OP_REPLACE_NEW:
	s = "RTQUEUE_OP_REPLACE_NEW";
	break;
    case RTQUEUE_OP_PUSH:
	s = "RTQUEUE_OP_PUSH";
	break;
    }
    if (_route_ref.route() != NULL)
	s += "\n" + _route_ref.route()->str();
    else
	s += "\n_route is NULL";
    if (_origin_peer != NULL)
	s += "\nOrigin Peer: " + _origin_peer->peername();
    else
	s += "\n_origin_peer is NULL";
    return s;
}
 
template class RouteQueueEntry<IPv4>;
template class RouteQueueEntry<IPv6>;
