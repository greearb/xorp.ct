// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
//
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "libxorp/xorp.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "ipv6.hh"


IPv6::IPv6(const uint8_t* from_uint8)
{
    memcpy(_addr, from_uint8, sizeof(_addr));
}

IPv6::IPv6(const uint32_t* from_uint32)
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
    const sockaddr_in6* sin6 = sockaddr2sockaddr_in6(&sa);
    memcpy(_addr, sin6->sin6_addr.s6_addr, sizeof(_addr));
}

IPv6::IPv6(const sockaddr_storage& ss) throw (InvalidFamily)
{
    if (ss.ss_family != AF_INET6)
	xorp_throw(InvalidFamily, ss.ss_family);
    const sockaddr* sa = sockaddr_storage2sockaddr(&ss);
    const sockaddr_in6* sin6 = sockaddr2sockaddr_in6(sa);
    memcpy(_addr, sin6->sin6_addr.s6_addr, sizeof(_addr));
}

IPv6::IPv6(const sockaddr_in6& sin6) throw (InvalidFamily)
{
    if (sin6.sin6_family != AF_INET6)
	xorp_throw(InvalidFamily, sin6.sin6_family);
    memcpy(_addr, sin6.sin6_addr.s6_addr, sizeof(_addr));
}

IPv6::IPv6(const char* from_cstring) throw (InvalidString)
{
    if (from_cstring == NULL)
	xorp_throw(InvalidString, "Null value" );
    if (inet_pton(AF_INET6, from_cstring, &_addr[0]) <= 0)
	xorp_throw(InvalidString, c_format("Bad IPv6 \"%s\"", from_cstring));
}

/**
 * Copy the raw address to memory pointed by @to.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_out(uint8_t* to_uint8) const
{
    memcpy(to_uint8, _addr, addr_bytelen());
    return addr_bytelen();
}

/**
 * Copy the raw address to @in6_addr.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_out(struct in6_addr& to_in6_addr) const
{
    return (copy_out((uint8_t* )&to_in6_addr));
}

/**
 * Copy the raw address to @to_sockaddr, and assign appropriately
 * the rest of the fields in @to_sockaddr.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_out(struct sockaddr& to_sockaddr) const
{
    return (copy_out(*sockaddr2sockaddr_in6(&to_sockaddr)));
}

/**
 * Copy the raw address to @to_sockaddr_storage, and assign appropriately
 * the rest of the fields in @to_sockaddr_storage.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_out(struct sockaddr_storage& to_sockaddr_storage) const
{
    return (copy_out(*sockaddr_storage2sockaddr(&to_sockaddr_storage)));
}

/**
 * Copy the raw address to @to_sockaddr_in6, and assign appropriately
 * the rest of the fields in @to_sockaddr_in6.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_out(struct sockaddr_in6& to_sockaddr_in6) const
{
    memset(&to_sockaddr_in6, 0, sizeof(to_sockaddr_in6));
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
    to_sockaddr_in6.sin6_len = sizeof(to_sockaddr_in6);
#endif
    to_sockaddr_in6.sin6_family = AF_INET6;

#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
#ifdef IPV6_STACK_KAME
    //
    // XXX: In case of KAME the local interface index (also the link-local
    // scope_id) is encoded in the third and fourth octet of an IPv6
    // address (for link-local unicast/multicast addresses or
    // interface-local multicast addresses only).
    //
    if (is_linklocal_unicast()
	|| is_linklocal_multicast()
	|| is_interfacelocal_multicast()) {
	uint32_t addr0 = ntohl(_addr[0]);
	uint16_t zoneid = (addr0 & 0xffff);		// XXX: 16 bits only
	to_sockaddr_in6.sin6_scope_id = zoneid;
    }
#endif // IPV6_STACK_KAME
#endif // HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID

    return (copy_out(to_sockaddr_in6.sin6_addr));
}

/**
 * Copy a raw address from the memory pointed by @from_uint8.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_in(const uint8_t* from_uint8)
{
    memcpy(_addr, from_uint8, addr_bytelen());
    return (addr_bytelen());
}

/**
 * Copy a raw address of family %AF_INET6 from @from_in6_addr.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_in(const in6_addr& from_in6_addr)
{
    return (copy_in(reinterpret_cast<const uint8_t*>(&from_in6_addr)));
}

/**
 * Copy a raw address from @from_sockaddr.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_in(const sockaddr& from_sockaddr) throw (InvalidFamily)
{
    return (copy_in(*sockaddr2sockaddr_in6(&from_sockaddr)));
}

/**
 * Copy a raw address from @from_sockaddr_storage.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_in(const sockaddr_storage& from_sockaddr_storage)
    throw (InvalidFamily)
{
    return (copy_in(*sockaddr_storage2sockaddr(&from_sockaddr_storage)));
}

/**
 * Copy a raw address from @from_sockaddr_in6.
 * @return the number of copied octets.
 */
