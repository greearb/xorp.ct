// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/random.h"
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

#include "output_table.hh"
#include "output_updates.hh"

// ----------------------------------------------------------------------------
// Utilities

inline static uint32_t
range_random(uint32_t lo, uint32_t hi)
{
    if (hi < lo) swap(hi, lo);
    return lo + ( xorp_random() % (hi - lo) );
}

// ----------------------------------------------------------------------------
// Address Family specific Port methods

#ifdef INSTANTIATE_IPV4

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

#endif // INSTANTIATE_IPV4


// ----------------------------------------------------------------------------
// Generic Port<A> Implementation

template <typename A>
Port<A>::Port(PortManagerBase<A>& pm)
    :  _pm(pm),
       _en(false),
       _cost(1),
       _horizon(SPLIT_POISON_REVERSE),
       _advertise(false),
       _adv_def_rt(true),
       _acc_def_rt(true),
       _passive(false),
       _acc_non_rip_reqs(true),
       _ur_out(0),
       _tu_out(0),
       _su_out(0)
{
    _packet_queue = new PacketQueue<A>();
}

template <typename A>
Port<A>::~Port()
{
    stop_output_processing();

    delete _ur_out;
    delete _su_out;
    delete _tu_out;

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

	EventLoop& e = _pm.eventloop();
	TimeVal now;
	e.current_time(now);
	peer->set_last_active(now);
	start_peer_gc_timer();
	return peer;
    }
    return 0;
}

template <typename A>
Peer<A>*
Port<A>::peer(const Addr& addr)
{
    typename PeerList::iterator i = find_if(_peers.begin(), _peers.end(),
					    peer_has_address<A>(addr));
    return (i == _peers.end()) ? 0 : *i;
}

template <typename A>
const Peer<A>*
Port<A>::peer(const Addr& addr) const
{
    typename PeerList::const_iterator i = find_if(_peers.begin(), _peers.end(),
						  peer_has_address<A>(addr));
    return (i == _peers.end()) ? 0 : *i;
}

template <typename A>
void
Port<A>::unsolicited_response_timeout()
{
    debug_msg("Unsolicited response timeout %p\n", this);

    //
    // Fast forward triggered updater because we're about to dump entire
    // table.
    //
    if (_tu_out->running()) {
	_tu_out->ffwd();
    }

    //
    // Check if unsolicited response process already exists and kill
    // it if so.
    //
    if (_ur_out->running()) {
	XLOG_WARNING("Starting unsolicited response process while an "
		     "existing one is already running.\n");
	_ur_out->stop();
    }

    // Start output process.
    _ur_out->start();

    //
    // Reschedule this callback in next interval
    //
    TimeVal interval = TimeVal(constants().update_interval(), 0);
    double factor = constants().update_jitter() / 100.0;
    _ur_timer.reschedule_after(random_uniform(interval, factor));
}

template <typename A>
void
Port<A>::triggered_update_timeout()
{
    debug_msg("Triggered update timeout %p\n", this);
    {
	RouteDB<A>& rdb = _pm.system().route_db();
	UNUSED(rdb);
	debug_msg("- Route DB routes = %u\n",
		  XORP_UINT_CAST(rdb.route_count()));
    }

    // Table dump is running, we should not be doing triggered updates.
    if (_ur_out->running())
	goto reschedule;

    //
    // Push triggered updater along.  It wont be running if we've just
    // instantiated it, or if it was running and ran out of updates to
    // announce.
    //
    if (_tu_out->running() == false) {
	_tu_out->start();
    }

 reschedule:
    TimeVal delay = TimeVal(constants().triggered_update_delay(), 0);
    double factor = constants().triggered_update_jitter() / 100.0;
    _tu_timer.reschedule_after(random_uniform(delay, factor));

}

