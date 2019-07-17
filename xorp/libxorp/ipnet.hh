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


#ifndef __LIBXORP_IPNET_HH__
#define __LIBXORP_IPNET_HH__

#include "xorp.h"
#include "exceptions.hh"
#include "c_format.hh"
#include "range.hh"
#include "utils.hh"
#include "ipv4.hh"
#include "ipv6.hh"

/**
 * @short A template class for subnets
 *
 * A "subnet" is specified by a base "address" and a "prefix length".
 */
template <class A>
class IPNet {
public:
    /**
     * Default constructor taking no parameters.
     *
     * Default value has INADDR_ANY/0.
     */
    IPNet() : _prefix_len(0) {}

    /**
     * Constructor from a given base address and a prefix length.
     *
     * @param a base address for the subnet.
     * @param prefix_len length of subnet mask (e.g., class C nets would have
     * prefix_len=24).
     */
    IPNet(const A& a, uint8_t prefix_len) throw (InvalidNetmaskLength)
	: _masked_addr(a), _prefix_len(prefix_len)
    {
	if (prefix_len > A::addr_bitlen())
	    xorp_throw(InvalidNetmaskLength, prefix_len);
	_masked_addr = a.mask_by_prefix_len(prefix_len);
    }

    /**
     * Constructor from a string.
     *
     * @param from_cstring C-style string with slash separated address
     * and prefix length.
     */
    IPNet(const char *from_cstring)
	throw (InvalidString, InvalidNetmaskLength)
    {
	initialize_from_string(from_cstring);
    }

    /**
     * Copy constructor
     *
     * @param n the subnet to copy from.
     */
    IPNet(const IPNet& n) {
	_masked_addr	= n.masked_addr();
	_prefix_len	= n.prefix_len();
    }

