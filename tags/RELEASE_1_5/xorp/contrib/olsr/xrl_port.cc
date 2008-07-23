// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/xrl_port.cc,v 1.1 2008/04/24 15:19:57 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/socket4_xif.hh"

#include "olsr.hh"
#include "io.hh"

#include "xrl_port.hh"
#include "xrl_io.hh"

// TODO: Support IPv4 multicast for OLSR control traffic.
// TODO: Support IPv6 multicast for OLSR control traffic.

// TODO: Use definition from fea/ip.h which is spelled
// differently but is always available ie IP_TOS_PREC_INTERNETCONTROL.
#ifndef IPTOS_PREC_INTERNETCONTROL
#define IPTOS_PREC_INTERNETCONTROL  0xc0
#endif /* !defined(IPTOS_PREC_INTERNETCONTROL) */

XrlPort::XrlPort(
    IO*		    io,
    EventLoop&	    eventloop,
    XrlRouter&	    xrl_router,
    const string&   ssname,
    const string&   ifname,
    const string&   vifname,
    const IPv4&	    local_addr,
    const uint16_t  local_port,
    const IPv4&	    remote_addr)
    : ServiceBase("OlsrXrlPort"),
      _io(io),
      _eventloop(eventloop),
      _xrl_router(xrl_router),
      _ss(ssname),
      _ifname(ifname),
      _vifname(vifname),
      _local_addr(local_addr),
      _local_port(local_port),
      _pending(false),
      _is_undirected_broadcast(false)
{
    if (remote_addr == IPv4::ALL_ONES())
	_is_undirected_broadcast = true;
}

XrlPort::~XrlPort()
{
}

int
XrlPort::startup()
{
    debug_msg("%p: startup()\n", this);

    _pending = true;

    set_status(SERVICE_STARTING);

    if (startup_socket() == false) {
	set_status(SERVICE_FAILED,
		   "Failed to find appropriate socket server.");
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
XrlPort::shutdown()
{
    debug_msg("%p: shutdown()\n", this);

    _pending = true;

    set_status(SERVICE_SHUTTING_DOWN);

    if (request_close() == true) {
	set_status(SERVICE_SHUTDOWN);
    }

    return XORP_OK;
}

bool
XrlPort::startup_socket()
{
    if (! request_udp_open_bind_broadcast()) {
	set_status(SERVICE_FAILED,
		  "Failed sending UDP broadcast socket open request.");
	return false;
    }

    return true;
}

bool
XrlPort::request_udp_open_bind_broadcast()
{
    XrlSocket4V0p1Client cl(&_xrl_router);

    // XXX Assumes that local and remote port are the same. We aren't
    // passed in a remote port because we currently assume we need to
    // use sendto() to transmit, although the special cases for
    // BSD undirected broadcast have now been moved into the FEA.
    debug_msg("%p: sending udp_open_bind_broadcast\n", this);
    return cl.send_udp_open_bind_broadcast(_ss.c_str(),
			    _xrl_router.instance_name(),
			    _ifname,
			    _vifname,
			    _local_port,
			    _local_port,		// XXX
			    true,			// reuse port
			    _is_undirected_broadcast,
			    false,			// don't connect()
			    callback(this,
			             &XrlPort::udp_open_bind_broadcast_cb));
}

void
XrlPort::udp_open_bind_broadcast_cb(const XrlError& e, const string* psid)
{
    if (e != XrlError::OKAY()) {
	set_status(SERVICE_FAILED, "Failed to open a UDP socket.");
	return;
    }

    _sockid = *psid;

    if (request_tos() == false) {
	set_status(SERVICE_FAILED, "Failed to set IP TOS bits.");
    }
}

bool
XrlPort::request_tos()
{
    XrlSocket4V0p1Client cl(&_xrl_router);
    debug_msg("%p: sending tos IPTOS_PREC_INTERNETCONTROL\n", this);
    return cl.send_set_socket_option(
	    _ss.c_str(),
	    _sockid,
	    "tos",
	    IPTOS_PREC_INTERNETCONTROL,
	    callback(this, &XrlPort::tos_cb));
}

void
XrlPort::tos_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_WARNING("Failed to set TOS.");
	return;
    }

    socket_setup_complete();
}

void
XrlPort::socket_setup_complete()
{
    _pending = false;
    debug_msg("%p: socket setup complete\n", this);
    set_status(SERVICE_RUNNING);
    //this->set_enabled(true);
}

//
// Close operations.
//

bool
XrlPort::request_close()
{
    XrlSocket4V0p1Client cl(&_xrl_router);

    debug_msg("%p: sending close\n", this);
    bool success = cl.send_close(_ss.c_str(),
				 _sockid,
				 callback(this, &XrlPort::close_cb));
    if (success)
	_pending = true;

    return success;
}

void
XrlPort::close_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	set_status(SERVICE_FAILED, "Failed to close UDP socket.");
    }

    _pending = false;
    debug_msg("%p: socket shutdown complete\n", this);
    set_status(SERVICE_SHUTDOWN);
    //this->set_enabled(false);
}

//
// Send operations.
//

bool
XrlPort::send_to(const IPv4& dst_addr,
		 const uint16_t dst_port,
		 const vector<uint8_t>& payload)
{
    if (_pending) {
	debug_msg("Port %p: send skipped (pending XRL)\n", this);
	return false;
    }

    XrlSocket4V0p1Client cl(&_xrl_router);

    bool success = cl.send_send_to(_ss.c_str(),
				   _sockid,
				   dst_addr,
				   dst_port,
				   payload,
				   callback(this, &XrlPort::send_cb));

    debug_msg("Sent %u bytes to %s:%u from %s:%u\n",
	      XORP_UINT_CAST(payload.size()),
	      cstring(dst_addr), XORP_UINT_CAST(dst_port),
	      cstring(_local_addr), XORP_UINT_CAST(_local_port));

    if (success)
	_pending = true;

    return success;
}

// TODO: Add an I/O completion notification.

void
XrlPort::send_cb(const XrlError& e)
{
    debug_msg("SendCB %s\n", e.str().c_str());

    if (e != XrlError::OKAY()) {
	XLOG_WARNING("Failed to send datagram.");
    }

    _pending = false;
    //this->_user.port_io_send_completion(xe == XrlError::OKAY());
}

