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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_OUTPUT_HH__
#define __RIP_OUTPUT_HH__

#include "libxorp/eventloop.hh"

#include "port.hh"

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
class OutputBase {
public:
    typedef A				Addr;
    typedef IPNet<A>			Net;

public:
    OutputBase(EventLoop& e, Port<A>& port, PacketQueue<A>& pkt_queue);
    virtual ~OutputBase() {};

    /**
     * @return true if output process is generating packets.
     */
    inline bool running() const;

    /**
     * Start packet train if sufficient data is available.  This instance
     * will remain in "running" so long as data is available and will
     * continue to generate packets until the data is exhausted.
     */
    inline void run();

protected:
    /**
     * Accessor for the inter-packet gap the output process should when
     * generating packet trains.
     */
    inline uint32_t interpacket_gap_ms() const;

    /**
     * Output packet if suitable data is available, and place it in
     * the PacketQueue associated with this instance.  Should data still be
     * available after packet is generated then implementations of this
     * method should reschedule a call to output_packet after
     * interpacket_gap_ms milliseconds.
     */
    virtual void output_packet() = 0;

private:
    OutputBase(const OutputBase<A>& o);			// Not implemented
    OutputBase<A>& operator=(const OutputBase<A>& o);	// Not implemented

protected:
    EventLoop&		_e;
    Port<A>&		_port;	    // Port associated with output
    PacketQueue<A>&	_pkt_queue; // Place for generated packets to go
    XorpTimer		_op_timer;  // Timer invoking output_packet()
};

template <typename A>
OutputBase<A>::OutputBase(EventLoop&	  e,
			  Port<A>&	  port,
			  PacketQueue<A>& pkt_queue)
    : _e(e), _port(port), _pkt_queue(pkt_queue)
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
OutputBase<A>::run()
{
    if (running() == false)
	output_packet();
}

template <typename A>
inline uint32_t
OutputBase<A>::interpacket_gap_ms() const
{
    return _port.constants().interpacket_delay_ms();
}

#endif // __RIP_OUTPUT_HH__
