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

// $XORP: xorp/rip/packet_queue.hh,v 1.13 2008/10/02 21:58:16 bms Exp $

#ifndef __RIP_PACKET_QUEUE_HH__
#define __RIP_PACKET_QUEUE_HH__

#include "libxorp/xorp.h"




#include "packets.hh"


/**
 * @short Outbound packet queue.
 *
 * The queue is of fixed size and does FIFO.  When the queue becomes
 * full the eldest packet behind the head is dropped, ie since the
 * head may be in transit.
 */
template <typename A>
class PacketQueue
{
public:
    typedef list<const RipPacket<A>*> QueueRep;

public:
    PacketQueue();
    ~PacketQueue();

    /**
     * Place packet in ready to sent queue.  The supplied packet is
     * expected to have been allocated with the standard new operator
     * and will be destructed by the packet queue when it is dropped
     * or popped from the queue.
     *
     * This may cause older packets in the queue to be dropped to make
     * sufficient space for new packet.
     */
    void enqueue_packet(const RipPacket<A>* pkt);

    /**
     * @return true if no packets are queued, false otherwise.
     */
    bool empty() const;

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
    QueueRep	_ready_packets;
    uint32_t	_buffered_bytes;
    uint32_t	_max_buffered_bytes;
    uint32_t	_drops;
};

#endif // __RIP_PACKET_QUEUE_HH__
