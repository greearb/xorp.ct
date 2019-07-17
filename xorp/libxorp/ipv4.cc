// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#include "ipv4.hh"


IPv4::IPv4(const uint8_t *from_uint8)
{
    memcpy(&_addr, from_uint8, sizeof(_addr));
}

IPv4::IPv4(const in_addr& from_in_addr)
{
    _addr = from_in_addr.s_addr;
}

IPv4::IPv4(const sockaddr& sa) throw (InvalidFamily)
{
    if (sa.sa_family != AF_INET)
	xorp_throw(InvalidFamily, sa.sa_family);
    const sockaddr_in* sin = sockaddr2sockaddr_in(&sa);
    _addr = sin->sin_addr.s_addr;
}

IPv4::IPv4(const sockaddr_storage& ss) throw (InvalidFamily)
{
    if (ss.ss_family != AF_INET)
	xorp_throw(InvalidFamily, ss.ss_family);
    const sockaddr* sa = sockaddr_storage2sockaddr(&ss);
    const sockaddr_in* sin = sockaddr2sockaddr_in(sa);
    _addr = sin->sin_addr.s_addr;
}

IPv4::IPv4(const sockaddr_in& sin) throw(InvalidFamily)
{
    if (sin.sin_family != AF_INET)
	xorp_throw(InvalidFamily, sin.sin_family);
    _addr = sin.sin_addr.s_addr;
}

IPv4::IPv4(const char *from_cstring) throw (InvalidString)
{
    if (from_cstring == NULL)
	xorp_throw(InvalidString, "Null value" );
    if (inet_pton(AF_INET, from_cstring, &_addr) <= 0)
	xorp_throw(InvalidString, c_format("Bad IPv4 \"%s\"", from_cstring));
}

/**
 * Copy the raw address to memory pointed by @to.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_out(uint8_t *to_uint8) const
{
    memcpy(to_uint8, &_addr, addr_bytelen());
    return addr_bytelen();
}

/**
 * Copy the raw address to @to_in_addr.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_out(in_addr& to_in_addr) const
{
    return (copy_out((uint8_t *)&to_in_addr));
}

/**
 * Copy the raw address to @to_sockaddr, and assign appropriately
 * the rest of the fields in @to_sockaddr.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_out(struct sockaddr& to_sockaddr) const
{
    return (copy_out(*sockaddr2sockaddr_in(&to_sockaddr)));
}

/**
 * Copy the raw address to @to_sockaddr_storage, and assign appropriately
 * the rest of the fields in @to_sockaddr_storage.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_out(struct sockaddr_storage& to_sockaddr_storage) const
{
    return (copy_out(*sockaddr_storage2sockaddr(&to_sockaddr_storage)));
}

/**
 * Copy the raw address to @to_sockaddr_in, and assign appropriately
 * the rest of the fields in @to_sockaddr_in.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_out(struct sockaddr_in& to_sockaddr_in) const
{
    memset(&to_sockaddr_in, 0, sizeof(to_sockaddr_in));
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    to_sockaddr_in.sin_len = sizeof(to_sockaddr_in);
#endif
    to_sockaddr_in.sin_family = AF_INET;
    to_sockaddr_in.sin_port = 0;                    // XXX: not used
    return (copy_out(to_sockaddr_in.sin_addr));
}

/**
 * Copy a raw address from the memory pointed by @from_uint8.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_in(const uint8_t *from_uint8)
{
    memcpy(&_addr, from_uint8, addr_bytelen());
    return (addr_bytelen());
}

/**
 * Copy a raw address of family %AF_INET from @from_in_addr.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_in(const in_addr& from_in_addr)
{
    return (copy_in(reinterpret_cast<const uint8_t *>(&from_in_addr)));
}

/**
 * Copy a raw address from @from_sockaddr.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_in(const sockaddr& from_sockaddr) throw (InvalidFamily)
{
    return (copy_in(*sockaddr2sockaddr_in(&from_sockaddr)));
}

/**
 * Copy a raw address from @from_sockaddr_storage.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_in(const sockaddr_storage& from_sockaddr_storage)
    throw (InvalidFamily)
{
    return (copy_in(*sockaddr_storage2sockaddr(&from_sockaddr_storage)));
}

/**
 * Copy a raw address from @from_sockaddr_in.
 * @return the number of copied octets.
 */
size_t
IPv4::copy_in(const sockaddr_in& from_sockaddr_in) throw (InvalidFamily)
{
    if (from_sockaddr_in.sin_family != AF_INET)
	xorp_throw(InvalidFamily, from_sockaddr_in.sin_family);
    return (copy_in(from_sockaddr_in.sin_addr));
}

IPv4
IPv4::operator<<(uint32_t left_shift) const
{
    if (left_shift >= 32) {
	// Clear all bits.
	// XXX: special case, because in C the behavior is undefined.
	return (IPv4::ZERO());
    }

    uint32_t tmp_addr = ntohl(_addr) << left_shift;
    return IPv4(htonl(tmp_addr));
}

IPv4
IPv4::operator>>(uint32_t right_shift) const
{
    if (right_shift >= 32) {
	// Clear all bits.
	// XXX: special case, because in C the behavior is undefined.
	return IPv4::ZERO();
    }

    uint32_t tmp_addr = ntohl(_addr) >> right_shift;
    return IPv4(htonl(tmp_addr));
}