template <typename A>
void
Port<A>::start_output_processing()
{
    EventLoop& e = _pm.eventloop();
    RouteDB<A>& rdb = _pm.system().route_db();

    // Create triggered update output process
    _tu_out = new OutputUpdates<A>(e, *this, *_packet_queue, rdb);

    // Schedule triggered update process
    TimeVal interval = TimeVal(constants().update_interval(), 0);
    double factor = constants().update_jitter() / 100.0;
    _ur_timer =
	e.new_oneoff_after(random_uniform(interval, factor),
			   callback(this,
				    &Port<A>::unsolicited_response_timeout));

    // Create unsolicited response (table dump) output process
    _ur_out = new OutputTable<A>(e, *this, *_packet_queue, rdb);

    // Schedule unsolicited output process
    TimeVal delay = TimeVal(constants().triggered_update_delay(), 0);
    factor = constants().triggered_update_jitter() / 100.0;
    _tu_timer =
	e.new_oneoff_after(random_uniform(delay, factor),
			   callback(this,
				    &Port<A>::triggered_update_timeout));
}

template <typename A>
void
Port<A>::stop_output_processing()
{
    delete _ur_out;
    _ur_out = 0;

    delete _tu_out;
    _tu_out = 0;

    _ur_timer.unschedule();
    _tu_timer.unschedule();
}

template <typename A>
void
Port<A>::start_request_table_timer()
{
    EventLoop& e = _pm.eventloop();

    if (constants().table_request_period_secs() == 0) {
	// Don't start the timer, but cancel it instead
	_rt_timer.unschedule();
	return;
    }
    _rt_timer = e.new_periodic_ms(
	constants().table_request_period_secs() * 1000,
	callback(this, &Port<A>::request_table_timeout));
}

template <typename A>
void
Port<A>::reschedule_request_table_timer()
{
    if (! _rt_timer.scheduled())
	return;

    start_request_table_timer();
}

template <typename A>
void
Port<A>::stop_request_table_timer()
{
    _rt_timer.unschedule();
}

template <typename A>
bool
Port<A>::request_table()
{
    RipPacket<A>* pkt = new RipPacket<A>(RIP_AF_CONSTANTS<A>::IP_GROUP(),
					 RIP_AF_CONSTANTS<A>::IP_PORT);

    list<RipPacket<A>*> auth_packets;
    RequestTablePacketAssembler<A> rtpa(*this);
    if (rtpa.prepare(pkt, auth_packets) == true) {
	typename list<RipPacket<A>*>::iterator iter;
	for (iter = auth_packets.begin(); iter != auth_packets.end(); ++iter) {
	    RipPacket<A>* auth_pkt = *iter;
	    _packet_queue->enqueue_packet(auth_pkt);
	    counters().incr_table_requests_sent();
	}
    } else {
	XLOG_ERROR("Failed to assemble table request.\n");
    }
    delete pkt;

    push_packets();
    debug_msg("Sending Request.\n");
    return true;
}

template <typename A>
bool
Port<A>::request_table_timeout()
{
    if (_peers.empty() == false)
	return false;

    return (request_table());
}

template <typename A>
void
Port<A>::start_peer_gc_timer()
{
    XLOG_ASSERT(_peers.empty() == false);

    // Set peer garbage collection timeout to 180 seconds since for RIP
    // MIB we need to keep track of quiescent peers for this long.
    EventLoop& e = _pm.eventloop();
    _gc_timer = e.new_periodic_ms(180 * 1000,
				  callback(this, &Port<A>::peer_gc_timeout));
}

template <typename A>
bool
Port<A>::peer_gc_timeout()
{
    typename PeerList::iterator i = _peers.begin();
    while (i != _peers.end()) {
	Peer<A>* pp = *i;

	if (pp->route_count() == 0) {
	    delete pp;
	    _peers.erase(i++);
	} else {
	    ++i;
	}
    }

    if (_peers.empty()) {
	start_request_table_timer();
	return false;
    }
    return true;
}

