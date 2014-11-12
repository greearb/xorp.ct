// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#include "libxorp/xorp.h"
#include "libxorp/debug.h"



#include "libxipc/xrl_router.hh"

#include "constants.hh"
#include "xrl_config.hh"
#include "xrl_port_io.hh"
#include "rip_module.h"
#include "libxorp/xlog.h"


// ----------------------------------------------------------------------------
// Multicast socket code.  We instantiate one multicast socket per
// socket server.

template <typename A>
class SocketManager {
public:
    static const string no_entry;

public:
    SocketManager() {}
    ~SocketManager() {}

    const string& sockid(const string& socket_server) const
    {
	map<string,string>::const_iterator ci = _m.find(socket_server);
	if (ci == _m.end()) {
	    return no_entry;
	}
	return ci->second;
    }

    void add_sockid(const string& socket_server, const string& sockid)
    {
	_m[socket_server] = sockid;
    }

public:
    map<string, string> _m;	// [ <socket server name> : <mcast socket id> ]
};

template <typename A> const string SocketManager<A>::no_entry;

#if defined(INSTANTIATE_IPV4)
static SocketManager<IPv4> socket_manager;
#elif defined(INSTANTIATE_IPV6)
static SocketManager<IPv6> socket_manager;
#endif

// ----------------------------------------------------------------------------
// IPv4 specialized XrlPortIO

#ifdef INSTANTIATE_IPV4

#include "xrl/interfaces/socket4_xif.hh"

template <>
bool
XrlPortIO<IPv4>::request_open_bind_socket()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_udp_open_and_bind(
	_ss.c_str(), _xr.instance_name(), IPv4::ANY(),
	RIP_AF_CONSTANTS<IPv4>::IP_PORT, vifname(), 1,
	callback(this, &XrlPortIO<IPv4>::open_bind_socket_cb)
	);
}

template <>
bool
XrlPortIO<IPv4>::request_ttl()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_ttl", RIP_TTL,
		callback(this, &XrlPortIO<IPv4>::ttl_cb));
}

template <>
bool
XrlPortIO<IPv4>::request_no_loop()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_loopback", 0,
		callback(this, &XrlPortIO<IPv4>::no_loop_cb));
}

template <>
bool
XrlPortIO<IPv4>::request_socket_join()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_udp_join_group(
		_ss.c_str(), socket_id(),
		RIP_AF_CONSTANTS<IPv4>::IP_GROUP(), this->address(),
		callback(this, &XrlPortIO<IPv4>::socket_join_cb));
}

template <>
bool
XrlPortIO<IPv4>::request_socket_leave()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_udp_leave_group(
		_ss.c_str(), socket_id(),
		RIP_AF_CONSTANTS<IPv4>::IP_GROUP(), this->address(),
		callback(this, &XrlPortIO<IPv4>::socket_leave_cb));
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
    if (dst_addr.is_multicast()) {
	if (cl.send_send_from_multicast_if(
		_ss.c_str(), socket_id(),
		dst_addr, dst_port, this->address(),
		rip_packet,
		callback(this, &XrlPortIO<IPv4>::send_cb))) {
	    debug_msg("Sent %u bytes to %s/%u from %s\n",
		      XORP_UINT_CAST(rip_packet.size()),
		      dst_addr.str().c_str(), dst_port,
		      this->address().str().c_str());
	    _pending = true;
	    return true;
	}
    } else {
	if (cl.send_send_to(_ss.c_str(), socket_id(),
			    dst_addr, dst_port, rip_packet,
			    callback(this, &XrlPortIO<IPv4>::send_cb))) {
	    debug_msg("Sent %u bytes to %s/%u\n",
		      XORP_UINT_CAST(rip_packet.size()),
		      dst_addr.str().c_str(), dst_port);
	    _pending = true;
	    return true;
	}
    }
    return false;
}

#endif // INSTANTIATE_IPV4


