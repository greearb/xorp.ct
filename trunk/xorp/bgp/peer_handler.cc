// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/peer_handler.cc,v 1.5 2003/01/29 22:17:10 rizzo Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "peer_handler.hh"
#include "plumbing.hh"
#include "packet.hh"
#include "main.hh"

PeerHandler::PeerHandler(const string &init_peername,
			 BGPPeer *peer, BGPPlumbing *plumbing)
    : _plumbing(plumbing), _peername(init_peername), _peer(peer),
      _packet(NULL)
{
    if (_plumbing != NULL) _plumbing->add_peering(this);
    _peering_is_up = true;

    // stats for debugging only
    _nlri_total = 0;
    _packets = 0;
}

/* The typical action is only to call peering_went_down when a peering
   goes down.

   In the case of BGP itself going down, we typically call stop on all
   the peerhandlers, then call peering_went_down on all the
   peerhandlers, then wait for all the peerhandlers to indicate idle,
   then call the destructors on the peerhandlers.

   In the case of de-configuring a peering (as opposed to it going
   down temporarily, we would call peering_went_down, then wait for
   the peerhandler to indicate idle, and finally call the destructor
   on the peerhandler. */


PeerHandler::~PeerHandler()
{
    debug_msg("PeerHandler destructor called\n");

    if (_plumbing != NULL)
	_plumbing->delete_peering(this);
    if (_packet != NULL)
	delete _packet;
}

void
PeerHandler::stop()
{
    debug_msg("PeerHandler STOP!\n");
    _plumbing->stop_peering(this);
}

void
PeerHandler::peering_went_down()
{
    _peering_is_up = false;
    _plumbing->peering_went_down(this);
}

void
PeerHandler::peering_came_up()
{
    _peering_is_up = true;
    _plumbing->peering_came_up(this);
}

/*
 * process_update_packet is called when we've received an Update
 * message from the peer.  We break down the update into its pieces,
 * and pass the information into the plumbing to be stored in the
 * RibIn and distributed as required.  */

int
PeerHandler::process_update_packet(const UpdatePacket *p)
{
    debug_msg("Processing packet\n %s\n", p->str().c_str());
    // The way multiprotocol BGP works, the routes from the Withdrawn
    // Routes part of the update packet can only be IPv4 routes.  IPv6
    // withdrawn routes would be in an MP_UNREACH_NLRI attribute.
    list <BGPUpdateAttrib>::const_iterator wi;
    wi = p->wr_list().begin();
    while (wi != p->wr_list().end()) {
	_plumbing->delete_route(wi->net(), this);
	++wi;
    }

    if (p->nlri_list().empty()) {
	_plumbing->push(this);
	return 0;
    }

    PathAttributeList<IPv4> pa_list;

    list <PathAttribute*>::const_iterator pai;
    pai = p->pa_list().begin();
    while (pai != p->pa_list().end()) {
	const PathAttribute* pa;
	pa = *pai;
	pa_list.add_path_attribute(*pa);
	++pai;
    }
    pa_list.rehash();
    assert(pa_list.complete());
    debug_msg("Built path attribute list: %s\n", pa_list.str().c_str());


    // The way multiprotocol BGP works, the routes from the NLRI part
    // of the update packet can only be IPv4 routes.  IPv6 withdrawn
    // routes would be in an MP_REACH_NLRI attribute.
    list <BGPUpdateAttrib>::const_iterator ni;
    ni = p->nlri_list().begin();
    while (ni != p->nlri_list().end()) {
	SubnetRoute<IPv4> msg_route(ni->net(), &pa_list);
	InternalMessage<IPv4> msg(&msg_route, this, GENID_UNKNOWN);
	_plumbing->add_route(msg, this);
	++ni;
    }

    _plumbing->push(this);
    return 0;
}

int
PeerHandler::start_packet(bool ibgp)
{
    // if a router came from IBGP, it shouldn't go to IBGP (unless
    // we're a route reflector)
    if (ibgp)
	assert(!_peer->ibgp());
    _ibgp = ibgp;

    assert(_packet == NULL);
    _packet = new UpdatePacket();
    return 0;
}

