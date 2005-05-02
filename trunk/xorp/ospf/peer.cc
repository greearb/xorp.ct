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
		     OspfTypes::LinkType linktype, OspfTypes::AreaID area,
		     OspfTypes::AreaType area_type)
    : _ospf(ospf), _interface(interface), _vif(vif),
      _address(address),
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

    // Try and find this neighbour.
    typename list<Neighbour<A> *>::iterator n;
    switch(_peerout.get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if ((*n)->get_source_address() == src)
		break;
	break;
    case OspfTypes::VirtualLink:
    case OspfTypes::PointToPoint:
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if ((*n)->get_router_id() == hello->get_router_id())
		break;
	break;
    }

    if (_neighbours.end() == n) {
	_neighbours.push_front(new Neighbour<A>(_ospf, *this,
						hello->get_router_id(),
						src));
	// An iterator is required so push to the front and call begin.
	n = _neighbours.begin();
	// Verify that we got back the one that we put in.
	XLOG_ASSERT((*n)->get_router_id() == hello->get_router_id());
	XLOG_ASSERT((*n)->get_source_address() == src);
	XLOG_ASSERT((*n)->get_state() == Neighbour<A>::Init);
    }

    (*n)->event_hello_received(hello);

    return true;
}

template <typename A>
bool
Peer<A>::process_data_description_packet(A dst,
					 A src,
					 DataDescriptionPacket *dd)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*dd));

    return true;
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
    return _peerout.get_address();
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
bool
Neighbour<A>::send_data_description_packet()
{
    _peer.populate_common_header(_data_description_packet);
    _data_description_packet.set_options(_peer.send_options());
    
    vector<uint8_t> pkt;
    _data_description_packet.encode(pkt);

    SimpleTransmit<A> *transmit;

    switch(_peer.get_linktype()) {
    case OspfTypes::PointToPoint:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::BROADCAST:
	transmit = new SimpleTransmit<A>(pkt,
					 A::OSPFIGP_ROUTERS(), 
					 _peer.get_address());
	break;
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	XLOG_UNFINISHED();
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
	      cstring(Peer<A>::get_candidate_id(get_source_address(),
						get_router_id())),
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


    bool is_dr = Peer<A>::get_candidate_id(get_source_address(),
					   get_router_id())
	== hello->get_designated_router();

    bool was_dr;

    if (first)
	was_dr = false;
    else
	was_dr = Peer<A>::get_candidate_id(get_source_address(),
						     get_router_id())
	    == previous_dr;

    if (is_dr && 
	OspfTypes::RouterID("0.0.0.0") == 
	hello->get_backup_designated_router() &&
	_peer.get_state() == Peer<A>::Waiting) {
	_peer.schedule_event("BackupSeen");
    } else if(is_dr != was_dr)
	_peer.schedule_event("NeighbourChange");

    bool is_bdr = Peer<A>::get_candidate_id(get_source_address(),
					   get_router_id())
	== hello->get_backup_designated_router();
	  
    bool was_bdr;
    if (first)
	was_bdr = false;
    else
	was_bdr = Peer<A>::get_candidate_id(get_source_address(),
						     get_router_id())
	    == previous_bdr;

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

template class PeerOut<IPv4>;
template class PeerOut<IPv6>;
