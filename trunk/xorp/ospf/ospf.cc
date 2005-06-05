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

#include "config.h"
#include <map>
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxorp/eventloop.hh"

#include "ospf.hh"

template <typename A>
Ospf<A>::Ospf(OspfTypes::Version version, EventLoop& eventloop, IO<A>* io)
    : _version(version), _eventloop(eventloop), _io(io), _running(true),
      _lsa_decoder(version), _peer_manager(*this)
{
    // Register the LSAs and packets with the associated decoder.
    switch(get_version()) {
    case OspfTypes::V2:
	_lsa_decoder.register_decoder(new RouterLsa(OspfTypes::V2));

	_packet_decoder.register_decoder(new HelloPacket(OspfTypes::V2));
	_packet_decoder.
	    register_decoder(new DataDescriptionPacket(OspfTypes::V2));
	_packet_decoder.
	    register_decoder(new LinkStateUpdatePacket(OspfTypes::V2,
						       _lsa_decoder));
	_packet_decoder.
	    register_decoder(new LinkStateRequestPacket(OspfTypes::V2));
	break;
    case OspfTypes::V3:
	_lsa_decoder.register_decoder(new RouterLsa(OspfTypes::V3));

	_packet_decoder.register_decoder(new HelloPacket(OspfTypes::V3));
	_packet_decoder.
	    register_decoder(new DataDescriptionPacket(OspfTypes::V3));
	_packet_decoder.
	    register_decoder(new LinkStateUpdatePacket(OspfTypes::V3,
						       _lsa_decoder));
	_packet_decoder.
	    register_decoder(new LinkStateRequestPacket(OspfTypes::V3));
	break;
    }

    // Now that all the packet decoders are in place register for
    // receiving packets.
    _io->register_receive(callback(this,&Ospf<A>::receive));

    // The peer manager will solicit packets from the various interfaces.
}

/**
 * All packets for OSPF are received through this interface. All good
 * packets are sent to the peer manager which verifies that the packet
 * is expected and authenticates the packet if necessary. The peer
 * manager can choose to accept the packet in which case it becomes
 * the owner. If the packet is rejected this routine will delete the
 * packet.
 */
template <typename A>
void 
Ospf<A>::receive(const string& interface, const string& vif,
		 A dst, A src, uint8_t* data, uint32_t len)
{
    debug_msg("Interface %s Vif %s dst %s src %s data %p len %u\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);

    Packet *packet;
    try {
	packet = _packet_decoder.decode(data, len);
    } catch(BadPacket& e) {
	XLOG_ERROR("%s", cstring(e));
	return;
    }

    debug_msg("%s\n", packet->str().c_str());
    // We have a packet and its good.

    bool packet_accepted = false;
    try {
	packet_accepted = _peer_manager.receive(interface, vif, dst, src,
						packet);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
    }

    if (!packet_accepted)
	delete packet;
}

template <typename A>
bool
Ospf<A>::enable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    return _io->enable_interface_vif(interface, vif);
}

template <typename A>
bool
Ospf<A>::disable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    return _io->disable_interface_vif(interface, vif);
}

template <typename A>
bool
Ospf<A>::transmit(const string& interface, const string& vif,
		  A dst, A src,
		  uint8_t* data, uint32_t len)
{
    debug_msg("Interface %s Vif %s data %p len %u",
	      interface.c_str(), vif.c_str(), data, len);

    return _io->send(interface, vif, dst, src, data, len);
}

template <typename A>
bool
Ospf<A>::set_network_mask(const string& interface, const string& vif,
			  OspfTypes::AreaID area, 
			  uint32_t network_mask)
{
    try {
	_peer_manager.set_network_mask(_peer_manager.get_peerid(interface,vif),
				       area, network_mask);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_interface_id(const string& interface, const string& vif,
		      OspfTypes::AreaID area,
		      uint32_t interface_id)
{
    try {
	_peer_manager.set_interface_id(_peer_manager.get_peerid(interface,vif),
				       area, interface_id);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_hello_interval(const string& interface, const string& vif,
		   OspfTypes::AreaID area,
		   uint16_t hello_interval)
{
    try {
	_peer_manager.set_hello_interval(_peer_manager.
					 get_peerid(interface, vif),
					 area, hello_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

#if	0
template <typename A>
bool 
Ospf<A>::set_options(const string& interface, const string& vif,
	    OspfTypes::AreaID area,
	    uint32_t options)
{
    try {
	_peer_manager.set_options(_peer_manager.get_peerid(interface, vif),
				  area, options);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}
#endif

template <typename A>
bool
Ospf<A>::set_router_priority(const string& interface, const string& vif,
		    OspfTypes::AreaID area,
		    uint8_t priority)
{
    try {
	_peer_manager.set_router_priority(_peer_manager.
					  get_peerid(interface, vif),
					  area, priority);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_router_dead_interval(const string& interface, const string& vif,
			 OspfTypes::AreaID area,
			 uint32_t router_dead_interval)
{
    try {
	_peer_manager.set_router_dead_interval(_peer_manager.
					       get_peerid(interface,vif),
					       area, router_dead_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}
    
template <typename A>
bool
Ospf<A>::add_route()
{
    return true;
}

template <typename A>
bool
Ospf<A>::delete_route()
{
    return true;
}

template class Ospf<IPv4>;
template class Ospf<IPv6>;
