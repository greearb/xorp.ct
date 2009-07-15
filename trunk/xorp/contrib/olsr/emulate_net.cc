// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/tokenize.hh"
#include "libxorp/test_main.hh"

#include "olsr.hh"
#include "debug_io.hh"
#include "emulate_net.hh"

#include <deque>

EmulateSubnet::EmulateSubnet(TestInfo& info, EventLoop& eventloop)
 : _info(info), _eventloop(eventloop),
   _queue_add(1), _queue_remove(2),
   _all_nodes_addr(IPv4::ALL_ONES())
{
}

EmulateSubnet::~EmulateSubnet()
{
}

void
EmulateSubnet::receive_frames(
    const string& interface, const string& vif,
    IPv4 dst, uint16_t dport, IPv4 src, uint16_t sport,
    uint8_t* data, uint32_t len, const string instance)
{
    DOUT(_info) << "receive(" << instance << ","
		<< interface << "/" << vif << "/"
		<< dst.str() << ":" << dport << ","
		<< src.str() << ":" << sport << ","
		<< len <<  "...)" << endl;

    _queue[_queue_add].
	push_back(Frame(interface, vif,
			dst, dport, src, sport, data, len, instance));

    if (_timer.scheduled())
	return;

    XLOG_ASSERT(_queue[_queue_add].size() == 1);

    _timer = _eventloop.
	 new_oneoff_after_ms(10, callback(this, &EmulateSubnet::next));
}

void
EmulateSubnet::bind_interface(
	       const string& instance,
	       const string& interface, const string& vif,
	       const IPv4& listen_addr, const uint16_t listen_port,
	       DebugIO& io)
{
    DOUT(_info) << "bind " << instance << ": "
	 << interface << "/" << vif << "/"
	 << listen_addr.str() << ":" << listen_port << endl;

    io.register_forward(interface, vif,
			callback(this,
				 &EmulateSubnet::receive_frames,
				 instance));

    _ios[Multiplex(instance, interface, vif,
		   listen_addr, listen_port)] = &io;
}

void
EmulateSubnet::unbind_interface(
	     const string& instance,
	     const string& interface, const string& vif,
	     const IPv4& listen_addr, const uint16_t listen_port,
	     DebugIO& io)
{
    DOUT(_info) << "unbind " << instance << ": "
	 << interface << "/" << vif << "/"
	 << listen_addr.str() << ":" << listen_port << endl;

    io.unregister_forward(interface, vif);

    map<const Multiplex, DebugIO *>::iterator ii =
	_ios.find(Multiplex(instance, interface, vif,
			    listen_addr, listen_port));
    XLOG_ASSERT(ii != _ios.end());
    _ios.erase(ii);
}

EmulateSubnet::Multiplex::Multiplex(
    const string& instance, const string& interface, const string& vif,
    IPv4 listen_addr, uint16_t listen_port)
 : _instance(instance),
   _interface(interface), _vif(vif),
   _listen_addr(listen_addr), _listen_port(listen_port)
{
}

EmulateSubnet::Frame::Frame(
    const string& interface, const string& vif,
    IPv4 dst, uint16_t dport,
    IPv4 src, uint16_t sport,
    uint8_t* data, uint32_t len,
    string instance)
 : _interface(interface), _vif(vif),
   _dst(dst), _dport(dport), _src(src), _sport(sport),
   _instance(instance)
{
    _pkt.resize(len);
    memcpy(&_pkt[0], data, len);
}

void
EmulateSubnet::next()
{
    if (0 == _queue_add) {
	_queue_add = 1;
	_queue_remove = 0;
    } else {
	_queue_add = 0;
	_queue_remove = 1;
    }
    while (!_queue[_queue_remove].empty()) {
	Frame frame = _queue[_queue_remove].front();
	_queue[_queue_remove].pop_front();
	forward(frame);
    }
}

