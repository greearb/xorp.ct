// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>
#include <set>
#include <queue>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "peer.hh"

template <typename A>
PeerOut<A>:: PeerOut(Ospf<A>& ospf, const string interface, const string vif, 
		     const A address,
		     OspfTypes::LinkType linktype, OspfTypes::AreaID area)
    : _ospf(ospf), _interface(interface), _vif(vif),
      _address(address),
      _linktype(linktype), _running(false)
{
    _areas[area] = new Peer<A>(ospf, *this, area);
}

template <typename A>
PeerOut<A>::~PeerOut()
{
    set_state(false);
    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++)
	delete (*i).second;
}

template <typename A>
bool
PeerOut<A>::add_area(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", area.str().c_str());

    // Only OSPFv3 is allows a peer to be connected to multiple areas.
    XLOG_ASSERT(OspfTypes::V3 == _ospf.get_version());

    Peer<A> *peer = _areas[area] = new Peer<A>(_ospf, *this, area);
    if (_running)
	peer->start();

    return true;
}

template <typename A>
bool
PeerOut<A>::remove_area(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", area.str().c_str());
    // All the peers are notified when an area is deleted.
    if (0 == _areas.count(area)) {
	return false;
    }
    
    delete _areas[area];
    _areas.erase(_areas.find(area));
    
    // If this peer is no longer serving any areas it can be deleted.
    if (_areas.empty())
	return true;
    else
	return false;
}

template <typename A>
void
PeerOut<A>::set_state(bool state)
{
    debug_msg("state %d\n", state);

    switch (_running) {
    case true:
	if (false == state) {
	    take_down_peering();
	    _running = false;
	}
	break;
    case false:
	if (true == state) {
	    bring_up_peering();
	    _running = true;
	}
	break;
    }
}

/**
 * XXX
 * The outgoing packets should be queued on the transmit queue, for
 * the time being just send them straight out.
 */
template <typename A>
bool
PeerOut<A>::transmit(typename Transmit<A>::TransmitRef tr)
{
    do {
	if (!tr->valid())
	    return true;
	size_t len;
	uint8_t *ptr = tr->generate(len);
	_ospf.transmit(_interface, _vif, tr->destination(), tr->source(), 
		       ptr, len);
    } while(tr->multiple());

    return true;
}

template <typename A>
bool
PeerOut<A>::receive(A dst, A src, Packet *packet)
    throw(BadPeer)
{
    debug_msg("dst %s src %s %s\n", cstring(dst), cstring(src),
	      cstring(*packet));
    OspfTypes::AreaID area = packet->get_area_id();
    // Does the area ID in the packet match any that are expecting.
    if (0 == _areas.count(area)) {
	    xorp_throw(BadPeer,
		       c_format("Area %s not handled by %s/%s",
				cstring(packet->get_area_id()),
				_interface.c_str(), _vif.c_str()));
    }

    return _areas[area]->receive(dst, src, packet);
}

template <typename A>
void
PeerOut<A>::bring_up_peering()
{
    // Start receiving packets on this peering.
    _ospf.enable_interface_vif(_interface, _vif);

    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++)
	(*i).second->start();
}

template <typename A>
void
PeerOut<A>::take_down_peering()
{
    _ospf.disable_interface_vif(_interface, _vif);

    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++)
	(*i).second->stop();
}

template <typename A>
bool 
PeerOut<A>::set_network_mask(OspfTypes::AreaID area, uint32_t network_mask)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", area.str().c_str());
	return false;
    }

    return _areas[area]->set_network_mask(network_mask);
}

template <typename A> 
bool
PeerOut<A>::set_interface_id(OspfTypes::AreaID area, uint32_t interface_id)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", area.str().c_str());
	return false;
    }

    return _areas[area]->set_interface_id(interface_id);
}

template <typename A>
bool
PeerOut<A>::set_hello_interval(OspfTypes::AreaID area, uint16_t hello_interval)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", area.str().c_str());
	return false;
    }

    return _areas[area]->set_hello_interval(hello_interval);
}

template <typename A> 
bool
PeerOut<A>::set_options(OspfTypes::AreaID area,	uint32_t options)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", area.str().c_str());
	return false;
    }

    return _areas[area]->set_options(options);
}

