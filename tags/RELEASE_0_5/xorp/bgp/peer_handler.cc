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

#ident "$XORP: xorp/bgp/peer_handler.cc,v 1.29 2003/10/30 04:44:01 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "peer_handler.hh"
#include "plumbing.hh"
#include "packet.hh"
#include "bgp.hh"

PeerHandler::PeerHandler(const string &init_peername,
			 BGPPeer *peer,
			 BGPPlumbing *plumbing_unicast,
			 BGPPlumbing *plumbing_multicast)
    : _plumbing_unicast(plumbing_unicast), 
      _plumbing_multicast(plumbing_multicast),
      _peername(init_peername), _peer(peer),
      _packet(NULL)
{
    debug_msg("peername: %s peer %p unicast %p multicast %p\n", 
	      _peername.c_str(), peer, _plumbing_unicast,
	      _plumbing_multicast);

    if (_plumbing_unicast != NULL) 
	_plumbing_unicast->add_peering(this);

    if (_plumbing_multicast != NULL) 
	_plumbing_multicast->add_peering(this);

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

    if (_plumbing_unicast != NULL)
	_plumbing_unicast->delete_peering(this);
    if (_plumbing_multicast != NULL)
	_plumbing_multicast->delete_peering(this);
    if (_packet != NULL)
	delete _packet;
}

void
PeerHandler::stop()
{
    debug_msg("PeerHandler STOP!\n");
    _plumbing_unicast->stop_peering(this);
    _plumbing_multicast->stop_peering(this);
}

void
PeerHandler::peering_went_down()
{
    _peering_is_up = false;
    _plumbing_unicast->peering_went_down(this);
    _plumbing_multicast->peering_went_down(this);
}

void
PeerHandler::peering_came_up()
{
    _peering_is_up = true;
    _plumbing_unicast->peering_came_up(this);
    _plumbing_multicast->peering_came_up(this);
}

template <>
bool
PeerHandler::add<IPv4>(const UpdatePacket *p,
		       PathAttributeList<IPv4>& pa_list,
		       Safi safi)
{
    switch(safi) {
    case SAFI_UNICAST: {
	if (p->nlri_list().empty())
	    return false;

	pa_list.rehash();
	XLOG_ASSERT(pa_list.complete());
	debug_msg("Built path attribute list: %s\n", pa_list.str().c_str());

	BGPUpdateAttribList::const_iterator ni4;
	ni4 = p->nlri_list().begin();
	while (ni4 != p->nlri_list().end()) {
	    SubnetRoute<IPv4>* msg_route 
		= new SubnetRoute<IPv4>(ni4->net(), &pa_list, NULL);
	    InternalMessage<IPv4> msg(msg_route, this, GENID_UNKNOWN);
	    _plumbing_unicast->add_route(msg, this);
	    msg_route->unref();
	    ++ni4;
	}
    }
	break;
    case SAFI_MULTICAST: {
	MPReachNLRIAttribute<IPv4> *mpreach = p->mpreach<IPv4>(safi);
	if(!mpreach)
	    return false;

	pa_list.rehash();
	XLOG_ASSERT(pa_list.complete());

	list<IPNet<IPv4> >::const_iterator ni;
	ni = mpreach->nlri_list().begin();
	while (ni != mpreach->nlri_list().end()) {
	    SubnetRoute<IPv4>* msg_route =
		new SubnetRoute<IPv4>(*ni, &pa_list, NULL);
	    InternalMessage<IPv4> msg(msg_route, this, GENID_UNKNOWN);
	    _plumbing_multicast->add_route(msg, this);
	    msg_route->unref();
	    ++ni;
	}
    }
	break;
    }

    return true;
}

