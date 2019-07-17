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


#ifndef __LIBXORP_IPV4_HH__
#define __LIBXORP_IPV4_HH__

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"
#include "libxorp/range.hh"
#include "libxorp/utils.hh"


struct in_addr;

/**
 * @short IPv4 address class
 *
 * The IPv4 address class is a trivial class for handling IPv4
 * addresses and for performing operations on them such as printing
 * and masking.
 */
class IPv4 {
public:
    typedef in_addr 		InAddrType;
    typedef sockaddr_in 	SockAddrType;

public:
    /**
     * Default constructor
     *
     * The address value is initialized to INADDR_ANY.
     */
    IPv4()              { _addr = 0; }

    /**
     * Constructor from another IPv4 address.
     *
     * @param ipv4 the IPv4 address to assign the address value from.
     */
    IPv4(const IPv4& ipv4) : _addr(ipv4._addr) {}

    /**
     * Assignment Operator
     */
    IPv4& operator=(const IPv4& other) {
	_addr = other._addr;
	return *this;
    }

    /**
     * Constructor from an integer value.
     *
     * @param value 32-bit unsigned integer to assign to the address.
     */
    explicit IPv4(uint32_t value)    { _addr = value; }

    /**
     * Constructor from a (uint8_t *) memory pointer.
     *
     * @param from_uint8 the pointer to the memory to copy the address value
     * from.
     */
    explicit IPv4(const uint8_t *from_uint8);

    /**
     * Constructor from in_addr structure.
     *
     * @param from_in_addr the storage to copy the address value from.
     */
    IPv4(const in_addr& from_in_addr);

    /**
     * Constructor from sockaddr structure.
     *
     * @param sa sockaddr to construct IPv4 addr from.
     */
    IPv4(const sockaddr& sa) throw (InvalidFamily);

    /**
     * Constructor from sockaddr_storage structure.
     *
     * @param ss sockaddr_storage to construct IPv4 addr from.
     */
    IPv4(const sockaddr_storage& ss) throw (InvalidFamily);

    /**
     * Constructor from sockaddr_in structure.
     *
     * @param sin sockaddr_in to construct IPv4 addr from.
     */
    IPv4(const sockaddr_in& sin) throw (InvalidFamily);

    /**
     * Constructor from a string.
     *
     * @param from_cstring C-style string in the IPv4 dotted decimal
     * human-readable format used for initialization.
     */
    IPv4(const char *from_string) throw (InvalidString);

    /**
     * Copy the IPv4 raw address to specified memory location.
     *
     * @param: to_uint8 the pointer to the memory to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    /**
     * Copy the IP4 raw address to an in_addr structure.
     *
     * @param to_in_addr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(in_addr& to_in_addr) const;

    /**
     * Copy the IPv4 raw address to a sockaddr structure.
     *
     * Copy the raw address held within an IPv4 instance to an sockaddr
     * structure and assign appropriately and set fields within sockaddr
     * appropriately.
     *
     * @param to_sockaddr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr& to_sockaddr) const;

    /**
     * Copy the IPv4 raw address to a sockaddr_storage structure.
     *
     * Copy the raw address held within an IPv4 instance to an sockaddr_storage
     * structure and assign appropriately and set fields within
     * sockaddr_storage appropriately.
     *
     * @param to_sockaddr_storage the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr_storage& to_sockaddr_storage) const;

    /**
     * Copy the IPv4 raw address to a sockaddr_in structure.
     *
     * Copy the raw address held within an IPv4 instance to an sockaddr_in
     * structure and assign appropriately and set fields within sockaddr_in
     * appropriately.
     *
     * @param to_sockaddr_in the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr_in& to_sockaddr_in) const;

    /**
     * Copy a raw IPv4 address from specified memory location into IPv4
     * structure.
     *
     * @param from_uint8 the memory address to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const uint8_t *from_uint8);

    /**
     * Copy a raw IPv4 address from a in_addr structure into IPv4 structure.
     *
     * @param from_in_addr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const in_addr& from_in_addr);

    /**
     * Copy a raw address from a sockaddr structure into IPv4 structure.
     *
     * Note that the address in the sockaddr structure must be of IPv4 address
     * family.
     *
     * @param from_sockaddr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr& from_sockaddr) throw (InvalidFamily);

    /**
     * Copy a raw address from a sockaddr_storage structure into IPv4
     * structure.
     *
     * Note that the address in the sockaddr_storage structure must be of
     * IPv4 address family.
     *
     * @param from_sockaddr_storage the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr_storage& from_sockaddr_storage)
	throw (InvalidFamily);

    /**
     * Copy a raw address from a sockaddr_in structure into IPv4 structure.
     *
     * Note that the address in the sockaddr structure must be of IPv4 address
     * family.
     *
     * @param from_sockaddr_in the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr_in& from_sockaddr_in) throw (InvalidFamily);

    /**
     * Bitwise-Negation Operator
     *
     * @return address complement (i.e., all 0s become 1s, and vice-versa).
     */
    IPv4 operator~() const { return IPv4(~_addr); }

