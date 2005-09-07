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

#ident "$XORP: xorp/ospf/xrl_io.cc,v 1.8 2005/09/07 08:58:10 atanu Exp $"

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

    if (_receive_cb.is_empty())
	return;

    //
    // XXX: create a copy of the payload, because the callback's argument
    // is not const-ified.
    //
    vector<uint8_t> payload_copy(payload);
    _receive_cb->dispatch(interface, vif, dst, src, &payload_copy[0],
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
	XLOG_FATAL("Cannot send a packet on interface %s vif %s: %s",
		   interface.c_str(), vif.c_str(), xrl_error.str().c_str());
	break;
    }
}

template <typename A>
bool
XrlIO<A>::register_receive(typename IO<A>::ReceiveCallback cb)
{
    _receive_cb = cb;

    return true;
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

template <typename A>
bool
XrlIO<A>::enabled(const string& interface, const string& vif, A address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    return true;
}

template <typename A>
uint32_t
XrlIO<A>::get_prefix_length(const string& interface, const string& vif,
			    A address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    return 0;
}

template <typename A>
uint32_t
XrlIO<A>::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    return 0;
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
bool
XrlIO<A>::add_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
		    bool discard)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s\n",
	      cstring(net), cstring(nexthop), metric, equal ? "true" : "false",
	      discard ? "true" : "false");

    return true;
}

template <typename A>
bool
XrlIO<A>::replace_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
			bool discard)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s\n",
	      cstring(net), cstring(nexthop), metric, equal ? "true" : "false",
	      discard ? "true" : "false");

    return true;
}

template <typename A>
bool
XrlIO<A>::delete_route(IPNet<A> net)
{
    debug_msg("Net %s\n", cstring(net));

    return true;
}

template class XrlIO<IPv4>;
template class XrlIO<IPv6>;
