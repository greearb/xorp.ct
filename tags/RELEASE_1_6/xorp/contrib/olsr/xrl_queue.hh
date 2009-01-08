// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/xrl_queue.hh,v 1.3 2008/10/02 21:56:37 bms Exp $

#ifndef __OLSR_XRL_QUEUE_HH__
#define __OLSR_XRL_QUEUE_HH__

class XrlIO;

/**
 * @short Helper class to queue route adds and deletes to the RIB.
 */
class XrlQueue {
public:
    XrlQueue(EventLoop& eventloop, XrlRouter& xrl_router);

    void set_io(XrlIO* io) { _io = io; }

    /**
     * Queue a route add to the RIB.
     *
     * @param ribname the name of the RIB XRL target to send to.
     * @param net the destination.
     * @param nexthop the next hop.
     * @param nexthop_id the libfeaclient ID of the outward interface.
     * @param metric the route metric.
     * @param policytags The policy tags for the route.
     */
    void queue_add_route(string ribname, const IPv4Net& net,
			 const IPv4& nexthop, uint32_t nexthop_id,
			 uint32_t metric, const PolicyTags& policytags);

    /**
     * Queue a route delete to the RIB.
     *
     * @param ribname the name of the RIB XRL target to send to.
     * @param net the destination.
     */
    void queue_delete_route(string ribname, const IPv4Net& net);

    /**
     * @return true if RIB commands are currently in flight.
     */
    bool busy();

private:
    struct Queued {
	bool add;
	string ribname;
	IPv4Net net;
	IPv4 nexthop;
	uint32_t nexthop_id;
	uint32_t metric;
	string comment;
	PolicyTags policytags;
    };

    static const size_t WINDOW = 100;

    EventLoop& eventloop() const;

    /**
     * @return true if the maximum number of XRLs flight has been exceeded.
     */
    inline bool maximum_number_inflight() const {
	return _flying >= WINDOW;
    }

    /**
     * Start the transmission of XRLs to tbe RIB.
     */
    void start();

    /**
     * The specialised method called by sendit to deal with IPv4/IPv6.
     *
     * @param q the queued command.
     * @param protocol "olsr"
     * @return True if the add/delete was queued.
     */
    bool sendit_spec(Queued& q, const char *protocol);

    /**
     * Callback method to: signal completion of a RIB command.
     *
     * @param error reference to an XrlError containing command status.
     * @param comment a textual description of the error.
     */
    void route_command_done(const XrlError& error, const string comment);

private:
    XrlIO*		_io;
    EventLoop&		_eventloop;
    XrlRouter&		_xrl_router;
    deque<Queued>	_xrl_queue;

    /**
     * Number of XRLs currently in flight.
     */
    size_t		_flying;
};

#endif // __OLSR_XRL_QUEUE_HH__
