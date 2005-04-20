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
		     OspfTypes::LinkType linktype, OspfTypes::AreaID area)
    : _ospf(ospf), _interface(interface), _vif(vif), 
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

    // Only OSPFv3 is allowed a peer to be connected to multiple areas.
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
PeerOut<A>::transmit(Transmit::TransmitRef tr)
{
    do {
	if (!tr->valid())
	    return true;
	size_t len;
	uint8_t *ptr = tr->generate(len);
	_ospf.transmit(_interface, _vif, ptr, len);
    } while(tr->multiple());

    return true;
}

template <typename A>
void
PeerOut<A>::receive(Packet *packet)
    throw(BadPeer)
{
    debug_msg("%s\n", cstring(*packet));
    OspfTypes::AreaID area = packet->get_area_id();
    // Does the area ID in the packet match any we are expecting.
    if (0 == _areas.count(area)) {
	    xorp_throw(BadPeer,
		       c_format("Area %s not handled by %s/%s",
				cstring(packet->get_area_id()),
				_interface.c_str(), _vif.c_str()));
    }

    _areas[area]->receive(packet);
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
PeerOut<A>::set_router_priority(OspfTypes::AreaID area,
				uint8_t priority)
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
void
Peer<A>::receive(Packet *packet)
{
    debug_msg("%s\n", cstring(*packet));
    XLOG_WARNING("TBD");
}

template <typename A>
void
Peer<A>::start()
{
    _state = Down;

    // Start sending hello packets.
    start_hello_timer();
}

template <typename A>
void
Peer<A>::stop()
{
    _hello_timer.clear();
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
bool
Peer<A>::send_hello_packet()
{
    vector<uint8_t> pkt;
    // Fetch out router ID.
    _hello_packet.set_router_id(_ospf.get_router_id());
    _hello_packet.encode(pkt);
    SimpleTransmit *transmit = new SimpleTransmit(pkt);

    Transmit::TransmitRef tr(transmit);

    _peerout.transmit(tr);

    return true;
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

