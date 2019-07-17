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



#include "xorp.h"
#include "ipvx.hh"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

//
// Static class members
//

// Construct an IN_ADDR_ANY address of family AF_INET.
IPvX::IPvX()
    : _af(AF_INET)
{
    memset(_addr, 0, sizeof(_addr));
}

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
    memcpy(_addr, from_uint8, addr_bytelen());
}

IPvX::IPvX(const IPv4& ipv4)
{
    x_static_assert(sizeof(_addr) >= sizeof(IPv4));
    x_static_assert(sizeof(IPv4) == 4);

    _af = AF_INET;
    memset(_addr, 0, sizeof(_addr));
    memcpy(_addr, &ipv4, 4);
}

IPvX::IPvX(const IPv6& ipv6)
{
    x_static_assert(sizeof(_addr) >= sizeof(IPv6));
    x_static_assert(sizeof(IPv6) == 16);

    _af = AF_INET6;
    memcpy(_addr, &ipv6, 16);
}

IPvX& IPvX::operator=(const IPvX& other) {
    _af = other._af;
    memcpy(_addr, other._addr, sizeof(_addr));
    return *this;
}

IPvX::IPvX(const IPvX& other) {
    _af = other._af;
    memcpy(_addr, other._addr, sizeof(_addr));
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

IPvX::IPvX(const sockaddr_storage& from_sockaddr_storage) throw (InvalidFamily)
{
    copy_in(from_sockaddr_storage);
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
    memset(_addr, 0, sizeof(_addr));
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
    x_static_assert(sizeof(_addr) == 16);
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
    memcpy(to_uint8, _addr, addr_bytelen());
    return (addr_bytelen());
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
    struct sockaddr *sa = &to_sockaddr;

    switch (_af) {
    case AF_INET:
        return (copy_out(*sockaddr2sockaddr_in(sa)));

    case AF_INET6:
        return (copy_out(*sockaddr2sockaddr_in6(sa)));

    default:
	xorp_throw(InvalidFamily, _af);
    }

    return ((size_t)-1);
}

/**
 * Copy the raw address to @to_sockaddr_storage, and assign appropriately
 * the rest of the fields in @to_sockaddr_storage.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_out(struct sockaddr_storage& to_sockaddr_storage)
    const throw (InvalidFamily)
{
    struct sockaddr *sa = sockaddr_storage2sockaddr(&to_sockaddr_storage);

    switch (_af) {
    case AF_INET:
        return (copy_out(*sockaddr2sockaddr_in(sa)));

    case AF_INET6:
        return (copy_out(*sockaddr2sockaddr_in6(sa)));

    default:
	xorp_throw(InvalidFamily, _af);
    }

    return ((size_t)-1);
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
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
	to_sockaddr_in.sin_len = sizeof(to_sockaddr_in);
#endif
	to_sockaddr_in.sin_family = _af;
	to_sockaddr_in.sin_port = 0;			// XXX: not used
	return (copy_out(to_sockaddr_in.sin_addr));

    default:
	xorp_throw(InvalidFamily, _af);
    }

    return ((size_t)-1);
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
    case AF_INET6:
	memset(&to_sockaddr_in6, 0, sizeof(to_sockaddr_in6));
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
	to_sockaddr_in6.sin6_len = sizeof(to_sockaddr_in6);
#endif
	to_sockaddr_in6.sin6_family = _af;

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

    default:
	xorp_throw(InvalidFamily, _af);
    }

    return ((size_t)-1);
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
	memset(_addr, 0, sizeof(_addr));
	// FALLTHROUGH
    case AF_INET6:
	memcpy(_addr, from_uint8, addr_bytelen());
	return (addr_bytelen());
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

    const struct sockaddr *sa = &from_sockaddr;

    switch (sa->sa_family) {
    case AF_INET:
        return (copy_in(*sockaddr2sockaddr_in(sa)));
    case AF_INET6:
        return (copy_in(*sockaddr2sockaddr_in6(sa)));
    default:
	xorp_throw(InvalidFamily, sa->sa_family);
    }

    return ((size_t)-1);
}

/**
 * Copy a raw address from @from_sockaddr_storage.
 * @return the number of copied octets.
 */
size_t
IPvX::copy_in(const sockaddr_storage& from_sockaddr_storage)
    throw (InvalidFamily)
{
    const struct sockaddr *sa = sockaddr_storage2sockaddr(&from_sockaddr_storage);

    switch (sa->sa_family) {
    case AF_INET:
        return (copy_in(*sockaddr2sockaddr_in(sa)));
    case AF_INET6:
        return (copy_in(*sockaddr2sockaddr_in6(sa)));
    default:
	xorp_throw(InvalidFamily, sa->sa_family);
    }
    
    return ((size_t)-1);
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
    default:
	xorp_throw(InvalidFamily, _af);
    }
    return ((size_t)-1);
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
    case AF_INET6:
	return (copy_in(from_sockaddr_in6.sin6_addr));
    default:
	xorp_throw(InvalidFamily, _af);
    }

    return ((size_t)-1);
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
IPvX::is_class_a() const
{
    if (_af == AF_INET)
	return get_ipv4().is_class_a();
    return (false);		// XXX: this method applies only for IPv4
}

