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
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "area_router.hh"
#include "peer.hh"

template <typename A>
PeerOut<A>:: PeerOut(Ospf<A>& ospf, const string interface, const string vif, 
		     const PeerID peerid,
		     const A interface_address, const uint16_t interface_mtu,
		     OspfTypes::LinkType linktype, OspfTypes::AreaID area,
		     OspfTypes::AreaType area_type)
    : _ospf(ospf), _interface(interface), _vif(vif), _peerid(peerid),
      _interface_address(interface_address), _interface_mtu(interface_mtu),
      _linktype(linktype), _running(false)
{
    _areas[area] = new Peer<A>(ospf, *this, area, area_type);
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
PeerOut<A>::add_area(OspfTypes::AreaID area, OspfTypes::AreaType area_type)
{
    debug_msg("Area %s\n", area.str().c_str());

    // Only OSPFv3 is allows a peer to be connected to multiple areas.
    XLOG_ASSERT(OspfTypes::V3 == _ospf.get_version());

    Peer<A> *peer = _areas[area] = new Peer<A>(_ospf, *this, area, area_type);
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

    for(i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->start();
	AreaRouter<A> *area_router = 
	    _ospf.get_peer_manager().get_area_router((*i).first);
	XLOG_ASSERT(area_router);
	area_router->peer_up(_peerid);
    }

}

template <typename A>
void
PeerOut<A>::take_down_peering()
{
    _ospf.disable_interface_vif(_interface, _vif);

    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->stop();
	AreaRouter<A> *area_router = 
	    _ospf.get_peer_manager().get_area_router((*i).first);
	XLOG_ASSERT(area_router);
	area_router->peer_down(_peerid);
    }
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

    XLOG_WARNING("TBD - Check this packet");

    HelloPacket *hello;
    DataDescriptionPacket *dd;

    if (0 != (hello = dynamic_cast<HelloPacket *>(packet))) {
	return process_hello_packet(dst, src, hello);
    } else if(0 != (dd = dynamic_cast<DataDescriptionPacket *>(packet))) {
	return process_data_description_packet(dst, src, dd);
    } else {
	XLOG_UNFINISHED();
    }

    return false;
}

template <typename A>
Neighbour<A> *
Peer<A>::find_neighbour(A src, Packet *packet)
{
    typename list<Neighbour<A> *>::iterator n;
    switch(_peerout.get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if ((*n)->get_neighbour_address() == src)
		return *n;
	break;
    case OspfTypes::VirtualLink:
    case OspfTypes::PointToPoint:
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if ((*n)->get_router_id() == packet->get_router_id())
		return *n;
	break;
    }

    return 0;
}

template <typename A>
bool
Peer<A>::process_hello_packet(A dst, A src, HelloPacket *hello)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*hello));

    // Sanity check this hello packet.

    // Check the network masks - OSPF V2 only.
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	if (OspfTypes::PointToPoint == _peerout.get_linktype() ||
	    OspfTypes::VirtualLink == _peerout.get_linktype())
	    break;
	if (_hello_packet.get_network_mask() !=
	    hello->get_network_mask()) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "Network masks don't match %d %s",
		       _hello_packet.get_network_mask(),
		       hello->str().c_str());
	    return false;
	}
	break;
    case OspfTypes::V3:
	break;
    }

    // Check the hello interval.
    if (_hello_packet.get_hello_interval() != 
	hello->get_hello_interval()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Hello intervals don't match %d %s",
		   _hello_packet.get_hello_interval(),
		   hello->str().c_str());
	return false;
    }

    // Check the router dead interval.
    if (_hello_packet.get_router_dead_interval() != 
	hello->get_router_dead_interval()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Router dead intervals don't match %d %s",
		   _hello_packet.get_router_dead_interval(),
		   hello->str().c_str());
	return false;
    }

    // Check the E-Bit in options.
    Options options(_ospf.get_version(), hello->get_options());
    switch(_area_type) {
    case OspfTypes::BORDER:
	if (!options.get_e_bit()) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "BORDER router but the E-bit is not set %s",
		       hello->str().c_str());
	    return false;
	}
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	if (options.get_e_bit()) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "STUB/NSSA router but the E-bit is set %s",
		       hello->str().c_str());
	    return false;
	}
	break;
    }

    Neighbour<A> *n = find_neighbour(src, hello);

    if (0 == n) {
	n = new Neighbour<A>(_ospf, *this, hello->get_router_id(), src);
	_neighbours.push_back(n);
    }

    n->event_hello_received(hello);

    return true;
}

