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

#ident "$XORP: xorp/rip/packet_queue.cc,v 1.1 2003/07/21 18:03:04 hodson Exp $"

#include "config.h"

#include "rip_module.h"

#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "packet_queue.hh"

template <typename A>
PacketQueue<A>::PacketQueue()
    : _buffered_bytes(0), _max_buffered_bytes(64000), _drops(0)
{
}

template <typename A>
PacketQueue<A>::~PacketQueue()
{
    flush_packets();
}

template <typename A>
void
PacketQueue<A>::enqueue_packet(const RipPacket<A>* pkt)
{
    while (_buffered_bytes + pkt->data_bytes() >= _max_buffered_bytes
	   && drop_old() == true);
    _buffered_bytes += pkt->data_bytes();
    _ready_packets.push_back(pkt);
}

template <typename A>
const RipPacket<A>*
PacketQueue<A>::head() const
{
    if (_ready_packets.empty())
	return 0;
    return _ready_packets.front();
}

template <typename A>
void
PacketQueue<A>::pop_head()
{
    if (_ready_packets.empty() == false) {
	_buffered_bytes -= _ready_packets.front()->data_bytes();
	delete _ready_packets.front();
	_ready_packets.pop_front();
    }
}

template <typename A>
bool
PacketQueue<A>::drop_old()
{
    if (_ready_packets.empty() == false) {
	typename QueueRep::iterator i = ++_ready_packets.begin();
	if (i != _ready_packets.end()) {
	    XLOG_INFO("Dropping outbound RIP packet");
	    delete *i;
	    _ready_packets.erase(i);
	    _drops++;
	    return true;
	}
    }
    return false;
}

template <typename A>
uint32_t
PacketQueue<A>::drop_count() const
{
    return _drops;
}

template <typename A>
void
PacketQueue<A>::reset_drop_count()
{
    _drops = 0;
}

template <typename A>
void
PacketQueue<A>::flush_packets()
{
    while (_ready_packets.empty() == false) {
	_buffered_bytes -= _ready_packets.front()->data_bytes();
	delete _ready_packets.front();
	_ready_packets.pop_front();
    }
    XLOG_ASSERT(_buffered_bytes == 0);
}

template <typename A>
void
PacketQueue<A>::set_max_buffered_bytes(uint32_t mbb)
{
    _max_buffered_bytes = mbb;
}

template <typename A>
uint32_t
PacketQueue<A>::max_buffered_bytes() const
{
    return _max_buffered_bytes;
}

template <typename A>
uint32_t
PacketQueue<A>::buffered_bytes() const
{
    return _buffered_bytes;
}

template class PacketQueue<IPv4>;
template class PacketQueue<IPv6>;