void
EmulateSubnet::forward(Frame frame)
{
    uint8_t* data = &frame._pkt[0];
    uint32_t len = frame._pkt.size();

    map<const Multiplex, DebugIO *>::iterator i;
    for(i = _ios.begin(); i != _ios.end(); i++) {
	Multiplex m = (*i).first;

	// Prevent loopback.
	if (m._instance == frame._instance)
	    continue;

	DOUT(_info) << "Send to: " << m._instance << ": "
	    << m._listen_addr.str() << ":" << m._listen_port
	    << " on " << m._interface << "/" << m._vif << " len "
	    << len << " from " << frame._src.str() << ":" << frame._sport
	    << endl;

	// Check if the packet was destined for us or one of the
	// configured broadcast addresses.
	if (frame._dport == m._listen_port &&
	    (frame._dst == _all_nodes_addr ||
	     frame._dst == m._listen_addr)) {
	    (*i).second->receive(m._interface, m._vif,
		frame._dst, frame._dport, frame._src,
		frame._sport, data, len);
	}
    }
}

EmulateSubnetHops::EmulateSubnetHops(TestInfo& info,
    EventLoop& eventloop, uint8_t hopdelta, uint8_t maxlinks)
 : EmulateSubnet(info, eventloop),
   _hopdelta(hopdelta),
   _maxlinks(maxlinks),
   _empty_pkt_drops(0),
   _ttl_msg_drops(0)
{
    // Only HELLO messages require special invariants when
    // traversing a simulation of multiple OLSR links.
    _md.register_decoder(new HelloMessage());
}

EmulateSubnetHops::~EmulateSubnetHops()
{
}

void
EmulateSubnetHops::bind_interface(const string& instance,
	       const string& interface, const string& vif,
	       const IPv4& listen_addr, const uint16_t listen_port,
	       DebugIO& io)
{
    if (_ios.size() >= _maxlinks)
	XLOG_UNREACHABLE();

    EmulateSubnet::bind_interface(instance, interface, vif,
				      listen_addr, listen_port, io);
}

void
EmulateSubnetHops::forward(Frame frame)
{
    // Short-circuit; forward as per base class if hopcount is 0.
    if (hopdelta() == 0) {
	EmulateSubnet::forward(frame);
	return;
    }

    uint8_t* data = &frame._pkt[0];
    uint32_t len = frame._pkt.size();

    // decode packet
    Packet* pkt = new Packet(_md, OlsrTypes::UNUSED_FACE_ID);
    try {
	pkt->decode(data, len);
    } catch (InvalidPacket& e) {
	debug_msg("bad packet\n");
	return;
    }

    vector<Message*>& messages = pkt->get_messages();
    vector<Message*>::iterator ii, jj;
    for (ii = messages.begin(); ii != messages.end(); ) {
	jj = ii++;
	Message* msg = (*jj);

	int new_hops = msg->hops() + hopdelta();
	int new_ttl = msg->ttl() - hopdelta();

	// If ttl underflowed for this message, drop it.
	if (new_hops > OlsrTypes::MAX_TTL || new_ttl < 0) {
	    //jj->release();	// if refcounting
	    delete (*jj);	// if not refcounting.
	    messages.erase(jj);
	    // Maintain a count of such drops.
	    ++_ttl_msg_drops;
	    continue;
	}

	// Invariant: OLSR HELLO messages should have been dropped
	// by hopcount simulation above.
	XLOG_ASSERT(0 == dynamic_cast<const HelloMessage *>(msg));

	// Rewrite the hop counts in the contained message.
	msg->set_hop_count(new_hops);
	msg->set_ttl(new_ttl);
    }

    if (messages.empty()) {
	// If no messages, drop packet and maintain a count of such drops.
	++_empty_pkt_drops;
    } else {
	// Encode rewritten packet into Frame's existing buffer.
	// The other properties of Frame are preserved.
	pkt->encode(frame.get_buffer());

	// Forward as per base class.
	EmulateSubnet::forward(frame);
    }

    // XXX no refcounting
#if 1
    vector<Message*>::iterator kk;
    for (kk = messages.begin(); kk != messages.end(); kk++)
	delete (*kk);
#endif

    delete pkt;
}
