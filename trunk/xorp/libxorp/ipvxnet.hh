// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/libxorp/ipvxnet.hh,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $

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
IPNet<IPvX>::IPNet(const IPvX& ipvx, size_t preflen)
    throw (InvalidNetmaskLength)
    : _prefix_len(preflen) 
{
    if (preflen > ipvx.addr_bitlen())
	xorp_throw(InvalidNetmaskLength, preflen);
    _masked_addr = ipvx.mask_by_prefix(preflen);
}

template <> void
IPNet<IPvX>::initialize_from_string(const char *cp) throw (InvalidString) 
{
    char *slash = strrchr(cp, '/');
    if (slash == 0) xorp_throw(InvalidString, "Missing slash");

    if (*(slash + 1) == 0) xorp_throw(InvalidString, "Missing prefix length");
    _prefix_len = atoi(slash + 1);

    string addr = string(cp, slash - cp);
    
    _masked_addr = IPvX(addr.c_str()).mask_by_prefix(_prefix_len);
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
    explicit IPvXNet(int family)	
	// : _masked_addr(family), _prefix_len(0)
	// XXX: G++ 2.95 croaks on this
    {
	_masked_addr = IPvX::ZERO(family);
	_prefix_len = 0;
    }

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
    IPvXNet(const char *cp) throw (InvalidString) : BaseIPvXNet(cp) {}

    /**
     * Constructor from a given base address and a prefix length.
     *
     * @param a base address for the subnet.
     * @param preflen length of subnet mask.
     */
    IPvXNet(const IPvX& a, int preflen) 
	: BaseIPvXNet(a, preflen) {}

    // The following methods are specific to IPvXNet

    /**
     * Test if this subnet is IPv4 subnet.
     * 
     * @return true if the subnet is IPv4.
     */
    inline bool is_ipv4() const { return masked_addr().is_ipv4(); }

    /**
     * Test if this subnet is IPv6 subnet.
     * 
     * @return true if the subnet is IPv6.
     */
    inline bool is_ipv6() const { return masked_addr().is_ipv6(); }

    /**
     * Get IPv4Net subnet.
     * 
     * @return IPv4Net subnet contained with IPvXNet structure.
     */
    inline IPv4Net get_ipv4Net() const
    {
    	return IPv4Net(masked_addr().get_ipv4(), prefix_len());
    }

    /**
     * Get IPv6Net subnet.
     * 
     * @return IPv6Net subnet contained with IPvXNet structure.
     */
    inline IPv6Net get_ipv6Net() const 
    {
    	return IPv6Net(masked_addr().get_ipv6(), prefix_len());
    }

    /**
     * Assign address value to an IPv4Net subnet.
     * 
     * @param to_ipv4net IPv4Net subnet to be assigned IPv4Net value contained
     * within this subnet.
     */
    inline void get(IPv4Net& to_ipv4net) const { to_ipv4net = get_ipv4Net(); }

    /**
     * Assign address value to an IPv6Net subnet.
     * 
     * @param to_ipv6net IPv6Net subnet to be assigned IPv6Net value contained
     * within this subnet.
     */
    inline void get(IPv6Net& to_ipv6net) const { to_ipv6net = get_ipv6Net(); }

    /**
     * Get the address family.
     * 
     * @return the address family of this subnet (AF_INET or AF_INET6).
     */
    inline int af() const { return masked_addr().af(); }

    /**
     * Get the multicast base subnet.
     * 
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_len = IPvXNet::ip_multicast_base_prefix(my_family);
     * 
     * @param family the address family.
     * @return the multicast base prefix address for address
     * family of @ref family.
     */
    inline static IPvXNet ip_multicast_base_prefix(int family) 
    {
	return IPvXNet(IPvX::MULTICAST_BASE(family),
		       IPvX::ip_multicast_base_address_masklen(family));
    }

    /**
     * Test if this subnet is a valid multicast prefix address.
     * 
     * @return true if this is a valid multicast prefix address.
     */
    bool is_multicast() const {
	return (ip_multicast_base_prefix(_masked_addr.af()).contains(*this));
    }
};

#endif // __LIBXORP_IPVXNET_HH__
