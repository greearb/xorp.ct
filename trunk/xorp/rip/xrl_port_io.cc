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

#ident "$XORP: xorp/rip/xrl_port_io.cc,v 1.1 2004/01/09 00:28:13 hodson Exp $"

#define DEBUG_LOGGING
#include "libxorp/debug.h"

#include "constants.hh"
#include "xrl_port_io.hh"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/socket4_locator_xif.hh"
#include "xrl/interfaces/socket4_xif.hh"

// ----------------------------------------------------------------------------
// IPv4 specialized XrlPortIO

template <>
bool
XrlPortIO<IPv4>::request_socket_server()
{
    XrlSocket4LocatorV0p1Client cl(&_xr);
    return cl.send_find_socket_server_for_addr("fea", _addr,
		callback(this, &XrlPortIO<IPv4>::socket_server_cb));
}

template <>
bool
XrlPortIO<IPv4>::request_socket_open()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_udp_open_bind_join(
		_ss.c_str(),
		_xr.instance_name(),
		_addr,
		RIP_AF_CONSTANTS<IPv4>::IP_PORT,
		RIP_AF_CONSTANTS<IPv4>::IP_GROUP(),
		RIP_TTL,
		false,
		callback(this, &XrlPortIO<IPv4>::socket_open_cb)
		);
}

template <>
bool
XrlPortIO<IPv4>::request_socket_close()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_close(_ss.c_str(), _sid,
			 callback(this, &XrlPortIO<IPv4>::socket_close_cb));
}

template <>
bool
XrlPortIO<IPv4>::send(const IPv4& 		dst_addr,
		      uint16_t 			dst_port,
		      const vector<uint8_t>&  	rip_packet)
{
    if (_pending) {
	debug_msg("Send skipped (pending).\n");
	return false;
    }

    XrlSocket4V0p1Client cl(&_xr);
    if (cl.send_send_to(_ss.c_str(), _sid, dst_addr, dst_port, rip_packet,
			callback(this, &XrlPortIO<IPv4>::send_cb))) {
	debug_msg("Sent %u bytes to %s/%u\n",
		  static_cast<uint32_t>(rip_packet.size()),
		  dst_addr.str().c_str(), dst_port);
	_pending = true;
	return true;
    }
    return false;
}


// ----------------------------------------------------------------------------
// IPv6 specialized XrlPortIO

template <>
bool
XrlPortIO<IPv6>::request_socket_server()
{
    return false;
}

template <>
bool
XrlPortIO<IPv6>::request_socket_open()
{
    return false;
}

template <>
bool
XrlPortIO<IPv6>::request_socket_close()
{
    return false;
}

template <>
bool
XrlPortIO<IPv6>::send(const IPv6& 		/* dst_addr */,
		      uint16_t			/* dst_port */,
		      const vector<uint8_t>&	/* rip_packet*/)

{
    return false;
}


// ----------------------------------------------------------------------------
// Non-specialized XrlPortIO methods.

template <typename A>
bool
XrlPortIO<A>::pending() const
{
    return _pending;
}

template <typename A>
void
XrlPortIO<A>::startup()
{
    _pending = true;
    set_status(STARTING);
    if (request_socket_server() == false) {
	set_status(FAILED, "Failed to find appropriate socket server.");
    }
}

template <typename A>
void
XrlPortIO<A>::shutdown()
{
    _pending = true;
    set_status(SHUTTING_DOWN);
    if (request_socket_close() == false) {
	set_status(SHUTDOWN);
    }
}

template <typename A>
void
XrlPortIO<A>::socket_server_cb(const XrlError& e, const string* pss)
{
    if (e != XrlError::OKAY()) {
	set_status(FAILED);
	return;
    }

    _ss = *pss;
    if (request_socket_open() == false) {
	set_status(FAILED, "Failed sending socket open request.");
    }
}

template <typename A>
void
XrlPortIO<A>::socket_open_cb(const XrlError& e, const string* psid)
{
    if (e != XrlError::OKAY()) {
	set_status(FAILED, "Failed to instantiate socket");
	return;
    }

    _sid = *psid;
    _pending = false;
    set_status(RUNNING);
    set_enabled(true);
}

template <typename A>
void
XrlPortIO<A>::send_cb(const XrlError& xe)
{
    debug_msg("SendCB %s\n", xe.str().c_str());
    _pending = false;
    _user.port_io_send_completion(xe == XrlError::OKAY());
}

template <typename A>
void
XrlPortIO<A>::socket_close_cb(const XrlError&)
{
    set_status(SHUTDOWN);
}

template XrlPortIO<IPv4>;
template XrlPortIO<IPv6>;
