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

#ident "$XORP: xorp/contrib/olsr/debug_io.cc,v 1.4 2008/10/02 21:56:33 bms Exp $"

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

DebugIO::DebugIO(TestInfo& info, EventLoop& eventloop)
    : _info(info), _eventloop(eventloop), _packets(0),
      _next_interface_id(1)
{
    initialize_message_decoder(_md);
}

DebugIO::~DebugIO()
{
}

void
DebugIO::pp(const string& which, int level,
	    const string& interface, const string& vif,
	    IPv4 dst, uint16_t dport, IPv4 src, uint16_t sport,
	    uint8_t* data, uint32_t len)
{

    TimeVal now;
    _eventloop.current_time(now);
    // 'now' is relative, so don't use pretty_print().
    DOUT_LEVEL(_info, level) << now.str() << endl;
    DOUT_LEVEL(_info, level) << which << "("
	<< interface << "/" << vif << "/"
	<< dst.str() << ":" << dport << ","
	<< src.str() << ":" << sport
	<< ")" << endl;
    // Try to decode the packet to print it.
    try {
	    Packet* pkt = new Packet(_md);
	    pkt->decode(data, len);
	    DOUT_LEVEL(_info, level) << pkt->str() << endl;

#if 1
	    // XXX no refcounting!
	    const vector<Message*>& msgs = pkt->messages();
	    vector<Message*>::const_iterator ii;
	    for (ii = msgs.begin(); ii != msgs.end(); ii++)
		delete (*ii);
#endif
	    delete pkt;
    } catch (InvalidPacket& e) {
	    DOUT_LEVEL(_info, level) << "Bad packet: " <<
	    cstring(e) << endl;
    }
}

int
DebugIO::startup()
{
    ServiceBase::set_status(SERVICE_READY);
    return (XORP_OK);
}

int
DebugIO::shutdown()
{
    ServiceBase::set_status(SERVICE_SHUTDOWN);
    return (XORP_OK);
}

bool
DebugIO::enable_address(const string& interface, const string& vif,
			const IPv4& address, const uint16_t& port,
			const IPv4& all_nodes_address)
{
    DOUT(_info) << "enable_address("
	<< interface << "/" << vif << "/"
	<< address.str() << ":" << port << ")"
	<< all_nodes_address.str() << endl;
    return true;
}

bool
DebugIO::disable_address(const string& interface, const string& vif,
			 const IPv4& address, const uint16_t& port)
{
    DOUT(_info) << "disable_address("
	<< interface << "/" << vif << "/"
	<< address.str() << ":" << port << ")" << endl;
    return true;
}

bool
DebugIO::is_interface_enabled(const string& interface) const
{
    DOUT(_info) << "enabled(" << interface << ")\n";
    return true;
}

bool
DebugIO::is_vif_enabled(const string & interface, const string & vif) const
{
    DOUT(_info) << "enabled(" << interface << "," << vif << ")" << endl;
    return true;
}

bool
DebugIO::is_vif_broadcast_capable(const string& interface, const string& vif)
{
    DOUT(_info) << "broadcast_capable(" << interface << "," << vif
	<< ")" << endl;
    return true;
}

bool
DebugIO::is_vif_multicast_capable(const string& interface, const string& vif)
{
    DOUT(_info) << "multicast_capable(" << interface << "," << vif
	<< ")" << endl;
    return true;
}

bool
DebugIO::is_vif_loopback(const string& interface, const string& vif)
{
    DOUT(_info) << "loopback(" << interface << "," << vif
	<< ")" << endl;
    return false;
}

bool
DebugIO:: is_address_enabled(const string& interface, const string& vif,
			     const IPv4& address) const
{
    DOUT(_info) << "address_enabled(" << interface << "," << vif
	<< "," << address.str() << ")" << endl;
    return false;
}

bool
DebugIO::get_addresses(const string& interface, const string& vif,
		       list<IPv4>& addresses) const
{
    DOUT(_info) << "get_addresses(" << interface << "," << vif
	<< ")" << endl;
    return false;
    UNUSED(addresses);
}

bool
DebugIO::get_broadcast_address(const string& interface,
			       const string& vif,
			       const IPv4& address,
			       IPv4& bcast_address) const
{
    bcast_address = IPv4::ALL_ONES();

    UNUSED(interface);
    UNUSED(vif);
    UNUSED(address);
    return true;
}

bool
DebugIO::get_interface_id(const string& interface, uint32_t& interface_id)
{
    DOUT(_info) << "get_interface_id(" << interface << ")\n";

    if (0 == _interface_ids.count(interface)) {
	interface_id = _next_interface_id++;
	_interface_ids[interface] = interface_id;
    } else {
	interface_id = _interface_ids[interface];
    }

    return true;
}

