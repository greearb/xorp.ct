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

#ident "$XORP: xorp/libxorp/ipnet.cc,v 1.9 2008/10/02 21:57:31 bms Exp $"

#include "xorp.h"
#include "ipnet.hh"

//
// Storage for defining specialized class methods for the IPNet<A> template
// class.
//

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
const IPNet<IPv4>
IPNet<IPv4>::ip_class_a_base_prefix()
{
    return IPNet(IPv4::CLASS_A_BASE(),
		 IPv4::ip_class_a_base_address_mask_len());
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
const IPNet<IPv4>
IPNet<IPv4>::ip_class_b_base_prefix()
{
    return IPNet(IPv4::CLASS_B_BASE(),
		 IPv4::ip_class_b_base_address_mask_len());
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
const IPNet<IPv4>
IPNet<IPv4>::ip_class_c_base_prefix()
{
    return IPNet(IPv4::CLASS_C_BASE(),
		 IPv4::ip_class_c_base_address_mask_len());
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
const IPNet<IPv4>
IPNet<IPv4>::ip_experimental_base_prefix()
{
    return IPNet(IPv4::EXPERIMENTAL_BASE(),
		 IPv4::ip_experimental_base_address_mask_len());
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
bool
IPNet<IPv4>::is_class_a() const
{
    return (ip_class_a_base_prefix().contains(*this));
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
bool
IPNet<IPv4>::is_class_b() const
{
    return (ip_class_b_base_prefix().contains(*this));
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
bool
IPNet<IPv4>::is_class_c() const
{
    return (ip_class_c_base_prefix().contains(*this));
}

//
// XXX: This method applies only for IPv4, hence we don't provide IPv6
// specialized version.
//
template <>
bool
IPNet<IPv4>::is_experimental() const
{
    return (ip_experimental_base_prefix().contains(*this));
}

template <>
bool
IPNet<IPv4>::is_unicast() const
{
    //
    // In case of IPv4 all prefixes that fall within the Class A, Class B or
    // Class C address space are unicast.
    // Note that the default route (0.0.0.0/0 for IPv4 or ::/0 for IPv6)
    // is also considered an unicast prefix.
    //
    if (prefix_len() == 0) {
	// The default route or a valid unicast route
	return (true);
    }

    if (ip_class_a_base_prefix().contains(*this)
	|| ip_class_b_base_prefix().contains(*this)
	|| ip_class_c_base_prefix().contains(*this)) {
	return (true);
    }

    return (false);
}

template <>
bool
IPNet<IPv6>::is_unicast() const
{
    //
    // In case of IPv6 all prefixes that don't contain the multicast
    // address space are unicast.
    // Note that the default route (0.0.0.0/0 for IPv4 or ::/0 for IPv6)
    // is also considered an unicast prefix.
    //
    if (prefix_len() == 0) {
	// The default route or a valid unicast route
	return (true);
    }

    IPNet<IPv6> base_prefix = ip_multicast_base_prefix();
    if (this->contains(base_prefix))
	return (false);
    if (base_prefix.contains(*this))
	return (false);

    return (true);
}
