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

#ident "$XORP: xorp/bgp/route_queue.cc,v 1.6 2003/10/03 00:26:59 atanu Exp $"

#include "config.h"
#include "bgp_module.h"
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
