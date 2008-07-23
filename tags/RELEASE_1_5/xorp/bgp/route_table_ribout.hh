// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/route_table_ribout.hh,v 1.18 2008/01/04 03:15:26 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_RIBOUT_HH__
#define __BGP_ROUTE_TABLE_RIBOUT_HH__

#include <set>
#include <list>

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "route_table_base.hh"
#include "route_queue.hh"
#include "parameter.hh"


template<class A>
class RibOutTable : public BGPRouteTable<A>  {
public:
    RibOutTable(string tablename,
		Safi safi,
		BGPRouteTable<A> *parent,
		PeerHandler *peer);
    ~RibOutTable();
    void print_queue(const list<const RouteQueueEntry<A> *>& queue) const;
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);

    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid) const;

    RouteTableType type() const {return RIB_OUT_TABLE;}
    string str() const;
    int dump_entire_table() { abort(); return 0; }

    /* mechanisms to implement flow control in the output plumbing */
    void wakeup();
    bool pull_next_route();

    /* output_no_longer_busy is called asynchronously by the peer
       handler when the state of the output queue goes from busy to
       non-busy.  When the output queue has been busy we will have
       signalled back upstream to stop further routes arriving, so we
       need to go and retrieve the queued routes */
    void output_no_longer_busy();

    bool get_next_message(BGPRouteTable<A> */*next_table*/) {
	abort();
	return false;
    }

    void reschedule_self();

    void peering_went_down(const PeerHandler *peer, uint32_t genid,
			   BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
			       BGPRouteTable<A> *caller);
    void peering_came_up(const PeerHandler *peer, uint32_t genid,
			 BGPRouteTable<A> *caller);
private:
    //the queue that builds, prior to receiving a push, so we can
    //send updates to our peers atomically
    list <const RouteQueueEntry<A> *> _queue;

    PeerHandler *_peer;
    bool _peer_busy;
    bool _peer_is_up;
    XorpTask _pull_routes_task;
};

#endif // __BGP_ROUTE_TABLE_RIBOUT_HH__
