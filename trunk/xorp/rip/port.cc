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

#ident "$XORP: xorp/rip/port.cc,v 1.8 2003/07/21 18:05:55 hodson Exp $"

#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "constants.hh"
#include "packets.hh"

#include "auth.hh"
#include "peer.hh"
#include "port.hh"
#include "port_manager.hh"

#include "packet_queue.hh"
#include "update_queue.hh"

// ----------------------------------------------------------------------------
// PortTimerConstants Implementation

PortTimerConstants::PortTimerConstants()
    : _expiry_secs(DEFAULT_EXPIRY_SECS),
      _deletion_secs(DEFAULT_DELETION_SECS),
      _triggered_update_min_wait_secs(DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS),
      _triggered_update_max_wait_secs(DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS),
      _interpacket_msecs(DEFAULT_INTERPACKET_DELAY_MS),
      _interquery_msecs(DEFAULT_INTERQUERY_GAP_MS)
{
}


// ----------------------------------------------------------------------------
// Generic Port<A> Implementation

template <typename A>
Port<A>::Port(PortManagerBase<A>& pm)
    :  _pm(pm),
       _update_queue(pm.system().route_db().update_queue()),
       _en(false),
       _cost(1),
       _horizon(SPLIT_POISON_REVERSE),
       _advertise(false),
       _adv_def_rt(false),
       _acc_def_rt(false)
{
    _packet_queue = new RipPacketQueue<A>();
}

template <typename A>
Port<A>::~Port()
{
    while (_peers.empty() == false) {
	delete _peers.front();
	_peers.pop_front();
    }
    delete _packet_queue;
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
    XLOG_INFO("RIP port %s/%s/%s received bad packet from %s:%u - %s\n",
	      _pio->ifname().c_str(), _pio->vifname().c_str(),
	      _pio->address().str().c_str(), host.str().c_str(), port,
	      why.c_str());
    counters().incr_bad_packets();
    if (p) {
	p->counters().incr_bad_packets();
    }
}

template <typename A>
void
Port<A>::record_bad_route(const string&	why,
			  const Addr&	host,
			  uint16_t	port,
			  Peer<A>*	p)
{
    XLOG_INFO("RIP port %s/%s/%s received bad route from %s:%u - %s\n",
	      _pio->ifname().c_str(), _pio->vifname().c_str(),
	      _pio->address().str().c_str(), host.str().c_str(), port,
	      why.c_str());
    counters().incr_bad_routes();
    if (p) {
	p->counters().incr_bad_routes();
    }
}

static void
noop()
{}

template <typename A>
void
Port<A>::block_queries()
{
    EventLoop& e = _pm.eventloop();
    _query_blocked_timer
	= e.new_oneoff_after_ms(constants().interquery_delay_ms(),
				callback(noop));
}

template <typename A>
bool
Port<A>::queries_blocked() const
{
    return _query_blocked_timer.scheduled();
}

template <typename A>
void
Port<A>::push_packets()
{
    if (io_handler()->pending())
	return;

    const RipPacket<A>* head = _packet_queue->head();
    if (head == 0)
	return;

    if (io_handler()->send(head->address(), head->port(),
			   head->data_ptr(), head->data_size())) {
	return;
    }

    XLOG_WARNING("Send failed: discarding outbound packets.");
    _packet_queue->flush_packets();
}

template <typename A>
void
Port<A>::port_io_send_completion(const uint8_t*	rip_packet,
				 bool		success)
{
    if (success == false) {
	XLOG_ERROR("Send failed");
    }

    const RipPacket<A>* head = _packet_queue->head();
    XLOG_ASSERT(head->data_ptr() == rip_packet);
    _packet_queue->pop_head();
    push_packets();
}

template <typename A>
void
Port<A>::port_io_enabled_change(bool)
{
}

template <typename A>
void
Port<A>::set_advertise_default_route(bool en)
{
    _adv_def_rt = en;
}

template <typename A>
void
Port<A>::set_accept_default_route(bool en)
{
    _acc_def_rt = en;
}

