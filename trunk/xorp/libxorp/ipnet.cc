// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/ipnet.cc,v 1.6 2007/02/16 22:46:19 pavlin Exp $"

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