    /**
     * Assignment operator
     *
     * @param n the subnet to assign from.
     * @return the subnet after the assignment.
     */
    IPNet& operator=(const IPNet& n) {
	_masked_addr	= n.masked_addr();
	_prefix_len	= n.prefix_len();
	return *this;
    }

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const IPNet& other) const {
	return ((_prefix_len == other._prefix_len) &&
		(_masked_addr == other._masked_addr));
    }

    /**
     * Less-than comparison for subnets (see body for description).
     *
     * @param other the right-hand side of the comparison.
     * @return true if the left-hand side is "smaller" than the right-hand
     * side according to the chosen order.
     */
    bool operator<(const IPNet& other) const;

    /**
     * Decrement Operator
     *
     * The numerical value of the prefix address is decrement by one.
     * Example: decrementing 128.2.0.0/16 results in 128.1.0.0/16.
     *
     * @return a reference to this subnet after the decrement
     */
    IPNet& operator--();

    /**
     * Increment Operator
     *
     * The numerical value of the prefix address is incremented by one.
     * Example: incrementing 128.2.0.0/16 results in 128.3.0.0/16.
     *
     * @return a reference to this subnet after the increment
     */
    IPNet& operator++();

    /**
     * Equality Operator for @ref U32Range operand.
     *
     * @param range the right-hand operand to compare against.
     * @return true if the prefix length falls inside the range defined
     * by the right-hand operand.
     */
    bool operator==(const U32Range& range) const {
	return (prefix_len() == range);
    }

    /**
     * Non-equality Operator for @ref U32Range operand.
     *
     * @param range the right-hand operand to compare against.
     * @return true if the prefix length falls outside the range defined
     * by the right-hand operand.
     */
    bool operator!=(const U32Range& range) const {
	return (prefix_len() != range);
    }

    /**
     * Less-than comparison for prefix lengths for @ref U32Range operand.
     *
     * @param range the right-hand side of the comparison.
     * @return true if the prefix length is bellow the range defined
     * by the right-hand operand.
     */
    bool operator<(const U32Range& range) const {
	return (prefix_len() < range);
    };

    /**
     * Less-than or equal comparison for prefix lengths for @ref U32Range
     * operand.
     *
     * @param range the right-hand side of the comparison.
     * @return true if the prefix length is bellow or within the range
     * defined by the right-hand operand.
     */
    bool operator<=(const U32Range& range) const {
	return (prefix_len() <= range);
    };

    /**
     * Greater-than comparison for prefix lengths for @ref U32Range operand.
     *
     * @param range the right-hand side of the comparison.
     * @return true if the prefix length is above the range defined
     * by the right-hand operand.
     */
    bool operator>(const U32Range& range) const {
	return (prefix_len() > range);
    };

    /**
     * Greater-than or equal comparison for prefix lengths for @ref U32Range
     * operand.
     *
     * @param range the right-hand side of the comparison.
     * @return true if the prefix length is above or within the range
     * defined by the right-hand operand.
     */
    bool operator>=(const U32Range& range) const {
	return (prefix_len() >= range);
    };

    /**
     * Convert this address from binary form to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the address.
     */
    string str() const;

    /**
     * Test if the object contains a real (non-default) value.
     *
     * @return true if the object stores a real (non-default) value.
     */
     bool is_valid() const { return _prefix_len != 0; }

    /**
     * Test if subnets overlap.
     *
     * @param other the subnet to compare against.
     * @return true if there is some overlap between the two subnets.
     */
    bool is_overlap(const IPNet& other) const;

    /**
     * Test if a subnet contains (or is equal to) another subnet.
     *
     * in LaTeX, x.contains(y) would be   $x \superseteq y$
     *
     * @param other the subnet to test against.
     * @return true if this subnet contains or is equal to @ref other.
     */
    bool contains(const IPNet& other) const;

    /**
     * Test if an address is within a subnet.
     *
     * @param addr the address to test against.
     * @return true if @ref addr is within this subnet.
     */
    bool contains(const A& addr) const {
	return addr.mask_by_prefix_len(_prefix_len) == _masked_addr;
    }

    /**
     * Determine the number of the most significant bits overlapping with
     * another subnet.
     *
     * @param other the subnet to test against.
     * @return the number of bits overlapping between @ref other and
     * this subnet.
     */
    uint32_t overlap(const IPNet& other) const;

    /**
     * Get the address family.
     *
     * @return the address family of this address.
     */
    static int af() { return A::af(); }

    /**
     * Get the base address.
     *
     * @return the base address for this subnet.
     */
    const A& masked_addr() const { return _masked_addr; }

    A& masked_addr_nc() { return _masked_addr; }

    /**
     * Get the prefix length.
     *
     * @return the prefix length for this subnet.
     */
    uint8_t prefix_len() const { return _prefix_len; }

    void set_prefix_len(uint8_t v) { _prefix_len = v; }

    /**
     * Get the network mask.
     *
     * @return the netmask associated with this subnet.
     */
    A netmask() const { return _masked_addr.make_prefix(_prefix_len); }

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
    bool is_unicast() const;

    /**
     * Return the subnet containing all multicast addresses.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPv4Net my_prefix = IPv4Net::ip_multicast_base_prefix();
     *   IPv4Net my_prefix = ipv4net.ip_multicast_base_prefix();
     *
     * @return the subnet containing multicast addresses.
     */
    static const IPNet<A> ip_multicast_base_prefix() {
	return IPNet(A::MULTICAST_BASE(),
		     A::ip_multicast_base_address_mask_len());
    }

    /**
     * Return the subnet containing all IPv4 Class A addresses
     * (0.0.0.0/1).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPv4Net my_prefix = IPv4Net::ip_class_a_base_prefix();
     *   IPv4Net my_prefix = ipv4net.ip_class_a_base_prefix();
     *
     * @return the subnet containing Class A addresses.
     */
    static const IPNet<A> ip_class_a_base_prefix();

    /**
     * Return the subnet containing all IPv4 Class B addresses
     * (128.0.0.0/2).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPv4Net my_prefix = IPv4Net::ip_class_b_base_prefix();
     *   IPv4Net my_prefix = ipv4net.ip_class_b_base_prefix();
     *
     * @return the subnet containing Class B addresses.
     */
    static const IPNet<A> ip_class_b_base_prefix();

    /**
     * Return the subnet containing all IPv4 Class C addresses
     * (192.0.0.0/3).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPv4Net my_prefix = IPv4Net::ip_class_c_base_prefix();
     *   IPv4Net my_prefix = ipv4net.ip_class_c_base_prefix();
     *
     * @return the subnet containing Class C addresses.
     */
    static const IPNet<A> ip_class_c_base_prefix();

    /**
     * Return the subnet containing all IPv4 experimental Class E addresses
     * (240.0.0.0/4).
     *
     * This method applies only for IPv4.
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPv4Net my_prefix = IPv4Net::ip_experimental_base_prefix();
     *   IPv4Net my_prefix = ipv4net.ip_experimental_base_prefix();
     *
     * @return the subnet containing experimental addresses.
     */
    static const IPNet<A> ip_experimental_base_prefix();

    /**
     * Test if this subnet is within the multicast address range.
     *
     * @return true if this subnet is within the multicast address range.
     */
    bool is_multicast() const {
	return (ip_multicast_base_prefix().contains(*this));
    }

    /**
     * Test if this subnet is within the IPv4 Class A
     * address range (0.0.0.0/1).
     *
     * This method applies only for IPv4.
     *
     * @return true if this subnet is within the IPv4 Class A address
     * range.
     */
    bool is_class_a() const;

    /**
     * Test if this subnet is within the IPv4 Class B
     * address range (128.0.0.0/2).
     *
     * This method applies only for IPv4.
     *
     * @return true if this subnet is within the IPv4 Class B address
     * range.
     */
    bool is_class_b() const;

    /**
     * Test if this subnet is within the IPv4 Class C
     * address range (192.0.0.0/3).
     *
     * This method applies only for IPv4.
     *
     * @return true if this subnet is within the IPv4 Class C address
     * range.
     */
    bool is_class_c() const;

    /**
     * Test if this subnet is within the IPv4 experimental Class E
     * address range (240.0.0.0/4).
     *
     * This method applies only for IPv4.
     *
     * @return true if this subnet is within the IPv4 experimental address
     * range.
     */
    bool is_experimental() const;

    /**
     * Get the highest address within this subnet.
     *
     * @return the highest address within this subnet.
     */
    A top_addr() const { return _masked_addr | ~netmask(); }

    /**
     * Get the smallest subnet containing both subnets.
     *
     * @return the smallest subnet containing both subnets passed
     * as arguments.
     */
    static IPNet<A> common_subnet(const IPNet<A> x, const IPNet<A> y) {
	return IPNet<A>(x.masked_addr(), x.overlap(y));
    }