// ----------------------------------------------------------------------------
// AuthManager<IPv4> Specialized methods
//

AuthHandlerBase*
AuthManager<IPv4>::set_auth_handler(AuthHandlerBase* nh)
{
    AuthHandlerBase* oh = _ah;
    _ah = nh;
    return oh;
}

const AuthHandlerBase*
AuthManager<IPv4>::auth_handler() const
{
    return _ah;
}

AuthHandlerBase*
AuthManager<IPv4>::auth_handler()
{
    return _ah;
}

// ----------------------------------------------------------------------------
// Port<IPv4> Specialized methods
//

template <>
void
Port<IPv4>::parse_request(const Addr&			src_addr,
			  uint16_t			src_port,
			  const PacketRouteEntry<IPv4>*	entries,
			  uint32_t			n_entries)
{
    if (port_io_enabled() == false) {
	XLOG_INFO("Discarding RIP request: port io disabled.");
    }

    if (n_entries == 1 &&
	entries[0].addr_family() == PacketRouteEntry<IPv4>::ADDR_FAMILY_DUMP) {
	if (src_port == RIP_PORT) {
	    // if already doing unsolicited dump, then ignore
	    // set unsolicited timer timeout to zero to trigger port
	    // route dump
	} else {
	    if (queries_blocked())
		return;
	    // if already doing a debug dump, then ignore
	    // start debug route dump
	    block_queries();
	}
	return;
    }

    if (queries_blocked())
	return;

    //
    // Answer query
    //
    RipPacket<IPv4>* p = _packet_queue->new_packet(src_addr, src_port);
    if (0 == p) {
	XLOG_INFO("Could not allocate packet for route request response");
	return;
    }

    if (n_entries > auth_handler()->max_routing_entries()) {
	n_entries = auth_handler()->max_routing_entries();
    }

    size_t rip_packet_bytes = sizeof(RipPacketHeader);
    rip_packet_bytes += (n_entries + auth_handler()->head_entries()) *
	sizeof(PacketRouteEntry<IPv4>);

    p->data().resize(rip_packet_bytes);

    // Fill in header
    RipPacketHeader* rph = new (p->data_ptr()) RipPacketHeader;
    rph->initialize(RipPacketHeader::RESPONSE, 1);

    uint32_t offset = sizeof(RipPacketHeader) +
	auth_handler()->head_entries() * sizeof(PacketRouteEntry<IPv4>);

    // Walk nets in supplied route entries and look them up
    RouteDB<IPv4>& rdb = _pm.system().route_db();
    for (uint32_t i = 0; i < n_entries; i++) {
	const RouteEntry<IPv4>* r = rdb.find_route(entries[i].net());
	PacketRouteEntry<IPv4>* pre =
	    new (p->data_ptr() + offset) PacketRouteEntry<IPv4>;
	if (r) {
	    pre->initialize(r->tag(), r->net(), r->nexthop(), r->cost());
	} else {
	    pre->initialize(0, entries[i].net(), IPv4::ZERO(), RIP_INFINITY);
	}
	offset += sizeof(PacketRouteEntry<IPv4>);
    }
    XLOG_ASSERT(offset == rip_packet_bytes);

    // Authenticate packet (this commonly forces a copy, and may force 2,
    // auth api needs looking at).
    vector<uint8_t> trailor;
    PacketRouteEntry<IPv4>* fe =
	reinterpret_cast<PacketRouteEntry<IPv4>*>(rph + 1);

    if (auth_handler()->authenticate(p->data_ptr(), rip_packet_bytes,
				     fe, trailor) != 0) {
	p->data().insert(p->data().end(), trailor.begin(), trailor.end());
	_packet_queue->enqueue_packet(p);
	push_packets();
    }  else {
	XLOG_WARNING("Response packet authentication failed for query.");
	_packet_queue->discard_packet(p);
    }
    block_queries();
}

