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

#ident "$XORP: xorp/libxorp/ipvx.cc,v 1.11 2003/11/05 20:24:28 hodson Exp $"

#include "xorp.h"
#include "ipvx.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//
// Static class members
//

// Construct an IN_ADDR_ANY address of @family.
IPvX::IPvX(int family) throw (InvalidFamily)
{
    if ((family != AF_INET) && (family != AF_INET6))
	xorp_throw(InvalidFamily, family);

    _af = family;
    memset(_addr, 0, sizeof(_addr));
}

// Construct an IPvX address by copying the raw address from @from_uint8
// memory.
IPvX::IPvX(int family, const uint8_t *from_uint8) throw (InvalidFamily)
{
    assert(from_uint8 != NULL);

    _af = family;
    memset(_addr, 0, sizeof(_addr));
    memcpy(_addr, from_uint8, addr_size());
}

IPvX::IPvX(const IPv4& ipv4)
{
    static_assert(sizeof(_addr) >= sizeof(IPv4));
    static_assert(sizeof(IPv4) == 4);

    _af = AF_INET;
    memset(_addr, 0, sizeof(_addr));
    memcpy(_addr, &ipv4, 4);
}

IPvX::IPvX(const IPv6& ipv6)
{
    static_assert(sizeof(_addr) >= sizeof(IPv6));
    static_assert(sizeof(IPv6) == 16);

    _af = AF_INET6;
    memcpy(_addr, &ipv6, 16);
}

IPvX::IPvX(const in_addr& from_in_addr)
{
    copy_in(AF_INET, reinterpret_cast<const uint8_t *>(&from_in_addr));
}

IPvX::IPvX(const in6_addr& from_in6_addr)
{
    copy_in(AF_INET6, reinterpret_cast<const uint8_t *>(&from_in6_addr));
}

IPvX::IPvX(const sockaddr& from_sockaddr) throw (InvalidFamily)
{
    copy_in(from_sockaddr);
}

IPvX::IPvX(const sockaddr_in& from_sockaddr_in) throw (InvalidFamily)
{
    copy_in(from_sockaddr_in);
}

IPvX::IPvX(const sockaddr_in6& from_sockaddr_in6) throw (InvalidFamily)
{
    copy_in(from_sockaddr_in6);
}

IPvX::IPvX(char const *from_cstring) throw (InvalidString)
{
    if (from_cstring == NULL) {
	xorp_throw(InvalidString, "Null value");
    } else if (inet_pton(AF_INET, from_cstring, _addr) > 0) {
	_af = AF_INET;
    } else if (inet_pton(AF_INET6, from_cstring, _addr) > 0) {
	_af = AF_INET6;
    } else {
	xorp_throw(InvalidString,
		   c_format("Bad IPvX \"%s\"", from_cstring));
    }
}


// TODO: if this method is used very often, then reimplement it
// by directly ~ of r._addr (see IPv6 implementation for details).
IPvX
IPvX::operator~() const
{
    if (is_ipv4()) {
	return (~get_ipv4());	// XXX: implicitly create IPvX return object
    } else {
	return (~get_ipv6());	// XXX: implicitly create IPvX return object
    }
}

IPvX
IPvX::operator|(const IPvX& other) const throw (InvalidCast)
{
    if (is_ipv4()) {
	return get_ipv4() | other.get_ipv4();
    } else {
	return get_ipv6() | other.get_ipv6();
    }
}

IPvX
IPvX::operator&(const IPvX& other) const throw (InvalidCast)
{
    if (is_ipv4()) {
	return get_ipv4() & other.get_ipv4();
    } else {
	return get_ipv6() & other.get_ipv6();
    }
}

IPvX
IPvX::operator^(const IPvX& other) const throw (InvalidCast)
{
    if (is_ipv4()) {
	return get_ipv4() ^ other.get_ipv4();
    } else {
	return get_ipv6() ^ other.get_ipv6();
    }
}

IPvX
IPvX::operator<<(uint32_t left_shift) const
{
    if (is_ipv4()) {
	return get_ipv4() << left_shift;
    } else {
	return get_ipv6() << left_shift;
    }
}

IPvX
IPvX::operator>>(uint32_t right_shift) const
{
    if (is_ipv4()) {
	return get_ipv4() >> right_shift;
    } else {
	return get_ipv6() >> right_shift;
    }
}

bool
IPvX::operator<(const IPvX& other) const
{
    static_assert(sizeof(_addr) == 16);
    int i;

    for (i = 0; i < 3; i++) {	// Loop ends intentionally at 3 not 4.
	if (_addr[i] != other._addr[i]) {
	    break;
	}
    }
    return ntohl(_addr[i]) < ntohl(other._addr[i]);
}

bool
IPvX::operator==(const IPvX& other) const
{
    if (is_ipv4() && other.is_ipv4())
	return (get_ipv4() == other.get_ipv4());
    else if (is_ipv6() && other.is_ipv6())
	return (get_ipv6() == other.get_ipv6());
    else
	return false;
}

