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

// $XORP: xorp/bgp/rib_ipc_handler.hh,v 1.3 2003/02/05 07:22:14 mjh Exp $

#ifndef __BGP_RIB_IPC_HANDLER_HH__
#define __BGP_RIB_IPC_HANDLER_HH__

#include <queue>

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
    XrlQueue(RibIpcHandler *rib_ipc_handler, XrlStdRouter *xrl_router);

    void queue_add_route(string ribname, bool ibgp, const IPNet<A>& net,
			 const A& nexthop);

    void queue_delete_route(string ribname, bool ibgp, const IPNet<A>& net);

    bool busy();
private:
    RibIpcHandler *_rib_ipc_handler;
    XrlStdRouter *_xrl_router;

    struct Queued {
	bool add;
	string ribname;
	bool ibgp;
	IPNet<A> net;
	A nexthop;
    };

    queue <Queued> _xrl_queue;
    static const int FLYING_LIMIT = 1;// XRL's allowed in flight at one time.
    int _flying;
    bool _previously_succeeded; //true if we've previously been
                                //successful in communicating with the RIB.
    uint32_t _errors; //no. of consecutive non-fatal errors where we retried.
    XorpTimer _delayed_send_timer; //used to resend after a certain delay.

    void sendit();
    void delayed_send(uint32_t delay_ms);
    EventLoop& get_eventloop() {return _rib_ipc_handler->eventloop();}

    void callback(const XrlError& error, const char *comment);
};

/*
 * RibIpcHandler's job is to convert to and from XRLs, and to handle the
 * XRL state machine for talking to the RIB process 
 */

class RibIpcHandler : public PeerHandler {
public:
    RibIpcHandler(XrlStdRouter *xrl_router, EventLoop& eventloop);

    ~RibIpcHandler();

    /**
    * Set the rib's name, allows for having a dummy rib or not having
    * a RIB at all.
    */ 
    bool register_ribname(const string& r);
    int start_packet(bool ibgp);
    /* add_route and delete_route are called to propagate a route *to*
       the RIB. */
    int add_route(const SubnetRoute<IPv4> &rt);
    int add_route(const SubnetRoute<IPv6> &rt);
    int replace_route(const SubnetRoute<IPv4> &old_rt,
		      const SubnetRoute<IPv4> &new_rt);
    int replace_route(const SubnetRoute<IPv6> &old_rt,
		      const SubnetRoute<IPv6> &new_rt);
    int delete_route(const SubnetRoute<IPv4> &rt);
    int delete_route(const SubnetRoute<IPv6> &rt);
    void callback(const XrlError& error, const char *comment);
    PeerOutputState push_packet();

    void set_plumbing(BGPPlumbing *plumbing) {_plumbing = plumbing;}
    /*
    ** Insert fake static route into routing table.
    */
    bool insert_static_route(const OriginType origin, const AsPath& aspath,
		      const IPv4& next_hop, const IPNet<IPv4>& nlri);
    /*
    ** Delete static route from routing table.
    */
    bool delete_static_route(const IPNet<IPv4>& nlri);
    EventLoop& eventloop() {return _eventloop;}
private:
    string _ribname;
    XrlStdRouter *_xrl_router;
    EventLoop& _eventloop;

    bool _ibgp; //did the current update message originate in IBGP?
    bool unregister_rib();

    XrlQueue<IPv4> _v4_queue;
    XrlQueue<IPv6> _v6_queue;
};

#endif // __BGP_RIB_IPC_HANDLER_HH__
