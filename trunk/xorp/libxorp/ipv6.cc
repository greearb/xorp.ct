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

#ident "$XORP: xorp/libxorp/ipv6.cc,v 1.2 2003/02/26 00:14:14 pavlin Exp $"

#include "xorp.h"
#include "ipv6.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  


IPv6::IPv6(const uint8_t *from_uint8)
{
    memcpy(_addr, from_uint8, sizeof(_addr));
}

IPv6::IPv6(const uint32_t *from_uint32)
{
    _addr[0] = from_uint32[0];
    _addr[1] = from_uint32[1];
    _addr[2] = from_uint32[2];
    _addr[3] = from_uint32[3];
}

IPv6::IPv6(const in6_addr& from_in6_addr)
{
    memcpy(_addr, &from_in6_addr, sizeof(_addr));
}

IPv6::IPv6(const sockaddr& sa) throw (InvalidFamily)
{
    if (sa.sa_family != AF_INET6)
	xorp_throw(InvalidFamily, sa.sa_family);
    const sockaddr_in6& sin = reinterpret_cast<const sockaddr_in6&>(sa);
    memcpy(_addr, sin.sin6_addr.s6_addr, sizeof(_addr));
}

IPv6::IPv6(const sockaddr_in6& sin) throw (InvalidFamily)
{
    if (sin.sin6_family != AF_INET6)
	xorp_throw(InvalidFamily, sin.sin6_family);
    memcpy(_addr, sin.sin6_addr.s6_addr, sizeof(_addr));
}

IPv6::IPv6(const char *from_cstring) throw (InvalidString)
{
    if (from_cstring == NULL)
	xorp_throw(InvalidString, "Null value" );
    if (inet_pton(AF_INET6, from_cstring, &_addr[0]) <= 0)
	xorp_throw(InvalidString, c_format("Bad IPv6 \"%s\"", from_cstring));
}

IPv6
IPv6::operator<<(size_t ls) const
{
    uint32_t tmp_addr[4];
    static_assert(sizeof(_addr) == sizeof(tmp_addr));
    
    // Shift the words, and at the same time convert them into host-order
    switch (ls / 32) {
    case 0:
	tmp_addr[0] = ntohl(_addr[0]);
	tmp_addr[1] = ntohl(_addr[1]);
	tmp_addr[2] = ntohl(_addr[2]);
	tmp_addr[3] = ntohl(_addr[3]);
	break;
    case 1:
	tmp_addr[0] = ntohl(_addr[1]);
	tmp_addr[1] = ntohl(_addr[2]);
	tmp_addr[2] = ntohl(_addr[3]);
	tmp_addr[3] = 0;
	break;
    case 2:
	tmp_addr[0] = ntohl(_addr[2]);
	tmp_addr[1] = ntohl(_addr[3]);
	tmp_addr[2] = 0;
	tmp_addr[3] = 0;
	break;
    case 3:
	tmp_addr[0] = ntohl(_addr[3]);
	tmp_addr[1] = 0;
	tmp_addr[2] = 0;
	tmp_addr[3] = 0;
	break;
    default:
	// ls >= 128; clear all bits
	return ZERO();
    }
    
    ls &= 0x1f;
    if (ls != 0) {
	uint32_t rs = 32 - ls;
	tmp_addr[0] = (tmp_addr[0] << ls) | (tmp_addr[1] >> rs);
	tmp_addr[1] = (tmp_addr[1] << ls) | (tmp_addr[2] >> rs);
	tmp_addr[2] = (tmp_addr[2] << ls) | (tmp_addr[3] >> rs);
	tmp_addr[3] = tmp_addr[3] << ls;
    }
    
    // Convert the words back into network-order
    tmp_addr[0] = htonl(tmp_addr[0]);
    tmp_addr[1] = htonl(tmp_addr[1]);
    tmp_addr[2] = htonl(tmp_addr[2]);
    tmp_addr[3] = htonl(tmp_addr[3]);
    
    return IPv6(tmp_addr);
}

IPv6
IPv6::operator>>(size_t rs) const
{
    uint32_t tmp_addr[4];
    static_assert(sizeof(_addr) == sizeof(tmp_addr));
    
    // Shift the words, and at the same time convert them into host-order
    switch (rs / 32) {
    case 0:
	tmp_addr[3] = ntohl(_addr[3]);
	tmp_addr[2] = ntohl(_addr[2]);
	tmp_addr[1] = ntohl(_addr[1]);
	tmp_addr[0] = ntohl(_addr[0]);
	break;
    case 1:
	tmp_addr[3] = ntohl(_addr[2]);
	tmp_addr[2] = ntohl(_addr[1]);
	tmp_addr[1] = ntohl(_addr[0]);
	tmp_addr[0] = 0;
	break;
    case 2:
	tmp_addr[3] = ntohl(_addr[1]);
	tmp_addr[2] = ntohl(_addr[0]);
	tmp_addr[1] = 0;
	tmp_addr[0] = 0;
	break;
    case 3:
	tmp_addr[3] = ntohl(_addr[0]);
	tmp_addr[2] = 0;
	tmp_addr[1] = 0;
	tmp_addr[0] = 0;
	break;
    default:
	// rs >= 128; clear all bits
	return ZERO();
    }
    
    rs &= 0x1f;
    if (rs != 0) {
	uint32_t ls = 32 - rs;
	tmp_addr[3] = (tmp_addr[3] >> rs) | (tmp_addr[2] << ls);
	tmp_addr[2] = (tmp_addr[2] >> rs) | (tmp_addr[1] << ls);
	tmp_addr[1] = (tmp_addr[1] >> rs) | (tmp_addr[0] << ls);
	tmp_addr[0] = (tmp_addr[0] >> rs);
    }
    
    // Convert the words back into network-order
    tmp_addr[0] = htonl(tmp_addr[0]);
    tmp_addr[1] = htonl(tmp_addr[1]);
    tmp_addr[2] = htonl(tmp_addr[2]);
    tmp_addr[3] = htonl(tmp_addr[3]);
    
    return IPv6(tmp_addr);
}

