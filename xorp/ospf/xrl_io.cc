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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <list>
#include <set>

#include "xrl/interfaces/fea_rawpkt4_xif.hh"
#ifdef HAVE_IPV6
#include "xrl/interfaces/fea_rawpkt6_xif.hh"
#endif
#include "xrl/interfaces/rib_xif.hh"

#include "ospf.hh"
#include "xrl_io.hh"


template <typename A>
void
XrlIO<A>::recv(const string& interface,
	       const string& vif,
	       A src,
	       A dst,
	       uint8_t ip_protocol,
	       int32_t ip_ttl,
	       int32_t ip_tos,
	       bool ip_router_alert,
	       bool ip_internet_control,
	       const vector<uint8_t>& payload)
{
    debug_msg("recv(interface = %s, vif = %s, src = %s, dst = %s, "
	      "protocol = %u, ttl = %d tos = 0x%x router_alert = %s, "
	      "internet_control = %s payload_size = %u\n",
	      interface.c_str(), vif.c_str(),
	      src.str().c_str(), dst.str().c_str(),
	      XORP_UINT_CAST(ip_protocol),
	      XORP_INT_CAST(ip_ttl),
	      ip_tos,
	      bool_c_str(ip_router_alert),
	      bool_c_str(ip_internet_control),
	      XORP_UINT_CAST(payload.size()));

    if (IO<A>::_receive_cb.is_empty())
	return;

    //
    // XXX: create a copy of the payload, because the callback's argument
    // is not const-ified.
    //
    vector<uint8_t> payload_copy(payload);
    IO<A>::_receive_cb->dispatch(interface, vif, dst, src, &payload_copy[0],
				 payload_copy.size());
}

template <>
bool
XrlIO<IPv4>::send(const string& interface, const string& vif,
		  IPv4 dst, IPv4 src,
		  int ttl, uint8_t* data, uint32_t len)
{
    bool success;

    debug_msg("XrlIO<IPv4>::send  send(interface = %s, vif = %s, src = %s, dst = %s, "
	      "payload_size = %u\n",
	      interface.c_str(), vif.c_str(),
	      src.str().c_str(), dst.str().c_str(),
	      XORP_UINT_CAST(len));

    // Copy the payload
    vector<uint8_t> payload(len);
    memcpy(&payload[0], data, len);

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_send(
	_feaname.c_str(),
	interface,
	vif,
	src,
	dst,
	get_ip_protocol_number(),
	ttl,
	-1,		// XXX: let the FEA set TOS
	get_ip_router_alert(),
	true,		// ip_internet_control
	payload,
	callback(this, &XrlIO::send_cb, interface, vif));

    return success;
}

template <typename A>
void
XrlIO<A>::send_cb(const XrlError& xrl_error, string interface, string vif)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	// Success
	break;

    case REPLY_TIMED_OUT:
	XLOG_ERROR("Cannot send a packet on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("Cannot send a packet on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily core dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_ERROR("Cannot send a packet on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;
    }
}

template <>
bool
XrlIO<IPv4>::enable_interface_vif(const string& interface, const string& vif)
{
    bool success;

    XLOG_WARNING("XRL-IO: Enable Interface %s Vif %s\n",
		 interface.c_str(), vif.c_str());

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_register_receiver(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	false,			// disable multicast loopback
	callback(this, &XrlIO::enable_interface_vif_cb, interface, vif));

    return success;
}


template <typename A>
void
XrlIO<A>::enable_interface_vif_cb(const XrlError& xrl_error, string interface,
				  string vif)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	// Success
	break;

    case REPLY_TIMED_OUT:
	XLOG_ERROR("Cannot enable interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("Cannot enable interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("Cannot enable interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;
    }
}

template <>
bool
XrlIO<IPv4>::disable_interface_vif(const string& interface, const string& vif)
{
    bool success;

    XLOG_WARNING("XRL-IO: Disable Interface %s Vif %s\n",
		 interface.c_str(), vif.c_str());

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_unregister_receiver(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	callback(this, &XrlIO::disable_interface_vif_cb, interface, vif));

    return success;
}

template <typename A>
void
XrlIO<A>::disable_interface_vif_cb(const XrlError& xrl_error, string interface,
				   string vif)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	// Success
	break;

    case REPLY_TIMED_OUT:
	XLOG_ERROR("Cannot disable interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("Cannot disable interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("Cannot disable interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;
    }
}