int
PeerHandler::add_route(const SubnetRoute<IPv4> &rt)
{
    debug_msg("PeerHandler::add_route(IPv4) %x\n", (u_int)(&rt));
    assert(_packet != NULL);

    if (_packet->big_enough()) {
	push_packet();
	start_packet(_ibgp);
    }

    // did we already add the packet attribute list?
    if (_packet->pa_list().empty()) {
	debug_msg("First add on this packet\n");
	debug_msg("SubnetRoute is %s\n", rt.str().c_str());
	// no, so add all the path attributes
	list <PathAttribute*>::const_iterator pai;
	pai = rt.attributes()->att_list().begin();
	while (pai != rt.attributes()->att_list().end()) {
	    debug_msg("Adding attribute\n");
	    _packet->add_pathatt(**pai);
	    ++pai;
	}
    }

    // add the NLRI information.
    BGPUpdateAttrib nlri(rt.net());
    _packet->add_nlri(nlri);
    return 0;
}

int
PeerHandler::add_route(const SubnetRoute<IPv6> &rt)
{
    debug_msg("PeerHandler::add_route(IPv6) %p\n", &rt);
    assert(_packet != NULL);
    abort();
    UNUSED(rt);
    return 0;
}

int
PeerHandler::replace_route(const SubnetRoute<IPv4> &old_rt,
			   const SubnetRoute<IPv4> &new_rt)
{
    // in the basic PeerHandler, we can ignore the old route, because
    // to change a route, the BGP protocol just sends the new
    // replacement route
    UNUSED(old_rt);
    return add_route(new_rt);
}

int
PeerHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
			   const SubnetRoute<IPv6> &new_rt)
{
    // in the basic PeerHandler, we can ignore the old route, because
    // to change a route, the BGP protocol just sends the new
    // replacement route
    UNUSED(old_rt);
    return add_route(new_rt);
}

int
PeerHandler::delete_route(const SubnetRoute<IPv4> &rt)
{
    debug_msg("PeerHandler::delete_route(IPv4) %x\n", (u_int)(&rt));
    assert(_packet != NULL);

    if (_packet->big_enough()) {
	push_packet();
	start_packet(_ibgp);
    }

    BGPUpdateAttrib wdr(rt.net());
    _packet->add_withdrawn(wdr);
    return 0;
}

int
PeerHandler::delete_route(const SubnetRoute<IPv6>& rt)
{
    debug_msg("PeerHandler::delete_route(IPv6) %p\n", &rt);
    assert(_packet != NULL);
    UNUSED(rt);
    abort();
    return 0;
}

PeerOutputState
PeerHandler::push_packet()
{
    debug_msg("PeerHandler::push_packet - sending packet:\n %s\n",
	      _packet->str().c_str());

    // do some sanity checking
    assert(_packet != NULL);
    int wdr = _packet->wr_list().size();
    int nlri = _packet->nlri_list().size();
    int pa = _packet->pa_list().size();
    assert( (wdr+nlri) > 0);
    if (nlri > 0) assert(pa > 0);
    _nlri_total += nlri;
    _packets++;
    debug_msg("Mean packet has %f nlri's\n",
	    ((float)_nlri_total)/_packets);

    PeerOutputState result;
    result = _peer->send_update_message(*_packet);
    delete _packet;
    _packet = NULL;
    return result;
}

void
PeerHandler::output_no_longer_busy()
{
    if (_peering_is_up)
	_plumbing->output_no_longer_busy(this);
}

uint32_t
PeerHandler::id() const
{
    return ntohl(_peer->peerdata()->id().addr());
}

const IPv4&
PeerHandler::bgp_id() const
{
    return _peer->peerdata()->id();
}

uint32_t
PeerHandler::neighbour_address() const
{
    return ntohl(_peer->peerdata()->iptuple().get_peer_addr().s_addr);
}

const IPv4&
PeerHandler::my_v4_nexthop() const
{
    return _peer->peerdata()->get_v4_local_addr();
}

const IPv6&
PeerHandler::my_v6_nexthop() const
{
    return _peer->peerdata()->get_v6_local_addr();
}

EventLoop *
PeerHandler::get_eventloop() const
{
    return _peer->main()->get_eventloop();
}
