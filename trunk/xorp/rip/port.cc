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

#ident "$XORP: xorp/rip/port.cc,v 1.13 2004/01/09 00:15:55 hodson Exp $"

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
#include "packet_assembly.hh"
#include "packet_queue.hh"
#include "system.hh"

// ----------------------------------------------------------------------------
// Address Family specific Port methods

PortAFSpecState<IPv4>::PortAFSpecState()
{
    set_auth_handler(new NullAuthHandler());
}

PortAFSpecState<IPv4>::~PortAFSpecState()
{
    delete auth_handler();
}

AuthHandlerBase*
PortAFSpecState<IPv4>::set_auth_handler(AuthHandlerBase* new_handler)
{
    AuthHandlerBase* old_handler = _ah;
    _ah = new_handler;
    return old_handler;
}

const AuthHandlerBase*
PortAFSpecState<IPv4>::auth_handler() const
{
    return _ah;
}

AuthHandlerBase*
PortAFSpecState<IPv4>::auth_handler()
{
    return _ah;
}

// ----------------------------------------------------------------------------
// Generic Port<A> Implementation

template <typename A>
Port<A>::Port(PortManagerBase<A>& pm)
    :  _pm(pm),
       _cost(1),
       _horizon(SPLIT_POISON_REVERSE),
       _advertise(false),
       _adv_def_rt(false),
       _acc_def_rt(false)
{
    _packet_queue = new PacketQueue<A>();
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
	start_peer_gc_timer();
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
Port<A>::start_request_table_timer()
{
    EventLoop& e = _pm.eventloop();
    _rt_timer = e.new_periodic(constants().table_request_period_secs() * 1000,
			       callback(this,
					&Port<A>::request_table_timeout));
}

template <typename A>
void
Port<A>::stop_request_table_timer()
{
    _rt_timer.unschedule();
}

template <typename A>
bool
Port<A>::request_table_timeout()
{
    if (_peers.empty() == false)
	return false;

    RipPacket<A>* pkt = new RipPacket<A>(RIP_AF_CONSTANTS<A>::IP_GROUP(),
					 RIP_AF_CONSTANTS<A>::IP_PORT);

    RequestTablePacketAssembler<A> rtpa(*this);
    if (rtpa.prepare(pkt) == true) {
	_packet_queue->enqueue_packet(pkt);
	counters().incr_table_requests_sent();
    } else {
	XLOG_ERROR("Failed to assemble table request.\n");
	delete pkt;
    }
    push_packets();
    fprintf(stderr, "Sending Request.\n");
    return true;
}

template <typename A>
void
Port<A>::start_peer_gc_timer()
{
    XLOG_ASSERT(_peers.empty() == false);

    EventLoop& e = _pm.eventloop();
    _gc_timer = e.new_periodic(4000,
			       callback(this, &Port<A>::peer_gc_timeout));
}

template <typename A>
bool
Port<A>::peer_gc_timeout()
{
    PeerList::iterator i = _peers.begin();
    while (i != _peers.end()) {
	Peer<A>* pp = *i;
	if (pp->route_count() == 0) {
	    _peers.erase(++i);
	} else {
	    i++;
	}
    }
    return _peers.empty() == false;
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

    if (io_handler()->send(head->address(), head->port(), head->data())) {
	return;
    }

    XLOG_WARNING("Send failed: discarding outbound packets.");
    _packet_queue->flush_packets();
}

template <typename A>
pair<A,uint16_t>
Port<A>::route_policy(const RouteEntry<A>& r) const
{
    if (r.net() == RIP_AF_CONSTANTS<A>::DEFAULT_ROUTE() &&
	advertise_default_route() == false) {
	return make_pair(A::ZERO(), ~0);
    }

    uint16_t cost = r.cost();

    const Peer<A>* peer = dynamic_cast<const Peer<A>*>(r.origin());
    if (peer == 0) {
	// Route did not come from a peer: it's a static route or a
	// redist route.  No horizon checking necessary.
	return make_pair(A::ZERO(), cost);
    }

    const Port<A>& peer_port = peer->port();
    if (&peer_port != this) {
	// Route did not originate from this Port instance. No horizon
	// checking necessary.
	return make_pair(A::ZERO(), cost);
    }

    switch (horizon()) {
    case NONE:
	// No processing
	break;
    case SPLIT:
	// Don't advertise route back to source
	cost = ~0;
	break;
    case SPLIT_POISON_REVERSE:
	// Advertise back at cost of infinity
	cost = RIP_INFINITY;
	break;
    }

    return make_pair(A::ZERO(), cost);
}

template <typename A>
void
Port<A>::port_io_send_completion(bool success)
{
    if (success == false) {
	XLOG_ERROR("Send failed\n");
    }

    const RipPacket<A>* head = _packet_queue->head();
    XLOG_ASSERT(head != 0);
    _packet_queue->pop_head();
    push_packets();
}

template <typename A>
void
Port<A>::port_io_enabled_change(bool en)
{
    if (en) {
	start_request_table_timer();
    } else {
	stop_request_table_timer();
    }
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

template <typename A>
void
Port<A>::parse_request(const Addr&			src_addr,
		       uint16_t				src_port,
		       const PacketRouteEntry<A>*	entries,
		       uint32_t				n_entries)
{
    if (port_io_enabled() == false) {
	XLOG_INFO("Discarding RIP request: port io disabled.");
    }

    if (n_entries == 1 && entries[0].is_table_request()) {
	if (src_port == RIP_AF_CONSTANTS<A>::IP_PORT) {
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


    // XXXX
    // NEED TO DISTINGUISH BETWEEN FULL TABLE QUERIES AND ROUTE QUERIES
    // XXXX

    //
    // Answer query
    //
    uint32_t i = 0;
    ResponsePacketAssembler<A> rpa(*this);
    RouteDB<A>& rdb = _pm.system().route_db();

    while (i != n_entries) {
	RipPacket<A>* pkt = new RipPacket<A>(src_addr, src_port);
	rpa.packet_start(pkt);
	while (rpa.packet_full() == false && i != n_entries) {
	    const RouteEntry<A>* r = rdb.find_route(entries[i].net());
	    if (r) {
		rpa.packet_add_route(r->net(), r->nexthop(),
				     r->cost(), r->tag());
	    } else {
		rpa.packet_add_route(entries[i].net(), A::ZERO(),
				     RIP_INFINITY, 0);
	    }
	    i++;
	}
	if (rpa.packet_finish() == true) {
	    _packet_queue->enqueue_packet(pkt);
	} else {
	    delete pkt;
	    break;
	}
    }

    push_packets();
    block_queries();
}


// ----------------------------------------------------------------------------
// Port<IPv4> Specialized methods
//

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
	if (net == RIP_AF_CONSTANTS<IPv4>::DEFAULT_ROUTE() &&
	    accept_default_route() == false) {
	    continue;
	}

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
    if (af_state().auth_handler() == NULL) {
	XLOG_FATAL("Received packet on interface without an authentication "
		   "handler.");
    }

    const PacketRouteEntry<IPv4>* entries = 0;
    uint32_t n_entries = 0;

    if (af_state().auth_handler()->authenticate(rip_packet,
						rip_packet_bytes,
						entries,
						n_entries) == false) {
	string cause = c_format("packet failed authentication (%s): %s",
				af_state().auth_handler()->name(),
				af_state().auth_handler()->error().c_str());
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