template <typename A>
bool
Peer<A>::process_data_description_packet(A dst,
					 A src,
					 DataDescriptionPacket *dd)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*dd));

    Neighbour<A> *n = find_neighbour(src, dd);

    if (0 == n) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "No matching neighbour found source %s %s",
		   cstring(src),
		   cstring(*dd));
	
	return false;
    }

    // Perform the MTU check.
    if (dd->get_interface_mtu() > get_interface_mtu()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Received MTU larger than %d %s",
		   get_interface_mtu(),
		   cstring(*dd));
	return false;
    }

    n->data_description_received(dd);

    return false;	// Never keep a copy of the packet.
}

template <typename A>
void
Peer<A>::start()
{
    //    _interface_state = Down;
    set_designated_router("0.0.0.0");
    set_backup_designated_router("0.0.0.0");
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
    const char *event_name = "InterfaceUp";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

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

    update_router_links();

    XLOG_ASSERT(Down != _interface_state);
}

template <typename A>
void
Peer<A>::event_wait_timer()
{
    const char *event_name = "WaitTimer";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    event_backup_seen();
    // Start sending hello packets.
    start_hello_timer();
}

template <typename A>
void
Peer<A>::event_backup_seen()
{
    const char *event_name = "BackupSeen";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

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

    update_router_links();
}

template <typename A>
void
Peer<A>::event_neighbour_change()
{
    const char *event_name = "NeighborChange";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

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

    update_router_links();
}

template <typename A>
void
Peer<A>::event_loop_ind()
{
    const char *event_name = "LoopInd";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    _interface_state = Loopback;

    tear_down_state();
    update_router_links();
}

template <typename A>
void
Peer<A>::event_unloop_ind()
{
    const char *event_name = "UnLoopInd";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

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

    update_router_links();
}

template <typename A>
void
Peer<A>::event_interface_down()
{
    const char *event_name = "InterfaceDown";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    _interface_state = Down;

    tear_down_state();
    update_router_links();

    XLOG_WARNING("TBD - KillNbr");
}

template <typename A>
void
Peer<A>::schedule_event(const char *event)
{
    if (_scheduled_events.empty()) {
	_event_timer = _ospf.get_eventloop().
	    new_oneoff_after_ms(0,
				callback(this,
					 &Peer<A>::process_scheduled_events));
    }

    _scheduled_events.push_back(event);
}

