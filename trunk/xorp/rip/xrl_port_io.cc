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

#ident "$XORP: xorp/rip/xrl_port_io.cc,v 1.7 2004/03/20 18:03:59 hodson Exp $"

#define DEBUG_LOGGING

#include "config.h"
#include <map>

#include "libxorp/debug.h"

#include "constants.hh"
#include "xrl_port_io.hh"

#include "libxipc/xrl_router.hh"

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

#include "xrl/interfaces/socket4_locator_xif.hh"
#include "xrl/interfaces/socket4_xif.hh"

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
XrlPortIO<IPv4>::request_open_bind_socket()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_udp_open_and_bind(
		_ss.c_str(), _xr.instance_name(), IPv4::ANY(),
		RIP_AF_CONSTANTS<IPv4>::IP_PORT,
		callback(this, &XrlPortIO<IPv4>::open_bind_socket_cb)
		);
}

template <>
bool
XrlPortIO<IPv4>::request_ttl_one()
{
    XrlSocket4V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_ttl", 1,
		callback(this, &XrlPortIO<IPv4>::ttl_one_cb));
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
		      static_cast<uint32_t>(rip_packet.size()),
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
		      static_cast<uint32_t>(rip_packet.size()),
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

#include "xrl/interfaces/socket6_locator_xif.hh"
#include "xrl/interfaces/socket6_xif.hh"

template <>
bool
XrlPortIO<IPv6>::request_socket_server()
{
    XrlSocket6LocatorV0p1Client cl(&_xr);
    return cl.send_find_socket_server_for_addr("fea", _addr,
		callback(this, &XrlPortIO<IPv6>::socket_server_cb));
}

template <>
bool
XrlPortIO<IPv6>::request_open_bind_socket()
{
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_udp_open_and_bind(
		_ss.c_str(), _xr.instance_name(), IPv6::ANY(),
		RIP_AF_CONSTANTS<IPv6>::IP_PORT,
		callback(this, &XrlPortIO<IPv6>::open_bind_socket_cb)
		);
}

template <>
bool
XrlPortIO<IPv6>::request_ttl_one()
{
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_hops", 1,
		callback(this, &XrlPortIO<IPv6>::ttl_one_cb));
}

template <>
bool
XrlPortIO<IPv6>::request_no_loop()
{
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_set_socket_option(
		_ss.c_str(), socket_id(), "multicast_loopback", 0,
		callback(this, &XrlPortIO<IPv6>::no_loop_cb));
}

template <>
bool
XrlPortIO<IPv6>::request_socket_join()
{
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_udp_join_group(
		_ss.c_str(), socket_id(),
		RIP_AF_CONSTANTS<IPv6>::IP_GROUP(), this->address(),
		callback(this, &XrlPortIO<IPv6>::socket_join_cb));
}

template <>
bool
XrlPortIO<IPv6>::request_socket_leave()
{
    XrlSocket6V0p1Client cl(&_xr);
    return cl.send_udp_leave_group(
		_ss.c_str(), socket_id(),
		RIP_AF_CONSTANTS<IPv6>::IP_GROUP(), this->address(),
		callback(this, &XrlPortIO<IPv6>::socket_leave_cb));
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

    XrlSocket6V0p1Client cl(&_xr);
    if (dst_addr.is_multicast()) {
	if (cl.send_send_from_multicast_if(
		_ss.c_str(), socket_id(),
		dst_addr, dst_port, this->address(),
		rip_packet,
		callback(this, &XrlPortIO<IPv6>::send_cb))) {
	    debug_msg("Sent %u bytes to %s/%u from %s\n",
		      static_cast<uint32_t>(rip_packet.size()),
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
		      static_cast<uint32_t>(rip_packet.size()),
		      dst_addr.str().c_str(), dst_port);
	    _pending = true;
	    return true;
	}
    }
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
    this->set_enabled(false);
    set_status(SHUTTING_DOWN);
    if (request_socket_leave() == false) {
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

    _sid = socket_manager.sockid(_ss);
    if (_sid == SocketManager<A>::no_entry) {
	// Nobody has created the RIP socket yet do it!
	// If we succeed here the path is:
	// request_open_bind_socket()
	// -> open_bind_socket_cb()
	//    -> request_ttl_one()
	//	 -> ttl_one_cb()
	//	     -> request_no_loop()
	//		-> no_loop_cb()
	//		   ->request_socket_join()
	//
	if (request_open_bind_socket() == false) {
	    set_status(FAILED,
		       "Failed sending RIP socket open request.");
	}
    } else {
	// RIP socket exists, join appropriate interface to multicast
	// group.
	if (request_socket_join() == false) {
	    set_status(FAILED,
		       "Failed sending multicast join request.");
	}
    }
}

template <typename A>
void
XrlPortIO<A>::open_bind_socket_cb(const XrlError& e, const string* psid)
{
    if (e != XrlError::OKAY()) {
	set_status(FAILED, "Failed to instantiate RIP socket.");
	return;
    }

    _sid = *psid;
    socket_manager.add_sockid(_ss, _sid);

    if (request_ttl_one() == false) {
	set_status(FAILED, "Failed requesting ttl/hops of 1.");
    }
}

template <typename A>
void
XrlPortIO<A>::ttl_one_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_WARNING("Failed to set ttl/hops to 1");
    }
    if (request_no_loop() == false) {
	set_status(FAILED, "Failed requesting multicast loopback off.");
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
	set_status(FAILED, "Failed to send join request.");
    }
}

template <typename A>
void
XrlPortIO<A>::socket_join_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	set_status(FAILED,
		   c_format("Failed to join group on %s/%s/%s.",
			    this->ifname().c_str(), this->vifname().c_str(),
			    this->address().str().c_str())
		   );
	return;
    }

    _pending = false;
    set_status(RUNNING);
    this->set_enabled(true);
}

template <typename A>
void
XrlPortIO<A>::socket_leave_cb(const XrlError& /* e */)
{
    set_status(SHUTDOWN);
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