private:
    void initialize_from_string(const char *s)
	throw (InvalidString, InvalidNetmaskLength);

    A		_masked_addr;
    uint8_t	_prefix_len;
};

/* ------------------------------------------------------------------------- */
/* Deferred method definitions */

template <class A>
inline bool
IPNet<A>::operator<(const IPNet& other) const
{
#if 1
    /*
     * say x = A/B and y = C/D, then
     *
     *	x < y :
     *
     *		if x.contains(y) // equal is fine
     *			return false
     *		else if y.strictly_contains(x) // equal already taken care of
     *			return true
     *		else
     *			return A < C
     *
     *	 	         |---x---|
     *  x=y	         |---y---|
     *  x>y	           |-y-|
     *	x<y	  |------y-------|
     *	x<y	         |-----y---------|
     *  x<y	                    |--y-|
     */
    if (masked_addr().af() != other.masked_addr().af())
	return (masked_addr().af() < other.masked_addr().af());

    if (this->contains(other))
	return false;
    else if (other.contains(*this))
	return true;
    else
	return this->masked_addr() < other.masked_addr();

#else	// old code
    const A& maddr_him = other.masked_addr();
    uint8_t his_prefix_len = other.prefix_len();

    //the ordering is important because we want the longest match to
    //be first.  For example, we want the following:
    //  128.16.0.0/24 < 128.16.64.0/24 <  128.16.0.0/16 < 128.17.0.0/24

    if (_prefix_len == his_prefix_len) return _masked_addr < maddr_him;

    // we need to check the case when one subnet is a subset of
    // the other
    if (_prefix_len < his_prefix_len) {
	A test_addr(maddr_him.mask_by_prefix_len(_prefix_len));
	if (_masked_addr == test_addr) {
	    //his subnet is a subset of mine, so he goes first.
	    return (false);
	}
    } else if (_prefix_len > his_prefix_len) {
	A test_addr(_masked_addr.mask_by_prefix_len(his_prefix_len));
	if (maddr_him == test_addr) {
	    //my subnet is a subset of his, so I go first.
	    return (true);
	}
    }
    //the subnets don't overlap (or are identical), so just order by address
    if (_masked_addr < maddr_him) {
	return (true);
    }
    return (false);
#endif
}

