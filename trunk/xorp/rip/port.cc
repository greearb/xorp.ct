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

#ident "$XORP: xorp/rip/port.cc,v 1.1 2003/04/10 00:27:43 hodson Exp $"

#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "constants.hh"
#include "packets.hh"
#include "port.hh"
#include "port_manager.hh"

// ----------------------------------------------------------------------------
// PortTimerConstants Implementation
//

PortTimerConstants::PortTimerConstants()
    : _expiry_secs(DEFAULT_EXPIRY_SECS),
      _deletion_secs(DEFAULT_DELETION_SECS),
      _triggered_update_min_wait_secs(DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS),
      _triggered_update_max_wait_secs(DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS)
{
}

// ----------------------------------------------------------------------------
// Generic Port<A> Implementation

template <typename A>
Port<A>::Port(PortManagerBase<A>& pm)
    : _pm(pm), _en(false), _cost(1), _horizon(SPLIT_POISON_REVERSE),
      _advertise(false)
{
}

template <typename A>
Port<A>::~Port()
{
    while (_peers.empty() == false) {
	delete _peers.front();
	_peers.pop_front();
    }
}

template <typename A>
Peer<A>*
Port<A>::create_peer(const Addr& addr)
{
    if (peer(addr) == 0) {
	Peer<A>* peer = new Peer<A>(*this, addr);
	_peers.push_back(peer);
	return peer;
    }
    return 0;
}

template <typename A>
Peer<A>*
Port<A>::peer(const Addr& addr)
{
    typename PeerList::iterator i = _peers.begin();
    while (i != _peers.end()) {
	if ((*i)->address() == addr) {
	    return *i;
	}
	++i;
    }
    return 0;
}

template <typename A>
const Peer<A>*
Port<A>::peer(const Addr& addr) const
{
    typename PeerList::const_iterator i = _peers.begin();
    while (i != _peers.end()) {
	if ((*i)->address() == addr) {
	    return *i;
	}
	++i;
    }
    return 0;
}

template <typename A>
void
Port<A>::record_packet(Peer<A>* p)
{
    counters().incr_packets_recv();
    if (p) {
	p->counters().incr_packets_recv();
    }
}


template <typename A>
void
Port<A>::record_bad_packet(const string& why,
			   const Addr&	 host,
			   uint16_t	 port,
			   Peer<A>*	 p)
{
    XLOG_INFO("Bad packet from %s:%u (%s)\n",
	      host.str().c_str(), port, why.c_str());
    counters().incr_bad_packets();
    if (p) {
	p->counters().incr_bad_packets();
    }
}

template <typename A>
void
Port<A>::port_io_send_completion(const uint8_t*	/* rip_packet */,
				 bool		/* success */)
{
}

template <typename A>
void
Port<A>::port_io_enabled_change(bool)
{
}

// ----------------------------------------------------------------------------
// Port<IPv4> Specialized methods
//

template <>
void
Port<IPv4>::port_io_receive(const IPv4&		src_address,
			    uint16_t 		src_port,
			    const uint8_t*	rip_packet,
			    size_t		rip_packet_bytes)
{
    Peer<IPv4>* p = peer(src_address);
    record_packet(p);

    static_assert(sizeof(RipPacketHeader) == 4);
    if (rip_packet_bytes < RIPv2_MIN_PACKET_SIZE) {
	record_bad_packet(c_format("Packet size less than minimum (%d < %d)",
				   uint32_t(rip_packet_bytes),
				   RIPv2_MIN_PACKET_SIZE),
			  src_address, src_port, p);
	return;
    }
    
    const RipPacketHeader *ph =
	reinterpret_cast<const RipPacketHeader*>(rip_packet);

    //
    // Basic RIP packet header validity checks
    //
    if (ph->valid_command() == false) {
	record_bad_packet("Invalid command", src_address, src_port, p);
	return;
    } else if (ph->valid_version(2) == false) {
	record_bad_packet(c_format("Invalid version (%d).", ph->version),
			  src_address, src_port, p);
	return;
    } else if (ph->valid_padding() == false) {
	record_bad_packet(c_format("Invalid padding (%u,%u).",
				   ph->unused[0], ph->unused[1]),
			  src_address, src_port, p);
	return;
    }

    //
    // Check this is not an attempt to inject routes from non-RIP port
    //
    if (ph->command == RipPacketHeader::RESPONSE && src_port != RIP_PORT) {
	record_bad_packet(c_format("RIP response originating on wrong port"
				   " (%d != %d)", src_port, RIP_PORT),
			  src_address, src_port, p);
	return;
    }
    
    if (src_port == RIP_PORT) {
	// Expecting a query
	
    } else {

    }
}

// ----------------------------------------------------------------------------
// Port<IPv6> Specialized methods
//

template <>
void
Port<IPv6>::port_io_receive(const IPv6&		/* address */,
			    uint16_t 		/* port */,
			    const uint8_t*	/* rip_packet */,
			    size_t		/* rip_packet_bytes */
			    )
{
}

template class Port<IPv4>;
template class Port<IPv6>;

