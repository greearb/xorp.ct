// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/ipv4.cc,v 1.14 2004/06/10 22:41:16 hodson Exp $"

#include "xorp.h"
#include "ipv4.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    const sockaddr_in& sin = reinterpret_cast<const sockaddr_in&>(sa);
    _addr = sin.sin_addr.s_addr;
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
    memcpy(to_uint8, &_addr, addr_size());
    return addr_size();
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
    return (copy_out(reinterpret_cast<sockaddr_in&>(to_sockaddr)));
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
#ifdef HAVE_SIN_LEN
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
    memcpy(&_addr, from_uint8, addr_size());
    return (addr_size());
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
    return (copy_in(reinterpret_cast<const sockaddr_in&>(from_sockaddr)));
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

bool
IPv4::operator<(const IPv4& other) const
{
    return ntohl(_addr) < ntohl(other._addr);
}

IPv4
IPv4::make_prefix(uint32_t mask_len) throw (InvalidNetmaskLength)
{
    if (mask_len > 32)
	xorp_throw(InvalidNetmaskLength, mask_len);
    uint32_t m = (mask_len == 0) ? 0 : ((~0) << (32 - mask_len));
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

IPv4&
IPv4::operator--()
{
    uint32_t tmp_addr = ntohl(_addr) - 1;
    _addr = htonl(tmp_addr);
    return *this;
}

IPv4&
IPv4::operator++()
{
    uint32_t tmp_addr = ntohl(_addr) + 1;
    _addr = htonl(tmp_addr);
    return *this;
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
	       || (addr4 & 0xff000000U) == 0));
}

bool
IPv4::is_multicast() const
{
    uint32_t addr4 = ntohl(_addr);

    return (IN_MULTICAST(addr4));
}

// XXX: in IPv4 there is no link-local unicast scope, therefore
// the return value is always false.
bool
IPv4::is_linklocal_unicast() const
{
    return (false);
}

// XXX: in IPv4 there is no node-local multicast scope, therefore
// the return value is always false.
bool
IPv4::is_nodelocal_multicast() const
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

const IPv4 IPv4Constants::zero(IPv4(0x0U));
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
