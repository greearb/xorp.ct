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

#ifndef __RIP_PACKET_QUEUE_HH__
#define __RIP_PACKET_QUEUE_HH__

#include "config.h"

#include <list>
#include <vector>

/**
 * @short Structure to contain outbound packet state.
 */
template <typename A>
class RipPacket
{
protected:
    A		    _addr;
    uint16_t	    _port;
    vector<uint8_t> _data;

public:
    RipPacket(const A& addr, uint16_t port)
	: _addr(addr), _port(port)
    {}
    inline const A& 		  address() const   { return _addr; }
    inline uint16_t 		  port() const	    { return _port; }
    inline vector<uint8_t>& 	  data()	    { return _data; }
    inline const vector<uint8_t>& data() const	    { return _data; }
    inline uint32_t 		  data_size() const { return _data.size(); }
    inline const uint8_t*	  data_ptr() const  { return &(_data[0]); }
    inline uint8_t*	  	  data_ptr()	    { return &(_data[0]); }
};

/**
 * @short Outbound packet queue.
 *
 * The queue is of fixed size and does FIFO.  When the queue becomes
 * full the eldest packet behind the head is dropped, ie since the
 * head may be in transit.
 */
template <typename A>
class RipPacketQueue
{
public:
    typedef list<RipPacket<A> > PacketQueue;
public:
    RipPacketQueue();
    ~RipPacketQueue();

    /**
     * Create a queued packet for a particular destination.  The packet
     * is created in the PacketQueue.  This call must be followed by
     * enqueue_packet() or discard_packet() before being called again.
     */
    RipPacket<A>* new_packet(const A& addr, uint16_t port);

    /**
     * Place packet in ready to sent queue.
     *
     * This may cause older packets in the queue to be dropped to make
     * sufficient space for new packet.
     */
    void enqueue_packet(const RipPacket<A>* pkt);

    /**
     * Release state associated with packet.
     */
    void discard_packet(const RipPacket<A>* pkt);

    /**
     * Peek at head packet if it exists.
     *
     * @return pointer to head packet if it exists, 0 otherwise.
     */
    const RipPacket<A>* head() const;

    /**
     * Remove head packet.
     */
    void pop_head();

    /**
     * Discard packet behind head packet to make space for new packets.
     *
     * @return true if an old packet was dropped, false if no packets
     * dropped.
     */
    bool drop_old();

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

    /**
     * Get the number of packets dropped from queue.
     */
    uint32_t drop_count() const;

    /**
     * Reset packet drop counter.
     */
    void reset_drop_count();

protected:
    PacketQueue _ready_packets;
    PacketQueue _candidate_packet;
    uint32_t	_buffered_bytes;
    uint32_t	_max_buffered_bytes;
    uint32_t	_drops;
};

#endif // __RIP_PACKET_QUEUE_HH__