template <typename A> 
bool
PeerOut<A>::set_router_priority(OspfTypes::AreaID area,	uint8_t priority)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", area.str().c_str());
	return false;
    }

    return _areas[area]->set_router_priority(priority);
}

template <typename A>
bool
PeerOut<A>::set_router_dead_interval(OspfTypes::AreaID area, 
				     uint32_t router_dead_interval)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", area.str().c_str());
	return false;
    }

    return _areas[area]->set_router_dead_interval(router_dead_interval);
}

/****************************************/

template <typename A>
bool
Peer<A>::receive(A dst, A src, Packet *packet)
{
    debug_msg("dst %s src %s %s\n", cstring(dst), cstring(src),
	      cstring(*packet));
    XLOG_WARNING("TBD");

    return false;
}

template <typename A>
void
Peer<A>::start()
{
    //    _interface_state = Down;
    _hello_packet.set_designated_router("0.0.0.0");
    _hello_packet.set_backup_designated_router("0.0.0.0");
    event_interface_up();
}

template <typename A>
void
Peer<A>::stop()
{
    event_interface_down();
}

template <typename A>
void
Peer<A>::event_interface_up()
{
    XLOG_ASSERT(Down == _interface_state);

    switch(_peerout.get_linktype()) {
    case OspfTypes::PointToPoint:
	_interface_state = Point2Point;
	break;
    case OspfTypes::BROADCAST:
	// Not eligible to be the designated router.
	if (0 == _hello_packet.get_router_priority()) {
	    _interface_state = DR_other;

	    // Start sending hello packets.
	    start_hello_timer();
	} else {
	    _interface_state = Waiting;
	    start_wait_timer();
	}
	break;
    case OspfTypes::NBMA:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	_interface_state = Point2Point;
	XLOG_UNFINISHED();
	break;
    }
    
    XLOG_ASSERT(Down != _interface_state);
}

template <typename A>
void
Peer<A>::event_wait_timer()
{
    event_backup_seen();
    // Start sending hello packets.
    start_hello_timer();
}

template <typename A>
void
Peer<A>::event_backup_seen()
{
    switch(_interface_state) {
    case Down:
    case Loopback:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(_interface_state).c_str());
	break;
    case Waiting:
	compute_designated_router_and_backup_designated_router();

	XLOG_ASSERT(_interface_state == DR_other ||
		    _interface_state == Backup ||
		    _interface_state == DR);

	break;
    case Point2Point:
    case DR_other:
    case Backup:
    case DR:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(_interface_state).c_str());
	break;
    }
}

template <typename A>
void
Peer<A>::event_neighbor_change()
{
    switch(_interface_state) {
    case Down:
    case Loopback:
    case Waiting:
    case Point2Point:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(_interface_state).c_str());
    case DR_other:
    case Backup:
    case DR:
	compute_designated_router_and_backup_designated_router();

	XLOG_ASSERT(_interface_state == DR_other ||
		    _interface_state == Backup ||
		    _interface_state == DR);

	break;
    }
}

template <typename A>
void
Peer<A>::event_loop_ind()
{
    _interface_state = Loopback;

    tear_down_state();
}

template <typename A>
void
Peer<A>::event_unloop_ind()
{
    switch(_interface_state) {
    case Down:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(_interface_state).c_str());
	break;
    case Loopback:
	_interface_state = Down;
	break;
    case Waiting:
    case Point2Point:
    case DR_other:
    case Backup:
    case DR:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(_interface_state).c_str());

	break;
    }
}

template <typename A>
void
Peer<A>::event_interface_down()
{
    _interface_state = Down;

    tear_down_state();

    XLOG_WARNING("KillNbr");
}

template <typename A>
void
Peer<A>::start_hello_timer()
{
    // XXX - The hello packet should have all its parameters set.

    // Schedule one for the future.
    _hello_timer = _ospf.get_eventloop().
	new_periodic(_hello_packet.get_hello_interval() * 1000,
		     callback(this, &Peer<A>::send_hello_packet));

    // Send one immediately.
    send_hello_packet();

    // A more logical way of structuring the code may have been to get
    // send_hello_packet to schedule the next hello packet. The
    // problem with such an approach is that the interval between
    // transmissions would not be fixed. Any elasticity would allow
    // hello packets to become synchronized which is bad.
}