size_t
IPv6::copy_in(const sockaddr_in6& from_sockaddr_in6) throw (InvalidFamily)
{
    if (from_sockaddr_in6.sin6_family != AF_INET6)
	xorp_throw(InvalidFamily, from_sockaddr_in6.sin6_family);
    return (copy_in(from_sockaddr_in6.sin6_addr));
}

IPv6
IPv6::operator<<(uint32_t ls) const
{
    uint32_t tmp_addr[4];
    x_static_assert(sizeof(_addr) == sizeof(tmp_addr));

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
IPv6::operator>>(uint32_t rs) const
{
    uint32_t tmp_addr[4];
    x_static_assert(sizeof(_addr) == sizeof(tmp_addr));

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

static uint32_t
init_prefixes(IPv6* v6prefix)
{
    uint32_t u[4];
    u[0] = u[1] = u[2] = u[3] = 0xffffffff;
    IPv6 a1(u);
    for (int i = 0; i <= 128; i++) {
	v6prefix[i] = a1 << (128 - i);
    }
    return 128;
}

const IPv6&
IPv6::make_prefix(uint32_t mask_len) throw (InvalidNetmaskLength)
{
    static IPv6 v6prefix[129];
    static uint32_t n_inited_prefixes = init_prefixes(&v6prefix[0]);

    if (mask_len > n_inited_prefixes)
	xorp_throw(InvalidNetmaskLength, mask_len);
    return v6prefix[mask_len];
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
    uint32_t* nc = const_cast<uint32_t*>(_addr);
    in6_addr* addr6 = reinterpret_cast<in6_addr*>(nc);

    return (! (IN6_IS_ADDR_MULTICAST(addr6)
	       || IN6_IS_ADDR_UNSPECIFIED(const_cast<in6_addr*>(addr6))));
}

bool
IPv6::is_multicast() const
{
    // Icky casting because of alternate definitions of IN6_IS_ADDR_MULTICAST
    uint32_t* nc = const_cast<uint32_t*>(_addr);
    in6_addr* addr6 = reinterpret_cast<in6_addr*>(nc);

    return (IN6_IS_ADDR_MULTICAST(addr6));
}

bool
IPv6::is_linklocal_unicast() const
{
    const in6_addr* addr6 = reinterpret_cast<const in6_addr*>(_addr);

    return (IN6_IS_ADDR_LINKLOCAL(addr6));
}

bool
IPv6::is_interfacelocal_multicast() const
{
    // Icky casting because of alternate definitions
    // of IN6_IS_ADDR_MC_NODELOCAL
    uint32_t* nc = const_cast<uint32_t*>(_addr);
    in6_addr* addr6 = reinterpret_cast<in6_addr*>(nc);

    return (IN6_IS_ADDR_MC_NODELOCAL(addr6));
}

bool
IPv6::is_linklocal_multicast() const
{
    // Icky casting because of alternate definitions
    // of IN6_IS_ADDR_MC_LINKLOCAL
    uint32_t* nc = const_cast<uint32_t*>(_addr);
    in6_addr* addr6 = reinterpret_cast<in6_addr*>(nc);

    return (IN6_IS_ADDR_MC_LINKLOCAL(addr6));
}

bool
IPv6::is_loopback() const
{
    const uint32_t* nc = _addr;
    const in6_addr* addr6 = reinterpret_cast<const in6_addr*>(nc);
    return IN6_IS_ADDR_LOOPBACK(addr6);
}

uint32_t
IPv6::mask_len() const
{
    uint32_t ctr = 0;

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

const string&
IPv6::ip_version_str()
{
    static const string IP_VERSION_STR("IPv6");
    return IP_VERSION_STR;
}

const IPv6 IPv6Constants::zero("::");
const IPv6 IPv6Constants::any(IPv6Constants::zero);
const IPv6 IPv6Constants::all_ones(~IPv6Constants::zero);
const IPv6 IPv6Constants::loopback("::1");
const IPv6 IPv6Constants::multicast_base("FF00::");
const IPv6 IPv6Constants::multicast_all_systems("FF02::1");
const IPv6 IPv6Constants::multicast_all_routers("FF02::2");
const IPv6 IPv6Constants::dvmrp_routers("FF02::4");
const IPv6 IPv6Constants::ospfigp_routers("FF02::5");
const IPv6 IPv6Constants::ospfigp_designated_routers("FF02::6");
const IPv6 IPv6Constants::rip2_routers("FF02::9");
const IPv6 IPv6Constants::pim_routers("FF02::D");
const IPv6 IPv6Constants::ssm_routers("FF02::16");