    /**
     * OR Operator
     *
     * @param other the right-hand operand to OR with.
     * @return bitwise OR of two addresses.
     */
    IPv4 operator|(const IPv4& other) const {
	return IPv4(_addr | other._addr);
    }

    /**
     * AND Operator
     *
     * @param other the right-hand operand to AND with.
     * @return bitwise AND of two addresses.
     */
    IPv4 operator&(const IPv4& other) const {
	return IPv4(_addr & other._addr);
    }

    /**
     * XOR Operator
     *
     * @return eXclusive-OR of two addresses.
     */
    IPv4 operator^(const IPv4& other) const {
	return IPv4(_addr ^ other._addr);
    }

    /**
     * Operator <<
     *
     * @param left_shift the number of bits to shift to the left.
     * @return IPv4 address that is shift bitwise to the left.
     */
    IPv4 operator<<(uint32_t left_shift) const;

    /**
     * Operator >>
     *
     * @param right_shift the number of bits to shift to the right.
     * @return IPv4 address that is shift bitwise to the right.
     */
    IPv4 operator>>(uint32_t right_shift) const;

    /**
     * Less-Than Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const IPv4& other) const;

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const IPv4& other) const { return (_addr == other._addr); }

    /**
     * Not-Equal Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically not same as the
     * right-hand operand.
     */
    bool operator!=(const IPv4& other) const { return (_addr != other._addr); }

    /**
     * Equality Operator for @ref IPv4 against @ref IPv4Range operand.
     *
     * @param rhs the right-hand @ref IPv4Range operand.
     * @return true if the value of the left-hand operand falls inside
     * the range defined by the right-hand operand.
     */
    bool operator==(const IPv4Range& rhs) const {
	return (_addr >= rhs.low().addr() && _addr <= rhs.high().addr());
    }

    /**
     * Not-equal Operator for @ref IPv4 against @ref IPv4Range operand.
     *
     * @param rhs the right-hand @ref IPv4Range operand.
     * @return true if the value of the left-hand operand falls outside
     * the range defined by the right-hand operand.
     */
    bool operator!=(const IPv4Range& rhs) const {
	return (_addr < rhs.low().addr() || _addr > rhs.high().addr());
    }

    /**
     * Less-than comparison for @ref IPv4 against @ref IPv4Range operand.
     *
     * @param rhs the right-hand @ref IPv4Range operand.
     * @return true if the value of the left-hand operand is bellow
     * the range defined by the right-hand operand.
     */
    bool operator<(const IPv4Range& rhs) const {
	return (_addr < rhs.low().addr());
    }

    /**
     * Less-than or equal comparison for @ref IPv4 against @ref IPv4Range
     *
     * @param rhs the right-hand @ref IPv4Range operand.
     * @return true if the value of the left-hand operand is bellow or within
     * the range defined by the right-hand operand.
     */
    bool operator<=(const IPv4Range& rhs) const {
	return (_addr <= rhs.high().addr());
    }