bool
IPvX::operator!=(const IPvX& other) const
{
    if (is_ipv4() && other.is_ipv4())
	return (get_ipv4() != other.get_ipv4());
    else if (is_ipv6() && other.is_ipv6())
	return (get_ipv6() != other.get_ipv6());
    else
	return true;
}

IPvX&
IPvX::operator--()  {
    if (is_ipv4()) {
        *this = --get_ipv4();
    } else {
        *this = --get_ipv6();
    }
    return (*this);
}

IPvX&
IPvX::operator++() {
    if (is_ipv4()) {
        *this = ++get_ipv4();
    } else {
        *this = ++get_ipv6();
    }
    return (*this);
}

IPvX
IPvX::make_prefix(int family, uint32_t mask_len)
    throw (InvalidFamily, InvalidNetmaskLength)
{
    if (family == AF_INET)
	return IPv4::make_prefix(mask_len);
    else if (family == AF_INET6)
	return IPv6::make_prefix(mask_len);
    else
	xorp_throw(InvalidFamily, family);
    return IPvX(0);	/* Not Reached */
}

IPvX
IPvX::mask_by_prefix_len(uint32_t prefix_len) const throw (InvalidNetmaskLength)
{
    if (_af == AF_INET)
	return get_ipv4().mask_by_prefix_len(prefix_len);
    else
	return get_ipv6().mask_by_prefix_len(prefix_len);
}

string
IPvX::str() const
{
    if (_af == AF_INET)
	return get_ipv4().str();
    else
	return get_ipv6().str();
}

uint32_t
IPvX::mask_len() const
{
    if (is_ipv4())
	return get_ipv4().mask_len();
    else
	return get_ipv6().mask_len();
}

/**
 * Copy the raw address to memory pointed by @to.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(uint8_t *to_uint8) const
{
    memcpy(to_uint8, _addr, addr_size());
    return (addr_size());
}

/**
 * Copy the raw address to @in_addr.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(struct in_addr& to_in_addr) const throw (InvalidFamily)
{
    if (_af != AF_INET)
	xorp_throw(InvalidFamily, _af);
    return (copy_out((uint8_t *)&to_in_addr));
}

/**
 * Copy the raw address to @in6_addr.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(struct in6_addr& to_in6_addr) const throw (InvalidFamily)
{
    if (_af != AF_INET6)
	xorp_throw(InvalidFamily, _af);
    return (copy_out((uint8_t *)&to_in6_addr));
}

/**
 * Copy the raw address to @to_sockaddr, and assign appropriately
 * the rest of the fields in @to_sockaddr.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(struct sockaddr& to_sockaddr) const throw (InvalidFamily)
{
    return (copy_out(reinterpret_cast<sockaddr_in&>(to_sockaddr)));
}

/**
 * Copy the raw address to @to_sockaddr_in, and assign appropriately
 * the rest of the fields in @to_sockaddr_in.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(struct sockaddr_in& to_sockaddr_in) const throw (InvalidFamily)
{
    switch (_af) {
    case AF_INET:
	memset(&to_sockaddr_in, 0, sizeof(to_sockaddr_in));
#ifdef HAVE_SIN_LEN
	to_sockaddr_in.sin_len = sizeof(to_sockaddr_in);
#endif
	to_sockaddr_in.sin_family = _af;
	to_sockaddr_in.sin_port = 0;			// XXX: not used
	return (copy_out(to_sockaddr_in.sin_addr));

    case AF_INET6:
	return (copy_out(reinterpret_cast<sockaddr_in6&>(to_sockaddr_in)));

    default:
	xorp_throw(InvalidFamily, _af);
    }

    return((size_t)-1);
}

/**
 * Copy the raw address to @to_sockaddr_in6, and assign appropriately
 * the rest of the fields in @to_sockaddr_in6.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(struct sockaddr_in6& to_sockaddr_in6) const
    throw (InvalidFamily)
{
    switch (_af) {
    case AF_INET:
	return (copy_out(reinterpret_cast<sockaddr_in&>(to_sockaddr_in6)));

    case AF_INET6:
	memset(&to_sockaddr_in6, 0, sizeof(to_sockaddr_in6));
#ifdef HAVE_SIN6_LEN
	to_sockaddr_in6.sin6_len = sizeof(to_sockaddr_in6);
#endif
	to_sockaddr_in6.sin6_family = _af;
	return (copy_out(to_sockaddr_in6.sin6_addr));
    default:
	xorp_throw(InvalidFamily, _af);
    }

    return((size_t)-1);
}

/**
 * Copy a raw address of family @family from the memory pointed by @from_uint8.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(int family, const uint8_t *from_uint8) throw (InvalidFamily)
{
    _af = family;

    switch (_af) {
    case AF_INET:
	// FALLTHROUGH
    case AF_INET6:
	memcpy(_addr, from_uint8, addr_size());
	return (addr_size());
    default:
	xorp_throw(InvalidFamily, _af);
    }
    return ((size_t)-1);
}

/**
 * Copy a raw address of family %AF_INET from @from_in_addr.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(const in_addr& from_in_addr)
{
    return (copy_in(AF_INET, reinterpret_cast<const uint8_t *>(&from_in_addr)));
}

/**
 * Copy a raw address of family %AF_INET6 from @from_in6_addr.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(const in6_addr& from_in6_addr)
{
    return (copy_in(AF_INET6, reinterpret_cast<const uint8_t *>(&from_in6_addr)));
}

/**
 * Copy a raw address from @from_sockaddr.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(const sockaddr& from_sockaddr) throw (InvalidFamily)
{
    return (copy_in(reinterpret_cast<const sockaddr_in&>(from_sockaddr)));
}

/**
 * Copy a raw address from @from_sockaddr_in.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(const sockaddr_in& from_sockaddr_in) throw (InvalidFamily)
{
    _af = from_sockaddr_in.sin_family;

    switch (_af) {
    case AF_INET:
	return (copy_in(from_sockaddr_in.sin_addr));
    case AF_INET6:
	return (copy_in(reinterpret_cast<const sockaddr_in6&>(from_sockaddr_in)));
    default:
	xorp_throw(InvalidFamily, _af);
    }
    return((size_t)-1);
}

/**
 * Copy a raw address from @from_sockaddr_in6.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(const sockaddr_in6& from_sockaddr_in6) throw (InvalidFamily)
{
    _af = from_sockaddr_in6.sin6_family;

    switch (_af) {
    case AF_INET:
	return (copy_in(reinterpret_cast<const sockaddr_in&>(from_sockaddr_in6)));
    case AF_INET6:
	return (copy_in(from_sockaddr_in6.sin6_addr));
    default:
	xorp_throw(InvalidFamily, _af);
    }

    return((size_t)-1);
}

bool
IPvX::is_zero() const
{
    if (_af == AF_INET)
	return get_ipv4().is_zero();
    return get_ipv6().is_zero();
}

bool
IPvX::is_unicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_unicast();
    return get_ipv6().is_unicast();
}

bool
IPvX::is_multicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_multicast();
    return get_ipv6().is_multicast();
}

bool
IPvX::is_linklocal_unicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_linklocal_unicast();
    return get_ipv6().is_linklocal_unicast();
}

bool
IPvX::is_nodelocal_multicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_nodelocal_multicast();
    return get_ipv6().is_nodelocal_multicast();
}

bool
IPvX::is_linklocal_multicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_linklocal_multicast();
    return get_ipv6().is_linklocal_multicast();
}

/**
 * @return IP protocol version.
 */