template <typename A>
bool
XrlIO<A>::is_interface_enabled(const string& interface) const
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_interface(interface);
    if (fi == NULL)
	return false;

    return (fi->enabled() && (! fi->no_carrier()));
}

template <typename A>
bool
XrlIO<A>::is_vif_enabled(const string& interface, const string& vif) const
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (! is_interface_enabled(interface))
	return false;

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    return (fv->enabled());
}

template <>
bool
XrlIO<IPv4>::is_address_enabled(const string& interface, const string& vif,
				const IPv4& address) const
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    if (! is_vif_enabled(interface, vif))
	return false;

    const IfMgrIPv4Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return false;

    return (fa->enabled());
}

template <>
bool
XrlIO<IPv4>::get_addresses(const string& interface, const string& vif,
			   list<IPv4>& addresses) const
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    IfMgrVifAtom::IPv4Map::const_iterator i;
    for (i = fv->ipv4addrs().begin(); i != fv->ipv4addrs().end(); i++)
	addresses.push_back(i->second.addr());

    return true;
}

template <>
bool
XrlIO<IPv4>::get_link_local_address(const string& interface, const string& vif,
				    IPv4& address)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    IfMgrVifAtom::IPv4Map::const_iterator i;
    for (i = fv->ipv4addrs().begin(); i != fv->ipv4addrs().end(); i++) {
	if (i->second.addr().is_linklocal_unicast()) {
	    address = i->second.addr();
	    return true;
	}
    }

    return false;
}

template <typename A>
bool
XrlIO<A>::get_interface_id(const string& interface, uint32_t& interface_id)
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_interface(interface);
    if (fi == NULL)
	return false;

    interface_id = fi->pif_index();

    return true;
}

template <>
uint32_t
XrlIO<IPv4>::get_prefix_length(const string& interface, const string& vif,
			       IPv4 address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    const IfMgrIPv4Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return 0;

    return (fa->prefix_len());
}

template <typename A>
uint32_t
XrlIO<A>::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_interface(interface);
    if (fi == NULL)
	return 0;

    return (fi->mtu());
}

template <>
bool
XrlIO<IPv4>::join_multicast_group(const string& interface, const string& vif,
				  IPv4 mcast)
{
    bool success;

    debug_msg("Join Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_join_multicast_group(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	mcast,
	callback(this, &XrlIO::join_multicast_group_cb, interface, vif));

    return success;
}

