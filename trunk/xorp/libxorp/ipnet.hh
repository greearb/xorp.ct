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

// $XORP: xorp/libxorp/ipnet.hh,v 1.7 2003/04/26 06:15:04 pavlin Exp $

#ifndef __LIBXORP_IPNET_HH__
#define __LIBXORP_IPNET_HH__

#include "xorp.h"
#include "exceptions.hh"
#include "c_format.hh"

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
    IPNet(const A& a, size_t prefix_len) throw (InvalidNetmaskLength)
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
	return ((prefix_len() == other.prefix_len()) &&
		(masked_addr() == other.masked_addr()));
    }

    /**
     * Less-Than Operator
     * 
     * Less-than comparison for subnets (see body for description).
     * 
     * @param other the right-hand side of the comparison.
     * @return true if the left-hand side is "smaller" than the right-hand
     * side according to the chosen order.
     */
    inline bool operator<(const IPNet& other) const;

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
     * Convert this address from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the address.
     */
    inline string str() const {
	return _masked_addr.str() + c_format("/%u", (uint32_t)_prefix_len);
    }

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
    inline bool is_overlap(const IPNet& other) const;

    /**
     * Test if a subnet contains (or is equal to) another subnet.
     *
     * in LaTeX, x.contains(y) would be   $x \superseteq y$
     * 
     * @param other the subnet to test against.
     * @return true if this subnet contains or is equal to @ref other.
     */
    inline bool contains(const IPNet& other) const;

    /**
     * Test if an address is within a subnet.
     *
     * @param addr the address to test against.
     * @return true if @ref addr is within this subnet.
     */
    inline bool contains(const A& addr) const {
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
    inline size_t overlap(const IPNet& other) const;

    /**
     * Get the address family.
     * 
     * @return the address family of this address.
     */
    static const int af() { return A::af(); }

    /**
     * Get the base address.
     * 
     * @return the base address for this subnet.
     */
    inline const A& masked_addr() const { return _masked_addr; }

    /**
     * Get the prefix length.
     * 
     * @return the prefix length for this subnet.
     */
    inline size_t prefix_len() const { return _prefix_len; }

    /**
     * Get the network mask.
     * 
     * @return the netmask associated with this subnet.
     */
    inline A netmask() const { return _masked_addr.make_prefix(_prefix_len); }

    /**
     * Return the subnet containing all multicast addresses.
     * 
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   IPv4Net my_prefix = IPv4Net::ip_multicast_base_prefix(); OK
     *   IPv4Net my_prefix = ipv4net.ip_multicast_base_prefix();  OK
     * 
     * @return the subnet containing multicast addresses.
     */
    static const IPNet<A> ip_multicast_base_prefix() {
	return IPNet(A::MULTICAST_BASE(),
		     A::ip_multicast_base_address_masklen());
    }
    
    /**
     * Test if this subnet is within the multicast address range.
     * 
     * @return true if this subnet is within the multicast address range.
     */
    bool is_multicast() const {
	return (ip_multicast_base_prefix().contains(*this));
    }
    
    /**
     * Get the highest address within this subnet.
     * 
     * @return the highest address within this subnet.
     */
    inline A top_addr() const { return _masked_addr | ~netmask(); }

    /**
     * Get the smallest subnet containing both subnets.
     * 
     * @return the smallest subnet containing both subnets passed
     * as arguments.
     */
    static IPNet<A> common_subnet(const IPNet<A> x, const IPNet<A> y) {
	return IPNet<A>(x.masked_addr(), x.overlap(y));
    }

protected:
    inline void initialize_from_string(const char *s)
	throw (InvalidString, InvalidNetmaskLength);

    A		_masked_addr;
    size_t	_prefix_len;
};

/* ------------------------------------------------------------------------- */
/* Deferred method definitions */

template <class A> bool
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
    if (this->contains(other))
	return false;
    else if (other.contains(*this))
	return true;
    else
	return this->masked_addr() < other.masked_addr();

#else	// old code
    const A& maddr_him = other.masked_addr();
    size_t his_prefix_len = other.prefix_len();

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

template <class A> bool
IPNet<A>::is_overlap(const IPNet<A>& other) const
{
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

template <class A> bool
IPNet<A>::contains(const IPNet<A>& other) const
{
    if (prefix_len() > other.prefix_len()) {
	// I have smaller prefix size, hence I don't contain other.
	return (false);
    }
    if (prefix_len() < other.prefix_len()) {
	// I have bigger prefix size
	IPNet other_masked(other.masked_addr(), prefix_len());
	return (other_masked.masked_addr() == masked_addr());
    }
    // Same prefix size
    return (other.masked_addr() == masked_addr());
}

template <class A> void
IPNet<A>::initialize_from_string(const char *cp)
    throw (InvalidString, InvalidNetmaskLength)
{
    char *slash = strrchr(cp, '/');
    if (slash == 0)
	xorp_throw(InvalidString, "Missing slash");
    
    if (*(slash + 1) == 0)
	xorp_throw(InvalidString, "Missing prefix length");
    _prefix_len = atoi(slash + 1);
    
    string addr = string(cp, slash - cp);
    
    _masked_addr = A(addr.c_str()).mask_by_prefix_len(_prefix_len);
}

template <class A> IPNet<A>&
IPNet<A>::operator--()
{
    _masked_addr = _masked_addr >> (_masked_addr.addr_bitlen() - _prefix_len);
    --_masked_addr;
    _masked_addr = _masked_addr << (_masked_addr.addr_bitlen() - _prefix_len);
    return (*this);
}

template <class A> IPNet<A>&
IPNet<A>::operator++()
{
    _masked_addr = _masked_addr >> (_masked_addr.addr_bitlen() - _prefix_len);
    ++_masked_addr;
    _masked_addr = _masked_addr << (_masked_addr.addr_bitlen() - _prefix_len);
    return (*this);
}

template <class A>
inline size_t
IPNet<A>::overlap(const IPNet<A>& other) const
{
    A addr = masked_addr() ^ other.masked_addr();

    size_t p = (prefix_len() < other.prefix_len()) ? 
	prefix_len() : other.prefix_len();

    size_t i = 0;

    for (A o(A::ALL_ONES(_masked_addr.af())); i <= p; o = o >> 1, i++) {
	if (o < addr) 
	    return i - 1;
    }
    return i - 1;
}

/**
 * Determine the number of the most significant bits overlapping
 * between two subnets.
 * 
 * @param a1 the first subnet.
 * @param a2 the subnet.
 * @return the number of bits overlapping between @ref a1 and @ref a2.
 */
template <class A> size_t
overlap(const IPNet<A>& a1, const IPNet<A>& a2)
{
    return a1.overlap(a2);
}

#endif // __LIBXORP_IPNET_HH__