uint32_t
IPvX::ip_version() const throw (InvalidFamily)
{
    if (_af == AF_INET)
	return (IPv4::ip_version());
    if (_af == AF_INET6)
	return (IPv6::ip_version());
    xorp_throw(InvalidFamily, _af);

    return ((size_t)-1);
}

/**
 * @return IP protocol version string.
 */
const string&
IPvX::ip_version_str() const throw (InvalidFamily)
{
    if (_af == AF_INET)
	return (IPv4::ip_version_str());
    if (_af != AF_INET6)
	xorp_throw(InvalidFamily, _af);
    return (IPv6::ip_version_str());
}

size_t
IPvX::addr_size(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::addr_size());
    if (family == AF_INET6)
	return (IPv6::addr_size());
    xorp_throw(InvalidFamily, family);

    return ((size_t)-1);
}

uint32_t
IPvX::ip_multicast_base_address_mask_len(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::ip_multicast_base_address_mask_len());
    if (family == AF_INET6)
	return (IPv6::ip_multicast_base_address_mask_len());

    xorp_throw(InvalidFamily, family);
    return ((uint32_t)-1);
}

//
// IPvX "Constants"
//
#define IPVX_CONSTANT_ACCESSOR(name)					      \
const IPvX& IPvX::name(int family) throw (InvalidFamily)		      \
{									      \
    static const IPvX c4_##name (IPv4::name());	/* IPv4 constant */	      \
    static const IPvX c6_##name (IPv6::name());	/* IPv6 constant */	      \
    if (family == AF_INET)						      \
	return c4_##name;						      \
    if (family == AF_INET6)						      \
	return c6_##name;						      \
    xorp_throw(InvalidFamily, family);					      \
}

IPVX_CONSTANT_ACCESSOR(ZERO);
IPVX_CONSTANT_ACCESSOR(ANY);
IPVX_CONSTANT_ACCESSOR(ALL_ONES);
IPVX_CONSTANT_ACCESSOR(MULTICAST_BASE);
IPVX_CONSTANT_ACCESSOR(MULTICAST_ALL_SYSTEMS);
IPVX_CONSTANT_ACCESSOR(MULTICAST_ALL_ROUTERS);
IPVX_CONSTANT_ACCESSOR(DVMRP_ROUTERS);
IPVX_CONSTANT_ACCESSOR(RIP2_ROUTERS);
IPVX_CONSTANT_ACCESSOR(PIM_ROUTERS);
IPVX_CONSTANT_ACCESSOR(OSPFIGP_ROUTERS);
IPVX_CONSTANT_ACCESSOR(OSPFIGP_DESIGNATED_ROUTERS);