template <>
bool
PeerHandler::withdraw<IPv4>(const UpdatePacket *p, Safi safi)
{
    switch(safi) {
    case SAFI_UNICAST: {
	if (p->wr_list().empty())
	    return false;

	BGPUpdateAttribList::const_iterator wi;
	wi = p->wr_list().begin();
	while (wi != p->wr_list().end()) {
	    _plumbing_unicast->delete_route(wi->net(), this);
	    ++wi;
	}
    }
	break;
    case SAFI_MULTICAST: {
	MPUNReachNLRIAttribute<IPv4> *mpunreach = p->mpunreach<IPv4>(safi);
	if (!mpunreach)
	    return false;
    
	list<IPNet<IPv4> >::const_iterator wi;
	wi = mpunreach->wr_list().begin();
	while (wi != mpunreach->wr_list().end()) {
	    _plumbing_multicast->delete_route(*wi, this);
	    ++wi;
	}
    }
	break;
    }

    return true;
}

template <>
bool
PeerHandler::add<IPv6>(const UpdatePacket *p,
		       PathAttributeList<IPv6>& pa_list,
		       Safi safi)
{
    MPReachNLRIAttribute<IPv6> *mpreach = p->mpreach<IPv6>(safi);
    if(!mpreach)
	return false;

    pa_list.rehash();
    XLOG_ASSERT(pa_list.complete());

    list<IPNet<IPv6> >::const_iterator ni;
    ni = mpreach->nlri_list().begin();
    while (ni != mpreach->nlri_list().end()) {
	SubnetRoute<IPv6>* msg_route =
	    new SubnetRoute<IPv6>(*ni, &pa_list, NULL);
	InternalMessage<IPv6> msg(msg_route, this, GENID_UNKNOWN);

	switch(safi) {
	case SAFI_UNICAST:
	    _plumbing_unicast->add_route(msg, this);
	    break;
	case SAFI_MULTICAST:
	    _plumbing_multicast->add_route(msg, this);
	    break;
	}
	msg_route->unref();
	++ni;
    }

    return false;
}

template <>
bool
PeerHandler::withdraw<IPv6>(const UpdatePacket *p, Safi safi)
{
    MPUNReachNLRIAttribute<IPv6> *mpunreach = p->mpunreach<IPv6>(safi);
    if (!mpunreach)
	return false;
    
    list<IPNet<IPv6> >::const_iterator wi;
    wi = mpunreach->wr_list().begin();
    while (wi != mpunreach->wr_list().end()) {
	switch(safi) {
	case SAFI_UNICAST:
	    _plumbing_unicast->delete_route(*wi, this);
	    break;
	case SAFI_MULTICAST:
	    _plumbing_multicast->delete_route(*wi, this);
	    break;
	}
	++wi;
    }

    return true;
}

inline
void
set_if_true(bool& orig, bool now)
{
    if (now)
	orig = true;
}

/*
 * process_update_packet is called when we've received an Update
 * message from the peer.  We break down the update into its pieces,
 * and pass the information into the plumbing to be stored in the
 * RibIn and distributed as required.
 */

