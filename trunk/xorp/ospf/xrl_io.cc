// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/ospf/xrl_io.cc,v 1.19 2005/10/20 00:27:43 atanu Exp $"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "xrl/interfaces/fea_rawpkt4_xif.hh"
#include "xrl/interfaces/fea_rawpkt6_xif.hh"
#include "xrl/interfaces/rib_xif.hh"

#include "ospf.hh"
#include "xrl_io.hh"

template <typename A>
void
XrlIO<A>::recv(const string& interface,
	       const string& vif,
	       A src,
	       A dst,
	       uint32_t ip_protocol,
	       int32_t ip_ttl,
	       int32_t ip_tos,
	       bool ip_router_alert,
	       const vector<uint8_t>& payload)
{
    debug_msg("recv(interface = %s, vif = %s, src = %s, dst = %s, "
	      "protocol = %u, ttl = %d tos = 0x%x router_alert = %s, "
	      "payload_size = %u\n",
	      interface.c_str(), vif.c_str(),
	      src.str().c_str(), dst.str().c_str(),
	      XORP_UINT_CAST(ip_protocol),
	      XORP_INT_CAST(ip_ttl),
	      ip_tos,
	      (ip_router_alert)? "true" : "false",
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
		  uint8_t* data, uint32_t len)
{
    bool success;

    debug_msg("send(interface = %s, vif = %s, src = %s, dst = %s, "
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
	-1,		// XXX: let the FEA set it
	-1,		// XXX: let the FEA set it
	true,
	payload,
	callback(this, &XrlIO::send_cb, interface, vif));

    return success;
}

template <>
bool
XrlIO<IPv6>::send(const string& interface, const string& vif,
		  IPv6 dst, IPv6 src,
		  uint8_t* data, uint32_t len)
{
    bool success;

    debug_msg("send(%s,%s,%s,%s,%p,%d\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);

    // Copy the payload
    vector<uint8_t> payload(len);
    memcpy(&payload[0], data, len);

    XrlRawPacket6V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_send(
	_feaname.c_str(),
	interface,
	vif,
	src,
	dst,
	get_ip_protocol_number(),
	-1,		// XXX: let the FEA set it
	-1,		// XXX: let the FEA set it
	true,
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
	// XXX - Temporarily code dump if this condition occurs.
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

    debug_msg("Enable Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_register_receiver(
	_feaname.c_str(),
	_class_name,		// XXX: maybe we should use _instance_name?
	interface,
	vif,
	get_ip_protocol_number(),
	false,			// disable multicast loopback
	callback(this, &XrlIO::enable_interface_vif_cb, interface, vif));

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
	_class_name,		// XXX: maybe we should use _instance_name?
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

    debug_msg("Disable Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    XrlRawPacket4V0p1Client fea_client(&_xrl_router);
    success = fea_client.send_unregister_receiver(
	_feaname.c_str(),
	_class_name,		// XXX: maybe we should use _instance_name?
	interface,
	vif,
	get_ip_protocol_number(),
	callback(this, &XrlIO::disable_interface_vif_cb, interface, vif));

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
	_class_name,		// XXX: maybe we should use _instance_name?
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

template <>
bool
XrlIO<IPv4>::enabled(const string& interface, const string& vif,
		     IPv4 address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    const IfMgrIPv4Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return false;

    return (fa->enabled());
}

template <>
bool
XrlIO<IPv6>::enabled(const string& interface, const string& vif,
		     IPv6 address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    const IfMgrIPv6Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return false;

    return (fa->enabled());
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

template <typename A>
uint32_t
XrlIO<A>::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_if(interface);
    if (fi == NULL)
	return 0;

    return (fi->mtu_bytes());
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
	_class_name,		// XXX: maybe we should use _instance_name?
	interface,
	vif,
	get_ip_protocol_number(),
	mcast,
	callback(this, &XrlIO::join_multicast_group_cb, interface, vif));

    return success;
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
	_class_name,		// XXX: maybe we should use _instance_name?
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
	XLOG_ERROR("Cannot join a multicast group on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("Cannot join a multicast group on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("Cannot join a multicast group on interface %s vif %s: %s",
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
	_class_name,		// XXX: maybe we should use _instance_name?
	interface,
	vif,
	get_ip_protocol_number(),
	mcast,
	callback(this, &XrlIO::leave_multicast_group_cb, interface, vif));

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
	_class_name,		// XXX: maybe we should use _instance_name?
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
	XLOG_FATAL("Cannot leave a multicast group on interface %s vif %s: %s",
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
	_running++;
    else
	_running--;
}

template <typename A>
bool
XrlIO<A>::add_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
		    bool discard, const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, equal ? "true" : "false",
	      discard ? "true" : "false", cstring(policytags));

    _rib_queue.queue_add_route(_ribname, net, nexthop, metric, policytags);

    return true;
}

template <typename A>
bool
XrlIO<A>::replace_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
			bool discard, const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, equal ? "true" : "false",
	      discard ? "true" : "false", cstring(policytags));

    // XXX - The queue should support replace see TODO 36.
    _rib_queue.queue_delete_route(_ribname, net);
    _rib_queue.queue_add_route(_ribname, net, nexthop, metric, policytags);

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
    : _eventloop(eventloop), _xrl_router(xrl_router), _flying(0)
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
			     const A& nexthop, uint32_t metric,
			     const PolicyTags& policytags)
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

template<>
bool
XrlQueue<IPv6>::sendit_spec(Queued& q, const char *protocol)
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
	    send_add_route6(q.ribname.c_str(),
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
    IfMgrVifAtom::V4Map::const_iterator ai;
    const IfMgrIfAtom* if_atom;
    const IfMgrVifAtom* vif_atom;
    const IfMgrIPv4Atom* addr_atom;
    const IfMgrIPv4Atom* other_addr_atom;

    if (_interface_status_cb.is_empty())
	return;		// XXX: no callback to invoke

    //
    // Check whether all old addresses are still there
    //
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	if_atom = &ii->second;
	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    for (ai = vif_atom->ipv4addrs().begin();
		 ai != vif_atom->ipv4addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		other_addr_atom = ifmgr_iftree().find_addr(if_atom->name(),
							   vif_atom->name(),
							   addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // The new address has disappeared
		    if (addr_atom->enabled()) {
			_interface_status_cb->dispatch(if_atom->name(),
						       vif_atom->name(),
						       addr_atom->addr(),
						       false);
		    }
		    continue;
		}
		if (addr_atom->enabled() != other_addr_atom->enabled()) {
		    _interface_status_cb->dispatch(if_atom->name(),
						   vif_atom->name(),
						   addr_atom->addr(),
						   other_addr_atom->enabled());
		    continue;
		}
	    }
	}
    }

    //
    // Check for new addresses
    //
    for (ii = ifmgr_iftree().ifs().begin();
	 ii != ifmgr_iftree().ifs().end();
	 ++ii) {
	if_atom = &ii->second;
	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    for (ai = vif_atom->ipv4addrs().begin();
		 ai != vif_atom->ipv4addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		other_addr_atom = _iftree.find_addr(if_atom->name(),
						    vif_atom->name(),
						    addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // This is a new address
		    if (addr_atom->enabled()) {
			_interface_status_cb->dispatch(if_atom->name(),
						       vif_atom->name(),
						       addr_atom->addr(),
						       true);
		    }
		    continue;
		}
		if (addr_atom->enabled() != other_addr_atom->enabled()) {
		    _interface_status_cb->dispatch(if_atom->name(),
						   vif_atom->name(),
						   addr_atom->addr(),
						   addr_atom->enabled());
		    continue;
		}
	    }
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();
}

template<>
void
XrlIO<IPv6>::updates_made()
{
    IfMgrIfTree::IfMap::const_iterator ii;
    IfMgrIfAtom::VifMap::const_iterator vi;
    IfMgrVifAtom::V6Map::const_iterator ai;
    const IfMgrIfAtom* if_atom;
    const IfMgrVifAtom* vif_atom;
    const IfMgrIPv6Atom* addr_atom;
    const IfMgrIPv6Atom* other_addr_atom;

    if (_interface_status_cb.is_empty())
	return;		// XXX: no callback to invoke

    //
    // Check whether all old addresses are still there
    //
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	if_atom = &ii->second;
	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    for (ai = vif_atom->ipv6addrs().begin();
		 ai != vif_atom->ipv6addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		other_addr_atom = ifmgr_iftree().find_addr(if_atom->name(),
							   vif_atom->name(),
							   addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // The new address has disappeared
		    if (addr_atom->enabled()) {
			_interface_status_cb->dispatch(if_atom->name(),
						       vif_atom->name(),
						       addr_atom->addr(),
						       false);
		    }
		    continue;
		}
		if (addr_atom->enabled() != other_addr_atom->enabled()) {
		    _interface_status_cb->dispatch(if_atom->name(),
						   vif_atom->name(),
						   addr_atom->addr(),
						   other_addr_atom->enabled());
		    continue;
		}
	    }
	}
    }

    //
    // Check for new addresses
    //
    for (ii = ifmgr_iftree().ifs().begin();
	 ii != ifmgr_iftree().ifs().end();
	 ++ii) {
	if_atom = &ii->second;
	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    for (ai = vif_atom->ipv6addrs().begin();
		 ai != vif_atom->ipv6addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		other_addr_atom = _iftree.find_addr(if_atom->name(),
						    vif_atom->name(),
						    addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // This is a new address
		    if (addr_atom->enabled()) {
			_interface_status_cb->dispatch(if_atom->name(),
						       vif_atom->name(),
						       addr_atom->addr(),
						       true);
		    }
		    continue;
		}
		if (addr_atom->enabled() != other_addr_atom->enabled()) {
		    _interface_status_cb->dispatch(if_atom->name(),
						   vif_atom->name(),
						   addr_atom->addr(),
						   addr_atom->enabled());
		    continue;
		}
	    }
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();
}

template class XrlIO<IPv4>;
template class XrlIO<IPv6>;