uint32_t
DebugIO::get_mtu(const string& interface)
{
    DOUT(_info) << "get_mtu(" << interface << ")\n";
    return 1500;
}

bool
DebugIO::send(const string& interface, const string& vif,
	      const IPv4 & src, const uint16_t & sport,
	      const IPv4 & dst, const uint16_t & dport,
	      uint8_t* data, const uint32_t & len)
{
    pp("SEND", 0, interface, vif, dst, dport, src, sport, data, len);

    _packets++;
    DOUT(_info) << "packets sent " << _packets << endl;

    map<pair<string, string>, IO::ReceiveCallback>::iterator ii =
	_forward_cbs.find(make_pair(interface, vif));
    if (ii != _forward_cbs.end()) {
	((*ii).second)->dispatch(interface, vif,
				 dst, dport, src, sport, data, len);
    }
    return true;
}

void
DebugIO::receive(const string& interface, const string& vif,
		 const IPv4 & dst, const uint16_t& dport,
		 const IPv4 & src, const uint16_t& sport,
		 uint8_t* data, const uint32_t & len)
{
    pp("RECEIVE", 1, interface, vif, dst, dport, src, sport, data, len);

    if (! IO::_receive_cb.is_empty()) {
	IO::_receive_cb->dispatch(interface, vif,
				  dst, dport, src, sport, data, len);
    }
}

bool
DebugIO::register_forward(const string& interface, const string& vif,
			  IO::ReceiveCallback cb)
{
    XLOG_ASSERT(0 == _forward_cbs.count(make_pair(interface, vif)));

    _forward_cbs[make_pair(interface, vif)] = cb;

    return true;
}

void
DebugIO::unregister_forward(const string& interface, const string& vif)
{
    map<pair<string, string>, IO::ReceiveCallback>::iterator ii =
	_forward_cbs.find(make_pair(interface, vif));
    if (ii != _forward_cbs.end()) {
	(*ii).second.release();
	_forward_cbs.erase(ii);
    }
}

bool
DebugIO::add_route(IPv4Net net, IPv4 nexthop, uint32_t nexthop_id,
		   uint32_t metric, const PolicyTags& policytags)
{
    DOUT(_info) << "Net: " << net.str() <<
	" nexthop: " << nexthop.str() <<
	" nexthop_id: " << nexthop_id <<
	" metric: " << metric <<
	" policy: " << policytags.str() << endl;

    XLOG_ASSERT(0 == _routing_table.count(net));
    DebugRouteEntry dre;
    dre._nexthop = nexthop;
    dre._metric = metric;
    dre._policytags = policytags;

    _routing_table[net] = dre;

    return true;
}

bool
DebugIO::replace_route(IPv4Net net, IPv4 nexthop,
		       uint32_t nexthop_id, uint32_t metric,
		       const PolicyTags & policytags)
{
    DOUT(_info) << "Net: " << net.str() <<
	" nexthop: " << nexthop.str() <<
	" nexthop_id: " << nexthop_id <<
	" metric: " << metric <<
	" policy: " << policytags.str() << endl;

    if (!delete_route(net))
	return false;

    return add_route(net, nexthop, nexthop_id, metric,
		     policytags);
}

bool
DebugIO::delete_route(IPv4Net net)
{
    DOUT(_info) << "Net: " << net.str() << endl;

    XLOG_ASSERT(1 == _routing_table.count(net));
    _routing_table.erase(_routing_table.find(net));

    return true;
}

void
DebugIO::routing_table_empty()
{
    _routing_table.clear();
}

uint32_t
DebugIO::routing_table_size()
{
    return _routing_table.size();
}

bool
DebugIO::routing_table_verify(IPv4Net net, IPv4 nexthop, uint32_t metric)
{
    DOUT(_info) << "Net: " << net.str() <<
	" nexthop: " << nexthop.str() <<
	" metric: " << metric << endl;

    if (0 == _routing_table.count(net)) {
	DOUT(_info) << "Net: " << net.str() << " not in table\n";
	return false;
    }

    DebugRouteEntry dre = _routing_table[net];
    if (dre._nexthop != nexthop) {
	DOUT(_info) << "Nexthop mismatch: " << nexthop.str() << " " <<
	    dre._nexthop.str() << endl;
	return false;
    }
    if (dre._metric != metric) {
	DOUT(_info) << "Metric mismatch: " << metric << " " <<
	    dre._metric << endl;
	return false;
    }

    return true;
}

void
DebugIO::routing_table_dump(ostream& o)
{
    map<IPv4Net, DebugRouteEntry>::iterator ii;
    for (ii = _routing_table.begin(); ii != _routing_table.end(); ++ii) {
	DebugRouteEntry& dre = (*ii).second;
	o << (*ii).first.str() << ": " << dre._nexthop.str() <<
	     " " << dre._metric << endl;
    }
}
