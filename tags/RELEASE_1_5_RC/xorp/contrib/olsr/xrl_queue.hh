// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP$

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
