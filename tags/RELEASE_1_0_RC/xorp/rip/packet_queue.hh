// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/rip/packet_queue.hh,v 1.2 2003/08/01 04:08:12 hodson Exp $

#ifndef __RIP_PACKET_QUEUE_HH__
#define __RIP_PACKET_QUEUE_HH__

#include "config.h"

#include <list>
#include <vector>

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