bool
IPv6::operator<(const IPv6& other) const
{
    int i;
    static_assert(sizeof(_addr) == 16);
    
    for (i = 0; i < 3; i++) {	// XXX: Loop ends intentionally at 3 not 4
	if (_addr[i] != other._addr[i])
	    break;
    }
    return ntohl(_addr[i]) < ntohl(other._addr[i]);
}

bool
IPv6::operator==(const IPv6& other) const
{
    return ((_addr[0] == other._addr[0])
	    && (_addr[1] == other._addr[1])
	    && (_addr[2] == other._addr[2])
	    && (_addr[3] == other._addr[3]));
}

bool
IPv6::operator!=(const IPv6& other) const
{
    return ((_addr[0] != other._addr[0])
	    || (_addr[1] != other._addr[1])
	    || (_addr[2] != other._addr[2])
	    || (_addr[3] != other._addr[3]));
}

IPv6& 
IPv6::operator--()
{ 
    for (int i = 3; i >= 0; i--) {
	if (_addr[i] == 0) {
	    _addr[i] = 0xffffffffU;
	} else {
	    uint32_t tmp_addr = ntohl(_addr[i]) - 1;
	    _addr[i] = htonl(tmp_addr); 
	    return *this;
	}
    }
    return *this;
}
    
IPv6& 
IPv6::operator++()
{ 
    for (int i = 3; i >= 0; i--) {
	if (_addr[i] == 0xffffffffU) {
	    _addr[i] = 0;
	} else {
	    uint32_t tmp_addr = ntohl(_addr[i]) + 1;
	    _addr[i] = htonl(tmp_addr);
	    return *this;
	}
    }
    return *this;
}


static IPv6 v6prefix[129];

static size_t
init_prefixes()
{
    uint32_t u[4];
    u[0] = u[1] = u[2] = u[3] = 0xffffffff;
    IPv6 a1(u);
    for (int i = 0; i <= 128; i++) {
	v6prefix[i] = a1 << (128 - i);
    }
    return 128;
}
static size_t n_inited_prefixes = init_prefixes();

const IPv6&
IPv6::make_prefix(size_t l)
{
    if (l < n_inited_prefixes)
	return v6prefix[l];
    assert(l <= n_inited_prefixes);
    return ALL_ONES();
}

string
IPv6::str() const
{
    char str_buffer[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
    
    inet_ntop(AF_INET6, &_addr[0], str_buffer, sizeof(str_buffer));
    return (str_buffer);	// XXX: implicitly create string return object
}

bool
IPv6::is_unicast() const
{
    // Icky casting because of alternate definitions of IN6_IS_ADDR_MULTICAST
    uint32_t *nc = const_cast<uint32_t*>(_addr);
    in6_addr *addr6 = reinterpret_cast<in6_addr *>(nc);
    
    return (! (IN6_IS_ADDR_MULTICAST(addr6)
	       || IN6_IS_ADDR_UNSPECIFIED(const_cast<in6_addr*>(addr6))));
}

bool
IPv6::is_multicast() const
{
    // Icky casting because of alternate definitions of IN6_IS_ADDR_MULTICAST
    uint32_t *nc = const_cast<uint32_t*>(_addr);
    in6_addr *addr6 = reinterpret_cast<in6_addr *>(nc);
    
    return (IN6_IS_ADDR_MULTICAST(addr6));
}

bool
IPv6::is_nodelocal_multicast() const
{
    // Icky casting because of alternate definitions
    // of IN6_IS_ADDR_MC_NODELOCAL
    uint32_t *nc = const_cast<uint32_t*>(_addr);
    in6_addr *addr6 = reinterpret_cast<in6_addr *>(nc);
    
    return (IN6_IS_ADDR_MC_NODELOCAL(addr6));
}

bool
IPv6::is_linklocal_multicast() const
{
    // Icky casting because of alternate definitions
    // of IN6_IS_ADDR_MC_LINKLOCAL
    uint32_t *nc = const_cast<uint32_t*>(_addr);
    in6_addr *addr6 = reinterpret_cast<in6_addr *>(nc);
    
    return (IN6_IS_ADDR_MC_LINKLOCAL(addr6));
}

size_t
IPv6::prefix_length() const
{
    size_t ctr = 0;
    
    for (int j = 0; j < 4; j++) {
	uint32_t shift = ntohl(_addr[j]);
	
	for (int i = 0; i < 32; i++) {
	    if ((shift & 0x80000000U) != 0) {
		ctr++;
		shift = shift << 1;
	    } else {
		return ctr;
	    }
	}
    }
    return ctr;
}

const IPv6 IPv6Constants::zero("::");
const IPv6 IPv6Constants::any(IPv6Constants::zero);
const IPv6 IPv6Constants::all_ones(~IPv6Constants::zero);
const IPv6 IPv6Constants::multicast_base("FF00::");
const IPv6 IPv6Constants::multicast_all_systems("FF02::1");
const IPv6 IPv6Constants::multicast_all_routers("FF02::2");
const IPv6 IPv6Constants::dvmrp_routers("FF02::4");
const IPv6 IPv6Constants::ospfigp_routers("FF02::5");
const IPv6 IPv6Constants::ospfigp_designated_routers("FF02::6");
const IPv6 IPv6Constants::rip2_routers("FF02::9");
const IPv6 IPv6Constants::pim_routers("FF02::D");