template <typename A>
void
XrlIO<A>::join_multicast_group_cb(const XrlError& xrl_error, string interface,
				  string vif)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	// Success
	break;

    case REPLY_TIMED_OUT:
	XLOG_ERROR("Cannot join a multicast group on interface %s vif %s: %s (TIMED_OUT)",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("Cannot join a multicast group on interface %s vif %s: %s (RESOLVE or SEND failed, or not such method)",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily core dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_ERROR("Cannot join a multicast group on interface %s vif %s: %s (BAD_ARGS, CMD_FAILED, INTERNAL_ERR)",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;
    }
}

template <>
bool
XrlIO<IPv4>::leave_multicast_group(const string& interface, const string& vif,
				   IPv4 mcast)
{
    bool success;

    debug_msg("Leave Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_leave_multicast_group(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	mcast,
	callback(this, &XrlIO::leave_multicast_group_cb, interface, vif));

    return success;
}

template <typename A>
void
XrlIO<A>::leave_multicast_group_cb(const XrlError& xrl_error, string interface,
				   string vif)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	// Success
	break;

    case REPLY_TIMED_OUT:
	XLOG_ERROR("Cannot leave a multicast group on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("Cannot leave a multicast group on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_ERROR("Cannot leave a multicast group on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;
    }
}

template <typename A>
void
XrlIO<A>::register_rib()
{
    XrlRibV0p1Client rib(&_xrl_router);
    
    //create our tables
    //ebgp - v4
    //name - "ebgp"
    //unicast - true
    //multicast - true
    if(!rib.send_add_igp_table4(_ribname.c_str(),
				"ospf", _xrl_router.class_name(),
				_xrl_router.instance_name(), true, true,
				callback(this,
					 &XrlIO<A>::rib_command_done,
					 true,
					 "add_table"))) {
	XLOG_FATAL("Failed to add OSPF table to RIB");
    }

    if(!rib.send_add_igp_table6(_ribname.c_str(),
				"ospf", _xrl_router.class_name(),
				_xrl_router.instance_name(), true, true,
				callback(this, 
					 &XrlIO<A>::rib_command_done,
					 true,
					 "add_table"))) {
	XLOG_FATAL("Failed to add OSPF table to RIB");
    }

}

template <typename A>
void
XrlIO<A>::unregister_rib()
{
    XrlRibV0p1Client rib(&_xrl_router);

    if(!rib.send_delete_igp_table4(_ribname.c_str(),
				   "ospf", _xrl_router.class_name(),
				   _xrl_router.instance_name(), true, true,
				   callback(this, 
					    &XrlIO<A>::rib_command_done,
					    false,
					    "delete table"))) {
	XLOG_FATAL("Failed to delete OSPF table to RIB");
    }

    if(!rib.send_delete_igp_table6(_ribname.c_str(),
				   "ospf", _xrl_router.class_name(),
				   _xrl_router.instance_name(), true, true,
				   callback(this, 
					    &XrlIO<A>::rib_command_done,
					    false,
					    "delete table"))) {
	XLOG_FATAL("Failed to delete OSPF table to RIB");
    }

}

template <typename A>
void
XrlIO<A>::rib_command_done(const XrlError& error, bool up,
			   const char *comment)
{
    debug_msg("callback %s %s\n", comment, cstring(error));

    switch (error.error_code()) {
    case OKAY:
	break;

    case REPLY_TIMED_OUT:
	// We should really be using a reliable transport where
	// this error cannot happen. But it has so lets retry if we can.
	XLOG_ERROR("callback: %s %s",  comment, cstring(error));
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("callback: %s %s",  comment, error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("callback: %s %s",  comment, error.str().c_str());
	break;
    }

    if (up)
	component_up("rib_command_done");
    else
	component_down("rib_command_done");
}

template <typename A>
bool
XrlIO<A>::add_route(IPNet<A> net, A nexthop, uint32_t nexthop_id, 
		    uint32_t metric, bool equal, bool discard,
		    const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric,
	      bool_c_str(equal), bool_c_str(discard), cstring(policytags));

    _rib_queue.queue_add_route(_ribname, net, nexthop, nexthop_id, metric,
			       policytags);

    return true;
}

template <typename A>
bool
XrlIO<A>::replace_route(IPNet<A> net, A nexthop, uint32_t nexthop_id,
			uint32_t metric, bool equal, bool discard,
			const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric,
	      bool_c_str(equal), bool_c_str(discard), cstring(policytags));

    // XXX - The queue should support replace see TODO 36.
    _rib_queue.queue_delete_route(_ribname, net);
    _rib_queue.queue_add_route(_ribname, net, nexthop, nexthop_id, metric,
			       policytags);

    return true;
}

template <typename A>
bool
XrlIO<A>::delete_route(IPNet<A> net)
{
    debug_msg("Net %s\n", cstring(net));

    _rib_queue.queue_delete_route(_ribname, net);

    return true;
}

template<class A>
XrlQueue<A>::XrlQueue(EventLoop& eventloop, XrlRouter& xrl_router)
    : _io(0), _eventloop(eventloop), _xrl_router(xrl_router), _flying(0)
{
}

template<class A>
EventLoop& 
XrlQueue<A>::eventloop() const 
{ 
    return _eventloop;
}

template<class A>
void
XrlQueue<A>::queue_add_route(string ribname, const IPNet<A>& net,
			     const A& nexthop, uint32_t nexthop_id,
			     uint32_t metric, const PolicyTags& policytags)
{
    Queued q;

#if	0
    if (_bgp.profile().enabled(profile_route_rpc_in))
	_bgp.profile().log(profile_route_rpc_in,
			   c_format("add %s", net.str().c_str()));
#endif

    q.add = true;
    q.ribname = ribname;
    q.net = net;
    q.nexthop = nexthop;
    q.nexthop_id = nexthop_id;
    q.metric = metric;
    q.comment = 
	c_format("add_route: ribname %s net %s nexthop %s",
		 ribname.c_str(),
		 net.str().c_str(),
		 nexthop.str().c_str());
    q.policytags = policytags;

    _xrl_queue.push_back(q);

    start();
}

template<class A>
void
XrlQueue<A>::queue_delete_route(string ribname, const IPNet<A>& net)
{
    Queued q;

#if	0
    if (_bgp.profile().enabled(profile_route_rpc_in))
	_bgp.profile().log(profile_route_rpc_in,
			   c_format("delete %s", net.str().c_str()));
#endif

    q.add = false;
    q.ribname = ribname;
    q.net = net;
    q.comment = c_format("delete_route: ribname %s net %s",
			 ribname.c_str(),
			 net.str().c_str());

    _xrl_queue.push_back(q);

    start();
}