int
PeerHandler::process_update_packet(const UpdatePacket *p)
{
    debug_msg("Processing packet\n %s\n", p->str().c_str());

    PathAttributeList<IPv4> pa_ipv4_unicast;
    PathAttributeList<IPv4> pa_ipv4_multicast;
    PathAttributeList<IPv6> pa_ipv6_unicast;
    PathAttributeList<IPv6> pa_ipv6_multicast;

    bool ipv4_unicast = false;
    bool ipv4_multicast = false;
    bool ipv6_unicast = false;
    bool ipv6_multicast = false;

    list <PathAttribute*>::const_iterator pai;
    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;
	
	if (dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai)) {
  	    MPReachNLRIAttribute<IPv6>* mpreach =
		dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai);
	    switch(mpreach->safi()) {
	    case SAFI_UNICAST:
		pa_ipv6_unicast.
		  add_path_attribute(IPv6NextHopAttribute(mpreach->nexthop()));
	    break;
	    case SAFI_MULTICAST:
		pa_ipv6_multicast.
		  add_path_attribute(IPv6NextHopAttribute(mpreach->nexthop()));
	    break;
	    }
	    continue;
	}
	
	if (dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(*pai)) {
	    continue;
	}

	if (dynamic_cast<MPReachNLRIAttribute<IPv4>*>(*pai)) {
  	    MPReachNLRIAttribute<IPv4>* mpreach =
		dynamic_cast<MPReachNLRIAttribute<IPv4>*>(*pai);
	    switch(mpreach->safi()) {
	    case SAFI_UNICAST:
		XLOG_FATAL("AFI == IPv4 && SAFI == UNICAST???");
// 		pa_ipv4_unicast.
// 		  add_path_attribute(IPv4NextHopAttribute(mpreach->nexthop()));
	    break;
	    case SAFI_MULTICAST:
		pa_ipv4_multicast.
		  add_path_attribute(IPv4NextHopAttribute(mpreach->nexthop()));
	    break;
	    }
	    continue;
	}

	if (dynamic_cast<MPUNReachNLRIAttribute<IPv4>*>(*pai)) {
	    continue;
	}

	pa_ipv4_unicast.add_path_attribute(*pa);

	/*
	** The nexthop path attribute applies only to IPv4 Unicast case.
	*/
	if (NEXT_HOP != pa->type()) {
	    pa_ipv4_multicast.add_path_attribute(*pa);
	    pa_ipv6_unicast.add_path_attribute(*pa);
	    pa_ipv6_multicast.add_path_attribute(*pa);
	}
    }

    // IPv4 Unicast Withdraws
    set_if_true(ipv4_unicast, withdraw<IPv4>(p, SAFI_UNICAST));

    // IPv4 Multicast Withdraws
    set_if_true(ipv4_multicast, withdraw<IPv4>(p, SAFI_MULTICAST));

    // IPv6 Unicast Withdraws
    set_if_true(ipv6_unicast, withdraw<IPv6>(p, SAFI_UNICAST));

    // IPv6 Multicast Withdraws
    set_if_true(ipv6_multicast, withdraw<IPv6>(p, SAFI_MULTICAST));

    // IPv4 Unicast Route add.
    set_if_true(ipv4_unicast, add<IPv4>(p, pa_ipv4_unicast, SAFI_UNICAST));

    // IPv4 Multicast Route add.
    set_if_true(ipv4_multicast, add<IPv4>(p,pa_ipv4_multicast,SAFI_MULTICAST));

    // IPv6 Unicast Route add.
    set_if_true(ipv6_unicast, add<IPv6>(p, pa_ipv6_unicast, SAFI_UNICAST));

    // IPv6 Multicast Route add.
    set_if_true(ipv6_multicast, add<IPv6>(p,pa_ipv6_multicast,SAFI_MULTICAST));

    if (ipv4_unicast)
	_plumbing_unicast->push_ipv4(this);
    if (ipv6_unicast)
	_plumbing_unicast->push_ipv6(this);
    if (ipv4_multicast)
	_plumbing_multicast->push_ipv4(this);
    if (ipv6_multicast)
	_plumbing_multicast->push_ipv6(this);
    return 0;
}

template <>
bool 
PeerHandler::multiprotocol<IPv4>(Safi safi, BGPPeerData::Direction d) const
{
    return _peer->peerdata()->multiprotocol<IPv4>(safi, d);
}

template <>
bool
PeerHandler::multiprotocol<IPv6>(Safi safi, BGPPeerData::Direction d) const
{
    return _peer->peerdata()->multiprotocol<IPv6>(safi, d);
}

int
PeerHandler::start_packet(bool ibgp)
{
    // if a router came from IBGP, it shouldn't go to IBGP (unless
    // we're a route reflector)
    if (ibgp)
	XLOG_ASSERT(!_peer->ibgp());
    _ibgp = ibgp;

    XLOG_ASSERT(_packet == NULL);
    _packet = new UpdatePacket();
    return 0;
}

