// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8: 

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/rip/port.hh,v 1.33 2008/10/02 21:58:16 bms Exp $

#ifndef __RIP_PORT_HH__
#define __RIP_PORT_HH__




#include "libxorp/eventloop.hh"

#include "constants.hh"
#include "port_vars.hh"
#include "port_io.hh"

//
// Forward declarations
//
class AuthHandlerBase;

template <typename A>
class OutputTable;

template <typename A>
class OutputUpdates;

template <typename A>
class PacketRouteEntry;

template <typename A>
class PacketQueue;

template <typename A>
class Peer;

template <typename A>
class PortManagerBase;

template <typename A>
class RouteEntry;


/**
 * @short Specializable Address Family state for Port classes.
 *
 * This class exists to be specialized with IPv4 and IPv6 state and
 * methods.
 */
template <typename A>
class PortAFSpecState
{};

/**
 * @short IPv4 specialized Port state.
 *
 * This class holds authentication handler state which is IPv4
 * specific data for a RIP Port.
 */
template <>
class PortAFSpecState<IPv4>
{
private:
    AuthHandlerBase* _ah;

public:
    /**
     * Constructor.
     *
     * Instantiates authentication handler as a @ref NullAuthHandler.
     */
    PortAFSpecState();

    /**
     * Destructor.
     *
     * Deletes authentication handler if non-null.
     */
    ~PortAFSpecState();

    /**
     * Set the authentication handler.
     *
     * @param ah authentication handler to be used.
     * @return pointer to former handler.
     */
    AuthHandlerBase* set_auth_handler(AuthHandlerBase* ah);

    /**
     * Get authentication handler.
     */
    const AuthHandlerBase* auth_handler() const;

    /**
     * Get authentication handler.
     */
    AuthHandlerBase* auth_handler();
};


/**
 * @short IPv6 specialized Port state.
 */
template <>
class PortAFSpecState<IPv6>
{
protected:
    uint32_t _mepp;	// Max route entries per packet

public:
    PortAFSpecState() : _mepp(50) {}

    /**
     * Get the maximum number of route entries placed in each RIPng response
     * packet.
     */
    uint32_t max_entries_per_packet() const	{ return _mepp; }

    /**
     * Set the maximum number of route entries placed in each RIPng response
     * packet.
     */
    void set_max_entries_per_packet(uint32_t n)	{ _mepp = n; }
};


/**
 * @short RIP Port
 *
 * A RIP Port is an origin and sink of RIP packets.  It is uniquely identified
 * by the tuplet of <interface, virtual interface, address>.  The Port sends
 * and receives RIP packets via an attached Port IO object
 * (@ref PortIOBase<A>).  The Port contains a list of Peers (@ref Peer<A>)
 * that it has received communication on and is responsible for updating
 * information sent by peers in the RIP route database (@ref RouteDB<A>).
 */
