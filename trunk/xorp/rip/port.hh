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

// $XORP: xorp/rip/port.hh,v 1.4 2003/04/23 17:06:48 hodson Exp $

#ifndef __RIP_PORT_HH__
#define __RIP_PORT_HH__

#include <list>
#include <string>

#include "libxorp/eventloop.hh"

#include "constants.hh"
#include "port_io.hh"
#include "update_queue.hh"

/**
 * @short Container of timer constants associated with a RIP port.
 */
class PortTimerConstants {
public:
    /**
     * Initialize contants with default values from RIPv2 spec.  The values
     * are defined in constants.hh.
     */
    PortTimerConstants();

    /**
     * Set the route expiration time.
     * @param t the expiration time in seconds.
     */
    inline void		set_expiry_secs(uint32_t t);

    /**
     * Get the route route expiration time.
     * @return expiry time in seconds.
     */
    inline uint32_t	expiry_secs() const;

    /**
     * Set the route deletion time.
     * @param t the deletion time in seconds.
     */
    inline void		set_deletion_secs(uint32_t t);

    /**
     * Get the route deletion time.
     * @return deletion time in seconds.
     */
    inline uint32_t	deletion_secs() const;

    /**
     * Set the lower bound of the triggered update interval.
     * @param t the lower bound of the triggered update interval in seconds.
     */
    inline void		set_triggered_update_min_wait_secs(uint32_t t);

    /**
     * Get the lower bound of the triggered update interval.
     * @return the lower bound of the triggered update interval in seconds.
     */
    inline uint32_t	triggered_update_min_wait_secs() const;

    /**
     * Set the upper bound of the triggered update interval.
     * @param t the upper bound of the triggered update interval in seconds.
     */
    inline void		set_triggered_update_max_wait_secs(uint32_t t);

    /**
     * Get the upper bound of the triggered update interval.
     * @return the upper bound of the triggered update interval in seconds.
     */
    inline uint32_t	triggered_update_max_wait_secs() const;

    /**
     * Set the interpacket packet delay.
     * @param t the interpacket delay for back-to-back packets in
     * milliseconds.
     */
    inline void		set_interpacket_delay_ms(uint32_t t);

    /**
     * Get the interpacket packet delay in milliseconds.
     */
    inline uint32_t	interpacket_delay_ms() const;

protected:
    uint32_t _expiry_secs;
    uint32_t _deletion_secs;
    uint32_t _triggered_update_min_wait_secs;
    uint32_t _triggered_update_max_wait_secs;
    uint32_t _interpacket_msecs;
};


/**
 * @short Container of counters associated with a Port.
 */
struct PortCounters {
public:
    PortCounters() : _packets_recv(0), _bad_routes(0), _bad_packets(0),
		     _triggered_updates(0)
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

protected:
    uint32_t _packets_recv;
    uint32_t _bad_routes;
    uint32_t _bad_packets;
    uint32_t _triggered_updates;
};


/**
 * @short Authentication Manager.
 *
 * A class completely for specialization for RIP address families that place
 * authentication information within the RIP payload (RIPv2 and not RIPng).
 *
 */
template <typename A>
struct AuthManager
{};

class AuthHandlerBase;

/**
 * @short Authentication Manager for RIPv2.
 *
 * Holds configuration state for RIPv2 Authentication.
 */
template<>
struct AuthManager<IPv4>
{
public:
    /**
     * Set authentication handler.
     *
     * @param h handler to be used.
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

private:
    AuthHandlerBase* _ah;
};


template <typename A>
class PortManagerBase;

template <typename A>
class Peer;

template <typename A>
class PortPacketQueue;

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
    : public AuthManager<A>,
      public PortIOUserBase<A>
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
    inline PortTimerConstants& constants() { return _constants; }

    /**
     * Get timer constants in use for routes received on this port.
     */
    inline const PortTimerConstants& constants() const { return _constants; }

    /**
     * Set enabled state.
     */
    void set_enabled(bool en);

    /**
     * Get enabled state.
     */
    inline bool enabled() const				{ return _en; }

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
    inline bool advertise() const			{ return _advertise; }

    /**
     * Set Port advertisement status.
     * @param en true if port should be advertised, false otherwise.
     */
    inline void set_advertise(bool en)			{ _advertise = en; }

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
    void record_bad_packet(const string&	why,
			   const Addr&		addr,
			   uint16_t 		port,
			   Peer<A>* 		p);

    /**
     * Parse request message.
     *
     * @param rip_request
     */
    void parse_request(const Addr&	src_addr,
		       uint16_t		src_port,
		       const uint8_t*	rip_request,
		       size_t		rip_request_bytes);

protected:
    /**
     * Send completion notification.  Called by PortIO instance when a
     * send request is completed.
     *
     * @param rip_packet pointer to RIP packet data requested to be sent.
     * @param success indication of whether send completed successfully.
     */
    void port_io_send_completion(const uint8_t*	rip_packet,
				 bool		success);

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
    PeerList		_peers;			// Peers on Port
    XorpTimer		_us_timer;		// Unsolicited update timer
    XorpTimer		_tu_timer;		// Triggered update timer

    UpdateQueue<A>&		 _update_queue; // RIP instance update queue
    UpdateQueue<A>::ReadIterator _uq_iter;

    bool		_en;			// Enabled state
    uint32_t		_cost;			// Cost metric of peer
    RipHorizon		_horizon;		// Port Horizon type
    bool		_advertise;		// Advertise IO port

    PortPacketQueue<A>*	_packet_queue;		// Outbound packet queue
    PortTimerConstants	_constants;		// Port related timer constants
    PortCounters	_counters;		// Packet counters
};


// ----------------------------------------------------------------------------
// Inline PortTimerConstants accessor and modifiers.

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

inline void
PortTimerConstants::set_deletion_secs(uint32_t t)
{
    _deletion_secs = t;
}

inline uint32_t
PortTimerConstants::deletion_secs() const
{
    return _deletion_secs;
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

inline void
PortTimerConstants::set_interpacket_delay_ms(uint32_t t)
{
    _interpacket_msecs = t;
}

inline uint32_t
PortTimerConstants::interpacket_delay_ms() const
{
    return _interpacket_msecs;
}

#endif // __RIP_PORT_HH__