IPv4
IPv4::make_prefix(uint32_t mask_len) throw (InvalidNetmaskLength)
{
    if (mask_len > 32)
	xorp_throw(InvalidNetmaskLength, mask_len);
    uint32_t m = (mask_len == 0) ? 0 : ((~0U) << (32 - mask_len));
    return IPv4(htonl(m));
}

uint32_t
IPv4::mask_len() const
{
    uint32_t ctr = 0;
    uint32_t shift = ntohl(_addr);

    for (int i = 0; i < 32; i++) {
	if ((shift & 0x80000000U) != 0) {
	    ctr++;
	    shift = shift << 1;
	} else {
	    return ctr;
	}
    }
    return ctr;
}

string
IPv4::str() const
{
    struct in_addr in;

    in.s_addr = _addr;
    return (inet_ntoa(in));	// XXX: implicitly create string return object
}

bool
IPv4::is_unicast() const
{
    uint32_t addr4 = ntohl(_addr);

    return (! (IN_MULTICAST(addr4)
	       || IN_BADCLASS(addr4)
	       || (addr4 == 0)));
}

bool
IPv4::is_multicast() const
{
    uint32_t addr4 = ntohl(_addr);

    return (IN_MULTICAST(addr4));
}

bool
IPv4::is_class_a() const
{
    uint32_t addr4 = ntohl(_addr);

    return (IN_CLASSA(addr4));
}

bool
IPv4::is_class_b() const
{
    uint32_t addr4 = ntohl(_addr);

    return (IN_CLASSB(addr4));
}

bool
IPv4::is_class_c() const
{
    uint32_t addr4 = ntohl(_addr);

    return (IN_CLASSC(addr4));
}

bool
IPv4::is_experimental() const
{
    uint32_t addr4 = ntohl(_addr);

    //
    // XXX: We use IN_BADCLASS() instead of IN_EXPERIMENTAL(), because
    // the definition of IN_EXPERIMENTAL() is broken in Linux's <netinet/in.h>
    // (it covers all addresses that start with 0xe0000000, which includes
    // multicast as well).
    //
    return (IN_BADCLASS(addr4));
}

// XXX: in IPv4 there is no link-local unicast scope, therefore
// the return value is always false.
bool
IPv4::is_linklocal_unicast() const
{
    return (false);
}

// XXX: in IPv4 there is no interface-local multicast scope, therefore
// the return value is always false.
bool
IPv4::is_interfacelocal_multicast() const
{
    return (false);
}

bool
IPv4::is_linklocal_multicast() const
{
    uint32_t addr4 = ntohl(_addr);

    return (IN_MULTICAST(addr4) && (addr4 <= INADDR_MAX_LOCAL_GROUP));
}

bool
IPv4::is_loopback() const
{
    static const uint32_t loopnet = IN_LOOPBACKNET << 24;
    uint32_t addr4 = ntohl(_addr);
    return ((addr4 & IN_CLASSA_NET) == loopnet);
}

const string&
IPv4::ip_version_str()
{
    static const string IP_VERSION_STR("IPv4");
    return IP_VERSION_STR;
}

const IPv4 IPv4Constants::zero(IPv4(static_cast<uint32_t>(0x0U)));
const IPv4 IPv4Constants::any(IPv4Constants::zero);
const IPv4 IPv4Constants::all_ones(IPv4(0xffffffffU));
const IPv4 IPv4Constants::loopback(IPv4(htonl_literal(0x7f000001U)));
const IPv4 IPv4Constants::multicast_base(IPv4(htonl_literal(0xe0000000U)));
const IPv4 IPv4Constants::multicast_all_systems(IPv4(htonl_literal(0xe0000001U)));
const IPv4 IPv4Constants::multicast_all_routers(IPv4(htonl_literal(0xe0000002U)));
const IPv4 IPv4Constants::dvmrp_routers(IPv4(htonl_literal(0xe0000004U)));
const IPv4 IPv4Constants::ospfigp_routers(IPv4(htonl_literal(0xe0000005U)));
const IPv4 IPv4Constants::ospfigp_designated_routers(IPv4(htonl_literal(0xe0000006U)));
const IPv4 IPv4Constants::rip2_routers(IPv4(htonl_literal(0xe0000009U)));
const IPv4 IPv4Constants::pim_routers(IPv4(htonl_literal(0xe000000dU)));
const IPv4 IPv4Constants::ssm_routers(IPv4(htonl_literal(0xe0000016U)));
const IPv4 IPv4Constants::class_a_base(IPv4(htonl_literal(0x00000000U)));
const IPv4 IPv4Constants::class_b_base(IPv4(htonl_literal(0x80000000U)));
const IPv4 IPv4Constants::class_c_base(IPv4(htonl_literal(0xc0000000U)));
const IPv4 IPv4Constants::experimental_base(IPv4(htonl_literal(0xf0000000U)));

#ifndef IPPROTO_OSPF
#define IPPROTO_OSPF 89
#endif
const char* ip_proto_str(uint8_t protocol) {
    switch (protocol) {
    case IPPROTO_IP: return "IP";
    case IPPROTO_ICMP: return "ICMP";
    case IPPROTO_IGMP: return "IGMP";
    case IPPROTO_PIM: return "PIM";
    case IPPROTO_OSPF: return "OSPF";
    default: return "UNKNOWN";
    };
}