// ----------------------------------------------------------------------------
// IPv6 specialized XrlPortIO

#ifdef INSTANTIATE_IPV6

// TODO:  Fix this up so none of this is compiled if IPv6 is disabled.
#ifdef HAVE_IPV6
#include "xrl/interfaces/socket6_xif.hh"
#endif

template <>
bool
XrlPortIO<IPv6>::request_open_bind_socket()
{
#ifdef HAVE_IPV6
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_udp_open_and_bind(
	_ss.c_str(), _xr.instance_name(), address(),
	RIP_AF_CONSTANTS<IPv6>::IP_PORT, vifname(), 1,
	callback(this, &XrlPortIO<IPv6>::open_bind_socket_cb)
	);
#else
    return false;
#endif
}

template <>
bool
XrlPortIO<IPv6>::request_ttl()
{
#ifdef HAVE_IPV6
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_ttl", RIP_NG_HOP_COUNT,
		callback(this, &XrlPortIO<IPv6>::ttl_cb));
#else
    return false;
#endif
}

template <>
bool
XrlPortIO<IPv6>::request_no_loop()
{
#ifdef HAVE_IPV6
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_loopback", 0,
		callback(this, &XrlPortIO<IPv6>::no_loop_cb));
#else
    return false;
#endif
}

template <>
bool
XrlPortIO<IPv6>::request_socket_join()
{
#ifdef HAVE_IPV6
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_udp_join_group(
		_ss.c_str(), socket_id(),
		RIP_AF_CONSTANTS<IPv6>::IP_GROUP(), this->address(),
		callback(this, &XrlPortIO<IPv6>::socket_join_cb));
#else
    return false;
#endif
}

template <>
bool
XrlPortIO<IPv6>::request_socket_leave()
{
#ifdef HAVE_IPV6
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_udp_leave_group(
		_ss.c_str(), socket_id(),
		RIP_AF_CONSTANTS<IPv6>::IP_GROUP(), this->address(),
		callback(this, &XrlPortIO<IPv6>::socket_leave_cb));
#else
    return false;
#endif
}

template <>
bool
XrlPortIO<IPv6>::send(const IPv6& 		dst_addr,
		      uint16_t 			dst_port,
		      const vector<uint8_t>&  	rip_packet)
{
    if (_pending) {
	debug_msg("Send skipped (pending).\n");
	return false;
    }

#ifdef HAVE_IPV6
    XrlSocket6V0p1Client cl(&_xr);
    if (dst_addr.is_multicast()) {
	if (cl.send_send_from_multicast_if(
		_ss.c_str(), socket_id(),
		dst_addr, dst_port, this->address(),
		rip_packet,
		callback(this, &XrlPortIO<IPv6>::send_cb))) {
	    debug_msg("Sent %u bytes to %s/%u from %s\n",
		      XORP_UINT_CAST(rip_packet.size()),
		      dst_addr.str().c_str(), dst_port,
		      this->address().str().c_str());
	    _pending = true;
	    return true;
	}
    } else {
	if (cl.send_send_to(_ss.c_str(), socket_id(),
			    dst_addr, dst_port, rip_packet,
			    callback(this, &XrlPortIO<IPv6>::send_cb))) {
	    debug_msg("Sent %u bytes to %s/%u\n",
		      XORP_UINT_CAST(rip_packet.size()),
		      dst_addr.str().c_str(), dst_port);
	    _pending = true;
	    return true;
	}
    }
#else
    UNUSED(dst_addr);
    UNUSED(dst_port);
    UNUSED(rip_packet);
#endif
    return false;
}

#endif // INSTANTIATE_IPV6


// ----------------------------------------------------------------------------
// Non-specialized XrlPortIO methods.

