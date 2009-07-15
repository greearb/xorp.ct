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



#include "rip_module.h"

#include "libxorp/xorp.h"
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
	   && drop_old() == true) {
	// XXX: Empty body
    }
    _buffered_bytes += pkt->data_bytes();
    _ready_packets.push_back(pkt);
}

template <typename A>
bool
PacketQueue<A>::empty() const
{
    return _ready_packets.empty();
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


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class PacketQueue<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class PacketQueue<IPv6>;
#endif
