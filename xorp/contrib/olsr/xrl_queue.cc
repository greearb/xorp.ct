// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"




#include "olsr.hh"

#include "xrl/interfaces/rib_xif.hh"
//#include "xrl_io.hh"

#include "libxipc/xrl_router.hh"
#include "xrl_queue.hh"

XrlQueue::XrlQueue(EventLoop& eventloop, XrlRouter& xrl_router)
    : _io(0), _eventloop(eventloop), _xrl_router(xrl_router), _flying(0)
{
}

EventLoop&
XrlQueue::eventloop() const
{
    return _eventloop;
}

void
XrlQueue::queue_add_route(string ribname, const IPv4Net& net,
			  const IPv4& nexthop, uint32_t nexthop_id,
			  uint32_t metric, const PolicyTags& policytags)
{
    Queued q;

    q.add = true;
    q.ribname = ribname;
    q.net = net;
    q.nexthop = nexthop;
    q.nexthop_id = nexthop_id;
    q.metric = metric;
    q.comment =
	c_format("add_route: ribname %s net %s nexthop %s",
		 ribname.c_str(),
		 cstring(net),
		 cstring(nexthop));
    q.policytags = policytags;

    _xrl_queue.push_back(q);

    start();
}

void
XrlQueue::queue_delete_route(string ribname, const IPv4Net& net)
{
    Queued q;

    q.add = false;
    q.ribname = ribname;
    q.net = net;
    q.comment = c_format("delete_route: ribname %s net %s",
			 ribname.c_str(),
			 cstring(net));

    _xrl_queue.push_back(q);

    start();
}

bool
XrlQueue::busy()
{
    return 0 != _flying;
}

void
XrlQueue::start()
{
    if (maximum_number_inflight())
	return;

    // Now there are no outstanding XRLs try and send as many of the queued
    // route commands as possible as possible.
    for (;;) {
	debug_msg("queue length %u\n", XORP_UINT_CAST(_xrl_queue.size()));

	if (_xrl_queue.empty()) {
	    debug_msg("Output no longer busy\n");
#if	0
	    _rib_ipc_handler.output_no_longer_busy();
#endif
	    return;
	}

	deque<XrlQueue::Queued>::const_iterator qi;

	qi = _xrl_queue.begin();

	XLOG_ASSERT(qi != _xrl_queue.end());

	Queued q = *qi;

	const char *protocol = "olsr";
	bool sent = sendit_spec(q, protocol);

	if (sent) {
	    _flying++;
	    _xrl_queue.pop_front();
	    if (maximum_number_inflight())
		return;
	    continue;
	}

	// We expect that the send may fail if the socket buffer is full.
	// It should therefore be the case that we have some route
	// adds/deletes in flight. If _flying is zero then something
	// unexpected has happended. We have no outstanding sends and
	// still its gone to poo.

	XLOG_ASSERT(0 != _flying);

	// We failed to send the last XRL. Don't attempt to send any more.
	return;
    }
}

bool
XrlQueue::sendit_spec(Queued& q, const char* protocol)
{
    bool sent;
    bool unicast = true;
    bool multicast = false;

    XrlRibV0p1Client rib(&_xrl_router);

    if (q.add) {
	debug_msg("adding route from %s peer to rib\n", protocol);
	sent = rib.
	    send_add_route4(q.ribname.c_str(),
			    protocol,
			    unicast, multicast,
			    q.net, q.nexthop, q.metric,
			    q.policytags.xrl_atomlist(),
			    callback(this,
				     &XrlQueue::route_command_done,
				     q.comment));
	if (!sent)
	    XLOG_WARNING("scheduling add route %s failed",
			 cstring(q.net));
    } else {
	debug_msg("deleting route from %s peer to rib\n", protocol);

	sent = rib.
	    send_delete_route4(q.ribname.c_str(),
			       protocol,
			       unicast, multicast,
			       q.net,
			       callback(this,
					&XrlQueue::route_command_done,
					q.comment));
	if (!sent)
	    XLOG_WARNING("scheduling delete route %s failed",
			 cstring(q.net));
    }

    return sent;
}

void
XrlQueue::route_command_done(const XrlError& error,
			     const string comment)
{
    _flying--;
    debug_msg("callback %s %s\n", comment.c_str(), cstring(error));

    switch (error.error_code()) {
    case OKAY:
	break;

    case REPLY_TIMED_OUT:
	// We should really be using a reliable transport where
	// this error cannot happen. But it has so lets retry if we can.
	XLOG_WARNING("callback: %s %s",  comment.c_str(), cstring(error));
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("callback: %s %s",  comment.c_str(), cstring(error));
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
//	_olsr.finder_death(__FILE__, __LINE__);
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	// XXX - Make this XLOG_FATAL when this has been debugged.
	// TODO 40.
	XLOG_ERROR("callback: %s %s",  comment.c_str(), cstring(error));
	break;
    }

    // Fire off more requests.
    start();
}