template <typename A>
XrlPortIO<A>::XrlPortIO(XrlRouter&	xr,
			PortIOUser&	port,
			const string& 	ifname,
			const string&	vifname,
			const Addr&	addr)
    : PortIOBase<A>(port, ifname, vifname, addr, false),
      ServiceBase("RIP I/O port"), _xr(xr), _pending(false)
{
    debug_msg("Created RIP I/O Port on %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
}


template <typename A>
bool
XrlPortIO<A>::pending() const
{
    debug_msg("%s/%s/%s pending %d\n",
	      this->ifname().c_str(),
	      this->vifname().c_str(),
	      this->address().str().c_str(),
	      _pending);
    return _pending;
}

template <typename A>
int
XrlPortIO<A>::startup()
{
    _pending = true;
    set_status(SERVICE_STARTING);
    if (startup_socket() == false) {
	set_status(SERVICE_FAILED,
		   "Failed to find appropriate socket server.");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

template <typename A>
int
XrlPortIO<A>::shutdown()
{
    _pending = true;
    this->set_enabled(false);
    set_status(SERVICE_SHUTTING_DOWN);
    if (request_socket_leave() == false) {
	set_status(SERVICE_SHUTDOWN);
    }

    return (XORP_OK);
}

template <typename A>
bool
XrlPortIO<A>::startup_socket()
{
    _ss = xrl_fea_name();

    if (_sid.size() == 0) {
	// Nobody has created the RIP socket yet do it!
	// If we succeed here the path is:
	// request_open_bind_socket()
	// -> open_bind_socket_cb()
	//    -> request_ttl()
	//	 -> ttl_cb()
	//	     -> request_no_loop()
	//		-> no_loop_cb()
	//		   ->request_socket_join()
	//
	if (request_open_bind_socket() == false) {
	    set_status(SERVICE_FAILED,
		       "Failed sending RIP socket open request.");
	    return false;
	}
    } else {
	// RIP socket exists, join appropriate interface to multicast
	// group.
	if (request_socket_join() == false) {
	    set_status(SERVICE_FAILED,
		       "Failed sending multicast join request.");
	    return false;
	}
    }

    return true;
}

template <typename A>
void
XrlPortIO<A>::open_bind_socket_cb(const XrlError& e, const string* psid)
{
    if (e != XrlError::OKAY()) {
	set_status(SERVICE_FAILED, "Failed to instantiate RIP socket.");
	return;
    }

    _sid = *psid;
    socket_manager.add_sockid(_ss, _sid);

    if (request_ttl() == false) {
	set_status(SERVICE_FAILED, "Failed requesting ttl/hops.");
    }
}

template <typename A>
void
XrlPortIO<A>::ttl_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_WARNING("Failed to set ttl/hops.");
    }
    if (request_no_loop() == false) {
	set_status(SERVICE_FAILED,
		   "Failed requesting multicast loopback off.");
    }
}

template <typename A>
void
XrlPortIO<A>::no_loop_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_WARNING("Failed to turn off multicast loopback.");
    }
    if (request_socket_join() == false) {
	set_status(SERVICE_FAILED, "Failed to send join request.");
    }
}

template <typename A>
void
XrlPortIO<A>::socket_join_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	set_status(SERVICE_FAILED,
		   c_format("Failed to join group on %s/%s/%s.",
			    this->ifname().c_str(), this->vifname().c_str(),
			    this->address().str().c_str())
		   );
	return;
    }

    _pending = false;
    set_status(SERVICE_RUNNING);
    this->set_enabled(true);
}

template <typename A>
void
XrlPortIO<A>::socket_leave_cb(const XrlError& /* e */)
{
    set_status(SERVICE_SHUTDOWN);
}

template <typename A>
void
XrlPortIO<A>::send_cb(const XrlError& xe)
{
    debug_msg("SendCB %s\n", xe.str().c_str());
    _pending = false;
    this->_user.port_io_send_completion(xe == XrlError::OKAY());
}

#ifdef INSTANTIATE_IPV4
template class XrlPortIO<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class XrlPortIO<IPv6>;
#endif