template <typename A>
class Port
    : public PortIOUserBase<A>
{
public:
    typedef A			Addr;
    typedef list<Peer<A>*>	PeerList;

public:
    Port(PortManagerBase<A>& manager);

    ~Port();

    PortManagerBase<A>& port_manager()		{ return _pm; }

    /**
     * Set enabled state.  When a port is enabled it can perform input
     * and output processing.  When not enabled, input data is
     * discarded and output processing can not occur.
     */
    void set_enabled(bool en);

    /**
     * Get enabled state.
     */
    bool enabled() const			{ return _en; }

    /**
     * Get timer constants in use for routes received on this port.
     */
    PortTimerConstants& constants()		{ return _constants; }

    /**
     * Get timer constants in use for routes received on this port.
     */
    const PortTimerConstants& constants() const	{ return _constants; }

    /**
     * Get Address Family specific state associated with port.  This is
     * state that only has meaning within the IP address family.
     */
    PortAFSpecState<A>& af_state()		{ return _af_state; }

    /**
     * Get Address Family specific state associated with port.  This is
     * state that only has meaning within the IP address family.
     */
    const PortAFSpecState<A>& af_state() const	{ return _af_state; }

    /**
     * Get cost metric associated with Port.
     */
    uint32_t cost() const			{ return _cost; }

    /**
     * Set cost metric associated with Port.
     */
    void set_cost(uint32_t cost)		{ _cost = cost; }

    /**
     * Get horizon type associated with Port.
     */
    const RipHorizon& horizon() const		{ return _horizon; }

    /**
     * Set horizon type associated with Port.
     * @param h horizon type.
     */
    void set_horizon(const RipHorizon& h)	{ _horizon = h; }

    /**
     * Determine whether Port address should be advertised.
     * @return true if port should be advertised to other hosts, false
     * otherwise.
     */
    bool advertise() const			{ return _advertise; }

    /**
     * Set Port advertisement status.
     * @param en true if port should be advertised, false otherwise.
     */
    void set_advertise(bool en)			{ _advertise = en; }

    /**
     * Include default route in RIP response messages.
     * @return true if default route is advertised.
     */
    bool advertise_default_route() const	{ return _adv_def_rt; }

    /**
     * Configure whether default route is advertised in RIP response
     * messages.
     * @param en true if default route should be advertised.
     */
    void set_advertise_default_route(bool en);

    /**
     * Accept default route if found in RIP response messages.
     * @return true if default route should be accepted.
     */
    bool accept_default_route() const		{ return _acc_def_rt; }

    /**
     * Accept default route if found in RIP response messages.
     * @param en true if default route should be accepted.
     */
    void set_accept_default_route(bool en);

    /**
     * Accept routes, but do not advertise them.
     * @return true if this port is passive.
     */
    bool passive() const			{ return _passive; }

    /**
     * Accept routes, but do not advertise them.
     * @param passive true if port should receive and not send packets.
     */
    void set_passive(bool passive);

    /**
     * Accept and respond to non-RIP requests.  Testing and Debugging
     * tools typically send requests from non-RIP IP ports.
     */
    bool accept_non_rip_requests() const	{ return _acc_non_rip_reqs; }

    /**
     * Accept and respond to non-RIP requests.  Testing and Debugging
     * tools typically send requests from non-RIP IP ports.
     */
    void set_accept_non_rip_requests(bool en);

    /**
     * Get Peers associated with this Port.
     */
    const PeerList& peers() const		{ return _peers; }

    /**
     * Get Peers associated with this Port.
     *
     * NB This method is a backdoor for testing purposes and should
     * not be relied upon to exist in future.
     */
    PeerList& peers()				{ return _peers; }

    /**
     * Get counters associated with Port.
     */
    const PortCounters& counters() const	{ return _counters; }

    /**
     *  Get counters associated with Port.
     */
    PortCounters& counters()			{ return _counters; }

    /**
     * Get Peer identified by address.
     *
     * @return pointer to Peer on success, 0 otherwise.
     */
    const Peer<A>* peer(const Addr& addr) const;

    /**
     * Set the maximum packet buffer size.
     */
    void set_max_packet_buffer_bytes(uint32_t max_bytes);

    /**
     * Get the maximum packet buffer size.
     */
    uint32_t set_max_packet_buffer_bytes() const;

    /**
     * Get the current number of bytes buffered in RIP packets.
     */
    uint32_t packet_buffer_bytes() const;

    /**
     * A dummy method to reschedule a non-existing timer.
     */
    void reschedule_dummy_timer() {}

    /**
     * Reschedule request table timer.  If the timer was not running,
     * then don't schedule it.
     */
    void reschedule_request_table_timer();

protected:
    /**
     * Start request table timer.  When there are no peers, this
     * schedules the periodic transmission of request table packets.
     */
    void start_request_table_timer();

    /**
     * Stop request table timer.
     */
    void stop_request_table_timer();

    /**
     * Send request packet.
     * @return true if packet sent, false if no packet sent.
     */
    bool request_table();

    /**
     * Send request packet if there are no peers.
     * @return true if packet sent, false if no packet sent.
     */
    bool request_table_timeout();

    /**
     * Start periodic timer to garbage collect peers.  Timer
     * deschedules itself when no peers exist.
     */
    void start_peer_gc_timer();

    /**
     * Poll peers and remove those with no routes.
     * return true if peers still exist, false otherwise.
     */
    bool peer_gc_timeout();

    /**
     * Kill peer routes.  Change cost of routes learned from peers to
     * infinity.  Typically called when port instance is disabled or
     * io system reports that it is not enabled.
     */
    void kill_peer_routes();

protected:
    /**
     * Get Peer identified by address.
     * @return pointer to Peer on success, 0 otherwise.
     */
    Peer<A>* peer(const Addr& addr);

    /**
     * Create Peer.
     * @return pointer to Peer if created, 0 on failure or peer already exists.
     */
    Peer<A>* create_peer(const Addr& addr);

    /**
     * Record packet arrival.  Updates port and peer counters.
     */
    void record_packet(Peer<A>* p);

    /**
     * Record response packet arrival.
     */
    void record_response_packet(Peer<A>* p);

    /**
     * Record request packet arrival.
     */
    void record_request_packet(Peer<A>* p);

    /**
     * Record bad packet.
     *
     * @param why reason packet marked
     */
    void record_bad_packet(const string& 	why,
			   const Addr&		addr,
			   uint16_t 		port,
			   Peer<A>* 		p);

    /**
     * Record bad authentication packet.
     */
    void record_bad_auth_packet(const string&	why,
				const Addr&	addr,
				uint16_t 	port,
				Peer<A>* 	p);

    /**
     * Record bad route.
     *
     * @param why reason packet marked
     */
    void record_bad_route(const string&	why,
			  const Addr&	src,
			  uint16_t	port,
			  Peer<A>* 	p);

    /**
     * Parse request message.
     */
    void parse_request(const Addr&		src_addr,
		       uint16_t			rc_port,
		       const uint8_t*		entries_ptr,
		       uint32_t			n_entries);

    /**
     * Parse response message.
     */
    void parse_response(const Addr&		src_addr,
			uint16_t		src_port,
			const uint8_t*		entries_ptr,
			uint32_t		n_entries);

    /**
     * Block route queries for amount of time determined by
     * @ref PortTimerConstants::interquery_delay_ms().
     */
    void block_queries();

    /**
     * Determine whether queries are currently blocked and should be
     * discarded.
     */
    bool queries_blocked() const;

    /**
     * Unsolicited update output timer timeout.
     */
    void unsolicited_response_timeout();

    /**
     * Triggered update timeout.
     */
    void triggered_update_timeout();

    /**
     * Query output process policy.
     *
     * Check whether instance is enabled, whether it is active or
     * passive, and whether the associated I/O object is enabled.
     */
    bool output_allowed() const;

    /**
     * Check whether output is allowed and start or stop output
     * processing accordingly.
     */
    void start_stop_output_processing();

    /**
     * Start output processing.
     *
     * Starts timers for unsolicited updates and triggered updates.
     */
    void start_output_processing();

    /**
     * Stop output processing.
     *
     * Stops timers for unsolicited updates and triggered updates.
     */
    void stop_output_processing();

public:
    /**
     * If I/O handler is not already sending a packet, take a packet from
     * packet queue and send it.
     */
    void push_packets();

    /**
     * Check policy on route.
     *
     * @returns tuple (nexthop,cost).  If route should not be
     * advertised the cost value will be greater than RIP_INFINITY.
     */
    pair<A,uint16_t> route_policy(const RouteEntry<A>& re) const;

    /**
     * Send completion notification.  Called by PortIO instance when a
     * send request is completed.
     *
     * @param success indication of whether send completed successfully.
     */
    void port_io_send_completion(bool success);

    /**
     * Receive RIP packet.  Called by PortIO instance when a RIP packet
     * arrives.
     *
     * @param addr source address of packet.
     * @param port source port of packet.
     * @param rip_packet pointer to RIP packet data.
     * @param rip_packet_bytes size of RIP packet data.
     */
    void port_io_receive(const Addr&	src_addr,
			 uint16_t	src_port,
			 const uint8_t*	rip_packet,
			 const size_t	rip_packet_bytes);

    /**
     * Notification that PortIO enabled state has changed.  Called by
     * PortIO when it's enabled status changes.
     *
     * @param en the enabled status of the I/O system.
     */
    void port_io_enabled_change(bool en);

protected:
    PortManagerBase<A>&	_pm;
    PortAFSpecState<A>  _af_state;		// Address family specific data

    PeerList		_peers;			// Peers on Port

    XorpTimer		_rt_timer;		// Request table timer
    XorpTimer		_gc_timer;		// Peer garbage collection
    XorpTimer		_ur_timer;		// Unsolicited response timer
    XorpTimer		_tu_timer;		// Triggered update timer
    XorpTimer		_query_blocked_timer;	// Rate limiting on queries

    bool		_en;			// enabled state
    uint32_t		_cost;			// Cost metric of port
    RipHorizon		_horizon;		// Port Horizon type
    bool		_advertise;		// Advertise IO port
    bool		_adv_def_rt;		// Advertise default route
    bool		_acc_def_rt;		// Accept default route
    bool		_passive;		// Passive (recv only port)
    bool		_acc_non_rip_reqs;	// Accept non-RIP requests

    PacketQueue<A>*	_packet_queue;		// Outbound packet queue
    PortTimerConstants	_constants;		// Port related timer constants
    PortCounters	_counters;		// Packet counters

    OutputTable<A>*	_ur_out;		// Unsolicited update output
    OutputUpdates<A>*	_tu_out;		// Triggered update output
    OutputTable<A>*	_su_out;		// Solicited update output
};

#endif // __RIP_PORT_HH__