    /**
     * Greater-than comparison for @ref IPv4 against @ref IPv4Range operand.
     *
     * @param rhs the right-hand @ref IPv4Range operand.
     * @return true if the value of the left-hand operand is above
     * the range defined by the right-hand operand.
     */
    bool operator>(const IPv4Range& rhs) const {
	return (_addr > rhs.high().addr());
    }

    /**
     * Greater-than or equal comparison for @ref IPv4 against @ref IPv4Range
     *
     * @param rhs the right-hand @ref IPv4Range operand.
     * @return true if the value of the left-hand operand is above or within
     * the range defined by the right-hand operand.
     */
    bool operator>=(const IPv4Range& rhs) const {
	return (_addr >= rhs.low().addr());
    }

    /**
     * Decrement Operator
     *
     * The numerical value of this address is decremented by one.
     * However, if the address value before the decrement was all-0s,
     * after the decrement its value would be all-1s (i.e., it will
     * wrap-around).
     *
     * @return a reference to this address after it was decremented by one.
     */
    IPv4& operator--();

    /**
     * Increment Operator
     *
     * The numerical value of this address is incremented by one.
     * However, if the address value before the increment was all-1s,
     * after the increment its value would be all-0s (i.e., it will
     * wrap-around).
     *
     * @return a reference to this address after it was incremented by one.
     */
    IPv4& operator++();

    /**
     * Convert this address from binary form to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the address.
     */
    string str() const;

    /**
     * Test if this address is numerically zero.
     *
     * @return true if the address is numerically zero.
     */
    bool is_zero() const		{ return (_addr == 0); }

    /**
     * Test if this address is a valid unicast address.
     *
     * Note that the numerically zero address is excluded.
     *
     * @return true if the address is a valid unicast address.
     */
    bool is_unicast() const;

    /**
     * Test if this address is a valid multicast address.
     *
     * @return true if the address is a valid multicast address.
     */
    bool is_multicast() const;

    /**
     * Test if this address belongs to the IPv4 Class A
     * address space (0.0.0.0/1).
     *
     * @return true if the address is a valid Class A address.
     */
    bool is_class_a() const;

    /**
     * Test if this address belongs to the IPv4 Class B
     * address space (128.0.0.0/2).
     *
     * @return true if the address is a valid Class B address.
     */
    bool is_class_b() const;

    /**
     * Test if this address belongs to the IPv4 Class C
     * address space (192.0.0.0/3).
     *
     * @return true if the address is a valid Class C address.
     */
    bool is_class_c() const;

    /**
     * Test if this address belongs to the IPv4 experimental Class E
     * address space (240.0.0.0/4).
     *
     * @return true if the address is a valid experimental address.
     */
    bool is_experimental() const;

    /**
     * Test if this address is a valid link-local unicast address.
     *
     * @return true if the address is a valid unicast address,
     * and the scope of the address is link-local.
     */
    bool is_linklocal_unicast() const;

    /**
     * Test if this address is a valid interface-local multicast address.
     *
     * Note that "node-local" multicast addresses were renamed
     * to "interface-local" by RFC-3513.
     *
     * @return true if the address is a valid multicast address,
     * and the scope of the address is interface-local.
     * XXX: in IPv4 there is no interface-local multicast scope, therefore
     * the return value is always false.
     */
    bool is_interfacelocal_multicast() const;

    /**
     * Test if this address is a valid node-local multicast address.
     *
     * Note that "node-local" multicast addresses were renamed
     * to "interface-local" by RFC-3513.
     * This method is kept for backward compatibility.
     *
     * @return true if the address is a valid multicast address,
     * and the scope of the address is node-local.
     * XXX: in IPv4 there is no node-local multicast scope, therefore
     * the return value is always false.
     */
    bool is_nodelocal_multicast() const { return is_interfacelocal_multicast(); }

    /**
     * Test if this address is a valid link-local multicast address.
     *
     * @return true if the address is a valid multicast address,
     * and the scope of the address is link-local.
     */
    bool is_linklocal_multicast() const;

    /**
     * Test if this address is a valid loopback address.
     *
     * @return true if the address is a valid loopback address.
     */
    bool is_loopback() const;

