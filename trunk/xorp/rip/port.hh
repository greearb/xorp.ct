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

// $XORP: xorp/rip/port.hh,v 1.13 2004/01/09 00:15:55 hodson Exp $

#ifndef __RIP_PORT_HH__
#define __RIP_PORT_HH__

#include <list>
#include <string>

#include "libxorp/eventloop.hh"

#include "constants.hh"
#include "port_io.hh"

//
// Forward declarations
//
class AuthHandlerBase;

template <typename A>
class PortManagerBase;

template <typename A>
class Peer;

template <typename A>
class PacketQueue;

template <typename A>
class RouteEntry;

template <typename A>
class PacketRouteEntry;


/**
 * @short Container of timer constants associated with a RIP port.
 */
class PortTimerConstants {
public:
    /**
     * Initialize contants with default values from RIPv2 spec.  The values
     * are defined in constants.hh.
     */
    inline PortTimerConstants();

    /**
     * Set the route expiration time.
     * @param t the expiration time in seconds.
     */
    inline void set_expiry_secs(uint32_t t);

    /**
     * Get the route route expiration time.
     * @return expiry time in seconds.
     */
    inline uint32_t expiry_secs() const;

    /**
     * Set the route deletion time.
     * @param t the deletion time in seconds (must be >= 1).
     * @return true on success, false if t == 0.
     */
    inline bool set_deletion_secs(uint32_t t);

    /**
     * Get the route deletion time.
     * @return deletion time in seconds.
     */
    inline uint32_t deletion_secs() const;

    /**
     * Set request packet transmission period.  Request packets are only
     * sent when there are no peers associated with a port.
     * @param t inter-packet interval in seconds.
     */
    inline void set_table_request_period_secs(uint32_t t);

    /**
     * Set request packet transmission period.
     * @return inter-packet interval in seconds.
     */
    inline uint32_t table_request_period_secs() const;

    /**
     * Set the lower bound of the triggered update interval.
     * @param t the lower bound of the triggered update interval in seconds.
     */
    inline void set_triggered_update_min_wait_secs(uint32_t t);

    /**
     * Get the lower bound of the triggered update interval.
     * @return the lower bound of the triggered update interval in seconds.
     */
    inline uint32_t triggered_update_min_wait_secs() const;

    /**
     * Set the upper bound of the triggered update interval.
     * @param t the upper bound of the triggered update interval in seconds.
     */
    inline void set_triggered_update_max_wait_secs(uint32_t t);

    /**
     * Get the upper bound of the triggered update interval.
     * @return the upper bound of the triggered update interval in seconds.
     */
    inline uint32_t triggered_update_max_wait_secs() const;

    /**
     * Set the interpacket packet delay.
     * @param t the interpacket delay for back-to-back packets in
     * milliseconds.
     * @return true on success, false if t is greater than
     * MAXIMUM_INTERPACKET_DELAY_MS.
     */
    inline bool	set_interpacket_delay_ms(uint32_t t);

    /**
     * Get the interpacket packet delay in milliseconds.
     */
    inline uint32_t interpacket_delay_ms() const;

    /**
     * Set the interquery gap.  This is the minimum temporal gap between
     * route request packets that query specific routes.  Queries arriving
     * at a faster rate are ignored.
     * @param t the interquery delay in milliseconds.
     */
    inline void	set_interquery_delay_ms(uint32_t t);

    /**
     * Get the interquery gap.  This is the minimum temporal gap between
     * route request packets that query specific routes.  Fast arriving
     * queries are ignored.
     * @return the interquery delay in milliseconds.
     */
    inline uint32_t interquery_delay_ms() const;

protected:
    uint32_t _expiry_secs;
    uint32_t _deletion_secs;
    uint32_t _table_request_secs;
    uint32_t _triggered_update_min_wait_secs;
    uint32_t _triggered_update_max_wait_secs;
    uint32_t _interpacket_msecs;
    uint32_t _interquery_msecs;
};


/**
 * @short Container of counters associated with a Port.
 */
struct PortCounters {
public:
    PortCounters() : _packets_recv(0), _bad_routes(0), _bad_packets(0),
		     _tr_sent(0), _tr_recv(), _triggered_updates(0)
    {}

