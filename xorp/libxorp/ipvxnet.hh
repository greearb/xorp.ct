// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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


#ifndef __LIBXORP_IPVXNET_HH__
#define __LIBXORP_IPVXNET_HH__

#include "xorp.h"
#include "ipnet.hh"
#include "ipvx.hh"
#include "ipv4net.hh"
#include "ipv6net.hh"

/**
 * Base IPvXNet just has @ref IPNet methods.  IPvXNet is derived
 * from BaseIPvXNet and has IPvX specific functions such as whether
 * contained type is an IPv4 network or an IPv6 network.
 *
 * See @ref IPNet for available methods.
 */
typedef IPNet<IPvX> BaseIPvXNet;

template<>
inline
IPNet<IPvX>::IPNet(const IPvX& ipvx, uint8_t prefix_len)
    throw (InvalidNetmaskLength)
    : _prefix_len(prefix_len)
{
    if (prefix_len > ipvx.addr_bitlen())
	xorp_throw(InvalidNetmaskLength, prefix_len);
    _masked_addr = ipvx.mask_by_prefix_len(prefix_len);
}

template <>
inline void
IPNet<IPvX>::initialize_from_string(const char *cp)
    throw (InvalidString, InvalidNetmaskLength)
{
    char *slash = strrchr(const_cast<char*>(cp), '/');
    if (slash == 0) xorp_throw(InvalidString, "Missing slash");

    if (*(slash + 1) == 0)
	xorp_throw(InvalidString, "Missing prefix length");
    _prefix_len = atoi(slash + 1);

    string addr = string(cp, slash - cp);

    _masked_addr = IPvX(addr.c_str()).mask_by_prefix_len(_prefix_len);
}

/**
 * IPvXNet class.  Container for IPv4 and IPv6 networks.
 *
 * Also see @ref IPNet for available methods.
 */
class IPvXNet : public BaseIPvXNet {
public:
    /**
     * Constructor for a specified address family.
     *
     * Creates a network address of specified family, and address value of
     * INADDR_ANY or IN6ADDR_ANY (for IPv4 and IPv6 respectively).
     *
     * @param family the address family.
     */
    explicit IPvXNet(int family) throw (InvalidFamily)
	: BaseIPvXNet(IPvX::ZERO(family), 0) {}
#ifdef XORP_USE_USTL
    IPvXNet() : BaseIPvXNet(IPvX::ZERO(AF_INET), 0) {}
#endif
    /**
     * Copy constructor for BaseIPvXNet subnet address
     *
     * @param n the subnet to copy from.
     */
    IPvXNet(const BaseIPvXNet& n) : BaseIPvXNet(n) {}

    /**
     * Copy constructor for IPvXNet subnet address
     *
     * @param n the subnet to copy from.
     */
    IPvXNet(const IPvXNet& n) : BaseIPvXNet(n) {}

    /**
     * Assignment Operator
     */
    IPvXNet& operator=(const IPvXNet& other) {
	*((BaseIPvXNet*)(this)) = (BaseIPvXNet)(other);
	return *this;
    }

    /**
     * Copy constructor for IPv4Net subnet address
     *
     * @param v4net the subnet to copy from.
     */
    IPvXNet(const IPv4Net& v4net)
	: BaseIPvXNet(v4net.masked_addr(), v4net.prefix_len()) {}

    /**
     * Copy constructor for IPv6Net subnet address
     *
     * @param v6net the subnet to copy from.
     */
    IPvXNet(const IPv6Net& v6net)
	: BaseIPvXNet(v6net.masked_addr(), v6net.prefix_len()) {}

    /**
     * Constructor from a string.
     *
     * @param from_cstring C-style string with slash separated address
     * and prefix length.
     * Examples: "12.34.56/24", "1234:5678/32::"
     */
    IPvXNet(const char *cp) throw (InvalidString, InvalidNetmaskLength)
	: BaseIPvXNet(cp) {}

    /**
     * Constructor from a given base address and a prefix length.
     *
     * @param a base address for the subnet.
     * @param prefix_len length of subnet mask.
     */
    IPvXNet(const IPvX& a, uint8_t prefix_len) throw (InvalidNetmaskLength)
	: BaseIPvXNet(a, prefix_len) {}

    // The following methods are specific to IPvXNet

    /**
     * Test if this subnet is IPv4 subnet.
     *
     * @return true if the subnet is IPv4.
     */
    bool is_ipv4() const { return masked_addr().is_ipv4(); }

    /**
     * Test if this subnet is IPv6 subnet.
     *
     * @return true if the subnet is IPv6.
     */
    bool is_ipv6() const { return masked_addr().is_ipv6(); }

    /**
     * Get IPv4Net subnet.
     *
     * @return IPv4Net subnet contained with IPvXNet structure.
     */
    IPv4Net get_ipv4net() const 	throw (InvalidCast) {
    	return IPv4Net(masked_addr().get_ipv4(), prefix_len());
    }

    /**
     * Get IPv6Net subnet.
     *
     * @return IPv6Net subnet contained with IPvXNet structure.
     */
    IPv6Net get_ipv6net() const 	throw (InvalidCast) {
    	return IPv6Net(masked_addr().get_ipv6(), prefix_len());
    }

    /**
     * Assign address value to an IPv4Net subnet.
     *
     * @param to_ipv4net IPv4Net subnet to be assigned IPv4Net value contained
     * within this subnet.
     */
    void get(IPv4Net& to_ipv4net) const throw (InvalidCast) {
	to_ipv4net = get_ipv4net();
    }