    /**
     * Get the address octet-size.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_size = IPv4::addr_bytelen();
     *   size_t my_size = ipv4.addr_bytelen();
     *
     * @return address size in number of octets.
     */
    static size_t addr_bytelen() {
	x_static_assert(sizeof(IPv4) == sizeof(uint32_t));
	return sizeof(IPv4);
    }

    /**
     * Get the address bit-length.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_bitlen = IPv4::addr_bitlen();
     *   uint32_t my_bitlen = ipv4.addr_bitlen();
     *
     * @return address size in number of bits.
     */
    static uint32_t addr_bitlen() {
	return uint32_t(8 * sizeof(uint8_t) * addr_bytelen());
    }

    /**
     * Get the mask length for the multicast base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_len = IPv4::ip_multicast_base_address_mask_len();
     *   uint32_t my_len = ipv4.ip_multicast_base_address_mask_len();
     *
     * @return the multicast base address mask length for family AF_INET.
     */
    static uint32_t ip_multicast_base_address_mask_len() {
#define IP_MULTICAST_BASE_ADDRESS_MASK_LEN_IPV4	4
	return (IP_MULTICAST_BASE_ADDRESS_MASK_LEN_IPV4);
#undef IP_MULTICAST_BASE_ADDRESS_MASK_LEN_IPV4
    }

    /**
     * Get the mask length for the Class A base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_len = IPv4::ip_class_a_base_address_mask_len();
     *   uint32_t my_len = ipv4.ip_class_a_base_address_mask_len();
     *
     * @return the Class A base address mask length for family AF_INET.
     */
    static uint32_t ip_class_a_base_address_mask_len() {
#define IP_CLASS_A_BASE_ADDRESS_MASK_LEN_IPV4		1
	return (IP_CLASS_A_BASE_ADDRESS_MASK_LEN_IPV4);
#undef IP_CLASS_A_BASE_ADDRESS_MASK_LEN_IPV4
    }

    /**
     * Get the mask length for the Class B base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_len = IPv4::ip_class_b_base_address_mask_len();
     *   uint32_t my_len = ipv4.ip_class_b_base_address_mask_len();
     *
     * @return the Class B base address mask length for family AF_INET.
     */
    static uint32_t ip_class_b_base_address_mask_len() {
#define IP_CLASS_B_BASE_ADDRESS_MASK_LEN_IPV4		2
	return (IP_CLASS_B_BASE_ADDRESS_MASK_LEN_IPV4);
#undef IP_CLASS_B_BASE_ADDRESS_MASK_LEN_IPV4
    }

    /**
     * Get the mask length for the Class C base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_len = IPv4::ip_class_c_base_address_mask_len();
     *   uint32_t my_len = ipv4.ip_class_c_base_address_mask_len();
     *
     * @return the Class C base address mask length for family AF_INET.
     */
    static uint32_t ip_class_c_base_address_mask_len() {
#define IP_CLASS_C_BASE_ADDRESS_MASK_LEN_IPV4		3
	return (IP_CLASS_C_BASE_ADDRESS_MASK_LEN_IPV4);
#undef IP_CLASS_C_BASE_ADDRESS_MASK_LEN_IPV4
    }

    /**
     * Get the mask length for the experimental base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_len = IPv4::ip_experimental_base_address_mask_len();
     *   uint32_t my_len = ipv4.ip_experimental_base_address_mask_len();
     *
     * @return the experimental base address mask length for family AF_INET.
     */
    static uint32_t ip_experimental_base_address_mask_len() {
#define IP_EXPERIMENTAL_BASE_ADDRESS_MASK_LEN_IPV4	4
	return (IP_EXPERIMENTAL_BASE_ADDRESS_MASK_LEN_IPV4);
#undef IP_EXPERIMENTAL_BASE_ADDRESS_MASK_LEN_IPV4
    }

    /**
     * Make an IPv4 mask prefix.
     *
     * @param mask_len the length of the mask to create.
     * @return a new IPv4 address that contains a mask of length @ref mask_len.
     */
    static IPv4 make_prefix(uint32_t mask_len) throw (InvalidNetmaskLength);