int
PeerHandler::add_route(const SubnetRoute<IPv4> &rt, Safi safi)
{
    debug_msg("PeerHandler::add_route(IPv4) %x\n", (u_int)(&rt));
    XLOG_ASSERT(_packet != NULL);

    // Check this peer want this NLRI
    if (!multiprotocol<IPv4>(safi, BGPPeerData::RECEIVED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet(_ibgp);
    }

    // did we already add the packet attribute list?
    if (_packet->pa_list().empty()) {
	debug_msg("First add on this packet\n");
	debug_msg("SubnetRoute is %s\n", rt.str().c_str());
	// no, so add all the path attributes
	PathAttributeList<IPv4>::const_iterator pai;
	pai = rt.attributes()->begin();
	while (pai != rt.attributes()->end()) {
	    debug_msg("Adding attribute %s\n", (*pai)->str().c_str());
	    /*
	    ** Don't put a multicast nexthop in the parameter list.
	    */
	    if (!(NEXT_HOP == (*pai)->type() && safi == SAFI_MULTICAST))
		_packet->add_pathatt(**pai);
	    ++pai;
	}
	if (SAFI_MULTICAST == safi) {
	    MPReachNLRIAttribute<IPv4> mp(safi);
	    mp.set_nexthop(rt.nexthop());
	    _packet->add_pathatt(mp);
	}
    }

    // add the NLRI information.
    switch(safi) {
    case SAFI_UNICAST: {
	BGPUpdateAttrib nlri(rt.net());
 	XLOG_ASSERT(_packet->pa_list().nexthop() == rt.nexthop());
	_packet->add_nlri(nlri);
    }
	break;
    case SAFI_MULTICAST: {
	XLOG_ASSERT(_packet->mpreach<IPv4>(SAFI_MULTICAST));
	XLOG_ASSERT(_packet->mpreach<IPv4>(SAFI_MULTICAST)->nexthop() == 
		    rt.nexthop());
	_packet->mpreach<IPv4>(SAFI_MULTICAST)->add_nlri(rt.net());
    }
	break;
    }

    return 0;
}

int
PeerHandler::add_route(const SubnetRoute<IPv6> &rt, Safi safi)
{
    debug_msg("PeerHandler::add_route(IPv6) %p\n", &rt);
    XLOG_ASSERT(_packet != NULL);

    // Check this peer want this NLRI
    if (!multiprotocol<IPv6>(safi, BGPPeerData::RECEIVED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet(_ibgp);
    }

    // did we already add the packet attribute list?
    if (_packet->pa_list().empty()) {
	debug_msg("First add on this packet\n");
	debug_msg("SubnetRoute is %s\n", rt.str().c_str());
	// no, so add all the path attributes
	PathAttributeList<IPv6>::const_iterator pai;
	pai = rt.attributes()->begin();
	while (pai != rt.attributes()->end()) {
	    debug_msg("Adding attribute\n");
	    /*
	    ** Don't put an IPv6 next hop in the IPv4 path attribute list.
	    */
	    if (NEXT_HOP != (*pai)->type())
		_packet->add_pathatt(**pai);
	    ++pai;
	}
	MPReachNLRIAttribute<IPv6> mp(safi);
	mp.set_nexthop(rt.nexthop());
	_packet->add_pathatt(mp);
    }

    XLOG_ASSERT(_packet->mpreach<IPv6>(safi));
    XLOG_ASSERT(_packet->mpreach<IPv6>(safi)->nexthop() == rt.nexthop());
    _packet->mpreach<IPv6>(safi)->add_nlri(rt.net());

    return 0;
}

int
PeerHandler::replace_route(const SubnetRoute<IPv4> &old_rt,
			   const SubnetRoute<IPv4> &new_rt, Safi safi)
{
    // in the basic PeerHandler, we can ignore the old route, because
    // to change a route, the BGP protocol just sends the new
    // replacement route
    UNUSED(old_rt);
    return add_route(new_rt, safi);
}

int
PeerHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
			   const SubnetRoute<IPv6> &new_rt, Safi safi)
{
    // in the basic PeerHandler, we can ignore the old route, because
    // to change a route, the BGP protocol just sends the new
    // replacement route
    UNUSED(old_rt);
    return add_route(new_rt, safi);
}

int
PeerHandler::delete_route(const SubnetRoute<IPv4> &rt, Safi safi)
{
    debug_msg("PeerHandler::delete_route(IPv4) %x\n", (u_int)(&rt));
    XLOG_ASSERT(_packet != NULL);

    // Check this peer want this NLRI
    if (!multiprotocol<IPv4>(safi, BGPPeerData::RECEIVED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet(_ibgp);
    }

    if (SAFI_MULTICAST == safi && 0 == _packet->mpunreach<IPv4>(safi)) {
	MPUNReachNLRIAttribute<IPv4> mp(safi);
	_packet->add_pathatt(mp);
    }
    
    switch(safi) {
    case SAFI_UNICAST: {
	BGPUpdateAttrib wdr(rt.net());
	_packet->add_withdrawn(wdr);
    }
	break;
    case SAFI_MULTICAST: {
	XLOG_ASSERT(_packet->mpunreach<IPv4>(SAFI_MULTICAST));
	_packet->mpunreach<IPv4>(SAFI_MULTICAST)->add_withdrawn(rt.net());
    }
	break;
    }

    return 0;
}

int
PeerHandler::delete_route(const SubnetRoute<IPv6>& rt, Safi safi)
{
    debug_msg("PeerHandler::delete_route(IPv6) %p\n", &rt);
    XLOG_ASSERT(_packet != NULL);

    // Check this peer want this NLRI
    if (!multiprotocol<IPv6>(safi, BGPPeerData::RECEIVED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet(_ibgp);
    }

    if (0 == _packet->mpunreach<IPv6>(safi)) {
	MPUNReachNLRIAttribute<IPv6> mp(safi);
	_packet->add_pathatt(mp);
    }

    XLOG_ASSERT(_packet->mpunreach<IPv6>(safi));
    _packet->mpunreach<IPv6>(safi)->add_withdrawn(rt.net());

    return 0;
}

PeerOutputState
PeerHandler::push_packet()
{
    debug_msg("PeerHandler::push_packet - sending packet:\n %s\n",
	      _packet->str().c_str());

    // do some sanity checking
    XLOG_ASSERT(_packet);
    int wdr = _packet->wr_list().size();
    int nlri = _packet->nlri_list().size();
    int pa = _packet->pa_list().size();

    if(_packet->mpreach<IPv4>(SAFI_MULTICAST))
	nlri += _packet->mpreach<IPv4>(SAFI_MULTICAST)->nlri_list().size();
    if(_packet->mpunreach<IPv4>(SAFI_MULTICAST))
	wdr += _packet->mpunreach<IPv4>(SAFI_MULTICAST)->wr_list().size();

    // Account for IPv6
    if(_packet->mpreach<IPv6>(SAFI_UNICAST))
	nlri += _packet->mpreach<IPv6>(SAFI_UNICAST)->nlri_list().size();
    if(_packet->mpunreach<IPv6>(SAFI_UNICAST))
	wdr += _packet->mpunreach<IPv6>(SAFI_UNICAST)->wr_list().size();
    if(_packet->mpreach<IPv6>(SAFI_MULTICAST))
	nlri += _packet->mpreach<IPv6>(SAFI_MULTICAST)->nlri_list().size();
    if(_packet->mpunreach<IPv6>(SAFI_MULTICAST))
	wdr += _packet->mpunreach<IPv6>(SAFI_MULTICAST)->wr_list().size();

//     XLOG_ASSERT( (wdr+nlri) > 0);
    // If we get here and wdr+nlri equals zero then we were trying to
    // push an AFI/SAFI that our peer did not request so just drop
    // the packet and return.
    if (0 == (wdr+nlri)) {
	delete _packet;
	_packet = NULL;
	return PEER_OUTPUT_OK;
    }
    
    if (nlri > 0)
	XLOG_ASSERT(pa > 0);

    _nlri_total += nlri;
    _packets++;
    debug_msg("Mean packet has %f nlri's\n", ((float)_nlri_total)/_packets);

    PeerOutputState result;
    result = _peer->send_update_message(*_packet);
    delete _packet;
    _packet = NULL;
    return result;
}

void
PeerHandler::output_no_longer_busy()
{
    if (_peering_is_up) {
	_plumbing_unicast->output_no_longer_busy(this);
	_plumbing_multicast->output_no_longer_busy(this);
    }
}

EventLoop&
PeerHandler::eventloop() const
{
    return _peer->main()->eventloop();
}