    /**
     * Assign address value to an IPv6Net subnet.
     *
     * @param to_ipv6net IPv6Net subnet to be assigned IPv6Net value contained
     * within this subnet.
     */
    void get(IPv6Net& to_ipv6net) const throw (InvalidCast) {
	to_ipv6net = get_ipv6net();
    }

    /**
     * Get the address family.
     *
     * @return the address family of this subnet (AF_INET or AF_INET6).
     */
    int af() const { return masked_addr().af(); }

    /**
     * Test if this subnet is a unicast prefix.
     *
     * In case of IPv4 all prefixes that fall within the Class A, Class B or
     * Class C address space are unicast.
     * In case of IPv6 all prefixes that don't contain the multicast
     * address space are unicast.
     * Note that the default route (0.0.0.0/0 for IPv4 or ::/0 for IPv6)
     * is also considered an unicast prefix.
     *
     * @return true if this subnet is a unicast prefix.
     */
    bool is_unicast() const {
	if (is_ipv4()) {
	    return (get_ipv4net().is_unicast());
	} else {
	    return (get_ipv6net().is_unicast());
	}
    }

    /**
     * Return the subnet containing all multicast addresses.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPvXNet my_prefix = IPvXNet::ip_multicast_base_prefix(my_family);
     *
     * @param family the address family.
     * @return the multicast base prefix address for address
     * family of @ref family.
     */
    static IPvXNet ip_multicast_base_prefix(int family) throw (InvalidFamily) {
	return IPvXNet(IPvX::MULTICAST_BASE(family),
		       IPvX::ip_multicast_base_address_mask_len(family));
    }

    /**
     * Return the subnet containing all IPv4 Class A addresses
     * (0.0.0.0/1).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPvXNet my_prefix = IPvXNet::ip_class_a_base_prefix(my_family);
     *
     * @param family the address family.
     * @return the Class A base prefix address for address
     * family of @ref family.
     */
    static IPvXNet ip_class_a_base_prefix(int family) throw (InvalidFamily) {
	return IPvXNet(IPvX::CLASS_A_BASE(family),
		       IPvX::ip_class_a_base_address_mask_len(family));
    }

    /**
     * Return the subnet containing all IPv4 Class B addresses
     * (128.0.0.0/2).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPvXNet my_prefix = IPvXNet::ip_class_b_base_prefix(my_family);
     *
     * @param family the address family.
     * @return the Class B base prefix address for address
     * family of @ref family.
     */
    static IPvXNet ip_class_b_base_prefix(int family) throw (InvalidFamily) {
	return IPvXNet(IPvX::CLASS_B_BASE(family),
		       IPvX::ip_class_b_base_address_mask_len(family));
    }

    /**
     * Return the subnet containing all IPv4 Class C addresses
     * (192.0.0.0/3).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPvXNet my_prefix = IPvXNet::ip_class_c_base_prefix(my_family);
     *
     * @param family the address family.
     * @return the Class C base prefix address for address
     * family of @ref family.
     */
    static IPvXNet ip_class_c_base_prefix(int family) throw (InvalidFamily) {
	return IPvXNet(IPvX::CLASS_C_BASE(family),
		       IPvX::ip_class_c_base_address_mask_len(family));
    }

    /**
     * Return the subnet containing all IPv4 experimental Class E addresses
     * (240.0.0.0/4).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPvXNet my_prefix = IPvXNet::ip_experimental_base_prefix(my_family);
     *
     * @param family the address family.
     * @return the experimental base prefix address for address
     * family of @ref family.
     */
    static IPvXNet ip_experimental_base_prefix(int family)
	throw (InvalidFamily) {
	return IPvXNet(IPvX::EXPERIMENTAL_BASE(family),
		       IPvX::ip_experimental_base_address_mask_len(family));
    }

    /**
     * Test if this subnet is within the multicast address range.
     *
     * @return true if this subnet is within the multicast address range.
     */
    bool is_multicast() const {
	return (ip_multicast_base_prefix(masked_addr().af()).contains(*this));
    }

    /**
     * Test if this subnet is within the IPv4 Class A
     * address range (0.0.0.0/1).
     *
     * This method applies only for IPv4, and always returns false for IPv6.
     *
     * @return true if this subnet is within the IPv4 Class A address
     * range.
     */
    bool is_class_a() const {
	if (is_ipv4()) {
	    return (ip_class_a_base_prefix(masked_addr().af()).contains(*this));
	}
	return (false);
    }

    /**
     * Test if this subnet is within the IPv4 Class B
     * address range (128.0.0.0/2).
     *
     * This method applies only for IPv4, and always returns false for IPv6.
     *
     * @return true if this subnet is within the IPv4 Class B address
     * range.
     */
    bool is_class_b() const {
	if (is_ipv4()) {
	    return (ip_class_b_base_prefix(masked_addr().af()).contains(*this));
	}
	return (false);
    }

    /**
     * Test if this subnet is within the IPv4 Class C
     * address range (192.0.0.0/3).
     *
     * This method applies only for IPv4, and always returns false for IPv6.
     *
     * @return true if this subnet is within the IPv4 Class C address
     * range.
     */
    bool is_class_c() const {
	if (is_ipv4()) {
	    return (ip_class_c_base_prefix(masked_addr().af()).contains(*this));
	}
	return (false);
    }

    /**
     * Test if this subnet is within the IPv4 experimental Class E
     * address range (240.0.0.0/4).
     *
     * This method applies only for IPv4, and always returns false for IPv6.
     *
     * @return true if this subnet is within the IPv4 experimental address
     * range.
     */
    bool is_experimental() const {
	if (is_ipv4()) {
	    return (ip_experimental_base_prefix(masked_addr().af()).contains(*this));
	}
	return (false);
    }
};

#endif // __LIBXORP_IPVXNET_HH__
