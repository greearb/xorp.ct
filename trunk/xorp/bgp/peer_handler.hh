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

// $XORP: xorp/bgp/peer_handler.hh,v 1.11 2003/11/05 06:39:44 atanu Exp $

#ifndef __BGP_PEER_HANDLER_HH__
#define __BGP_PEER_HANDLER_HH__

class BGPMain;
class BGPPeer;
class BGPPlumbing;

#include "libxorp/debug.h"
#include "internal_message.hh"
#include "packet.hh"
#include "peer.hh"
#include "parameter.hh"

/**
 * PeerHandler's job is primarily format conversion. But it also
 * servers as a handle to tie together the input and output sides of a
 * RIB.
 */
class PeerHandler {
public:
    PeerHandler(const string &peername, BGPPeer *peer,
		BGPPlumbing *plumbing_unicast,
		BGPPlumbing *plumbing_multicast);
    virtual ~PeerHandler();

    void stop();
    void peering_went_down();
    void peering_came_up();
    bool peering_is_up() const { return _peering_is_up; }

    /**
     * process_update_packet is called when an update packet has been
     * received by this peer
     */
    int process_update_packet(const UpdatePacket *p);
    
    /**
     * Given an update packet find all the NLRIs with <AFI,SAFI>
     * specified and inject one by one into the plumbing.
     *
     * @param p - packet to tease apart
     * @param safi - Subsequent address family identifier
     *
     * @return true if an <AFI,SAFI> was found.
     */
    template <typename A> bool add(const UpdatePacket *p,
				   PathAttributeList<A>& pa_list,Safi safi);
    /**
     * Given an update packet find all the WITHDRAWs with <AFI,SAFI>
     * specified and inject one by on into the plumbing.
     *
     * @param p - packet to tease apart
     * @param safi - Subsequent address family identifier
     *
     * @return true if an <AFI,SAFI> was found.
     */
    template <typename A> bool withdraw(const UpdatePacket *p, Safi safi);

    template <typename A> bool multiprotocol(Safi safi, 
					     BGPPeerData::Direction d) const;


    /**
     * add_route and delete_route are called by the plumbing to
     * propagate a route *to* the peer.
     */
    virtual int start_packet(bool ibgp);

    virtual int add_route(const SubnetRoute<IPv4> &rt, Safi safi);
    virtual int add_route(const SubnetRoute<IPv6> &rt, Safi safi);
    virtual int replace_route(const SubnetRoute<IPv4> &old_rt,
			      const SubnetRoute<IPv4> &new_rt,
			      Safi safi);
    virtual int replace_route(const SubnetRoute<IPv6> &old_rt,
			      const SubnetRoute<IPv6> &new_rt,
			      Safi safi);
    virtual int delete_route(const SubnetRoute<IPv4> &rt, Safi safi);
    virtual int delete_route(const SubnetRoute<IPv6> &rt, Safi safi);
    virtual PeerOutputState push_packet();
    virtual void output_no_longer_busy();

    AsNum AS_number() const		{ return _peer->peerdata()->as(); }
    const string& peername() const	{ return _peername; }
    bool ibgp() const			{
	if (0 == _peer)	{
	    XLOG_ASSERT(originate_route_handler());
	    return false;
	}
	return _peer->peerdata()->get_internal_peer();
    }

    /**
     * @return true if this is the originate route handler.
     */
    virtual bool originate_route_handler() const {return false;}

    /**
     * @return the neighbours BGP ID as an integer for use by decision.
     */
    virtual const IPv4& id() const		{ return _peer->peerdata()->id(); }

    /**
     * @return the neighbours IP address an as integer for use be
     * decision.
     */
    uint32_t neighbour_address() const	{
	return ntohl(_peer->peerdata()->iptuple().get_peer_addr().s_addr);
    }

    const IPv4& my_v4_nexthop() const	{
	return _peer->peerdata()->get_v4_local_addr();
    }

    const IPv6& my_v6_nexthop() const	{
	return _peer->peerdata()->get_v6_local_addr();
    }

    EventLoop& eventloop() const;
protected:
    BGPPlumbing *_plumbing_unicast;
    BGPPlumbing *_plumbing_multicast;
    bool _ibgp; // did the current update message originate in IBGP?
private:
    string _peername;
    BGPPeer *_peer;
    bool _peering_is_up; /*whether we still think it's up (it may be
                           down, but the FSM hasn't told us yet) */
    UpdatePacket *_packet; /* this is a packet we construct to send */

    /*stats*/
    uint32_t _nlri_total;
    uint32_t _packets;
};

#endif // __BGP_PEER_HANDLER_HH__
