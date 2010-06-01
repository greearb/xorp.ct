// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8: 

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

// $XORP: xorp/rip/output.hh,v 1.15 2008/10/02 21:58:16 bms Exp $

#ifndef __RIP_OUTPUT_HH__
#define __RIP_OUTPUT_HH__

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"



#include "policy/backend/policy_filters.hh"

#include "port.hh"
#include "system.hh"
#include "rip_varrw.hh"

template <typename A>
class PacketQueue;

/**
 * @short Base class for RIP output processing components.
 *
 * RIP output processing components generate periodic trains of RIP
 * response packets.  This class and it's children are intended to
 * generate the packet trains from RIP output processing.  Each packet
 * within the train is temporally separated from it's neighbouring
 * packets.  Timers elsewhere schedule the start of each packet train.
 */
template <typename A>
class OutputBase :
    public NONCOPYABLE
{
public:
    typedef A		Addr;
    typedef IPNet<A>	Net;

public:
    OutputBase(EventLoop&	e,
	       Port<A>&		port,
	       PacketQueue<A>&	pkt_queue,
	       const A&		ip_addr,
	       uint16_t		ip_port);

    virtual ~OutputBase() {};

    /**
     * Accessor for destination IP address applied to output packets.
     */
    const A& ip_addr() const			{ return _ip_addr; }

    /**
     * Accessor for destination IP port applied to output packets.
     */
    uint16_t ip_port() const			{ return _ip_port; }

    /**
     * @return true if output process is generating packets.
     */
    bool running() const;

    /**
     * Start packet train if sufficient data is available.  This instance
     * will remain in "running" so long as data is available and will
     * continue to generate packets until the data is exhausted.
     */
    void start();

    /**
     * Stop packet train.
     */
    void stop();

    /**
     * Get number of packets placed on packet queue for output.
     */
    void packets_sent() const			{ return _pkts_out; }

protected:
    /**
     * Accessor for the inter-packet gap the output process should when
     * generating packet trains.
     */
    uint32_t interpacket_gap_ms() const;

    /**
     * Derived classes should implement this to start output processing.
     * It is invoked when start() is called.
     */
    virtual void start_output_processing() = 0;

    /**
     * Derived classes should implement this to stop output processing.
     * It is invoked when stop() is called.
     */
    virtual void stop_output_processing() = 0;

    /**
     * Output packet if suitable data is available, and place it in
     * the PacketQueue associated with this instance.  Should data still be
     * available after packet is generated then implementations of this
     * method should reschedule a call to output_packet after
     * interpacket_gap_ms milliseconds.
     */
    virtual void output_packet() = 0;

    void incr_packets_sent()			{ _pkts_out++; }

    /**
     * Policy filters the route.
     *
     * @param r route to filter.
     * @return true if the route was accepted, false otherwise.
     */
    bool do_filtering(RouteEntry<A>* r);

protected:
    EventLoop&		_e;
    Port<A>&		_port;	    // Port associated with output
    PacketQueue<A>&	_pkt_queue; // Place for generated packets to go
    const A		_ip_addr;   // IP address for output packets
    const uint16_t	_ip_port;   // IP port for output packets
    XorpTimer		_op_timer;  // Timer invoking output_packet()
    uint32_t		_pkts_out;  // Packets sent

    PolicyFilters&	_policy_filters;	// Global policy filters
};

template <typename A>
OutputBase<A>::OutputBase(EventLoop&	  e,
			  Port<A>&	  port,
			  PacketQueue<A>& pkt_queue,
			  const A&	  ip_addr,
			  uint16_t	  ip_port)
    : _e(e), _port(port), _pkt_queue(pkt_queue),
      _ip_addr(ip_addr), _ip_port(ip_port), _pkts_out(0),
      _policy_filters(port.port_manager().system().policy_filters())
{
}

template <typename A>
inline bool
OutputBase<A>::running() const
{
    return _op_timer.scheduled();
}

template <typename A>
inline void
OutputBase<A>::start()
{
    if (running() == false)
	start_output_processing();
}

template <typename A>
inline void
OutputBase<A>::stop()
{
    stop_output_processing();
}

template <typename A>
inline uint32_t
OutputBase<A>::interpacket_gap_ms() const
{
    return _port.constants().interpacket_delay_ms();
}


template <typename A>
bool
OutputBase<A>::do_filtering(RouteEntry<A>* route)
{
    try {
	RIPVarRW<A> varrw(*route);
	
	debug_msg("[RIP] Running export filter on route: %s\n",
		  route->net().str().c_str());

	bool accepted = _policy_filters.run_filter(filter::EXPORT,
						   varrw);

	return accepted;
    } catch(const PolicyException& e) {
	XLOG_FATAL("PolicyException: %s", e.str().c_str());
	XLOG_UNFINISHED();
    }
}

#endif // __RIP_OUTPUT_HH__