template<class A>
bool
XrlQueue<A>::busy()
{
    return 0 != _flying;
}

template<class A>
void
XrlQueue<A>::start()
{
    // If we are currently busy don't attempt to send any more XRLs.
#if	0
    if (busy())
	return;
#else
    if (maximum_number_inflight())
	return;
#endif

    // Now there are no outstanding XRLs try and send as many of the queued
    // route commands as possible as possible.

    for(;;) {
	debug_msg("queue length %u\n", XORP_UINT_CAST(_xrl_queue.size()));

	if(_xrl_queue.empty()) {
	    debug_msg("Output no longer busy\n");
#if	0
	    _rib_ipc_handler.output_no_longer_busy();
#endif
	    return;
	}

	typename deque<typename XrlQueue<A>::Queued>::const_iterator qi;

	qi = _xrl_queue.begin();

	XLOG_ASSERT(qi != _xrl_queue.end());

	Queued q = *qi;

	const char *protocol = "ospf";
	bool sent = sendit_spec(q, protocol);

	if (sent) {
	    _flying++;
	    _xrl_queue.pop_front();
	    if (maximum_number_inflight())
		return;
 	    continue;
	}
	
	// We expect that the send may fail if the socket buffer is full.
	// It should therefore be the case that we have some route
	// adds/deletes in flight. If _flying is zero then something
	// unexpected has happended. We have no outstanding sends and
	// still its gone to poo.

	XLOG_ASSERT(0 != _flying);

	// We failed to send the last XRL. Don't attempt to send any more.
	return;
    }
}

template<>
bool
XrlQueue<IPv4>::sendit_spec(Queued& q, const char *protocol)
{
    bool sent;
    bool unicast = true;
    bool multicast = false;

    XrlRibV0p1Client rib(&_xrl_router);
    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", protocol);
#if	0
	if (_bgp.profile().enabled(profile_route_rpc_out))
	    _bgp.profile().log(profile_route_rpc_out, 
			       c_format("add %s", q.net.str().c_str()));
#endif
	sent = rib.
	    send_add_route4(q.ribname.c_str(),
			    protocol,
			    unicast, multicast,
			    q.net, q.nexthop, q.metric,
			    q.policytags.xrl_atomlist(),
			    callback(this, &XrlQueue::route_command_done,
				     q.comment));
	if (!sent)
	    XLOG_WARNING("scheduling add route %s failed",
			 q.net.str().c_str());
    } else {
	debug_msg("deleting route from %s peer to rib\n", protocol);
#if	0
	if (_bgp.profile().enabled(profile_route_rpc_out))
	    _bgp.profile().log(profile_route_rpc_out, 
			       c_format("delete %s", q.net.str().c_str()));
#endif
	sent = rib.
	    send_delete_route4(q.ribname.c_str(),
			       protocol,
			       unicast, multicast,
			       q.net,
			       ::callback(this,
					  &XrlQueue::route_command_done,
					  q.comment));
	if (!sent)
	    XLOG_WARNING("scheduling delete route %s failed",
			 q.net.str().c_str());
    }

    return sent;
}

