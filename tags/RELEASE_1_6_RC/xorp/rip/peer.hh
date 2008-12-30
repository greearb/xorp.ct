// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rip/peer.hh,v 1.14 2008/07/23 05:11:35 pavlin Exp $

#ifndef __RIP_PEER_HH__
#define __RIP_PEER_HH__

#include "libxorp/timeval.hh"
#include "route_entry.hh"

/**
 * @short Container of counters associated with a peer.
 */
struct PeerCounters {
public:
    PeerCounters() : _packets_recv(0), _updates_recv(0), _requests_recv(0),
		     _bad_routes(0), _bad_packets(0), _bad_auth_packets(0)
    {}

    /**
     * Get the total number of packets received.
     */
    uint32_t packets_recv() const		{ return _packets_recv; }

    /**
     * Increment the total number of packets received.
     */
    void incr_packets_recv()			{ _packets_recv++; }

    /**
     * Get the total number of update packets received.
     */
    uint32_t update_packets_recv() const	{ return _updates_recv; }

    /**
     * Increment the total number of update packets received.
     */
    void incr_update_packets_recv()		{ _updates_recv++; }

    /**
     * Get the total number of table request packets received.
     */
    uint32_t table_requests_recv() const	{ return _requests_recv; }

    /**
     * Increment the total number of table request packets received.
     */
    void incr_table_requests_recv()		{ _requests_recv++; }

    /**
     * Get the number of bad routes received (eg invalid metric,
     * invalid address family).
     */
    uint32_t bad_routes() const			{ return _bad_routes; }

    /**
     * Increment the number of bad routes received.
     */
    void incr_bad_routes()			{ _bad_routes++; }

    /**
     * Get the number of bad response packets received.
     */
    uint32_t bad_packets() const		{ return _bad_packets; }

    /**
     * Increment the number of bad response packets received.
     */
    void incr_bad_packets()			{ _bad_packets++; }

    /**
     * Get the number of bad authentication packets received.
     */
    uint32_t bad_auth_packets() const		{ return _bad_auth_packets; }

    /**
     * Increment the number of bad authentication packets received.
     */
    void incr_bad_auth_packets()		{ _bad_auth_packets++; }

protected:
    uint32_t _packets_recv;
    uint32_t _updates_recv;
    uint32_t _requests_recv;
    uint32_t _bad_routes;
    uint32_t _bad_packets;
    uint32_t _bad_auth_packets;
};

// Forward declaration of Peer class
template <typename A> class Peer;

/**
 * @short RIP Peer Routes
 *
 * A class for storing the original routes received from a Peer host.
 * Those routes are used to push them whenever the routing policy is modified.
 */
template <typename A>
class PeerRoutes : public RouteEntryOrigin<A> {
public:
    PeerRoutes(Peer<A>& peer) : RouteEntryOrigin<A>(false), _peer(peer) {}

private:
    uint32_t expiry_secs() const;
    uint32_t deletion_secs() const;

    Peer<A>&	_peer;		// The corresponding peer
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
 * It also contains the original routes as received from the Peer host.
 * Those routes are used to push them whenever the routing policy is modified.
 */
template <typename A>
class Peer : public RouteEntryOrigin<A>
{
public:
    typedef A			Addr;
    typedef Port<A>		RipPort;
    typedef RouteEntry<A>	Route;

public:
    Peer(RipPort& p, const Addr& addr);
    ~Peer();

    /**
     * Get address of Peer.
     */
    const Addr& address() const			{ return _addr; }

    /**
     * Get counters associated with Peer.
     */
    PeerCounters& counters()			{ return _counters; }

    /**
     * Get counters associated with Peer.
     */
    const PeerCounters& counters() const	{ return _counters; }

    /**
     * Get port associated with Peer.
     */
    RipPort& port()				{ return _port; }

    /**
     * Get port associated with Peer.
     */
    const RipPort& port() const			{ return _port; }

    /**
     * Set last active time.
     */
    void set_last_active(const TimeVal& t)	{ _last_active = t; }

    /**
     * Get last active time.
     */
    const TimeVal& last_active() const		{ return _last_active; }

    /**
     * Update Route Entry in database for specified route.
     *
     * @param net the network route being updated.
     * @param nexthop the corresponding nexthop address.
     * @param cost the corresponding metric value as received from the
     *	      route originator.
     * @param tag the corresponding route tag.
     * @param policytags the policytags of this route.
     * @return true if an update occurs, false otherwise.
     */
    bool update_route(const IPNet<A>&	net,
		      const A&		nexthop,
		      uint32_t		cost,
		      uint32_t		tag,
		      const PolicyTags& policytags);

    /**
     * Push routes through the system.
     *
     * This is needed to apply the policy filters for re-filtering.
     */
    void push_routes();

    uint32_t expiry_secs() const;

    uint32_t deletion_secs() const;

protected:
    void set_expiry_timer(Route* route);
    void expire_route(Route* route);

    RipPort&		_port;
    Addr		_addr;
    PeerCounters	_counters;
    TimeVal		_last_active;
    PeerRoutes<A>	_peer_routes;
};


/**
 * Unary Function Predicate class for use with STL to determine if a
 * peer has an address.
 */
template <typename A>
struct peer_has_address {
    peer_has_address(const A& addr) : _a(addr) {}

    bool operator() (const Peer<A>& p) const { return p.address() == _a; }

    bool operator() (const Peer<A>* p) const { return p->address() == _a; }

private:
    A _a;
};

#endif // __RIP_PEER_HH__