template <class A> string
IPNet<A>::str() const
{
    return _masked_addr.str() + c_format("/%u", XORP_UINT_CAST(_prefix_len));
}

template <class A> bool
IPNet<A>::is_overlap(const IPNet<A>& other) const
{
    if (masked_addr().af() != other.masked_addr().af())
	return (false);

    if (prefix_len() > other.prefix_len()) {
	// I have smaller prefix size
	IPNet other_masked(masked_addr(), other.prefix_len());
	return (other_masked.masked_addr() == other.masked_addr());
    }
    if (prefix_len() < other.prefix_len()) {
	// I have bigger prefix size
	IPNet other_masked(other.masked_addr(), prefix_len());
	return (other_masked.masked_addr() == masked_addr());
    }
    // Same prefix size
    return (other.masked_addr() == masked_addr());
}

template <>
inline bool
IPNet<IPv4>::contains(const IPv4& addr) const
{
    return addr.mask_by_prefix_len_uint(_prefix_len) == _masked_addr.addr();
}

template <>
inline bool
IPNet<IPv6>::contains(const IPv6& addr) const
{
    uint32_t prefixed_addr[4];
    addr.mask_by_prefix_len_uint(_prefix_len, prefixed_addr);
    return ((prefixed_addr[0] == _masked_addr.addr()[0])
	    && (prefixed_addr[1] == _masked_addr.addr()[1])
	    && (prefixed_addr[2] == _masked_addr.addr()[2])
	    && (prefixed_addr[3] == _masked_addr.addr()[3]));
}

template <class A>
inline bool
IPNet<A>::contains(const IPNet<A>& other) const
{
    if (_prefix_len > other._prefix_len)
	// I have smaller prefix size, hence I don't contain other.
	return (false);

    if (_prefix_len == other._prefix_len)
	return (other._masked_addr == _masked_addr);

    // I have bigger prefix size
    return (contains(other._masked_addr));
}

template <class A> void
IPNet<A>::initialize_from_string(const char *cp)
    throw (InvalidString, InvalidNetmaskLength)
{
    char *slash = strrchr(const_cast<char*>(cp), '/');
    if (slash == 0)
	xorp_throw(InvalidString, "Missing slash");

    if (*(slash + 1) == 0)
	xorp_throw(InvalidString, "Missing prefix length");
    char *n = slash + 1;
    while (*n != 0) {
	if (*n < '0' || *n > '9') {
	    xorp_throw(InvalidString, "Bad prefix length");
	}
	n++;
    }
    _prefix_len = atoi(slash + 1);

    string addr = string(cp, slash - cp);

    _masked_addr = A(addr.c_str()).mask_by_prefix_len(_prefix_len);
}

template <class A>
inline IPNet<A>&
IPNet<A>::operator--()
{
    _masked_addr = _masked_addr >> (_masked_addr.addr_bitlen() - _prefix_len);
    --_masked_addr;
    _masked_addr = _masked_addr << (_masked_addr.addr_bitlen() - _prefix_len);
    return (*this);
}

template <class A>
inline IPNet<A>&
IPNet<A>::operator++()
{
    _masked_addr = _masked_addr >> (_masked_addr.addr_bitlen() - _prefix_len);
    ++_masked_addr;
    _masked_addr = _masked_addr << (_masked_addr.addr_bitlen() - _prefix_len);
    return (*this);
}

template <class A>
inline uint32_t
IPNet<A>::overlap(const IPNet<A>& other) const
{
    if (masked_addr().af() != other.masked_addr().af())
	return 0;

    A xor_addr = masked_addr() ^ other.masked_addr();
    uint32_t done = xor_addr.leading_zero_count();

    uint32_t p = (prefix_len() < other.prefix_len()) ?
	prefix_len() : other.prefix_len();
    if (done > p)
	done = p;
    return done;
}

/**
 * Determine the number of the most significant bits overlapping
 * between two subnets.
 *
 * @param a1 the first subnet.
 * @param a2 the subnet.
 * @return the number of bits overlapping between @ref a1 and @ref a2.
 */
template <class A> uint32_t
overlap(const IPNet<A>& a1, const IPNet<A>& a2)
{
    return a1.overlap(a2);
}

#endif // __LIBXORP_IPNET_HH__