    /**
     * Get the total number of packets received.
     */
    inline uint32_t packets_recv() const	{ return _packets_recv; }

    /**
     * Increment the total number of packets received.
     */
    inline void incr_packets_recv()		{ _packets_recv++; }

    /**
     * Get the number of bad routes received (eg invalid metric,
     * invalid address family).
     */
    inline uint32_t bad_routes() const		{ return _bad_routes; }

    /**
     * Increment the number of bad routes received.
     */
    inline void incr_bad_routes()		{ _bad_routes++; }

    /**
     * Get the number of bad response packets received.
     */
    inline uint32_t bad_packets() const		{ return _bad_packets; }

    /**
     * Increment the number of bad response packets received.
     */
    inline void incr_bad_packets()		{ _bad_packets++; }

    /**
     * Get the number of triggered updates sent.
     */
    inline uint32_t triggered_updates() const	{ return _triggered_updates; }

    /**
     * Increment the number of triggered updates sent.
     */
    inline void incr_triggered_updates() 	{ _triggered_updates++; }

    /**
     * Get the number of table requests sent.
     */
    inline uint32_t table_requests_sent() const	{ return _tr_sent; }

    /**
     * Increment the number of table requests updates sent.
     */
    inline void incr_table_requests_sent() 	{ _tr_sent++; }

    /**
     * Get the number of table requests received.
     */
    inline uint32_t table_requests_recv() const	{ return _tr_recv; }

    /**
     * Increment the number of table requests updates received.
     */
    inline void incr_table_requests_recv() 	{ _tr_recv++; }

protected:
    uint32_t _packets_recv;
    uint32_t _bad_routes;
    uint32_t _bad_packets;
    uint32_t _tr_sent;			// table requests sent
    uint32_t _tr_recv;			// table requests received
    uint32_t _triggered_updates;
};


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
     * Set authentication handler.
     *
     * @param h authentication handler to be used.
     * @return pointer to former handler.
     */
    AuthHandlerBase* set_auth_handler(AuthHandlerBase* h);

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
    inline uint32_t max_entries_per_packet() const	{ return _mepp; }

    /**
     * Set the maximum number of route entries placed in each RIPng response
     * packet.
     */
    inline void set_max_entries_per_packet(uint32_t n)	{ _mepp = n; }
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

    /**
     * Get timer constants in use for routes received on this port.
     */
    inline PortTimerConstants& constants()		{ return _constants; }

    /**
     * Get timer constants in use for routes received on this port.
     */
    inline const PortTimerConstants& constants() const	{ return _constants; }

    /**
     * Get Address Family specific state associated with port.  This is
     * state that only has meaning within the IP address family.
     */
    inline PortAFSpecState<A>& af_state()		{ return _af_state; }

    /**
     * Get Address Family specific state associated with port.  This is
     * state that only has meaning within the IP address family.
     */
    inline const PortAFSpecState<A>& af_state() const	{ return _af_state; }

    /**
     * Get cost metric associated with Port.
     */
    inline uint32_t cost() const			{ return _cost; }

    /**
     * Set cost metric associated with Port.
     */
    inline void set_cost(uint32_t cost)			{ _cost = cost; }

    /**
     * Get horizon type associated with Port.
     */
    inline const RipHorizon& horizon() const		{ return _horizon; }

    /**
     * Set horizon type associated with Port.
     * @param h horizon type.
     */
    inline void set_horizon(const RipHorizon& h)	{ _horizon = h; }

    /**
     * Determine whether Port address should be advertised.
     * @return true if port should be advertised to other hosts, false
     * otherwise.
     */
    bool advertise() const				{ return _advertise; }

    /**
     * Set Port advertisement status.
     * @param en true if port should be advertised, false otherwise.
     */
    inline void set_advertise(bool en)			{ _advertise = en; }

    /**
     * Include default route in RIP response messages.
     * @return true if default route is advertised.
     */
    inline bool advertise_default_route() const		{ return _adv_def_rt; }

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
    inline bool accept_default_route() const		{ return _acc_def_rt; }

    /**
     * Accept default route if found in RIP response messages.
     * @param en true if default route should be accepted.
     */
    void set_accept_default_route(bool en);

    /**
     * Get Peers associated with this Port.
     */
    inline const PeerList& peers() const		{ return _peers; }

    /**
     * Get Peers associated with this Port.
     *
     * NB This method is a backdoor for testing purposes and should
     * not be relied upon to exist in future.
     */
    inline PeerList& peers()				{ return _peers; }

    /**
     * Get counters associated with Port.
     */
    inline const PortCounters& counters() const		{ return _counters; }

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

