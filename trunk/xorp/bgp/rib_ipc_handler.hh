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

// $XORP: xorp/bgp/rib_ipc_handler.hh,v 1.23 2003/12/19 07:39:10 atanu Exp $

#ifndef __BGP_RIB_IPC_HANDLER_HH__
#define __BGP_RIB_IPC_HANDLER_HH__

#include <deque>

#include "peer_handler.hh"
#include "plumbing.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/rib_xif.hh"

class RibIpcHandler;
class EventLoop;

template <class A>
class XrlQueue {
public:
    XrlQueue(RibIpcHandler *rib_ipc_handler, XrlStdRouter *xrl_router,
	     BGPMain *_bgp);

    void queue_add_route(string ribname, bool ibgp, Safi, const IPNet<A>& net,
			 const A& nexthop);

    void queue_delete_route(string ribname, bool ibgp, Safi,
			    const IPNet<A>& net);

    bool busy();
private:
    RibIpcHandler *_rib_ipc_handler;
    XrlStdRouter *_xrl_router;
    BGPMain *_bgp;

    struct Queued {
	bool add;
	string ribname;
	bool ibgp;
	Safi safi;
	IPNet<A> net;
	A nexthop;
	string comment;
    };

    deque <Queued> _xrl_queue;
    size_t _flying; //XRLs currently in flight

    /**
     * Start the transmission of XRLs to tbe RIB.
     */
    void start();

    /**
     * The specialised method called by sendit to deal with IPv4/IPv6.
     *
     * @param q the queued command.
     * @param rib XRL handle to RIB
     * @param bgp "ibgp" or "ebgp".
     * @return True if the add/delete was queued.
     */
    bool sendit_spec(Queued& q, XrlRibV0p1Client& rib, const char *bgp);

    inline EventLoop& eventloop() { return _rib_ipc_handler->eventloop(); }

    void route_command_done(const XrlError& error, const string comment);
};

/*
 * RibIpcHandler's job is to convert to and from XRLs, and to handle the
 * XRL state machine for talking to the RIB process 
 */

class RibIpcHandler : public PeerHandler {
public:
    RibIpcHandler(XrlStdRouter *xrl_router, BGPMain& bgp);

    ~RibIpcHandler();

    /**
    * Set the rib's name, allows for having a dummy rib or not having
    * a RIB at all.
    */ 
    bool register_ribname(const string& r);
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
    void rib_command_done(const XrlError& error, const char *comment);
    PeerOutputState push_packet();

    void set_plumbing(BGPPlumbing *plumbing_unicast,
		      BGPPlumbing *plumbing_multicast) {
	_plumbing_unicast = plumbing_unicast;
	_plumbing_multicast = plumbing_multicast;
    }

    /**
     * @return true if routes are being sent to the RIB.
     */
    bool busy() {
	return _v4_queue.busy() || _v4_queue.busy();
    }

    /*
    ** Insert fake static route into routing table.
    */
    bool insert_static_route(const OriginType origin, const AsPath& aspath,
		      const IPv4& next_hop, const IPNet<IPv4>& nlri);
    /*
    ** Delete static route from routing table.
    */
    bool delete_static_route(const IPNet<IPv4>& nlri);

    //fake a zero IP address so the RIB IPC handler gets listed first
    //in the Fanout Table.
    const IPv4& id() const		{ return _fake_id; }

private:
    bool unregister_rib();

    string _ribname;
    XrlStdRouter *_xrl_router;

    bool _ibgp; //did the current update message originate in IBGP?

    XrlQueue<IPv4> _v4_queue;
    XrlQueue<IPv6> _v6_queue;
    IPv4 _fake_id;
};

#endif // __BGP_RIB_IPC_HANDLER_HH__