template <>
void
Port<IPv4>::parse_response(const Addr&				src_addr,
			   uint16_t				src_port,
			   const PacketRouteEntry<IPv4>*	entries,
			   uint32_t				n_entries)
{
    static IPv4 local_net("127.0.0.0");
    static IPv4 net_filter("255.0.0.0");
    static IPv4 class_e_net("240.0.0.0");
    IPv4 zero;

    Peer<IPv4>* p = peer(src_addr);
    if (p == 0)
	p = create_peer(src_addr);

    RouteDB<IPv4>& rdb = _pm.system().route_db();

    for (uint32_t i = 0; i < n_entries; i++) {
	if (entries[i].addr_family() != AF_INET) {
	    record_bad_route("bad address family", src_addr, src_port, p);
	    continue;
	}

	uint16_t metric = entries[i].metric();
	if (metric > RIP_INFINITY) {
	    record_bad_route("bad metric", src_addr, src_port, p);
	    continue;
	}

	IPv4Net net = entries[i].net();
	IPv4	masked_net = net.masked_addr() & net_filter;
	if (masked_net.is_multicast()) {
	    record_bad_route("multicast route", src_addr, src_port, p);
	    continue;
	}
	if (masked_net == local_net) {
	    record_bad_route("loopback route", src_addr, src_port, p);
	    continue;
	}
	if (masked_net >= class_e_net) {
	    record_bad_route("experimental route", src_addr, src_port, p);
	    continue;
	}
	if (masked_net == zero) {
	    if (net.prefix_len() != 0) {
		record_bad_route("net 0", src_addr, src_port, p);
		continue;
	    } else if (accept_default_route() == false) {
		record_bad_route("default route", src_addr, src_port, p);
		continue;
	    }
	}

	//
	// XXX review
	// Should we check nh is visible to us here or not?
	//
	IPv4 nh = entries[i].nexthop();
	if (nh == zero) {
	    nh = src_addr;
	}

	metric += metric + cost();
	if (metric > RIP_INFINITY) {
	    metric = RIP_INFINITY;
	}

	//
	// XXX review
	// Want to do anything with tag?
	//
	uint16_t tag = entries[i].tag();

	rdb.update_route(net, nh, metric, tag, p);
    }
}

template <>
void
Port<IPv4>::port_io_receive(const IPv4&		src_address,
			    uint16_t 		src_port,
			    const uint8_t*	rip_packet,
			    size_t		rip_packet_bytes)
{
    static_assert(sizeof(RipPacketHeader) == 4);
    static_assert(sizeof(PacketRouteEntry<IPv4>) == 20);

    Peer<IPv4>* p = peer(src_address);
    record_packet(p);

    if (rip_packet_bytes < RIPv2_MIN_PACKET_BYTES) {
	record_bad_packet(c_format("Packet size less than minimum (%u < %u)",
				   uint32_t(rip_packet_bytes),
				   uint32_t(RIPv2_MIN_PACKET_BYTES)),
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
    } else if (ph->valid_version(RipPacketHeader::IPv4_VERSION) == false) {
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

    //
    // Authenticate packet (actually we should check what packet wants
    // before authenticating - we may not care in all cases...)
    //
    if (auth_handler() == NULL) {
	XLOG_FATAL("Received packet on interface without an authentication "
		   "handler.");
    }

    const PacketRouteEntry<IPv4>* entries = 0;
    uint32_t n_entries = 0;

    if (auth_handler()->authenticate(rip_packet,
				     rip_packet_bytes,
				     entries,
				     n_entries) == false) {
	string cause = c_format("packet failed authentication (%s): %s",
				auth_handler()->name(),
				auth_handler()->error().c_str());
	record_bad_packet(cause, src_address, src_port, p);
	return;
    }

    if (n_entries == 0) {
	// No entries in packet, nothing further to do.
	return;
    }

    if (src_port == RIP_PORT && ph->command == RipPacketHeader::RESPONSE) {
	parse_response(src_address, src_port, entries, n_entries);
    } else {
	XLOG_ASSERT(ph->command == RipPacketHeader::REQUEST);
	parse_request(src_address, src_port, entries, n_entries);
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

