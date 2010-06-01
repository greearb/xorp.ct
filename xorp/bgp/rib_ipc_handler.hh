// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/bgp/rib_ipc_handler.hh,v 1.51 2008/11/08 06:14:37 mjh Exp $

#ifndef __BGP_RIB_IPC_HANDLER_HH__
#define __BGP_RIB_IPC_HANDLER_HH__

#include <deque>

#include "peer_handler.hh"
#include "plumbing.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxipc/xrl_std_router.hh"

#include "policy/backend/policytags.hh"

class RibIpcHandler;
class EventLoop;

template <class A>
class XrlQueue {
public:
    XrlQueue(RibIpcHandler &rib_ipc_handler, XrlStdRouter &xrl_router,
	     BGPMain &_bgp);

    void queue_add_route(string ribname, bool ibgp, Safi, const IPNet<A>& net,
			 const A& nexthop, const PolicyTags& policytags);

    void queue_delete_route(string ribname, bool ibgp, Safi,
			    const IPNet<A>& net);

    bool busy();
private:
    static const size_t XRL_HIWAT = 100;	// Maximum number of XRLs
						// allowed in flight.
    static const size_t XRL_LOWAT = 10;		// Low watermark for XRL
						// in-flight flow control
						// hysteresis.

    RibIpcHandler &_rib_ipc_handler;
    XrlStdRouter &_xrl_router;
    BGPMain &_bgp;

    struct Queued {
	bool add;
	string ribname;
	bool ibgp;
	Safi safi;
	IPNet<A> net;
	A nexthop;
	string comment;
	PolicyTags policytags;
    };

    deque <Queued> _xrl_queue;
    size_t _flying; //XRLs currently in flight
    bool _flow_controlled;

    /**
     * Flow control hysteresis
     */
    bool flow_controlled() {
	if (_flying >= XRL_HIWAT)
	    _flow_controlled = true;
	else if (_flying <= XRL_LOWAT)
	    _flow_controlled = false;
	return _flow_controlled;
    }

    /**
     * Start the transmission of XRLs to tbe RIB.
     */
    void start();

    /**
     * The specialised method called by sendit to deal with IPv4/IPv6.
     *
     * @param q the queued command.
     * @param bgp "ibg"p or "ebgp".
     * @return True if the add/delete was queued.
     */
    bool sendit_spec(Queued& q, const char *bgp);

    EventLoop& eventloop() const;

    void route_command_done(const XrlError& error, const string comment);
};

/*
 * RibIpcHandler's job is to convert to and from XRLs, and to handle the
 * XRL state machine for talking to the RIB process 
 */

class RibIpcHandler : public PeerHandler {
public:
    RibIpcHandler(XrlStdRouter& xrl_router, BGPMain& bgp);

    ~RibIpcHandler();

    /**
    * Set the rib's name, allows for having a dummy rib or not having
    * a RIB at all.
    */ 
    bool register_ribname(const string& r);
    int start_packet();
    /* add_route and delete_route are called to propagate a route *to*
       the RIB. */
    int add_route(const SubnetRoute<IPv4> &rt, 
		  FPAList4Ref& pa_list,   
		  bool ibgp, Safi safi);
    int replace_route(const SubnetRoute<IPv4> &old_rt, bool old_ibgp, 
		      const SubnetRoute<IPv4> &new_rt, bool new_ibgp, 
		      FPAList4Ref& pa_list,   
		      Safi safi);
    int delete_route(const SubnetRoute<IPv4> &rt, 
		     FPAList4Ref& pa_list,   
		     bool ibgp, Safi safi);
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
	return (_v4_queue.busy()
#ifdef HAVE_IPV6
		|| _v6_queue.busy()
#endif
	    );
    }

    /**
     * Originate an IPv4 route
     *
     * @param origin of the path information
     * @param aspath
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     * @param policytags policy-tags associated with the route
     *
     * @return true on success
     */
    bool originate_route(const OriginType origin,
			 const ASPath& aspath,
			 const IPv4Net& nlri,
			 const IPv4& next_hop,
			 const bool& unicast,
			 const bool& multicast,
			 const PolicyTags& policytags);

    /**
     * Withdraw an IPv4 route
     *
     * @param nlri subnet to withdraw
     * @param unicast if true withdraw from unicast routing table
     * @param multicast if true withdraw from multicast routing table
     *
     * @return true on success
     */
    bool withdraw_route(const IPv4Net&	nlri,
			const bool& unicast,
			const bool& multicast);

    virtual uint32_t get_unique_id() const	{ return _fake_unique_id; }

    //fake a zero IP address so the RIB IPC handler gets listed first
    //in the Fanout Table.
    const IPv4& id() const		{ return _fake_id; }

    virtual PeerType get_peer_type() const {
	return PEER_TYPE_INTERNAL;
    }                                                                           
    /**
     * This is the handler that deals with originating routes.
     *
     * @return true
     */
    virtual bool originate_route_handler() const {return true;}

    /**
     * @return the main eventloop
     */
    virtual EventLoop& eventloop() const { return _xrl_router.eventloop();}


    /** IPv6 stuff */
#ifdef HAVE_IPV6
    int add_route(const SubnetRoute<IPv6> &rt, 
		  FPAList6Ref& pa_list,   
		  bool ibgp, Safi safi);
    int replace_route(const SubnetRoute<IPv6> &old_rt, bool old_ibgp, 
		      const SubnetRoute<IPv6> &new_rt, bool new_ibgp, 
		      FPAList6Ref& pa_list,   
		      Safi safi);
    int delete_route(const SubnetRoute<IPv6> &rt, 
		     FPAList6Ref& pa_list,   
		     bool ibgp, Safi safi);

    /**
     * Originate an IPv6 route
     *
     * @param origin of the path information
     * @param aspath
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     * @param policytags policy-tags associated with the route
     *
     * @return true on success
     */
    bool originate_route(const OriginType origin,
			 const ASPath& aspath,
			 const IPv6Net& nlri,
			 const IPv6& next_hop,
			 const bool& unicast,
			 const bool& multicast,
			 const PolicyTags& policytags);

    /**
     * Withdraw an IPv6 route
     *
     * @return true on success
     */
    bool withdraw_route(const IPv6Net&	nlri,
			const bool& unicast,
			const bool& multicast);
#endif // ipv6

private:
    bool unregister_rib(string ribname);

    string _ribname;
    XrlStdRouter& _xrl_router;

    XrlQueue<IPv4> _v4_queue;
#ifdef HAVE_IPV6
    XrlQueue<IPv6> _v6_queue;
#endif
    const uint32_t _fake_unique_id;
    IPv4 _fake_id;
};

#endif // __BGP_RIB_IPC_HANDLER_HH__
