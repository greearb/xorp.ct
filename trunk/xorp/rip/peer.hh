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

// $XORP: xorp/rip/peer.hh,v 1.1 2003/04/10 00:27:43 hodson Exp $

#ifndef __RIP_PEER_HH__
#define __RIP_PEER_HH__

#include "libxorp/timeval.hh"
#include "route_entry.hh"

/**
 * @short Container of counters associated with a peer.
 */
struct PeerCounters {
public:
    PeerCounters() : _packets_recv(0), _bad_routes(0), _bad_packets(0)
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

protected:
    uint32_t _packets_recv;
    uint32_t _bad_routes;
    uint32_t _bad_packets;
};


// Forward declaration of Port class
template <typename A>
class Port;

/**
 * @short RIP Peer
 *
 * A RIP peer is a host that sent RIP packets, originating routes, to
 * the local RIP Port (@ref Port<A>) that have originated routes.
 * Most of a Peer's work is conducted by the Port associated with
 * the Peer instance.  The Peer class just acts as a container of
 * information about the Peer host, such as the number of packets sent,
 * the time of last update, etc.
 */
template <typename A>
class Peer : public RouteEntryOrigin<A>
{
public:
    typedef A Addr;
    typedef Port<A> RipPort;
    
public:
    Peer(RipPort& p, const Addr& addr) : _port(p), _addr(addr) {}

    /**
     * Get address of Peer.
     */
    inline const Addr& address() const			{ return _addr; }
    
    /**
     * Get counters associated with Peer.
     */
    inline PeerCounters& counters()			{ return _counters; }

    /**
     * Get counters associated with Peer.
     */
    inline const PeerCounters& counters() const		{ return _counters; }

    /**
     * Get port associated with Peer.
     */
    inline RipPort& port()				{ return _port; }

    /**
     * Get port associatd with Peer.
     */
    inline const RipPort& port() const			{ return _port; }

    /**
     * Set last active time.
     */
    inline void set_last_active(const TimeVal& t)	{ _last_active = t; }

    /**
     * Get last active time.
     */
    inline const TimeVal& last_active() const	     { return _last_active; }
    
protected:
    RipPort&		_port;
    Addr		_addr;
    PeerCounters	_counters;
    TimeVal		_last_active;
};

#endif // __RIP_PEER_HH__