template <typename A>
void
Peer<A>::start_wait_timer()
{
    _wait_timer = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(_hello_packet.get_router_dead_interval()),
			 callback(this, &Peer<A>::event_wait_timer));
}

template <typename A>
bool
Peer<A>::send_hello_packet()
{
    vector<uint8_t> pkt;
    // Fetch the router ID.
    _hello_packet.set_router_id(_ospf.get_router_id());
    _hello_packet.encode(pkt);

    SimpleTransmit<A> *transmit;

    switch(_peerout.get_linktype()) {
    case OspfTypes::PointToPoint:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::BROADCAST:
	transmit = new SimpleTransmit<A>(pkt, A::OSPFIGP_ROUTERS(), 
					 _peerout.get_address());
	break;
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	XLOG_UNFINISHED();
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peerout.transmit(tr);

    return true;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::backup_designated_router(list<Candidate>& candidates) const
{
    // Step (2)
    // Calculate the the new backup designated router.
    // Look for routers that do not consider themselves to be the DR
    // but do consider themselves to the the BDR.
    Candidate c("0.0.0.0", "0.0.0.0", "0.0.0.0", 0);
    typename list<Candidate>::const_iterator i;
    for(i = candidates.begin(); i != candidates.end(); i++) {
	if ((*i)._router_id != (*i)._dr && (*i)._router_id == (*i)._bdr) {
	    if ((*i)._router_priority > c._router_priority)
		c = *i;
	    else if ((*i)._router_priority == c._router_priority &&
		     (*i)._router_id > c._router_id)
		c = *i;
		
	}
    }

    // It is possible that no router was selected because no router
    // had itself as BDR.
    if (0 == c._router_priority) {
	for(i = candidates.begin(); i != candidates.end(); i++) {
	    if ((*i)._router_id != (*i)._dr) {
		if ((*i)._router_priority > c._router_priority)
		    c = *i;
		else if ((*i)._router_priority == c._router_priority &&
			 (*i)._router_id > c._router_id)
		    c = *i;
		
	    }
	}
    }

    return c._router_id;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::designated_router(list<Candidate>& candidates) const
{
    // Step (3)
    // Calculate the designated router.
    Candidate c("0.0.0.0", "0.0.0.0", "0.0.0.0", 0);
    typename list<Candidate>::const_iterator i;
    for(i = candidates.begin(); i != candidates.end(); i++) {
	if ((*i)._router_id == (*i)._dr) {
	    if ((*i)._router_priority > c._router_priority)
		c = *i;
	    else if ((*i)._router_priority == c._router_priority &&
		     (*i)._router_id > c._router_id)
		c = *i;
		
	}
    }

    // It is possible that no router was selected because no router
    // had itself as DR.
    if (0 == c._router_priority) {
	for(i = candidates.begin(); i != candidates.end(); i++) {
	    if ((*i)._router_priority > c._router_priority)
		c = *i;
	    else if ((*i)._router_priority == c._router_priority &&
		     (*i)._router_id > c._router_id)
		c = *i;
	}
    }

    return c._router_id;
}

template <typename A>
void
Peer<A>::compute_designated_router_and_backup_designated_router()
{
    list<Candidate> candidates;

    // Is this router a candidate?
    if (0 != _hello_packet.get_router_priority()) {
	candidates.
	    push_back(Candidate(_ospf.get_router_id(),
				_hello_packet.get_designated_router(),
				_hello_packet.get_backup_designated_router(),
				_hello_packet.get_router_priority()));
					
    }

    // Go through the neighbours and pick possible candidates.
    typename map<OspfTypes::RouterID, Neighbor *>::const_iterator n;
    for (n = _neighbors.begin(); n != _neighbors.end(); n++) {
	const HelloPacket *hello = (*n).second->get_hello_packet();
	if (0 != hello->get_router_priority() &&
	    Neighbor::TwoWay <= (*n).second->get_neighbor_state()) {
	    candidates.
		push_back(Candidate(hello->get_router_id(),
				    hello->get_designated_router(),
				    hello->get_backup_designated_router(),
				    hello->get_router_priority()));
	}
    }
    
    // Step (2)
    // Calculate the the new backup designated router.
    OspfTypes::RouterID bdr = backup_designated_router(candidates);

    // Step (3)
    // Calculate the designated router.
    OspfTypes::RouterID dr = designated_router(candidates);

    // Step (4)
    // If the router has become the DR or BDR or it was the DR or BDR
    // and no longer is then steps (2) and (3) need to be repeated.

    // If nothing has changed out of here.
    if (_hello_packet.get_designated_router() == dr &&
	_hello_packet.get_backup_designated_router() == bdr)
	return;

    bool recompute = false;
    // Has this router just become the DR or BDR
    if (_ospf.get_router_id() == dr && 
	_hello_packet.get_designated_router() != dr)
	recompute = true;
    if (_ospf.get_router_id() == bdr && 
	_hello_packet.get_backup_designated_router() != bdr)
	recompute = true;

    // Was this router the DR or BDR
    if (_ospf.get_router_id() != dr && 
	_hello_packet.get_designated_router() == _ospf.get_router_id())
	recompute = true;
    if (_ospf.get_router_id() != bdr && 
	_hello_packet.get_backup_designated_router() == _ospf.get_router_id())
	recompute = true;

    if (recompute) {
	typename list<Candidate>::iterator i = candidates.begin();
	// Verify that the first entry in the candidate list is this router.
	XLOG_ASSERT((*i)._router_id == _ospf.get_router_id());
	// Update the DR and BDR
	(*i)._dr = dr;
	(*i)._bdr = bdr;
	// Repeat steps (2) and (3).
	bdr = backup_designated_router(candidates);
	dr = designated_router(candidates);
    }
    
    // Step(5)
    _hello_packet.set_designated_router(dr);
    _hello_packet.set_backup_designated_router(bdr);

    if (_ospf.get_router_id() == dr)
	_interface_state = DR;
    else if (_ospf.get_router_id() == bdr)
	_interface_state = Backup;
    else
	_interface_state = DR_other;

    // Step(6)
    if(OspfTypes::NBMA ==_peerout.get_linktype())
	XLOG_UNFINISHED();
	

    // Step(7)
    // Need to send AdjOK to all neighbours that are least 2-Way.
    XLOG_WARNING("TBD: Send AdjOK");
}

template <typename A>
void
Peer<A>::tear_down_state()
{
    _hello_timer.clear();
    _wait_timer.clear();
    _neighbors.clear();
}

template <typename A>
string
Peer<A>::pp_interface_state(InterfaceState is)
{
    switch(is) {
    case Peer<A>::Down:
	return "Down";
    case Peer<A>::Loopback:
	return "Loopback";
    case Peer<A>::Waiting:
	return "Waiting";
    case Peer<A>::Point2Point:
	return "Point2Point";
    case Peer<A>::DR_other:
	return "DR_other";
    case Peer<A>::Backup:
	return "Backup";
    case Peer<A>::DR:
	return "DR";
    }
    XLOG_UNREACHABLE();
}

template <typename A>
bool
Peer<A>::set_network_mask(uint32_t network_mask)
{
    _hello_packet.set_network_mask(network_mask);

    return true;
}


template <typename A>
bool
Peer<A>::set_interface_id(uint32_t interface_id)
{
    _hello_packet.set_interface_id(interface_id);

    return true;
}

template <typename A>
bool
Peer<A>::set_hello_interval(uint16_t hello_interval)
{
    _hello_packet.set_hello_interval(hello_interval);

    return true;
}

template <typename A>
bool
Peer<A>::set_options(uint32_t options)
{
    _hello_packet.set_options(options);

    return true;
}

template <typename A>
bool
Peer<A>::set_router_priority(uint8_t priority)
{
    _hello_packet.set_router_priority(priority);

    return true;
}

template <typename A>
bool
Peer<A>::set_router_dead_interval(uint32_t router_dead_interval)
{
    _hello_packet.set_router_dead_interval(router_dead_interval);

    return true;
}

template class PeerOut<IPv4>;
template class PeerOut<IPv6>;

