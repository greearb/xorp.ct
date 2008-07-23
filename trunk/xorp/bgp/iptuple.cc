// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/iptuple.cc,v 1.23 2008/01/04 03:15:19 pavlin Exp $"

// #define DEBUG_LOGGING 
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/callback.hh"

#include "socket.hh"
#include "iptuple.hh"


Iptuple::Iptuple()
{}

Iptuple::Iptuple(const char *local_interface, uint16_t local_port,
		 const char *peer_interface, uint16_t peer_port)
    throw(UnresolvableHost,AddressFamilyMismatch)
    : _local_interface(local_interface),
      _peer_interface(peer_interface),
      _local_port(local_port),
      _peer_port(peer_port)
{
    _local_sock_len = sizeof(_local_sock);
    _bind_sock_len = sizeof(_bind_sock);
    _peer_sock_len = sizeof(_peer_sock);

    fill_address(local_interface, local_port, _local_sock, _local_sock_len,
		 _local_address);
    string bind_address; // We don't care about this address
    fill_address(local_interface, 0, _bind_sock, _bind_sock_len,
		 bind_address);
    fill_address(peer_interface, peer_port, _peer_sock, _peer_sock_len,
		 _peer_address);

    //  Test for an address family mismatch
    if (_local_sock.ss_family != _peer_sock.ss_family)
	xorp_throw(AddressFamilyMismatch,
		   c_format("mismatch %s (%u) %s (%u)",
			    local_interface, _local_sock.ss_family, 
			    peer_interface, _peer_sock.ss_family));

    _local_address_ipvx = IPvX(_local_address.c_str());
    _peer_address_ipvx = IPvX(_peer_address.c_str());
}

Iptuple::Iptuple(const Iptuple& rhs)
{
    copy(rhs);
}

Iptuple&
Iptuple::operator=(const Iptuple& rhs)
{
    if (&rhs == this)
	return *this;

    copy(rhs);

    return *this;
}

void
Iptuple::copy(const Iptuple& rhs)
{
    _local_interface = rhs._local_interface;
    _peer_interface = rhs._peer_interface;

    memcpy(&_local_sock, &rhs._local_sock, sizeof(_local_sock));
    _local_sock_len = rhs._local_sock_len;

    memcpy(&_bind_sock, &rhs._bind_sock, sizeof(_bind_sock));
    _bind_sock_len = rhs._bind_sock_len;

    memcpy(&_peer_sock, &rhs._peer_sock, sizeof(_peer_sock));
    _peer_sock_len = rhs._peer_sock_len;
    
    _local_address = rhs._local_address;
    _local_address_ipvx = rhs._local_address_ipvx;
    _peer_address = rhs._peer_address;
    _peer_address_ipvx = rhs._peer_address_ipvx;

    _local_port = rhs._local_port;
    _peer_port = rhs._peer_port;
}

bool
Iptuple::operator==(const Iptuple& rhs) const
{
    /*
    ** Note: we don't include the peer port in the comparison.
    ** This peer port is not useful when deciding if an iptuple is
    ** unique as it is not necessarily fixed.
    */
    return _local_address == rhs._local_address &&
	_local_port == rhs._local_port &&
	_peer_address == rhs._peer_address;
}

/*
** Take an IP[46] address and return a completed sockaddr as well as
** the numeric representation of the address.
*/
void
Iptuple::fill_address(const char *interface, uint16_t local_port,
		      struct sockaddr_storage& ss, size_t& len,
		      string& numeric_interface)
    throw(UnresolvableHost)
{
    string port = c_format("%d", local_port);
    const char *servname;
    if (local_port == 0) {
	servname = 0;
    } else {
	servname = port.c_str();
    }

    int error;
    struct addrinfo hints, *res0;
    // Need to provide a hint because we are providing a numeric port number.
    memset(&hints, 0, sizeof(hints));
#ifdef HOST_OS_WINDOWS
    hints.ai_family = PF_INET;
#else
    hints.ai_family = PF_UNSPEC;
#endif
    hints.ai_socktype = SOCK_STREAM;
    if ((error = getaddrinfo(interface, servname, &hints, &res0))) {
	const char *error_string = gai_strerror(error);
	xorp_throw(UnresolvableHost,
		   c_format("getaddrinfo(%s,%s,...) failed: %s",
			    interface, port.c_str(),
			    error_string));

    }

    debug_msg("addrlen %u len %u\n", XORP_UINT_CAST(res0->ai_addrlen),
	XORP_UINT_CAST(len));
    XLOG_ASSERT(res0->ai_addrlen <= len);
    memcpy(&ss, res0->ai_addr, res0->ai_addrlen);
    debug_msg("family %d\n", ss.ss_family);

    len = res0->ai_addrlen;

    // Recover the numeric form of the address
    char hostname[1024];
    if ((error = getnameinfo(res0->ai_addr, res0->ai_addrlen, 
			     hostname, sizeof(hostname),
			     0, 0, NI_NUMERICHOST))) {
	const char *error_string = gai_strerror(error);
	xorp_throw(UnresolvableHost,
		   c_format("getnameinfo() failed: %s", error_string));
    }

    numeric_interface = hostname;
    debug_msg("Original %s Hostname: %s\n", interface,
	      numeric_interface.c_str());
    
    freeaddrinfo(res0);
}

const struct sockaddr *
Iptuple::get_local_socket(size_t& len) const
{
    len = _local_sock_len;
    return reinterpret_cast<const struct sockaddr *>(&_local_sock);
}

string
Iptuple::get_local_addr() const
{
    return _local_address;
}

bool
Iptuple::get_local_addr(IPv4& addr) const
{
    if (!_local_address_ipvx.is_ipv4())
	return false;

    addr = _local_address_ipvx.get_ipv4();

    return true;
}

bool
Iptuple::get_local_addr(IPv6& addr) const
{
    if (!_local_address_ipvx.is_ipv6())
	return false;

    addr = _local_address_ipvx.get_ipv6();

    return true;
}

uint16_t
Iptuple::get_local_port() const
{
    return _local_port;
}

const struct sockaddr *
Iptuple::get_bind_socket(size_t& len) const
{
    len = _bind_sock_len;
    return reinterpret_cast<const struct sockaddr *>(&_bind_sock);
}

const struct sockaddr *
Iptuple::get_peer_socket(size_t& len) const
{
    len = _peer_sock_len;
    return reinterpret_cast<const struct sockaddr *>(&_peer_sock);
}

string
Iptuple::get_peer_addr() const
{
    return _peer_address;
}

bool
Iptuple::get_peer_addr(IPv4& addr) const
{
    if (!_peer_address_ipvx.is_ipv4())
	return false;

    addr = _peer_address_ipvx.get_ipv4();

    return true;
}

bool
Iptuple::get_peer_addr(IPv6& addr) const
{
    if (!_peer_address_ipvx.is_ipv6())
	return false;

    addr = _peer_address_ipvx.get_ipv6();

    return true;
}

uint16_t
Iptuple::get_peer_port() const
{
    return _peer_port;
}

string
Iptuple::str() const
{
    return c_format("{%s(%d) %s(%d)}",
		    _local_interface.c_str(), _local_port,
		    _peer_interface.c_str(),  _peer_port);
}