bool
IPvX::is_class_b() const
{
    if (_af == AF_INET)
	return get_ipv4().is_class_b();
    return (false);		// XXX: this method applies only for IPv4
}

bool
IPvX::is_class_c() const
{
    if (_af == AF_INET)
	return get_ipv4().is_class_c();
    return (false);		// XXX: this method applies only for IPv4
}

bool
IPvX::is_experimental() const
{
    if (_af == AF_INET)
	return get_ipv4().is_experimental();
    return (false);		// XXX: this method applies only for IPv4
}

bool
IPvX::is_linklocal_unicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_linklocal_unicast();
    return get_ipv6().is_linklocal_unicast();
}

bool
IPvX::is_interfacelocal_multicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_interfacelocal_multicast();
    return get_ipv6().is_interfacelocal_multicast();
}

bool
IPvX::is_linklocal_multicast() const
{
    if (_af == AF_INET)
	return get_ipv4().is_linklocal_multicast();
    return get_ipv6().is_linklocal_multicast();
}

bool
IPvX::is_loopback() const
{
    if (_af == AF_INET)
	return get_ipv4().is_loopback();
    return get_ipv6().is_loopback();
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

    return ((uint32_t)-1);
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
IPvX::addr_bytelen(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::addr_bytelen());
    if (family == AF_INET6)
	return (IPv6::addr_bytelen());
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

uint32_t
IPvX::ip_class_a_base_address_mask_len(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::ip_class_a_base_address_mask_len());
    // XXX: this method applies only for IPv4

    xorp_throw(InvalidFamily, family);
    return ((uint32_t)-1);
}

uint32_t
IPvX::ip_class_b_base_address_mask_len(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::ip_class_b_base_address_mask_len());
    // XXX: this method applies only for IPv4

    xorp_throw(InvalidFamily, family);
    return ((uint32_t)-1);
}

uint32_t
IPvX::ip_class_c_base_address_mask_len(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::ip_class_c_base_address_mask_len());
    // XXX: this method applies only for IPv4

    xorp_throw(InvalidFamily, family);
    return ((uint32_t)-1);
}

uint32_t
IPvX::ip_experimental_base_address_mask_len(int family) throw (InvalidFamily)
{
    if (family == AF_INET)
	return (IPv4::ip_experimental_base_address_mask_len());
    // XXX: this method applies only for IPv4

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

// IPvX "Constant" that applies only for IPv4
#define IPVX_CONSTANT_ACCESSOR_IPV4(name)				      \
const IPvX& IPvX::name(int family) throw (InvalidFamily)		      \
{									      \
    static const IPvX c4_##name (IPv4::name());	/* IPv4 constant */	      \
    if (family == AF_INET)						      \
	return c4_##name;						      \
    xorp_throw(InvalidFamily, family);					      \
}

// IPvX "Constant" that applies only for IPv6
#define IPVX_CONSTANT_ACCESSOR_IPV6(name)				      \
const IPvX& IPvX::name(int family) throw (InvalidFamily)		      \
{									      \
    static const IPvX c6_##name (IPv6::name());	/* IPv6 constant */	      \
    if (family == AF_INET6)						      \
	return c6_##name;						      \
    xorp_throw(InvalidFamily, family);					      \
}

IPVX_CONSTANT_ACCESSOR(ZERO);
IPVX_CONSTANT_ACCESSOR(ANY);
IPVX_CONSTANT_ACCESSOR(ALL_ONES);
IPVX_CONSTANT_ACCESSOR(LOOPBACK);
IPVX_CONSTANT_ACCESSOR(MULTICAST_BASE);
IPVX_CONSTANT_ACCESSOR(MULTICAST_ALL_SYSTEMS);
IPVX_CONSTANT_ACCESSOR(MULTICAST_ALL_ROUTERS);
IPVX_CONSTANT_ACCESSOR(DVMRP_ROUTERS);
IPVX_CONSTANT_ACCESSOR(OSPFIGP_ROUTERS);
IPVX_CONSTANT_ACCESSOR(OSPFIGP_DESIGNATED_ROUTERS);
IPVX_CONSTANT_ACCESSOR(RIP2_ROUTERS);
IPVX_CONSTANT_ACCESSOR(PIM_ROUTERS);
IPVX_CONSTANT_ACCESSOR(SSM_ROUTERS);
IPVX_CONSTANT_ACCESSOR_IPV4(CLASS_A_BASE);
IPVX_CONSTANT_ACCESSOR_IPV4(CLASS_B_BASE);
IPVX_CONSTANT_ACCESSOR_IPV4(CLASS_C_BASE);
IPVX_CONSTANT_ACCESSOR_IPV4(EXPERIMENTAL_BASE);
