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

#ident "$XORP: xorp/rip/port.cc,v 1.6 2003/07/09 00:11:02 hodson Exp $"

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
#include "update_queue.hh"

// ----------------------------------------------------------------------------
// PortTimerConstants Implementation

PortTimerConstants::PortTimerConstants()
    : _expiry_secs(DEFAULT_EXPIRY_SECS),
      _deletion_secs(DEFAULT_DELETION_SECS),
      _triggered_update_min_wait_secs(DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS),
      _triggered_update_max_wait_secs(DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS),
      _interpacket_msecs(DEFAULT_INTERPACKET_DELAY_MS)
{
}


// ----------------------------------------------------------------------------
// PortPacketQueue implementation

/**
 * @short Structure to contain outbound packet state.
 */
template <typename A>
class PortPacket
{
protected:
    A		    _addr;
    uint16_t	    _port;
    vector<uint8_t> _data;

public:
    PortPacket(const A& addr, uint16_t port)
	: _addr(addr), _port(port)
    {}
    inline const A& address() const		{ return _addr; }
    inline uint16_t port() const		{ return _port; }
    inline vector<uint8_t>& data() 		{ return _data; }
    inline const vector<uint8_t>& data() const	{ return _data; }
    inline uint32_t size() const 		{ return _data.size(); }
};

/**
 * @short Outbound packet queue
 */
template <typename A>
class PortPacketQueue
{
public:
    typedef list<PortPacket<A> > PacketQueue;
public:
    PortPacketQueue(Port<A>& p)
	: _port(p), _buffered_bytes(0), _max_buffered_bytes(64000)
    {}

    /**
     * Create a queued packet for a particular destination.  The packet
     * is created in the PacketQueue.  This call must be followed by
     * enqueue_packet() or discard_packet() before being called again.
     *
     * @return 0 if there is insufficient buffer space for a new packet.
     */
    PortPacket<A>* new_packet(const A& addr, uint16_t port);

    /**
     * Place packet in ready to sent queue.
     */
    void enqueue_packet(const PortPacket<A>* pkt);

    /**
     * Release state associated with packet.
     */
    void discard_packet(const PortPacket<A>* pkt);

    /**
     * Ack packet that has previously been sent.
     * @param data pointer to data segment of packet.
     */
    void dequeue_packet(const uint8_t* data);

    /**
     * Flush queued packets.
     */
    void flush_packets();

    /**
     * Set the maximum amount of data to buffer.
     */
    void set_max_buffered_bytes(uint32_t mb);

    /**
     * Get the maximum amount of buffered data.
     */
    uint32_t max_buffered_bytes() const;

    /**
     * Get the current amount of buffered data.
     */
    uint32_t buffered_bytes() const;

protected:
    void push_queue();

protected:
    Port<A>&	_port;
    XorpTimer	_pkt_timer;
    PacketQueue _ready_packets;
    PacketQueue _candidate_packet;
    uint32_t	_buffered_bytes;
    uint32_t	_max_buffered_bytes;
};

template <typename A>
PortPacket<A>*
PortPacketQueue<A>::new_packet(const A& addr, uint16_t port)
{
    XLOG_ASSERT(_candidate_packet.empty() == true);
    if (_buffered_bytes >= _max_buffered_bytes) {
	return 0;
    }
    _candidate_packet.push_back(PortPacket<A>(addr, port));
    return &_candidate_packet.back();
}

template <typename A>
void
PortPacketQueue<A>::enqueue_packet(const PortPacket<A>* pkt)
{
    XLOG_ASSERT(_candidate_packet.empty() == false);
    XLOG_ASSERT(pkt == &_candidate_packet.back());
    _buffered_bytes += pkt->size();
    _ready_packets.splice(_ready_packets.end(),
			  _candidate_packet,
			  _candidate_packet.begin());
    push_queue();
}

template <typename A>
void
PortPacketQueue<A>::discard_packet(const PortPacket<A>* pkt)
{
    XLOG_ASSERT(_candidate_packet.empty() == false);
    XLOG_ASSERT(pkt == &_candidate_packet.back());
    _candidate_packet.erase(_candidate_packet.begin());
}

template <typename A>
void
PortPacketQueue<A>::dequeue_packet(const uint8_t* data)
{
    typename PacketQueue::iterator i = _ready_packets.begin();
    while (i != _ready_packets.end()) {
	if (&(i->data()[0]) == data) {
	    _buffered_bytes -= i->size();
	    _ready_packets.erase(i);
	    return;
	}
	i++;
    }
    XLOG_WARNING("Could not free packet.");
}

template <typename A>
void
PortPacketQueue<A>::flush_packets()
{
    while (_ready_packets.empty() == false) {
	_buffered_bytes -= _ready_packets.front().size();
	_ready_packets.pop_front();
    }
}

template <typename A>
void
PortPacketQueue<A>::set_max_buffered_bytes(uint32_t mbb)
{
    _max_buffered_bytes = mbb;
}

template <typename A>
uint32_t
PortPacketQueue<A>::max_buffered_bytes() const
{
    return _max_buffered_bytes;
}

template <typename A>
uint32_t
PortPacketQueue<A>::buffered_bytes() const
{
    return _buffered_bytes;
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
    _packet_queue = new PortPacketQueue<A>(*this);
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
Port<IPv4>::parse_request(const Addr&			/* src_addr */,
			  uint16_t			/* src_port */,
			  const PacketRouteEntry<IPv4>*	/* entries */,
			  uint32_t			/* n_entries */)
{
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
	    record_bad_route("bad inet family", src_addr, src_port, p);
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