template<class A>
void
XrlQueue<A>::route_command_done(const XrlError& error,
				const string comment)
{
    _flying--;
    debug_msg("callback %s %s\n", comment.c_str(), error.str().c_str());

    switch (error.error_code()) {
    case OKAY:
	break;

    case REPLY_TIMED_OUT:
	// We should really be using a reliable transport where
	// this error cannot happen. But it has so lets retry if we can.
	XLOG_WARNING("callback: %s %s",  comment.c_str(), error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("callback: %s %s",  comment.c_str(), error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
// 	_ospf.finder_death(__FILE__, __LINE__);
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	// XXX - Make this XLOG_FATAL when this has been debugged.
	// TODO 40.
	XLOG_ERROR("callback: %s %s",  comment.c_str(), error.str().c_str());
	break;
    }

    // Fire of more requests.
    start();
}

template<class A>
void
XrlIO<A>::tree_complete()
{
    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

template<>
void
XrlIO<IPv4>::updates_made()
{
    IfMgrIfTree::IfMap::const_iterator ii;
    IfMgrIfAtom::VifMap::const_iterator vi;
    IfMgrVifAtom::IPv4Map::const_iterator ai;
    const IfMgrIfAtom* if_atom;
    const IfMgrIfAtom* other_if_atom;
    const IfMgrVifAtom* vif_atom;
    const IfMgrVifAtom* other_vif_atom;
    const IfMgrIPv4Atom* addr_atom;
    const IfMgrIPv4Atom* other_addr_atom;

    XLOG_WARNING("XrlIO<IPv4>::updates_made, _iftree:\n%s", _iftree.toString().c_str());

    //
    // Check whether the old interfaces, vifs and addresses are still there
    //
    for (ii = _iftree.interfaces().begin(); ii != _iftree.interfaces().end(); ++ii) {
	bool is_old_interface_enabled = false;
	bool is_new_interface_enabled = false;
	bool is_old_vif_enabled = false;
	bool is_new_vif_enabled = false;
	bool is_old_address_enabled = false;
	bool is_new_address_enabled = false;

	if_atom = &ii->second;
	is_old_interface_enabled = if_atom->enabled();
	is_old_interface_enabled &= (! if_atom->no_carrier());

	// Check the interface
	other_if_atom = ifmgr_iftree().find_interface(if_atom->name());
	if (other_if_atom == NULL) {
	    // The interface has disappeared
	    is_new_interface_enabled = false;
	} else {
	    is_new_interface_enabled = other_if_atom->enabled();
	    is_new_interface_enabled &= (! other_if_atom->no_carrier());
	}

	if ((is_old_interface_enabled != is_new_interface_enabled)
	    && (! _interface_status_cb.is_empty())) {
	    // The interface's enabled flag has changed
	    _interface_status_cb->dispatch(if_atom->name(),
					   is_new_interface_enabled);
	}

	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    is_old_vif_enabled = vif_atom->enabled();
	    is_old_vif_enabled &= is_old_interface_enabled;

	    // Check the vif
	    other_vif_atom = ifmgr_iftree().find_vif(if_atom->name(),
						     vif_atom->name());
	    if (other_vif_atom == NULL) {
		// The vif has disappeared
		is_new_vif_enabled = false;
	    } else {
		is_new_vif_enabled = other_vif_atom->enabled();
	    }
	    is_new_vif_enabled &= is_new_interface_enabled;

	    if ((is_old_vif_enabled != is_new_vif_enabled)
		&& (! _vif_status_cb.is_empty())) {
		// The vif's enabled flag has changed
		XLOG_WARNING("Vif: %s/%s changed enabled state to: %i, in XrlIO::updates_made\n",
			     if_atom->name().c_str(), vif_atom->name().c_str(), (int)(is_new_vif_enabled));
		_vif_status_cb->dispatch(if_atom->name(),
					 vif_atom->name(),
					 is_new_vif_enabled);
	    }

	    for (ai = vif_atom->ipv4addrs().begin();
		 ai != vif_atom->ipv4addrs().end();
		 ++ai) {
		addr_atom = &ai->second;
		is_old_address_enabled = addr_atom->enabled();
		is_old_address_enabled &= is_old_vif_enabled;

		// Check the address
		other_addr_atom = ifmgr_iftree().find_addr(if_atom->name(),
							   vif_atom->name(),
							   addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // The address has disappeared
		    is_new_address_enabled = false;
		} else {
		    is_new_address_enabled = other_addr_atom->enabled();
		}
		is_new_address_enabled &= is_new_vif_enabled;

		if ((is_old_address_enabled != is_new_address_enabled)
		    && (! _address_status_cb.is_empty())) {
		    // The address's enabled flag has changed
		    _address_status_cb->dispatch(if_atom->name(),
						 vif_atom->name(),
						 addr_atom->addr(),
						 is_new_address_enabled);
		}
	    }
	}
    }

    //
    // Check for new interfaces, vifs and addresses
    //
    for (ii = ifmgr_iftree().interfaces().begin();
	 ii != ifmgr_iftree().interfaces().end();
	 ++ii) {
	if_atom = &ii->second;

	// Check the interface
	other_if_atom = _iftree.find_interface(if_atom->name());
	if (other_if_atom == NULL) {
	    // A new interface
	    if (if_atom->enabled()
		&& (! if_atom->no_carrier())
		&& (! _interface_status_cb.is_empty())) {
		_interface_status_cb->dispatch(if_atom->name(), true);
	    }
	}

	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;

	    // Check the vif
	    other_vif_atom = _iftree.find_vif(if_atom->name(),
					      vif_atom->name());
	    if (other_vif_atom == NULL) {
		// A new vif
		if (if_atom->enabled()
		    && (! if_atom->no_carrier())
		    && (vif_atom->enabled())
		    && (! _vif_status_cb.is_empty())) {
		    XLOG_WARNING("Vif: %s/%s changed enabled state to TRUE (new vif), in XrlIO::updates_made\n",
				 if_atom->name().c_str(), vif_atom->name().c_str());
		    _vif_status_cb->dispatch(if_atom->name(), vif_atom->name(),
					     true);
		}
	    }

	    for (ai = vif_atom->ipv4addrs().begin();
		 ai != vif_atom->ipv4addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		// Check the address
		other_addr_atom = _iftree.find_addr(if_atom->name(),
						    vif_atom->name(),
						    addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // A new address
		    if (if_atom->enabled()
			&& (! if_atom->no_carrier())
			&& (vif_atom->enabled())
			&& (addr_atom->enabled())
			&& (! _address_status_cb.is_empty())) {
			_address_status_cb->dispatch(if_atom->name(),
						     vif_atom->name(),
						     addr_atom->addr(),
						     true);
		    }
		}
	    }
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();
}

template class XrlQueue<IPv4>;
template class XrlIO<IPv4>;


/** IPv6 Stuff */

#ifdef HAVE_IPV6

template <>
bool
XrlIO<IPv6>::send(const string& interface, const string& vif,
		  IPv6 dst, IPv6 src,
		  int ttl, uint8_t* data, uint32_t len)
{
    bool success;

    debug_msg("send(%s,%s,%s,%s,%p,%d\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);

    // Copy the payload
    vector<uint8_t> payload(len);
    memcpy(&payload[0], data, len);
    XrlAtomList ext_headers_type;
    XrlAtomList ext_headers_payload;

    XrlRawPacket6V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_send(
	_feaname.c_str(),
	interface,
	vif,
	src,
	dst,
	get_ip_protocol_number(),
	dst.is_multicast() ? 1 : ttl,
	-1,					// XXX: let the FEA set TOS
	get_ip_router_alert(),
	true,					// ip_internet_control
	ext_headers_type,
	ext_headers_payload,
	payload,
	callback(this, &XrlIO::send_cb, interface, vif));

    return success;
}

template <>
bool
XrlIO<IPv6>::enable_interface_vif(const string& interface, const string& vif)
{
    bool success;

    debug_msg("Enable Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    XrlRawPacket6V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_register_receiver(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	false,			// disable multicast loopback
	callback(this, &XrlIO::enable_interface_vif_cb, interface, vif));

    return success;
}


template <>
bool
XrlIO<IPv6>::disable_interface_vif(const string& interface, const string& vif)
{
    bool success;

    debug_msg("Disable Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    XrlRawPacket6V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_unregister_receiver(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	callback(this, &XrlIO::disable_interface_vif_cb, interface, vif));

    return success;
}

template <>
bool
XrlIO<IPv6>::is_address_enabled(const string& interface, const string& vif,
				const IPv6& address) const
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    if (! is_vif_enabled(interface, vif))
	return false;

    const IfMgrIPv6Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return false;

    return (fa->enabled());
}

template <>
bool
XrlIO<IPv6>::get_addresses(const string& interface, const string& vif,
			   list<IPv6>& addresses) const
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    IfMgrVifAtom::IPv6Map::const_iterator i;
    for (i = fv->ipv6addrs().begin(); i != fv->ipv6addrs().end(); i++)
	addresses.push_back(i->second.addr());

    return true;
}

template <>
bool
XrlIO<IPv6>::get_link_local_address(const string& interface, const string& vif,
				    IPv6& address)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    IfMgrVifAtom::IPv6Map::const_iterator i;
    for (i = fv->ipv6addrs().begin(); i != fv->ipv6addrs().end(); i++) {
	if (i->second.addr().is_linklocal_unicast()) {
	    address = i->second.addr();
	    return true;
	}
    }

    return false;
}

template <>
uint32_t
XrlIO<IPv6>::get_prefix_length(const string& interface, const string& vif,
			       IPv6 address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    const IfMgrIPv6Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return 0;

    return (fa->prefix_len());
}

template <>
bool
XrlIO<IPv6>::join_multicast_group(const string& interface, const string& vif,
				  IPv6 mcast)
{
    bool success;

    debug_msg("Join Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    XrlRawPacket6V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_join_multicast_group(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	mcast,
	callback(this, &XrlIO::join_multicast_group_cb, interface, vif));

    return success;
}

template <>
bool
XrlIO<IPv6>::leave_multicast_group(const string& interface, const string& vif,
				   IPv6 mcast)
{
    bool success;

    debug_msg("Leave Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    XrlRawPacket6V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_leave_multicast_group(
	_feaname.c_str(),
	_xrl_router.instance_name(),
	interface,
	vif,
	get_ip_protocol_number(),
	mcast,
	callback(this, &XrlIO::leave_multicast_group_cb, interface, vif));

    return success;
}

template<>
bool
XrlQueue<IPv6>::sendit_spec(Queued& q, const char *protocol)
{
    bool sent = false;
    bool unicast = true;
    bool multicast = false;

    XrlRibV0p1Client rib(&_xrl_router);
    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", protocol);
#if	0
	if (_bgp.profile().enabled(profile_route_rpc_out))
	    _bgp.profile().log(profile_route_rpc_out, 
			       c_format("add %s", q.net.str().c_str()));
#endif
	if (OspfTypes::UNUSED_INTERFACE_ID == q.nexthop_id) {
	    sent = rib.
		send_add_route6(q.ribname.c_str(),
				protocol,
				unicast, multicast,
				q.net, q.nexthop, q.metric, 
				q.policytags.xrl_atomlist(),
				callback(this, &XrlQueue::route_command_done,
					 q.comment));
	} else {
	    string interface;
	    string vif;
	    XLOG_ASSERT(_io);
	    if (!_io->get_interface_vif_by_interface_id(q.nexthop_id,
							interface, vif)) {
		XLOG_ERROR("Unable to find interface/vif associated with %u",
			   q.nexthop_id);
		return false;
	    }
	    sent = rib.
		send_add_interface_route6(q.ribname.c_str(),
					  protocol,
					  unicast, multicast,
					  q.net, q.nexthop,
					  interface, vif,
					  q.metric, 
					  q.policytags.xrl_atomlist(),
					  callback(this,
					    &XrlQueue::route_command_done,
						   q.comment));
	}
	if (!sent)
	    XLOG_WARNING("scheduling add route %s failed",
			 q.net.str().c_str());
    } else {
	debug_msg("deleting route from %s peer to rib\n", protocol);
#if	0
	if (_bgp.profile().enabled(profile_route_rpc_out))
	    _bgp.profile().log(profile_route_rpc_out, 
			       c_format("delete %s", q.net.str().c_str()));
#endif
	sent = rib.
	    send_delete_route6(q.ribname.c_str(),
			       protocol,
			       unicast, multicast,
			       q.net,
			       callback(this, &XrlQueue::route_command_done,
					q.comment));
	if (!sent)
	    XLOG_WARNING("scheduling delete route %s failed",
			 q.net.str().c_str());
    }

    return sent;
}

template<>
void
XrlIO<IPv6>::updates_made()
{
    IfMgrIfTree::IfMap::const_iterator ii;
    IfMgrIfAtom::VifMap::const_iterator vi;
    IfMgrVifAtom::IPv6Map::const_iterator ai;
    const IfMgrIfAtom* if_atom;
    const IfMgrIfAtom* other_if_atom;
    const IfMgrVifAtom* vif_atom;
    const IfMgrVifAtom* other_vif_atom;
    const IfMgrIPv6Atom* addr_atom;
    const IfMgrIPv6Atom* other_addr_atom;

    //
    // Check whether the old interfaces, vifs and addresses are still there
    //
    for (ii = _iftree.interfaces().begin(); ii != _iftree.interfaces().end(); ++ii) {
	bool is_old_interface_enabled = false;
	bool is_new_interface_enabled = false;
	bool is_old_vif_enabled = false;
	bool is_new_vif_enabled = false;
	bool is_old_address_enabled = false;
	bool is_new_address_enabled = false;

	if_atom = &ii->second;
	is_old_interface_enabled = if_atom->enabled();
	is_old_interface_enabled &= (! if_atom->no_carrier());

	// Check the interface
	other_if_atom = ifmgr_iftree().find_interface(if_atom->name());
	if (other_if_atom == NULL) {
	    // The interface has disappeared
	    is_new_interface_enabled = false;
	} else {
	    is_new_interface_enabled = other_if_atom->enabled();
	    is_new_interface_enabled &= (! other_if_atom->no_carrier());
	}

	if ((is_old_interface_enabled != is_new_interface_enabled)
	    && (! _interface_status_cb.is_empty())) {
	    // The interface's enabled flag has changed
	    _interface_status_cb->dispatch(if_atom->name(),
					   is_new_interface_enabled);
	}

	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    is_old_vif_enabled = vif_atom->enabled();
	    is_old_vif_enabled &= is_old_interface_enabled;

	    // Check the vif
	    other_vif_atom = ifmgr_iftree().find_vif(if_atom->name(),
						     vif_atom->name());
	    if (other_vif_atom == NULL) {
		// The vif has disappeared
		is_new_vif_enabled = false;
	    } else {
		is_new_vif_enabled = other_vif_atom->enabled();
	    }
	    is_new_vif_enabled &= is_new_interface_enabled;

	    if ((is_old_vif_enabled != is_new_vif_enabled)
		&& (! _vif_status_cb.is_empty())) {
		// The vif's enabled flag has changed
		XLOG_WARNING("Vif: %s/%s changed enabled state to: %i, in XrlIO<IPv6>::updates_made\n",
			     if_atom->name().c_str(), vif_atom->name().c_str(), (int)(is_new_vif_enabled));
		_vif_status_cb->dispatch(if_atom->name(),
					 vif_atom->name(),
					 is_new_vif_enabled);
	    }

	    for (ai = vif_atom->ipv6addrs().begin();
		 ai != vif_atom->ipv6addrs().end();
		 ++ai) {
		addr_atom = &ai->second;
		is_old_address_enabled = addr_atom->enabled();
		is_old_address_enabled &= is_old_vif_enabled;

		// Check the address
		other_addr_atom = ifmgr_iftree().find_addr(if_atom->name(),
							   vif_atom->name(),
							   addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // The address has disappeared
		    is_new_address_enabled = false;
		} else {
		    is_new_address_enabled = other_addr_atom->enabled();
		}
		is_new_address_enabled &= is_new_vif_enabled;

		if ((is_old_address_enabled != is_new_address_enabled)
		    && (! _address_status_cb.is_empty())) {
		    // The address's enabled flag has changed
		    _address_status_cb->dispatch(if_atom->name(),
						 vif_atom->name(),
						 addr_atom->addr(),
						 is_new_address_enabled);
		}
	    }
	}
    }

    //
    // Check for new interfaces, vifs and addresses
    //
    for (ii = ifmgr_iftree().interfaces().begin();
	 ii != ifmgr_iftree().interfaces().end();
	 ++ii) {
	if_atom = &ii->second;

	// Check the interface
	other_if_atom = _iftree.find_interface(if_atom->name());
	if (other_if_atom == NULL) {
	    // A new interface
	    if (if_atom->enabled()
		&& (! if_atom->no_carrier())
		&& (! _interface_status_cb.is_empty())) {
		_interface_status_cb->dispatch(if_atom->name(), true);
	    }
	}

	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;

	    // Check the vif
	    other_vif_atom = _iftree.find_vif(if_atom->name(),
					      vif_atom->name());
	    if (other_vif_atom == NULL) {
		// A new vif
		if (if_atom->enabled()
		    && (! if_atom->no_carrier())
		    && (vif_atom->enabled())
		    && (! _vif_status_cb.is_empty())) {
		    XLOG_WARNING("Vif: %s/%s changed enabled state to TRUE (new vif), in XrlIO<IPv6>::updates_made\n",
				 if_atom->name().c_str(), vif_atom->name().c_str());
		    _vif_status_cb->dispatch(if_atom->name(), vif_atom->name(),
					     true);
		}
	    }

	    for (ai = vif_atom->ipv6addrs().begin();
		 ai != vif_atom->ipv6addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		// Check the address
		other_addr_atom = _iftree.find_addr(if_atom->name(),
						    vif_atom->name(),
						    addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // A new address
		    if (if_atom->enabled()
			&& (! if_atom->no_carrier())
			&& (vif_atom->enabled())
			&& (addr_atom->enabled())
			&& (! _address_status_cb.is_empty())) {
			_address_status_cb->dispatch(if_atom->name(),
						     vif_atom->name(),
						     addr_atom->addr(),
						     true);
		    }
		}
	    }
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();
}

template class XrlQueue<IPv6>;
template class XrlIO<IPv6>;

#endif // ipv6