template <typename A>
void
Port<A>::record_packet(Peer<A>* p)
{
    counters().incr_packets_recv();
    if (p) {
	EventLoop& e = _pm.eventloop();
	TimeVal now;
	e.current_time(now);
	p->counters().incr_packets_recv();
	p->set_last_active(now);
    }
}

template <typename A>
void
Port<A>::record_response_packet(Peer<A>* p)
{
    counters().incr_update_packets_recv();
    if (p) {
	p->counters().incr_update_packets_recv();
    }
}

template <typename A>
void
Port<A>::record_request_packet(Peer<A>* p)
{
    counters().incr_table_requests_recv();
    if (p) {
	p->counters().incr_table_requests_recv();
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
	      this->_pio->ifname().c_str(), this->_pio->vifname().c_str(),
	      this->_pio->address().str().c_str(), host.str().c_str(), port,
	      why.c_str());
    UNUSED(why); // in case XLOG_INFO is compiled out.
    UNUSED(host);
    UNUSED(port);

    counters().incr_bad_packets();
    if (p) {
	p->counters().incr_bad_packets();
    }
}

template <typename A>
void
Port<A>::record_bad_auth_packet(const string&	why,
				const Addr&	host,
				uint16_t	port,
				Peer<A>*	p)
{
    XLOG_INFO("RIP port %s/%s/%s authentication failed %s:%u - %s\n",
	      this->_pio->ifname().c_str(), this->_pio->vifname().c_str(),
	      this->_pio->address().str().c_str(), host.str().c_str(), port,
	      why.c_str());
    UNUSED(why); // in case XLOG_INFO is compiled out.
    UNUSED(host);
    UNUSED(port);

    counters().incr_bad_auth_packets();
    if (p) {
	p->counters().incr_bad_auth_packets();
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
	      this->_pio->ifname().c_str(), this->_pio->vifname().c_str(),
	      this->_pio->address().str().c_str(), host.str().c_str(), port,
	      why.c_str());

    UNUSED(why); // in case XLOG_INFO is compiled out.
    UNUSED(host);
    UNUSED(port);

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
    if (this->io_handler()->pending())
	return;

    const RipPacket<A>* head = _packet_queue->head();
    if (head == 0)
	return;

    if (this->io_handler()->send(head->address(), head->port(), 
				 head->data())) {
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
	return make_pair(A::ZERO(), RIP_MAX_COST);
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
	cost = RIP_MAX_COST;
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
    start_stop_output_processing();
    if (en == false)
	kill_peer_routes();
}

template <typename A>
void
Port<A>::start_stop_output_processing()
{
    if (output_allowed()) {
	start_request_table_timer();
	start_output_processing();
	request_table();
    } else {
	stop_request_table_timer();
	stop_output_processing();
    }
}

template <typename A>
void
Port<A>::kill_peer_routes()
{
#ifdef INSTANTIATE_IPV4
    // Reset the authentication handler
    PortAFSpecState<IPv4>& pss = af_state();
    if (pss.auth_handler() != NULL)
	pss.auth_handler()->reset();
#endif

    typename PeerList::iterator pli = _peers.begin();
    while (pli != _peers.end()) {
	vector<const RouteEntry<A>*> routes;
	Peer<A>* p = *pli;
	p->dump_routes(routes);
	typename vector<const RouteEntry<A>*>::const_iterator ri;
	for (ri = routes.begin(); ri != routes.end(); ++ri) {
	    const RouteEntry<A>* r = *ri;
	    p->update_route(r->net(), r->nexthop(), RIP_INFINITY, r->tag(),
			    r->policytags());
	}
	pli++;
    }
}

template <typename A>
bool
Port<A>::output_allowed() const
{
    return enabled() && this->port_io_enabled() && (passive() == false);
}

template <typename A>
void
Port<A>::set_enabled(bool en)
{
    bool old_allowed = output_allowed();
    _en = en;
    bool allowed = output_allowed();
    if (allowed != old_allowed) {
	start_stop_output_processing();
    }

    if (en == false)
	kill_peer_routes();
}

template <typename A>
void
Port<A>::set_passive(bool p)
{
    bool old_allowed = output_allowed();
    _passive = p;
    bool allowed = output_allowed();
    if (allowed != old_allowed) {
	start_stop_output_processing();
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
Port<A>::set_accept_non_rip_requests(bool en)
{
    _acc_non_rip_reqs = en;
}

template <typename A>
void
Port<A>::parse_request(const Addr&			src_addr,
		       uint16_t				src_port,
		       const uint8_t*			entries_ptr,
		       uint32_t				n_entries)
{
    if (this->port_io_enabled() == false) {
	debug_msg("Discarding RIP request: port io system not enabled.");
	return;
    }

    const PacketRouteEntry<A> pre(entries_ptr);

    if (n_entries == 1 && pre.is_table_request()) {
	if (src_port == RIP_AF_CONSTANTS<A>::IP_PORT) {
	    Peer<A>* p = peer(src_addr);
	    if (p == 0) {
		p = create_peer(src_addr);
		p->counters().incr_packets_recv();
		p->counters().incr_table_requests_recv();
	    }
	    // if already doing unsolicited dump, then ignore
	    // set unsolicited timer timeout to zero to trigger port
	    // route dump
	    unsolicited_response_timeout();
	} else {
	    if (queries_blocked())
		return;

	    // if already doing a debug dump, then ignore
	    // start debug route dump
	    if (_su_out && _su_out->running())
		return;

	    // Delete existing solicited update output, which is just lying
	    // around, and re-instantiate to reply to table dump request
	    delete _su_out;

	    EventLoop& e = _pm.eventloop();
	    RouteDB<A>& rdb = _pm.system().route_db();
	    _su_out = new OutputTable<A>(e, *this, *_packet_queue, rdb,
					 src_addr, src_port);
	    _su_out->start();

	    block_queries();
	}
	return;
    }

    if (queries_blocked())
	return;

    //
    // This is a query for a set of routes.  Answer it.
    //
    uint32_t i = 0;
    ResponsePacketAssembler<A> rpa(*this);
    RouteDB<A>& rdb = _pm.system().route_db();

    while (i != n_entries) {
	RipPacket<A>* pkt = new RipPacket<A>(src_addr, src_port);
	rpa.packet_start(pkt);
	while (rpa.packet_full() == false && i != n_entries) {
	    const uint8_t* pre_ptr = entries_ptr + i * PacketRouteEntry<A>::size();
	    const PacketRouteEntry<A> pre(pre_ptr);
	    if (pre.prefix_len() > A::ADDR_BITLEN) {
		// Route request has an address with a bad prefix length
		// Unfortunately it's non-trivial for us to propagate this
		// back to the offending enquirer so we just stop processing
		// the request.
		delete (pkt);
		break;
	    }

	    const RouteEntry<A>* r = rdb.find_route(pre.net());
	    if (r) {
		rpa.packet_add_route(r->net(), r->nexthop(),
				     r->cost(), r->tag());
	    } else {
		rpa.packet_add_route(pre.net(), A::ZERO(),
				     RIP_INFINITY, 0);
	    }
	    i++;
	}

	list<RipPacket<A>*> auth_packets;
	if (rpa.packet_finish(auth_packets) == true) {
	    typename list<RipPacket<A>*>::iterator iter;
	    for (iter = auth_packets.begin(); iter != auth_packets.end(); ++iter) {
		RipPacket<A>* auth_pkt = *iter;
		_packet_queue->enqueue_packet(auth_pkt);
		counters().incr_non_rip_updates_sent();
	    }
	    delete pkt;
	} else {
	    delete pkt;
	    break;
	}
    }

    push_packets();
    block_queries();
}

template <typename A>
void
Port<A>::port_io_receive(const A&	src_address,
			 uint16_t 	src_port,
			 const uint8_t*	rip_packet,
			 size_t		rip_packet_bytes)
{
    string why;

    static_assert(RipPacketHeader::SIZE == 4);
    static_assert(PacketRouteEntry<A>::SIZE == 20);

    if (enabled() == false) {
	debug_msg("Discarding RIP packet: Port not enabled.");
	return;
    }

    Peer<A>* p = 0;
    if (src_port == RIP_AF_CONSTANTS<A>::IP_PORT) {
	p = peer(src_address);
    } else {
	if (accept_non_rip_requests() == false) {
	    return;
	}
	XLOG_ASSERT(p == 0);
    }

    record_packet(p);

    if (rip_packet_bytes < RIPv2_MIN_PACKET_BYTES) {
	why = c_format("Packet size less than minimum (%u < %u)",
		       XORP_UINT_CAST(rip_packet_bytes),
		       XORP_UINT_CAST(RIPv2_MIN_PACKET_BYTES));
	record_bad_packet(why, src_address, src_port, p);
	return;
    }

    const RipPacketHeader rph(rip_packet);

    //
    // Basic RIP packet header validity checks
    //
    if (rph.valid_command() == false) {
	why = c_format("Invalid command");
	record_bad_packet(why, src_address, src_port, p);
	return;
    } else if (rph.valid_version(RIP_AF_CONSTANTS<A>::PACKET_VERSION) == false) {
	why = c_format("Invalid version (%d)", rph.version());
	record_bad_packet(why, src_address, src_port, p);
	return;
    } else if (rph.valid_padding() == false) {
	why = c_format("Invalid padding (%u,%u)",
		       rph.unused0(), rph.unused1());
	record_bad_packet(why,src_address, src_port, p);
	return;
    }

    //
    // Check this is not an attempt to inject routes from non-RIP port
    //
    if (rph.command() == RipPacketHeader::RESPONSE &&
	src_port != RIP_AF_CONSTANTS<A>::IP_PORT) {
	why = c_format("RIP response originating on wrong port (%d != %d)",
		       src_port, RIP_AF_CONSTANTS<A>::IP_PORT);
	record_bad_packet(why, src_address, src_port, p);
	return;
    }

#if defined (INSTANTIATE_IPV4)
    const uint8_t* entries_ptr = NULL;
    uint32_t n_entries = 0;
    bool new_peer = (p == NULL);

    if ((p != NULL) && (p->route_count() == 0)) {
	//
	// XXX: If the peer hasn't been active for long enough, then
	// consider it a new peer for authentication purpose.
	// The reason we need this modification is because the idle
	// peer state may be kept for a little bit too long (e.g., 2*180
	// seconds), and if the peer is restarted before that we won't
	// accept its initial packet with sequence number of zero.
	// With this modification we can accept the first authentication
	// packet with sequence number of zero immediately after
	// all routes have expired (e.g., after 300 seconds).
	//
	new_peer = true;
    }

    if (af_state().auth_handler()->authenticate_inbound(rip_packet,
							rip_packet_bytes,
							entries_ptr,
							n_entries,
							src_address,
							new_peer) == false) {
	why = c_format("packet failed authentication (%s): %s",
		       af_state().auth_handler()->effective_name(),
		       af_state().auth_handler()->error().c_str());
	record_bad_auth_packet(why, src_address, src_port, p);
	return;
    }

    if (n_entries == 0) {
	// No entries in packet, nothing further to do.
	return;
    }
#elif defined (INSTANTIATE_IPV6)
    const uint8_t* entries_ptr = rip_packet + RipPacketHeader::size();
    uint32_t n_entries = (rip_packet_bytes - RipPacketHeader::size()) /
	PacketRouteEntry<A>::size();
    size_t calc_bytes = n_entries * PacketRouteEntry<A>::size()
	+ RipPacketHeader::size();
    if (calc_bytes != rip_packet_bytes) {
	why = c_format("Packet did not contain an integral number of route "
		       "entries");
	record_bad_packet(why, src_address, src_port, p);
    }
#endif

    if (src_port == RIP_AF_CONSTANTS<A>::IP_PORT &&
	rph.command() == RipPacketHeader::RESPONSE) {
	record_response_packet(p);
	parse_response(src_address, src_port, entries_ptr, n_entries);
    } else {
	XLOG_ASSERT(rph.command() == RipPacketHeader::REQUEST);
	if (src_port == RIP_AF_CONSTANTS<A>::IP_PORT) {
	    record_request_packet(p);
	} else {
	    counters().incr_non_rip_requests_recv();
	}
	parse_request(src_address, src_port, entries_ptr, n_entries);
    }
}


// ----------------------------------------------------------------------------
// Port<IPv4> Specialized methods
//

#ifdef INSTANTIATE_IPV4

template <>
void
Port<IPv4>::parse_response(const Addr&				src_addr,
			   uint16_t				src_port,
			   const uint8_t*			entries_ptr,
			   uint32_t				n_entries)
{
    static IPv4 net_filter("255.0.0.0");
    static IPv4 class_b_net("128.0.0.0");
    static IPv4 class_c_net("192.0.0.0");
    static IPv4 class_d_net("224.0.0.0");
    static IPv4 class_e_net("240.0.0.0");
    string why;

    Peer<Addr>* p = peer(src_addr);
    if (p == 0) {
	p = create_peer(src_addr);
	p->counters().incr_packets_recv();
	p->counters().incr_update_packets_recv();
    }

    for (uint32_t i = 0; i < n_entries; i++) {
	const uint8_t* pre_ptr = entries_ptr + i * PacketRouteEntry<IPv4>::size();
	const PacketRouteEntry<IPv4> pre(pre_ptr);
	if (pre.addr_family() != AF_INET) {
	    why = c_format("Bad address family (%u instead of %u)",
			   pre.addr_family(), AF_INET);
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}

	uint16_t metric = pre.metric();
	if (metric > RIP_INFINITY) {
	    why = c_format("Bad metric (%u > %u)", metric, RIP_INFINITY);
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}

	uint32_t prefix_len = pre.prefix_len();
	if (prefix_len > Addr::ADDR_BITLEN) {
	    why = c_format("Bad prefix length (%u > %u)",
			   prefix_len, Addr::ADDR_BITLEN);
	    record_bad_packet(why, src_addr, src_port, p);
	    continue;
	}

	IPv4Net net = pre.net();
	IPv4 addr = pre.addr();
	if (prefix_len == 0 && addr != IPv4::ZERO()) {
	    // Subnet mask not specified, thus apply a clasfull mask
	    if (addr < class_b_net) {
	        prefix_len = 8;			// Class A
	    } else if (addr < class_c_net) {
		prefix_len = 16;		// Class B
	    } else if (addr < class_d_net) {
		prefix_len = 24;		// Class C
	    } else {
		prefix_len = 32;		// XXX check RFC!
	    }
	    net = IPv4Net(IPv4(addr), prefix_len);
	}
	if (net == RIP_AF_CONSTANTS<Addr>::DEFAULT_ROUTE() &&
	    accept_default_route() == false) {
	    continue;
	}

	IPv4 masked_net = net.masked_addr() & net_filter;
	if (masked_net.is_multicast()) {
	    why = c_format("Multicast route (%s)", masked_net.str().c_str());
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}
	if (masked_net.is_loopback()) {
	    why = c_format("Loopback route (%s)", masked_net.str().c_str());
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}
	if (masked_net >= class_e_net) {
	    why = c_format("Experimental route (%s)",
			   masked_net.str().c_str());
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}
	if (masked_net == IPv4::ZERO()) {
	    if (net.prefix_len() != 0) {
		why = c_format("Net 0");
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    } else if (accept_default_route() == false) {
		why = c_format("Default route");
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    }
	}
	if (prefix_len == Addr::ADDR_BITLEN) {
	    //
	    // Check if the route is for one of my own addresses or
	    // for a directly connected broadcast address.
	    //
	    bool my_addr_found = false;
	    bool bcast_addr_found = false;
	    
	    const IfMgrIfTree& iftree = _pm.iftree();
	    IfMgrIfTree::IfMap::const_iterator if_iter;
	    for (if_iter = iftree.interfaces().begin();
		 if_iter != iftree.interfaces().end();
		 ++if_iter) {
		const IfMgrIfAtom& iface = if_iter->second;

		// Test if interface is enabled and the link state is up
		if ((! iface.enabled()) || iface.no_carrier())
		    continue;

		IfMgrIfAtom::VifMap::const_iterator vif_iter;
		for (vif_iter = iface.vifs().begin();
		     vif_iter != iface.vifs().end();
		     ++vif_iter) {
		    const IfMgrVifAtom& vif = vif_iter->second;

		    // Test if vif is enabled
		    if (! vif.enabled())
			continue;

		    //
		    // Test if there is a matching interface address
		    // or a broadcast address.
		    //
		    IfMgrVifAtom::IPv4Map::const_iterator a4_iter;

		    for (a4_iter = vif.ipv4addrs().begin();
			 a4_iter != vif.ipv4addrs().end();
			 ++a4_iter) {
			const IfMgrIPv4Atom& a4 = a4_iter->second;

			if (! a4.enabled())
			    continue;

			// Test if my own address
			if (a4.addr() == net.masked_addr()) {
			    my_addr_found = true;
			    break;
			}

			// Test if the broadcast address
			if (a4.has_broadcast()
			    && (a4.broadcast_addr() == net.masked_addr())) {
			    bcast_addr_found = true;
			    break;
			}
		    }
		}
	    }

	    if (my_addr_found) {
		why = c_format("My interface address (%s)",
			       net.masked_addr().str().c_str());
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    }
	    if (bcast_addr_found) {
		why = c_format("My broadcast address (%s)",
			       net.masked_addr().str().c_str());
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    }
	}

	IPv4 nh = pre.nexthop();
	if (nh == IPv4::ZERO()) {
	    nh = src_addr;
	} else if (nh == _pio->address()) {
	    // Nexthop points to us, ignore route (either poison-rev or bogus)
	    continue;
        } else {
	    // Test if nh is on the receiving subnet
	    const IfMgrIfTree& iftree = _pm.iftree();
	    const IfMgrIPv4Atom* ifa = iftree.find_addr(_pio->ifname(),
							_pio->vifname(),
							_pio->address());
	    XLOG_ASSERT(ifa != NULL);
	    if (IPv4Net(nh, ifa->prefix_len())
		!= IPv4Net(ifa->addr(), ifa->prefix_len())) {
		nh = src_addr;
	    }
	}

	metric += cost();
	if (metric > RIP_INFINITY) {
	    metric = RIP_INFINITY;
	}

	//
	// XXX review
	// Want to do anything with tag?
	//
	uint16_t tag = pre.tag();

	p->update_route(net, nh, metric, tag, PolicyTags());
    }
}

#endif // INSTANTIATE_IPV4


// ----------------------------------------------------------------------------
// Port<IPv6> Specialized methods
//

#ifdef INSTANTIATE_IPV6

template <>
void
Port<IPv6>::parse_response(const Addr&				src_addr,
			   uint16_t				src_port,
			   const uint8_t*			entries_ptr,
			   uint32_t				n_entries)
{
    string why;

    Peer<Addr>* p = peer(src_addr);
    if (p == 0) {
	p = create_peer(src_addr);
	p->counters().incr_packets_recv();
	p->counters().incr_update_packets_recv();
    }

    // ALL_ONES is used as a magic value to indicate no nexthop has been set.
    IPv6 nh = IPv6::ALL_ONES();
    for (uint32_t i = 0; i < n_entries; i++) {
	const uint8_t* pre_ptr = entries_ptr + i * PacketRouteEntry<IPv6>::size();
	const PacketRouteEntry<IPv6> pre(pre_ptr);
	if (pre.is_nexthop()) {
	    nh = pre.nexthop();
	    if (! nh.is_linklocal_unicast())
		nh = IPv6::ZERO();
	    if (nh == IPv6::ZERO()) {
		nh = src_addr;
	    }
	    continue;
	} else if (nh == IPv6::ALL_ONES()) {
	    why = c_format("Route specified before nexthop");
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}

	uint16_t metric = pre.metric();
	if (metric > RIP_INFINITY) {
	    why = c_format("Bad metric (%u > %u)", metric, RIP_INFINITY);
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}

	if (pre.prefix_len() > Addr::ADDR_BITLEN) {
	    why = c_format("Bad prefix length (%u > %u)",
			   pre.prefix_len(), Addr::ADDR_BITLEN);
	    record_bad_packet(why, src_addr, src_port, p);
	    continue;
	}

	IPv6Net net = pre.net();

	IPv6 masked_net = net.masked_addr();
	if (masked_net.is_multicast()) {
	    why = c_format("Multicast route (%s)", masked_net.str().c_str());
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}
	if (masked_net.is_linklocal_unicast()) {
	    why = c_format("Linklocal route (%s)", masked_net.str().c_str());
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}
	if (masked_net.is_loopback()) {
	    why = c_format("Loopback route (%s)", masked_net.str().c_str());
	    record_bad_route(why, src_addr, src_port, p);
	    continue;
	}

	if (masked_net == IPv6::ZERO()) {
	    if (net.prefix_len() != 0) {
		why = c_format("Net 0");
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    } else if (accept_default_route() == false) {
		why = c_format("Default route");
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    }
	}

	if (pre.prefix_len() == Addr::ADDR_BITLEN) {
	    //
	    // Check if the route is for one of my own addresses.
	    //
	    bool my_addr_found = false;
	    
	    const IfMgrIfTree& iftree = _pm.iftree();
	    IfMgrIfTree::IfMap::const_iterator if_iter;
	    for (if_iter = iftree.interfaces().begin();
		 if_iter != iftree.interfaces().end();
		 ++if_iter) {
		const IfMgrIfAtom& iface = if_iter->second;

		// Test if interface is enabled and the link state is up
		if ((! iface.enabled()) || iface.no_carrier())
		    continue;

		IfMgrIfAtom::VifMap::const_iterator vif_iter;
		for (vif_iter = iface.vifs().begin();
		     vif_iter != iface.vifs().end();
		     ++vif_iter) {
		    const IfMgrVifAtom& vif = vif_iter->second;

		    // Test if vif is enabled
		    if (! vif.enabled())
			continue;

		    //
		    // Test if there is a matching interface address
		    // or a broadcast address.
		    //
		    IfMgrVifAtom::IPv6Map::const_iterator a6_iter;

		    for (a6_iter = vif.ipv6addrs().begin();
			 a6_iter != vif.ipv6addrs().end();
			 ++a6_iter) {
			const IfMgrIPv6Atom& a6 = a6_iter->second;

			if (! a6.enabled())
			    continue;

			// Test if my own address
			if (a6.addr() == net.masked_addr()) {
			    my_addr_found = true;
			    break;
			}
		    }
		}
	    }

	    if (my_addr_found) {
		why = c_format("My interface address (%s)",
			       net.masked_addr().str().c_str());
		record_bad_route(why, src_addr, src_port, p);
		continue;
	    }
	}

	metric += metric + cost();
	if (metric > RIP_INFINITY) {
	    metric = RIP_INFINITY;
	}

	//
	// XXX review
	// Want to do anything with tag?
	//
	uint16_t tag = pre.tag();

	p->update_route(net, nh, metric, tag, PolicyTags());
    }

}

#endif // INSTANTIATE_IPV6


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class Port<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class Port<IPv6>;
#endif
