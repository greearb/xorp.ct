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

#ident "$XORP: xorp/bgp/iptuple.cc,v 1.3 2003/03/10 23:19:58 hodson Exp $"

// #define DEBUG_LOGGING 
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/callback.hh"

#include "socket.hh"
#include "iptuple.hh"

Iptuple::Iptuple()
{}

Iptuple::Iptuple(const char *local_interface, uint16_t local_port,
		 const char *peer_interface, uint16_t peer_port)
    throw(UnresolvableHost)
    : _local_interface(local_interface),
      _peer_interface(peer_interface)
{
    _local = get_addr(local_interface);
    _local_port = htons(local_port);

    _peer = get_addr(peer_interface);
    _peer_port = htons(peer_port);
}

Iptuple::Iptuple(const IPv4& local_ip,  uint16_t local_port,
		 const IPv4& peer_ip, uint16_t peer_port)
{
    _local.s_addr = local_ip.addr();
    _local_port = htons(local_port);
    _peer.s_addr = peer_ip.addr();
    _peer_port = htons(peer_port);
}

Iptuple::Iptuple(const Iptuple& rhs)
{
    copy(rhs);
}

Iptuple
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

    _local = rhs._local;
    _local_port = rhs._local_port;
    _peer = rhs._peer;
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
    return _local.s_addr == rhs._local.s_addr &&
	_local_port == rhs._local_port &&
	_peer.s_addr == rhs._peer.s_addr;
}

/*
** Convert a domain name (or IP address string) to an IP address in
** host byte order.
*/
struct in_addr
Iptuple::get_addr(const char *host) throw(UnresolvableHost)
{
    struct in_addr addr;

    if (0 == host) {
	addr.s_addr = 0;
	return addr;
    }

    if (xorp_isalpha(*host)) {
	struct hostent *hostinfo = gethostbyname(host);
	if (hostinfo == NULL)
	    xorp_throw(UnresolvableHost, c_format("Unknown host <%s>",
						       host));

	XLOG_ASSERT(4 == hostinfo->h_length);
	memcpy(&addr.s_addr, hostinfo->h_addr_list[0], hostinfo->h_length);
    } else {
	addr.s_addr = inet_addr(host);
    }

    debug_msg("IP address = %s\n", inet_ntoa(addr));

    return addr;
}

struct in_addr
Iptuple::get_local_addr() const
{
    return _local;
}

uint16_t
Iptuple::get_local_port() const
{
    return _local_port;
}

struct in_addr
Iptuple::get_peer_addr() const
{
    return _peer;
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
		    _local_interface.c_str(), ntohs(_local_port),
		    _peer_interface.c_str(),  ntohs(_peer_port));
}
