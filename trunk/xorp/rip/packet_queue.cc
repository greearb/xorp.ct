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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "config.h"

#include "rip_module.h"

#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "packet_queue.hh"

template <typename A>
RipPacketQueue<A>::RipPacketQueue()
    : _buffered_bytes(0), _max_buffered_bytes(64000), _drops(0)
{
}

template <typename A>
RipPacketQueue<A>::~RipPacketQueue()
{
    flush_packets();
}

template <typename A>
RipPacket<A>*
RipPacketQueue<A>::new_packet(const A& addr, uint16_t port)
{
    XLOG_ASSERT(_candidate_packet.empty() == true);
    _candidate_packet.push_back(RipPacket<A>(addr, port));
    return &_candidate_packet.back();
}

template <typename A>
void
RipPacketQueue<A>::enqueue_packet(const RipPacket<A>* pkt)
{
    XLOG_ASSERT(_candidate_packet.empty() == false);
    XLOG_ASSERT(pkt == &_candidate_packet.back());
    while (_buffered_bytes + pkt->data_size() >= _max_buffered_bytes
	   && drop_old() == true);
    _buffered_bytes += pkt->data_size();
    _ready_packets.splice(_ready_packets.end(),
			  _candidate_packet,
			  _candidate_packet.begin());
}

template <typename A>
void
RipPacketQueue<A>::discard_packet(const RipPacket<A>* pkt)
{
    XLOG_ASSERT(_candidate_packet.empty() == false);
    XLOG_ASSERT(pkt == &_candidate_packet.back());
    _candidate_packet.erase(_candidate_packet.begin());
}

template <typename A>
const RipPacket<A>*
RipPacketQueue<A>::head() const
{
    if (_ready_packets.empty())
	return 0;
    return &(_ready_packets.front());
}

template <typename A>
void
RipPacketQueue<A>::pop_head()
{
    if (_ready_packets.empty() == false) {
	_buffered_bytes -= _ready_packets.front().data_size();
	_ready_packets.pop_front();
    }
}

template <typename A>
bool
RipPacketQueue<A>::drop_old()
{
    if (_ready_packets.empty() == false) {
	typename PacketQueue::iterator i = ++_ready_packets.begin();
	if (i != _ready_packets.end()) {
	    XLOG_INFO("Dropping outbound RIP packet");
	    _ready_packets.erase(i);
	    _drops++;
	    return true;
	}
    }
    return false;
}

template <typename A>
uint32_t
RipPacketQueue<A>::drop_count() const
{
    return _drops;
}

template <typename A>
void
RipPacketQueue<A>::reset_drop_count()
{
    _drops = 0;
}

template <typename A>
void
RipPacketQueue<A>::flush_packets()
{
    while (_ready_packets.empty() == false) {
	_buffered_bytes -= _ready_packets.front().data_size();
	_ready_packets.pop_front();
    }
    XLOG_ASSERT(_buffered_bytes == 0);
}

template <typename A>
void
RipPacketQueue<A>::set_max_buffered_bytes(uint32_t mbb)
{
    _max_buffered_bytes = mbb;
}

template <typename A>
uint32_t
RipPacketQueue<A>::max_buffered_bytes() const
{
    return _max_buffered_bytes;
}

template <typename A>
uint32_t
RipPacketQueue<A>::buffered_bytes() const
{
    return _buffered_bytes;
}

template class RipPacketQueue<IPv4>;
template class RipPacketQueue<IPv6>;