    static uint32_t make_prefix_uint(uint32_t mask_len) throw (InvalidNetmaskLength);

    /**
     * Make an IPv4 address prefix.
     *
     * @param prefix_len the length of the mask of the prefix to create.
     * @return a new IPv4 address created by masking this address with a mask
     * of length @ref prefix_len.
     */
    IPv4 mask_by_prefix_len(uint32_t mask_len) const
	throw (InvalidNetmaskLength) {
	return (*this) & make_prefix(mask_len);
    }

    uint32_t mask_by_prefix_len_uint(uint32_t mask_len) const
	throw (InvalidNetmaskLength) {
	return this->_addr & make_prefix_uint(mask_len);
    }

    /**
     * Get the mask length.
     *
     * @return the prefix length of the contiguous mask presumably stored
     * as an IPv4 address.
     */
    uint32_t mask_len() const;

    /**
     * Get the uint32_t raw value of this address.
     *
     * @return the value of this IPv4 address as an unsigned 32-bit integer.
     */
    uint32_t addr() const		{ return _addr; }

    /**
     * Set the address value.
     *
     * @param value unsigned 32-bit integer value to set the address to.
     */
    void set_addr(uint32_t value)	{ _addr = value; }

    /**
     * Constant for address family
     */
    enum { AF = AF_INET };

    /**
     * Constant for IP protocol version
     */
    enum { IPV = 4 };

    /**
     * Get the address family.
     *
     * @return the address family of this address.
     */
    static int af() { return AF; }

    /**
     * Get the IP protocol version.
     *
     * @return the IP protocol version of this address.
     */
    static uint32_t ip_version() { return IPV; }

    /**
     * Get the human-readable string with the IP protocol version.
     *
     * @return the human-readable string with the IP protocol version of
     * this address.
     */
    static const string& ip_version_str();

    /**
     * Extract bits from an address.
     *
     * @param lsb starting bit position (from the right) to extract.
     * @param len number of bits to extract. The maximum value is 32.
     * @return the first @ref len bits starting from the rightmost
     * position @ref lsb. The returned bits are in host order.
     */
    uint32_t bits(uint32_t lsb, uint32_t len) const;

    /**
     * Count the number of bits that are set in this address.
     *
     * @return the number of bits that are set in this address.
     */
    uint32_t bit_count() const;

    /**
     * Count the number of leading zeroes in this address.
     *
     * @return the number of leading zeroes in this address.
     */
    uint32_t leading_zero_count() const;

    /**
     * Pre-defined IPv4 address constants.
     */
    static const IPv4& ZERO(int af = AF_INET);
    static const IPv4& ANY(int af = AF_INET);
    static const IPv4& ALL_ONES(int af = AF_INET);
    static const IPv4& LOOPBACK(int af = AF_INET);
    static const IPv4& MULTICAST_BASE(int af = AF_INET);
    static const IPv4& MULTICAST_ALL_SYSTEMS(int af = AF_INET);
    static const IPv4& MULTICAST_ALL_ROUTERS(int af = AF_INET);
    static const IPv4& DVMRP_ROUTERS(int af = AF_INET);
    static const IPv4& OSPFIGP_ROUTERS(int af = AF_INET);
    static const IPv4& OSPFIGP_DESIGNATED_ROUTERS(int af = AF_INET);
    static const IPv4& RIP2_ROUTERS(int af = AF_INET);
    static const IPv4& PIM_ROUTERS(int af = AF_INET);
    static const IPv4& SSM_ROUTERS(int af = AF_INET);
    static const IPv4& CLASS_A_BASE(int af = AF_INET);
    static const IPv4& CLASS_B_BASE(int af = AF_INET);
    static const IPv4& CLASS_C_BASE(int af = AF_INET);
    static const IPv4& EXPERIMENTAL_BASE(int af = AF_INET);

    /**
     * Number of bits in address as a constant.
     */
    static const uint32_t ADDR_BITLEN = 32;

    /**
     * Number of bytes in address as a constant.
     */
    static const uint32_t ADDR_BYTELEN = ADDR_BITLEN / 8;

private:
    uint32_t _addr;		// The address value (in network-order)
};