template <typename A>
void
Peer<A>::process_scheduled_events()
{
    struct event {
	string event_name;
	XorpCallback0<void>::RefPtr cb;
    } events[] = {
	{"NeighbourChange", callback(this, &Peer<A>::event_neighbour_change)},
    };
    list<string>::const_iterator e;
    for(e = _scheduled_events.begin(); e != _scheduled_events.end(); e++) {
	bool found = false;
	for (size_t i = 0; i < sizeof(events) / sizeof(struct event); i++) {
	    if ((*e) == events[i].event_name) {
		events[i].cb->dispatch();
		found = true;
		break;
	    }
	}
	if (!found)
	    XLOG_FATAL("Unknown event %s", (*e).c_str());
    }
    _scheduled_events.clear();
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
uint32_t
Peer<A>::send_options()
{
    // Set/UnSet E-Bit.
    Options options(_ospf.get_version(), 0);
    switch(_area_type) {
    case OspfTypes::BORDER:
	options.set_e_bit(true);
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	options.set_e_bit(false);
	break;
    }

    return options.get_options();
}

template <typename A>
void
Peer<A>::populate_common_header(Packet& packet)
{
    // Fetch the router ID.
    packet.set_router_id(_ospf.get_router_id());

    // Set the Area ID
    packet.set_area_id(get_area_id());
}

template <typename A>
bool
Peer<A>::send_hello_packet()
{
    vector<uint8_t> pkt;

    // Fetch the router ID.
    _hello_packet.set_router_id(_ospf.get_router_id());

    // Options.
    _hello_packet.set_options(send_options());

    // Put the neighbours into the hello packet.
    _hello_packet.get_neighbours().clear();
    typename list<Neighbour<A> *>::iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	_hello_packet.get_neighbours().push_back((*n)->get_router_id());

    _hello_packet.encode(pkt);

    SimpleTransmit<A> *transmit;

    switch(_peerout.get_linktype()) {
    case OspfTypes::PointToPoint:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::BROADCAST:
	transmit = new SimpleTransmit<A>(pkt, A::OSPFIGP_ROUTERS(), 
					 _peerout.get_interface_address());
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

template <>
OspfTypes::RouterID
Peer<IPv4>::get_candidate_id(IPv4 source_address, OspfTypes::RouterID)
{
    return source_address;
}

template <>
OspfTypes::RouterID
Peer<IPv6>::get_candidate_id(IPv6, OspfTypes::RouterID router_id)
{
    return router_id;
}

template <>
OspfTypes::RouterID
Peer<IPv4>::get_candidate_id(IPv4) const
{
    return _peerout.get_interface_address();
}

template <>
OspfTypes::RouterID
Peer<IPv6>::get_candidate_id(IPv6) const
{
    return _ospf.get_router_id();
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
	    push_back(Candidate(get_candidate_id(),
				_hello_packet.get_designated_router(),
				_hello_packet.get_backup_designated_router(),
				_hello_packet.get_router_priority()));
					
    }

    // Go through the neighbours and pick possible candidates.
    typename list<Neighbour<A> *>::const_iterator n;
    for (n = _neighbours.begin(); n != _neighbours.end(); n++) {
	const HelloPacket *hello = (*n)->get_hello_packet();
	// A priority of 0 means a router is not a candidate.
	if (0 != hello->get_router_priority() &&
	    Neighbour<A>::TwoWay <= (*n)->get_state()) {
	    candidates.
		push_back(Candidate((*n)->get_candidate_id(),
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
    if (get_candidate_id() == dr && 
	_hello_packet.get_designated_router() != dr)
	recompute = true;
    if (get_candidate_id() == bdr && 
	_hello_packet.get_backup_designated_router() != bdr)
	recompute = true;

    // Was this router the DR or BDR
    if (get_candidate_id() != dr && 
	_hello_packet.get_designated_router() == get_candidate_id())
	recompute = true;
    if (get_candidate_id() != bdr && 
	_hello_packet.get_backup_designated_router() == get_candidate_id())
	recompute = true;

    if (recompute) {
	typename list<Candidate>::iterator i = candidates.begin();
	// Verify that the first entry in the candidate list is this router.
	XLOG_ASSERT((*i)._router_id == get_candidate_id());
	// Update the DR and BDR
	(*i)._dr = dr;
	(*i)._bdr = bdr;
	// Repeat steps (2) and (3).
	bdr = backup_designated_router(candidates);
	dr = designated_router(candidates);
    }
    
    // Step(5)
    set_designated_router(dr);
    set_backup_designated_router(bdr);

    if (get_candidate_id() == dr)
	_interface_state = DR;
    else if (get_candidate_id() == bdr)
	_interface_state = Backup;
    else
	_interface_state = DR_other;

    // Step(6)
    if(OspfTypes::NBMA ==_peerout.get_linktype())
	XLOG_UNFINISHED();
	

    // Step(7)
    // Need to send AdjOK to all neighbours that are least 2-Way.
    XLOG_WARNING("TBD - Send AdjOK");
}

#if	0
/**
 * Utility function to extract a 32 quantity, basically an IPv4
 * address. The code uses templates so sometimes code is generated to
 * extract a 32 bit quantity from an IPv6 address.
 *
 */
template <typename A> uint32_t get32(A a);

template <>
inline
uint32_t
get32<IPv4>(IPv4 a)
{
    return a.addr();
}

template <>
inline
uint32_t
get32<IPv6>(IPv6)
{
    XLOG_FATAL("Only IPv4 not IPv6");
    return 0;
}
#endif

template <typename A>
void
Peer<A>::update_router_links()
{
    OspfTypes::Version version = _ospf.get_version();

    switch(version) {
    case OspfTypes::V2:
	update_router_linksV2();	
	break;
    case OspfTypes::V3:
	update_router_linksV3();
	break;
    }
}

template <>
uint32_t
Peer<IPv4>::get_designated_router_interface_id(IPv4) const
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	XLOG_FATAL("OSPF V3 Only");
	break;
    case OspfTypes::V3:
	break;
    }

    XLOG_UNREACHABLE();

    return 0;
}

template <>
uint32_t
Peer<IPv6>::get_designated_router_interface_id(IPv6) const
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	XLOG_FATAL("OSPF V3 Only");
	break;
    case OspfTypes::V3:
	break;
    }

    switch(_interface_state) {
    case Down:
    case Loopback:
    case Waiting:
    case Point2Point:
    case Backup:
	break;
    case DR_other: {
	// Scan through the neighbours until we find the DR.
	list<Neighbour<IPv6> *>::const_iterator n;
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if (get_designated_router() == (*n)->get_router_id()) {
		return (*n)->get_hello_packet()->get_interface_id();
		break;
	    }
	XLOG_FATAL("Designated router not found");
    }
	break;
    case DR:
	// If this is the DR simple.
	return get_interface_id();
	break;
    }
    XLOG_FATAL(
	       "Designated router interface ID "
	       "available in states DR and DR Other not %s",
	       pp_interface_state(_interface_state).c_str());

    return 0;
}

template <>
void
Peer<IPv4>::update_router_linksV2()
{
    OspfTypes::Version version = _ospf.get_version();
    RouterLink router_link(version);

    switch(_interface_state) {
    case Down:
	// Notify the area database this router link in no longer available
	get_area_router()->remove_router_link(_peerout.get_peerid());
	return;
	break;
    case Loopback:
	// XXX - We should be checking to see if this is p2p unnumbered.
	router_link.set_type(RouterLink::stub);
	router_link.set_link_id(get_interface_address().addr());
	router_link.set_link_data(0xffffffff);
	router_link.set_metric(0);
	goto done;
	break;
    case Waiting:
	break;
    case Point2Point:
	// Unconditional
	// Option 1
	router_link.set_type(RouterLink::stub);
	router_link.set_link_id(get_p2p_neighbour_address().addr());
	router_link.set_link_data(0xffffffff);
	router_link.set_metric(_peerout.get_interface_cost());
	// Option 2
	// ...
	break;
    case DR_other:
	break;
    case Backup:
	break;
    case DR:
	break;
    }
    
    switch(_peerout.get_linktype()) {
    case OspfTypes::PointToPoint:
	router_link.set_type(RouterLink::p2p);
	router_link.set_link_id(get_interface_address().addr());
	router_link.set_link_data(0xffffffff);
	router_link.set_metric(0);
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA: {
	bool adjacent = false;
	switch(_interface_state) {
	case Down:
	    break;
	case Loopback:
	    break;
	case Waiting: {
	    router_link.set_type(RouterLink::stub);
	    IPNet<IPv4> net(get_interface_address(),
			    _peerout.get_interface_prefix_length());
	    router_link.set_link_id(net.masked_addr().addr());
	    router_link.set_link_data(net.netmask().addr());
	    router_link.set_metric(_peerout.get_interface_cost());
	}
	    break;
	case Point2Point:
	    XLOG_UNFINISHED();
	    break;
	case DR_other: {
	    // Do we have an adjacency with the DR?
	    list<Neighbour<IPv4> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (get_designated_router() == (*n)->get_candidate_id()) {
		    break;
		}
	    XLOG_ASSERT(n != _neighbours.end());
	    if (Neighbour<IPv4>::Full == (*n)->get_state())
		adjacent = true;
	}
	    /* FALLTHROUGH */
	case DR: {
	    // Try and find any adjacency.
	    list<Neighbour<IPv4> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (Neighbour<IPv4>::Full == (*n)->get_state()) {
		    adjacent = true;
		    break;
		}
	    if (adjacent) {
		router_link.set_type(RouterLink::transit);

		router_link.set_link_id(get_designated_router().addr());
		router_link.set_link_data(get_interface_address().addr());

	    } else {
		router_link.set_type(RouterLink::stub);
		IPNet<IPv4> net(get_interface_address(),
			     _peerout.get_interface_prefix_length());
		router_link.set_link_id(net.masked_addr().addr());
		router_link.set_link_data(net.netmask().addr());

	    }
	    router_link.set_metric(_peerout.get_interface_cost());
	}
	    break;
	case Backup:
	    // Not clear what should be done here.
	    XLOG_WARNING("TBD BACKUP");
	    break;
	}
    }
    
	break;
    case OspfTypes::PointToMultiPoint:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::VirtualLink:
	XLOG_UNFINISHED();
	break;
    }

 done:
    // Notify the area database this router link in now available
    get_area_router()->add_router_link(_peerout.get_peerid(), router_link);
}

template <>
void
Peer<IPv6>::update_router_linksV2()
{
    XLOG_FATAL("OSPFv2 can't carry IPv6 addresses");
}

template <>
void
Peer<IPv4>::update_router_linksV3()
{
    XLOG_FATAL("OSPFv3 can't carry IPv4 addresses yet!");
}

template <>
void
Peer<IPv6>::update_router_linksV3()
{
    RouterLink router_link(OspfTypes::V3);

    switch(_interface_state) {
    case Down:
    case Loopback:
	// Notify the area database this router link in no longer available
	get_area_router()->remove_router_link(_peerout.get_peerid());
	return;
	break;
    case Waiting:
	break;
    case Point2Point:
	break;
    case DR_other:
	break;
    case Backup:
	break;
    case DR:
	break;
    }

    router_link.set_interface_id(get_interface_id());
    router_link.set_metric(_peerout.get_interface_cost());

    switch(_peerout.get_linktype()) {
    case OspfTypes::PointToPoint: {
	// Find the neighbour. If the neighbour is fully adjacent then
	// configure a router link.
	list<Neighbour<IPv6> *>::iterator n = _neighbours.begin();
	if (n != _neighbours.end() &&
	    Neighbour<IPv6>::Full == (*n)->get_state()) {
	    router_link.set_type(RouterLink::p2p);
	    router_link.set_neighbour_interface_id((*n)->get_hello_packet()->
						   get_interface_id());
	    router_link.set_neighbour_router_id((*n)->get_router_id().addr());

	    get_area_router()->add_router_link(_peerout.get_peerid(),
					       router_link);
	}
	
    }
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA: {
	bool adjacent = false;
	switch(_interface_state) {
	case Down:
	    break;
	case Loopback:
	    break;
	case Waiting:
	    break;
	case Point2Point:
	    break;
	case DR_other: {
	    // Do we have an adjacency with the DR?
	    list<Neighbour<IPv6> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (get_designated_router() == (*n)->get_candidate_id()) {
		    break;
		}
	    XLOG_ASSERT(n != _neighbours.end());
	    if (Neighbour<IPv6>::Full == (*n)->get_state())
		adjacent = true;
	}
	    /* FALLTHROUGH */
	case DR: {
	    // Try and find any adjacency.
	    list<Neighbour<IPv6> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (Neighbour<IPv6>::Full == (*n)->get_state()) {
		    adjacent = true;
		    break;
		}
	    if (adjacent) {
		router_link.set_type(RouterLink::transit);
		// DR interface ID.
		router_link.
		    set_neighbour_interface_id(
				       get_designated_router_interface_id());
		// DR router ID.
		router_link.set_neighbour_router_id(get_designated_router().
						    addr());

		get_area_router()->add_router_link(_peerout.get_peerid(),
					       router_link);
	    }
	}
	    break;
	case Backup:
	    break;
	}
    }
	
    	break;
    case OspfTypes::PointToMultiPoint:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::VirtualLink:
	XLOG_UNFINISHED();
	break;
    }
}

template <typename A>
void
Peer<A>::tear_down_state()
{
    _hello_timer.clear();
    _wait_timer.clear();
    _neighbours.clear();
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
	return "Point-to-point";
    case Peer<A>::DR_other:
	return "DR Other";
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
uint32_t
Peer<A>::get_interface_id() const
{
    return _hello_packet.get_interface_id();
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

template <typename A>
bool
Peer<A>::set_rxmt_interval(uint32_t rxmt_interval)
{
    _rxmt_interval = rxmt_interval;

    return true;
}

template <typename A>
uint32_t
Peer<A>::get_rxmt_interval()
{
    return _rxmt_interval;
}

template <typename A>
bool
Peer<A>::set_designated_router(OspfTypes::RouterID dr)
{
    _hello_packet.set_designated_router(dr);

    return true;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::get_designated_router() const
{
    return _hello_packet.get_designated_router();
}

template <typename A>
bool
Peer<A>::set_backup_designated_router(OspfTypes::RouterID dr)
{
    _hello_packet.set_backup_designated_router(dr);

    return true;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::get_backup_designated_router() const
{
    return _hello_packet.get_backup_designated_router();
}

/****************************************/

template <typename A>
string
Neighbour<A>::pp_state(State ns)
{
    switch(ns) {
    case Neighbour<A>::Down:
	return "Down";
    case Neighbour<A>::Attempt:
	return "Attempt";
    case Neighbour<A>::Init:
	return "Init";
    case Neighbour<A>::TwoWay:
	return "TwoWay";
    case Neighbour<A>::ExStart:
	return "ExStart";
    case Neighbour<A>::Exchange:
	return "Exchange";
    case Neighbour<A>::Loading:
	return "Loading";
    case Neighbour<A>::Full:
	return "Full";
    }
    XLOG_UNREACHABLE();
}

/**
 * RFC 2328 Section 10.4. Whether to become adjacent
 */
template <typename A>
bool
Neighbour<A>::establish_adjacency_p() const
{
    bool become_adjacent = false;

    switch(_peer.get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
	// The router itself is the Designated Router
	if (_peer.get_candidate_id() == _peer.get_designated_router()) {
	    become_adjacent = true;
	    break;
	}
	// The router itself is the Backup Designated Router
	if (_peer.get_candidate_id() == _peer.get_backup_designated_router()) {
	    become_adjacent = true;
	    break;
	}
	// The neighboring router is the Designated Router
	if (get_candidate_id() == 
	    get_hello_packet()->get_designated_router()) {
	    become_adjacent = true;
	    break;
	}
	// The neighboring router is the Backup Designated Router
	if (get_candidate_id() == 
	    get_hello_packet()->get_backup_designated_router()) {
	    become_adjacent = true;
	    break;
	}
	break;
    case OspfTypes::PointToPoint:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	become_adjacent = true;
	break;
    }

    return become_adjacent;
}

template <typename A>
void
Neighbour<A>::start_rxmt_timer(RxmtCallback rcb)
{
    // Send one immediately.
    rcb->dispatch();

    // Schedule one for the future.
    _rxmt_timer = _ospf.get_eventloop().
	new_periodic(_peer.get_rxmt_interval() * 1000, rcb);
}

template <typename A>
void
Neighbour<A>::stop_rxmt_timer()
{
    _rxmt_timer.clear();
}

template <typename A>
void
Neighbour<A>::build_data_description_packet()
{
    // Open the database if the handle is invalid.
    if (!_database_handle.valid())
	_database_handle = get_area_router()->open_database();

    // Clear out previous LSA headers.
    _data_description_packet.get_lsa_headers().clear();

    bool last;
    do {
	Lsa::LsaRef lsa = get_area_router()->
	    get_entry_database(_database_handle, last);
	_data_description_packet.get_lsa_headers().
	    push_back(lsa->get_header());

	// XXX - We are testing to see if there is space left in the
	// packet by repeatedly encoding, this is very inefficient. We
	// should encode to find the base size of the packet and then
	// compare against the constant LSA header length each time.
	vector<uint8_t> pkt;
	_data_description_packet.encode(pkt);
	if (pkt.size() + Lsa_header::length() >= _peer.get_interface_mtu())
	    return;
    } while(last == false);

    // No more headers.
    _data_description_packet.set_m_bit(false);
    
    get_area_router()->close_database(_database_handle);
}

template <typename A>
bool
Neighbour<A>::send_data_description_packet()
{
    _peer.populate_common_header(_data_description_packet);
    _data_description_packet.set_interface_mtu(_peer.get_interface_mtu());
    _data_description_packet.set_options(_peer.send_options());
    
    vector<uint8_t> pkt;
    _data_description_packet.encode(pkt);

    SimpleTransmit<A> *transmit;

    switch(_peer.get_linktype()) {
    case OspfTypes::PointToPoint:
	transmit = new SimpleTransmit<A>(pkt,
					 A::OSPFIGP_ROUTERS(), 
					 _peer.get_interface_address());
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	transmit = new SimpleTransmit<A>(pkt,
					 get_neighbour_address(),
					 _peer.get_interface_address());
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peer.transmit(tr);
    
    return true;
}

/**
 * RFC 2328 Section 10.5 Receiving Hello Packets, neighbour component.
 */
template <typename A>
void
Neighbour<A>::event_hello_received(HelloPacket *hello)
{
    const char *event_name = "HelloReceived";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      cstring(get_candidate_id()),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*hello));

    bool first = 0 ==_hello_packet;
    uint8_t previous_router_priority;
    OspfTypes::RouterID previous_dr;
    OspfTypes::RouterID previous_bdr;
    if (first) {
    } else {
	previous_router_priority = _hello_packet->get_router_priority();
	previous_dr = _hello_packet->get_designated_router();
	previous_bdr = _hello_packet->get_backup_designated_router();
	delete _hello_packet;
    }
    _hello_packet = hello;

    // Search for this router in the neighbour list.
    list<OspfTypes::RouterID> li = hello->get_neighbours();
    list<OspfTypes::RouterID>::iterator i = li.begin();
    for(; i != li.end(); i++) {
	if ((*i) == _ospf.get_router_id())
	    break;
    }

    if (i == li.end()) {
	// This neighbour has no knowledge of this router.
	event_1_way_received();
	return;
    }
    event_2_way_received();

    if (first || previous_router_priority != hello->get_router_priority())
	_peer.schedule_event("NeighbourChange");


    bool is_dr = get_candidate_id() == hello->get_designated_router();

    bool was_dr;

    if (first)
	was_dr = false;
    else
	was_dr = get_candidate_id() == previous_dr;

    if (is_dr && 
	OspfTypes::RouterID("0.0.0.0") == 
	hello->get_backup_designated_router() &&
	_peer.get_state() == Peer<A>::Waiting) {
	_peer.schedule_event("BackupSeen");
    } else if(is_dr != was_dr)
	_peer.schedule_event("NeighbourChange");

    bool is_bdr = get_candidate_id() == hello->get_backup_designated_router();
	  
    bool was_bdr;
    if (first)
	was_bdr = false;
    else
	was_bdr = get_candidate_id() == previous_bdr;

    if (is_bdr && _peer.get_state() == Peer<A>::Waiting) {
	_peer.schedule_event("BackupSeen");
    } else  if(is_bdr != was_bdr)
	_peer.schedule_event("NeighbourChange");

    if (OspfTypes::NBMA ==_peer.get_linktype())
	XLOG_WARNING("TBD");
}


template <typename A>
void
Neighbour<A>::event_1_way_received()
{
    const char *event_name = "1-WayReceived";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
    case Attempt:
	XLOG_WARNING("Unexpected state %s", pp_state(get_state()).c_str());
	break;
    case Init:
	// No change
	break;
    case TwoWay:
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	set_state(Init);
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_2_way_received()
{
    const char *event_name = "2-WayReceived";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());
    XLOG_WARNING("TBD");

    switch(get_state()) {
    case Down:
	XLOG_WARNING("Unhandled state %s", pp_state(get_state()).c_str());
	break;
    case Attempt:
	XLOG_ASSERT(_peer.get_linktype() == OspfTypes::NBMA);
	break;
    case Init:
	if (establish_adjacency_p()) {
	    set_state(ExStart);
	    uint32_t seqno = _data_description_packet.get_dd_seqno();
	    _data_description_packet.set_dd_seqno(++seqno);
	    _data_description_packet.set_i_bit(true);
	    _data_description_packet.set_m_bit(true);
	    _data_description_packet.set_ms_bit(true);
	    _data_description_packet.get_lsa_headers().clear();

	    start_rxmt_timer(callback(this,
				      &Neighbour<A>::
				      send_data_description_packet));
	} else {
	    set_state(TwoWay);
	}
	break;
    case TwoWay:
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	// Cool nothing to do.
	break;
    }
}

/**
 * Save specific fields (not all) in Database Description Packets:
 * initialize(I), more(M), master(MS), options, DD sequence number.
 * Please turn this into "operator=".
 */
inline
void
assign(DataDescriptionPacket& lhs, const DataDescriptionPacket& rhs)
{
    if (&lhs == &rhs)
	return;

    lhs.set_i_bit(rhs.get_i_bit());
    lhs.set_m_bit(rhs.get_m_bit());
    lhs.set_ms_bit(rhs.get_ms_bit());
    lhs.set_options(rhs.get_options());
    lhs.set_dd_seqno(rhs.get_dd_seqno());
}

/**
 * Compare specific fields (not all) in Database Description Packets:
 * initialize(I), more(M), master(MS), options, DD sequence number.
 */
inline
bool
operator==(const DataDescriptionPacket& lhs, const DataDescriptionPacket& rhs)
{
    if (&lhs == &rhs)
	return true;

    if (lhs.get_i_bit() != rhs.get_i_bit())
	return false;

    if (lhs.get_m_bit() != rhs.get_m_bit())
	return false;

    if (lhs.get_ms_bit() != rhs.get_ms_bit())
	return false;

    if (lhs.get_options() != rhs.get_options())
	return false;

    if (lhs.get_dd_seqno() != rhs.get_dd_seqno())
	return false;

    return true;
}

/**
 * RFC 2328 Section 10.6 Receiving Database Description Packets,
 * neighbour component.
 */
template <typename A>
void
Neighbour<A>::data_description_received(DataDescriptionPacket *dd)
{
    const char *event_name = "DataDescriptionReceived-pseudo-event";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      cstring(get_candidate_id()),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*dd));

    switch(get_state()) {
    case Down:
	// Reject Packet
	break;
    case Attempt:
	// Reject Packet
	break;
    case Init:
	event_2_way_received();
	if (ExStart == get_state())
	    ;// FALLTHROUGH
	else
	    break;
    case ExStart:
	{
	bool negotiation_done = false;

	// Save some fields for later duplicate detection.
	assign(_last_dd, *dd);

	_all_headers_sent = false;

	if (dd->get_i_bit() && dd->get_m_bit() && dd->get_ms_bit() && 
	    dd->get_lsa_headers().empty() && 
	    dd->get_router_id() > _ospf.get_router_id()) { // Router is slave
	    _last_dd.set_dd_seqno(dd->get_dd_seqno());
	    negotiation_done = true;
	}

	
	if (!dd->get_i_bit() && !dd->get_m_bit() && !dd->get_ms_bit() && 
	    _last_dd.get_dd_seqno() == dd->get_dd_seqno() && 
	    dd->get_router_id() < _ospf.get_router_id()) {  // Router is master
	    negotiation_done = true;
	}

	if (negotiation_done)
	    event_negotiation_done();
	}
	break;
    case TwoWay:
	// Ignore Packet
	break;
    case Exchange:
	{
	// Make sure the saved value is the same as the incoming.
	if (_last_dd.get_ms_bit() != dd->get_ms_bit()) {
	    event_sequence_number_mismatch();
	    break;
	}
	
	if (dd->get_i_bit())  {
	    event_sequence_number_mismatch();
	    break;
	}

	if (dd->get_options() != _last_dd.get_options())  {
	    event_sequence_number_mismatch();
	    break;
	}
	
	bool in_sequence = false;
	if (_last_dd.get_ms_bit()) { // Router is slave
	    if (_last_dd.get_dd_seqno() + 1 == dd->get_dd_seqno())
		in_sequence = true;
	} else {		     // Router is master
	    if (_last_dd.get_dd_seqno() == dd->get_dd_seqno())
		in_sequence = true;
	}

	if (!in_sequence)  {
	    event_sequence_number_mismatch();
	    break;
	}

	list<Lsa_header> li = dd->get_lsa_headers();
	list<Lsa_header>::const_iterator i;
	for (i = li.begin(); i != li.end(); i++) {
	    uint16_t ls_type = i->get_ls_type();

	    // Do we recognise this LSA?
	    // XXX - We should be making a call to the LsaDecoder to
	    // determine if a LS type is valid.
	    if (ls_type > 5) {
		XLOG_TRACE(_ospf.trace()._input_errors,
			   "LS type > 5 %s", cstring(*dd));
		event_sequence_number_mismatch();
		return;
		break;
	    }

	    // Deal with external-LSAs.
	    switch(_peer.get_area_type()) {
	    case OspfTypes::BORDER:
		break;
	    case OspfTypes::STUB:
		if (ls_type == 5) {
		    XLOG_TRACE(_ospf.trace()._input_errors,
			       "external-LSA not allowed in STUB arear %s",
			       cstring(*dd));
		    event_sequence_number_mismatch();
		    return;
		}
		break;
	    case OspfTypes::NSSA:
		// XXX - Are external-LSAs allowed in NSSA.
		XLOG_UNFINISHED();
		break;
	    }
	    
	    // Check to see if this is a newer LSA.
	    if (get_area_router()->newer_lsa(*i))
		XLOG_WARNING("TBD - Add to Link State Request List");
	}

	if (_last_dd.get_ms_bit()) { // Router is slave
	    _last_dd.set_dd_seqno(dd->get_dd_seqno());
	    build_data_description_packet();
	    if (!_data_description_packet.get_m_bit() &&
		!dd->get_m_bit()) {
		event_exchange_done();
	    }
	    send_data_description_packet();
	} else {		     // Router is master
	    _last_dd.set_dd_seqno(_last_dd.get_dd_seqno() + 1);
	    if (_all_headers_sent && !dd->get_m_bit()) {
		event_exchange_done();
	    } else {
		build_data_description_packet();
		send_data_description_packet();
	    }
	}
	    
	}
	break;
    case Loading:
    case Full:
	// Check for duplicates
	if (_last_dd == *dd) {
	    if (_last_dd.get_ms_bit()) { // Router is slave
		send_data_description_packet();
	    } else {		     // Router is master
		// Discard
	    }
	} else {
	    event_sequence_number_mismatch();
	}
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_negotiation_done()
{
    const char *event_name = "NegotiationDone";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      cstring(get_candidate_id()),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    XLOG_WARNING("TBD");

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
	break;
    case Init:
	break;
    case TwoWay:
	break;
    case ExStart:
	break;
    case Exchange:
	break;
    case Loading:
	break;
    case Full:
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_sequence_number_mismatch()
{
    const char *event_name = "SequenceNumberMismatch";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      cstring(get_candidate_id()),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    XLOG_WARNING("TBD");

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
	break;
    case Init:
	break;
    case TwoWay:
	break;
    case ExStart:
	break;
    case Exchange:
	break;
    case Loading:
	break;
    case Full:
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_exchange_done()
{
    const char *event_name = "ExchangeDone";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       get_candidate_id().str().c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      cstring(get_candidate_id()),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    XLOG_WARNING("TBD");

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
	break;
    case Init:
	break;
    case TwoWay:
	break;
    case ExStart:
	break;
    case Exchange:
	break;
    case Loading:
	break;
    case Full:
	break;
    }
}

template class PeerOut<IPv4>;
template class PeerOut<IPv6>;