protected:
    /**
     *  Get counters associated with Port.
     */
    inline PortCounters& counters()			{ return _counters; }

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
     * Record bad packet.
     *
     * @param why reason packet marked
     */
    void record_bad_packet(const string& 	why,
			   const Addr&		addr,
			   uint16_t 		port,
			   Peer<A>* 		p);

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
    void parse_request(const Addr&		  src_addr,
		       uint16_t			  src_port,
		       const PacketRouteEntry<A>* entries,
		       uint32_t			  n_entries);

    /**
     * Parse response message.
     */
    void parse_response(const Addr&		   src_addr,
			uint16_t		   src_port,
			const PacketRouteEntry<A>* entries,
			uint32_t		   n_entries);

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
    XorpTimer		_us_timer;		// Unsolicited update timer
    XorpTimer		_tu_timer;		// Triggered update timer
    XorpTimer		_query_blocked_timer;	// Rate limiting on queries

    uint32_t		_cost;			// Cost metric of port
    RipHorizon		_horizon;		// Port Horizon type
    bool		_advertise;		// Advertise IO port
    bool		_adv_def_rt;		// Advertise default route
    bool		_acc_def_rt;		// Accept default route

    PacketQueue<A>*	_packet_queue;		// Outbound packet queue
    PortTimerConstants	_constants;		// Port related timer constants
    PortCounters	_counters;		// Packet counters
};


// ----------------------------------------------------------------------------
// Inline PortTimerConstants accessor and modifiers.

PortTimerConstants::PortTimerConstants()
    : _expiry_secs(DEFAULT_EXPIRY_SECS),
      _deletion_secs(DEFAULT_DELETION_SECS),
      _table_request_secs(DEFAULT_TABLE_REQUEST_SECS),
      _triggered_update_min_wait_secs(DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS),
      _triggered_update_max_wait_secs(DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS),
      _interpacket_msecs(DEFAULT_INTERPACKET_DELAY_MS),
      _interquery_msecs(DEFAULT_INTERQUERY_GAP_MS)
{
}

inline void
PortTimerConstants::set_expiry_secs(uint32_t t)
{
    _expiry_secs = t;
}

inline uint32_t
PortTimerConstants::expiry_secs() const
{
    return _expiry_secs;
}

inline bool
PortTimerConstants::set_deletion_secs(uint32_t t)
{
    if (t < 1)
	return false;
    _deletion_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::deletion_secs() const
{
    return _deletion_secs;
}

inline void
PortTimerConstants::set_table_request_period_secs(uint32_t t)
{
    _table_request_secs = t;
}

inline uint32_t
PortTimerConstants::table_request_period_secs() const
{
    return _table_request_secs;
}

inline void
PortTimerConstants::set_triggered_update_min_wait_secs(uint32_t t)
{
    _triggered_update_min_wait_secs = t;
}

inline uint32_t
PortTimerConstants::triggered_update_min_wait_secs() const
{
    return _triggered_update_min_wait_secs;
}

inline void
PortTimerConstants::set_triggered_update_max_wait_secs(uint32_t t)
{
    _triggered_update_max_wait_secs = t;
}

inline uint32_t
PortTimerConstants::triggered_update_max_wait_secs() const
{
    return _triggered_update_max_wait_secs;
}

inline bool
PortTimerConstants::set_interpacket_delay_ms(uint32_t t)
{
    if (t > MAXIMUM_INTERPACKET_DELAY_MS)
	return false;
    _interpacket_msecs = t;
    return true;
}

inline uint32_t
PortTimerConstants::interpacket_delay_ms() const
{
    return _interpacket_msecs;
}

inline void
PortTimerConstants::set_interquery_delay_ms(uint32_t t)
{
    _interquery_msecs = t;
}

inline uint32_t
PortTimerConstants::interquery_delay_ms() const
{
    return _interquery_msecs;
}

#endif // __RIP_PORT_HH__