inline uint32_t
IPv4::bits(uint32_t lsb, uint32_t len) const
{
    uint32_t mask = ~(0xffffffffU << len);

    if (len >= 32)
	mask = 0xffffffffU;	// XXX: shifting with >= 32 bits is undefined
    return (ntohl(_addr) >> lsb) & mask;
}

inline uint32_t
IPv4::make_prefix_uint(uint32_t mask_len) throw (InvalidNetmaskLength)
{
    if (mask_len > 32)
	xorp_throw(InvalidNetmaskLength, mask_len);
    uint32_t m = (mask_len == 0) ? 0 : ((~0U) << (32 - mask_len));
    return htonl(m);
}

inline IPv4&
IPv4::operator--()
{
    uint32_t tmp_addr = ntohl(_addr) - 1;
    _addr = htonl(tmp_addr);
    return *this;
}

inline IPv4&
IPv4::operator++()
{
    uint32_t tmp_addr = ntohl(_addr) + 1;
    _addr = htonl(tmp_addr);
    return *this;
}

inline uint32_t
IPv4::bit_count() const
{
    // XXX: no need for ntohl()
    return (xorp_bit_count_uint32(_addr));
}

inline uint32_t
IPv4::leading_zero_count() const
{
    return (xorp_leading_zero_count_uint32(ntohl(_addr)));
}

inline bool
IPv4::operator<(const IPv4& other) const
{
    return ntohl(_addr) < ntohl(other._addr);
}

struct IPv4Constants {
    static const IPv4 zero,
	any,
	all_ones,
	loopback,
	multicast_base,
	multicast_all_systems,
	multicast_all_routers,
	dvmrp_routers,
	ospfigp_routers,
	ospfigp_designated_routers,
	rip2_routers,
	pim_routers,
	ssm_routers,
	class_a_base,
	class_b_base,
	class_c_base,
	experimental_base;
};

inline const IPv4& IPv4::ZERO(int) {
    return IPv4Constants::zero;
}

inline const IPv4& IPv4::ANY(int) {
    return IPv4Constants::any;
}

inline const IPv4& IPv4::ALL_ONES(int) {
    return IPv4Constants::all_ones;
}

inline const IPv4& IPv4::LOOPBACK(int) {
    return IPv4Constants::loopback;
}

inline const IPv4& IPv4::MULTICAST_BASE(int) {
    return IPv4Constants::multicast_base;
}

inline const IPv4& IPv4::MULTICAST_ALL_SYSTEMS(int) {
    return IPv4Constants::multicast_all_systems;
}

inline const IPv4& IPv4::MULTICAST_ALL_ROUTERS(int) {
    return IPv4Constants::multicast_all_routers;
}

inline const IPv4& IPv4::DVMRP_ROUTERS(int) {
    return IPv4Constants::dvmrp_routers;
}

inline const IPv4& IPv4::OSPFIGP_ROUTERS(int) {
    return IPv4Constants::ospfigp_routers;
}

inline const IPv4& IPv4::OSPFIGP_DESIGNATED_ROUTERS(int) {
    return IPv4Constants::ospfigp_designated_routers;
}

inline const IPv4& IPv4::RIP2_ROUTERS(int) {
    return IPv4Constants::rip2_routers;
}

inline const IPv4& IPv4::PIM_ROUTERS(int) {
    return IPv4Constants::pim_routers;
}

inline const IPv4& IPv4::SSM_ROUTERS(int) {
    return IPv4Constants::ssm_routers;
}

inline const IPv4& IPv4::CLASS_A_BASE(int) {
    return IPv4Constants::class_a_base;
}

inline const IPv4& IPv4::CLASS_B_BASE(int) {
    return IPv4Constants::class_b_base;
}

inline const IPv4& IPv4::CLASS_C_BASE(int) {
    return IPv4Constants::class_c_base;
}

inline const IPv4& IPv4::EXPERIMENTAL_BASE(int) {
    return IPv4Constants::experimental_base;
}

const char* ip_proto_str(uint8_t protocol);

#endif // __LIBXORP_IPV4_HH__
