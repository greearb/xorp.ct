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

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "ospf.hh"

template <typename A>
Ospf<A>::Ospf(OspfTypes::Version version, IO* io)
    : _version(version), _io(io), _lsa_decoder(version), _peer_manager(*this),
      _database(*this)
{
    // Register the packets with the decoder.
    switch(get_version()) {
    case OspfTypes::V2:
	_lsa_decoder.register_decoder(new RouterLsa(OspfTypes::V2));

	_packet_decoder.register_decoder(new HelloPacket(OspfTypes::V2));
	_packet_decoder.
	    register_decoder(new DataDescriptionPacket(OspfTypes::V2));
	_packet_decoder.
	    register_decoder(new LinkStateRequestPacket(OspfTypes::V2));
	break;
    case OspfTypes::V3:
	_lsa_decoder.register_decoder(new RouterLsa(OspfTypes::V3));

	_packet_decoder.register_decoder(new HelloPacket(OspfTypes::V3));
	_packet_decoder.
	    register_decoder(new DataDescriptionPacket(OspfTypes::V3));
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
 * is expected and authenticates the packet if necessary. If the
 * packet contains LSAs and it is approriate the packet is sent to the
 * link state database manager. The packet is deleted by the receive
 * routine so if a copy is required the peer manager should take a
 * copy.
 */
template <typename A>
void 
Ospf<A>::receive(const char* interface, const char *vif,
		 const char *src,
		 const char *dst,
		 uint8_t* data, uint32_t len)
{
    debug_msg("Interface %s Vif %s src %s dst %s data %p len %u",
	      interface, vif, src, dst, data, len);

    Packet *packet;
    try {
	packet = _packet_decoder.decode(data, len);
    } catch(BadPacket& e) {
	XLOG_ERROR("%s", cstring(e));
	return;
    }

    debug_msg("%s\n", packet->str().c_str());
    // We have a packet and its good.

    _peer_manager.incoming_packet(packet);

    delete packet;
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
