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

// $XORP: xorp/libxorp/ipv6.hh,v 1.17 2004/02/24 23:50:52 hodson Exp $

#ifndef __LIBXORP_IPV6_HH__
#define __LIBXORP_IPV6_HH__

#include <string.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "config.h"
#include "xorp.h"
#include "exceptions.hh"

struct in6_addr;

/**
 * @short IPv6 address class
 *
 * The IPv6 address class is a trivial class for handling IPv6
 * addresses and for performing operations on them such as printing
 * and masking.
 */
class IPv6 {
public:
    typedef in6_addr 		InAddrType;
    typedef sockaddr_in6 	SockAddrType;

public:
    /**
     * Default constructor
     *
     * The address value is initialized to IN6ADDR_ANY.
     */
    IPv6() { _addr[0] = _addr[1] = _addr[2] = _addr[3] = 0; }

    /**
     * Constructor from a (uint8_t *) memory pointer.
     *
     * @param from_uint8 the pointer to the memory to copy the address value
     * from.
     */
    explicit IPv6(const uint8_t *from_uint8);

    /**
     * Constructor from a (uint32_t *) memory pointer.
     *
     * @param from_uint32 the pointer to the memory to copy the address value
     * from.
     */
    explicit IPv6(const uint32_t *from_uint32);

    /**
     * Constructor from in6_addr structure.
     *
     * @param from_in6_addr the storage to copy the address value from.
     */
    IPv6(const in6_addr& from_in6_addr);

    /**
     * Constructor from sockaddr structure.
     *
     * @param sa sockaddr to construct IPv6 addr from.
     */
    IPv6(const sockaddr& sa) throw (InvalidFamily);

    /**
     * Constructor from sockaddr_in6 structure.
     *
     * @param sin sockaddr_in6 to construct IPv6 addr from.
     */
    IPv6(const sockaddr_in6& sin) throw (InvalidFamily);

    /**
     * Constructor from a string.
     *
     * @param from_cstring C-style string in the IPv6 canonical human-readable.
     * format used for initialization.
     */
    IPv6(const char *from_cstring) throw (InvalidString);

    /**
     * Copy the IPv6 raw address to specified memory location.
     *
     * @param: to_uint8 the pointer to the memory to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    /**
     * Copy the IPv6 raw address to an in6_addr structure.
     *
     * @param to_in6_addr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(in6_addr& to_in6_addr) const;

    /**
     * Copy the IPv6 raw address to a sockaddr structure.
     *
     * Copy the raw address held within an IPv6 instance to an sockaddr
     * structure and assign appropriately and set fields within sockaddr
     * appropriately.
     *
     * @param to_sockaddr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr& to_sockaddr) const;

    /**
     * Copy the IPv6 raw address to a sockaddr_in6 structure.
     *
     * Copy the raw address held within an IPv6 instance to a sockaddr_in6
     * structure and assign appropriately and set fields within sockaddr_in
     * appropriately.
     *
     * @param to_sockaddr_in6 the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr_in6& to_sockaddr_in6) const;

    /**
     * Copy a raw IPv6 address from specificed memory location into IPv6
     * structure.
     *
     * @param from_uint8 the memory address to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const uint8_t *from_uint8);

    /**
     * Copy a raw IPv6 address from a in6_addr structure into IPv6 structure.
     *
     * @param from_in6_addr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const in6_addr& from_in6_addr);

    /**
     * Copy a raw IPv6 address from a sockaddr structure into IPv6 structure.
     *
     * Note that the address in the sockaddr structure must be of IPv6 address
     * family.
     *
     * @param from_sockaddr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr& from_sockaddr) throw (InvalidFamily);

    /**
     * Copy a raw address from sockaddr_in6 structure into IPv6 structure.
     *
     * Note that the address in the sockaddr structure must be of IPv6 address
     * family.
     *
     * @param from_sockaddr_in6 the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr_in6& from_sockaddr_in6)
	throw (InvalidFamily);

    /**
     * Bitwise-Negation Operator
     *
     * @return address complement (i.e., all 0s become 1s, and vice-versa).
     */
    inline IPv6 operator~() const;

    /**
     * OR Operator
     *
     * @param other the right-hand operand to OR with.
     * @return bitwise OR of two addresses.
     */
    inline IPv6 operator|(const IPv6& other) const;

    /**
     * AND Operator
     *
     * @param other the right-hand operand to AND with.
     * @return bitwise AND of two addresses.
     */
    inline IPv6 operator&(const IPv6& other) const;

    /**
     * XOR Operator
     *
     * @param other the right-hand operand to XOR with.
     * @return bitwize eXclusive-OR of two addresses.
     */
    inline IPv6 operator^(const IPv6& other) const;

    /**
     * Operator <<
     *
     * @param left_shift the number of bits to shift to the left.
     * @return IPv6 address that is shift bitwise to the left.
     */
    IPv6 operator<<(uint32_t left_shift) const;

    /**
     * Operator >>
     *
     * @param right_shift the number of bits to shift to the right.
     * @return IPv6 address that is shift bitwise to the right.
     */
    IPv6 operator>>(uint32_t right_shift) const;

    /**
     * Less-Than Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const IPv6& other) const;

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const IPv6& other) const;

    /**
     * Not-Equal Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically not same as the
     * right-hand operand.
     */
    bool operator!=(const IPv6& other) const;

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
    IPv6& operator--();

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
    IPv6& operator++();

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
    inline bool is_zero() const		{ return *this == ZERO(); }

    /**
     * Test if this address is a valid unicast address.
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
     * Test if this address is a valid link-local unicast address.
     *
     * @return true if the address is a valid unicast address,
     * and the scope of the address is link-local.
     */
    bool is_linklocal_unicast() const;

    /**
     * Test if this address is a valid node-local multicast address.
     *
     * @return true if the address is a valid multicast address,
     * and the scope of the address is node-local.
     */
    bool is_nodelocal_multicast() const;

    /**
     * Test if this address is a valid link-local multicast address.
     *
     * @return true if the address is a valid multicast address,
     * and the scope of the address is link-local.
     */
    bool is_linklocal_multicast() const;

    /**
     * Test if this address is the loopback address.
     *
     * @return true if the address is the loopback address.
     */
    bool is_loopback() const;

    /**
     * Get the address octet-size.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_size = IPv6::addr_size();			OK
     *   size_t my_size = ipv6.addr_size();			OK
     *
     * @return address size in number of octets.
     */
    inline static size_t addr_size() {
	static_assert(sizeof(IPv6) == 4 * sizeof(uint32_t));
	return sizeof(IPv6);
    }

    /**
     * Get the address bit-length.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_bitlen = IPv6::addr_bitlen();		OK
     *   uint32_t my_bitlen = ipv6.addr_bitlen();		OK
     *
     * @return address size in number of bits.
     */
    inline static uint32_t addr_bitlen() {
	return uint32_t(8 * sizeof(uint8_t) * addr_size());
    }

    /**
     * Get the mask length for the multicast base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_len = IPv6::ip_multicast_base_address_mask_len();	OK
     *   size_t my_len = ipv6.ip_multicast_base_address_mask_len();	OK
     *
     * @return the multicast base address mask_len for family AF_INET6.
     */
    static uint32_t ip_multicast_base_address_mask_len() {
#define IP_MULTICAST_BASE_ADDRESS_MASK_LEN_IPV6	8
	return (IP_MULTICAST_BASE_ADDRESS_MASK_LEN_IPV6);
#undef IP_MULTICAST_BASE_ADDRESS_MASK_LEN_IPV6
    }

    /**
     * Make an IPv6 mask prefix.
     *
     * @param mask_len the length of the mask to create.
     * @return a new IPv6 address that contains a mask of length @ref mask_len.
     */
    static const IPv6& make_prefix(uint32_t mask_len) throw (InvalidNetmaskLength);

    /**
     * Make an IPv6 address prefix.
     *
     * @param prefix_len the length of the mask of the prefix to create.
     * @return a new IPv6 address created by masking this address with a mask
     * of length @ref prefix_len.
     */
    inline IPv6 mask_by_prefix_len(uint32_t prefix_len) const
	throw (InvalidNetmaskLength) {
	return (*this) & make_prefix(prefix_len);
    }

    /**
     * Get the mask length.
     *
     * @return the prefix length of the contiguous mask presumably stored
     * as an IPv6 address.
     */
    uint32_t mask_len() const;

    /**
     * Get the uint32_t raw value of this address.
     *
     * @return the value of this IPv6 address as a pointer to an array
     * of 4 unsigned 32-bit integers.
     */
    const uint32_t *addr() const { return _addr; }

    /**
     * Set the address value.
     *
     * @param from the pointer to an array of 16 8-bit unsigned integers
     * with the value to set this address to.
     */
    void set_addr(const uint8_t *from_uint8) { memcpy(_addr, from_uint8, 16); }

    /**
     * Constant for address family
     */
    enum { AF = AF_INET6 };

    /**
     * Constant for IP protocol version
     */
    enum { IPV = 6 };

    /**
     * Get the address family.
     *
     * @return the address family of this address.
     */
    inline static const int af() { return AF; }

    /**
     * Get the IP protocol version.
     *
     * @return the IP protocol version of this address.
     */
    inline static const uint32_t ip_version() { return IPV; }

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
    inline uint32_t bits(uint32_t lsb, uint32_t len) const;

    /**
     * Pre-defined IPv6 address constants.
     */
    inline static const IPv6& ZERO(int af = AF_INET6);
    inline static const IPv6& ANY(int af = AF_INET6);
    inline static const IPv6& ALL_ONES(int af = AF_INET6);
    inline static const IPv6& MULTICAST_BASE(int af = AF_INET6);
    inline static const IPv6& MULTICAST_ALL_SYSTEMS(int af = AF_INET6);
    inline static const IPv6& MULTICAST_ALL_ROUTERS(int af = AF_INET6);
    inline static const IPv6& DVMRP_ROUTERS(int af = AF_INET6);
    inline static const IPv6& OSPFIGP_ROUTERS(int af = AF_INET6);
    inline static const IPv6& OSPFIGP_DESIGNATED_ROUTERS(int af = AF_INET6);
    inline static const IPv6& RIP2_ROUTERS(int af = AF_INET6);
    inline static const IPv6& PIM_ROUTERS(int af = AF_INET6);

    /**
     * Number of bits in address as a constant.
     */
    static const uint32_t ADDR_BITLEN = 128;

private:
    uint32_t _addr[4];		// The address value (in network-order)
};

inline uint32_t
IPv6::bits(uint32_t lsb, uint32_t len) const
{
    uint32_t mask = ~(0xffffffffU << len);

    if (len >= 32)
	mask = 0xffffffffU;	// XXX: shifting with >= 32 bits is undefined

    return ntohl((*this >> lsb)._addr[3]) & mask;
}

struct IPv6Constants {
    static const IPv6 zero, any, all_ones, multicast_base,
	multicast_all_systems, multicast_all_routers,
	dvmrp_routers, rip2_routers, pim_routers,
	ospfigp_routers, ospfigp_designated_routers;
};

inline const IPv6& IPv6::ZERO(int) {
    return IPv6Constants::zero;
}

inline const IPv6& IPv6::ANY(int) {
    return IPv6Constants::any;
}

inline const IPv6& IPv6::ALL_ONES(int) {
    return IPv6Constants::all_ones;
}

inline const IPv6& IPv6::MULTICAST_BASE(int) {
    return IPv6Constants::multicast_base;
}

inline const IPv6& IPv6::MULTICAST_ALL_SYSTEMS(int) {
    return IPv6Constants::multicast_all_systems;
}

inline const IPv6& IPv6::MULTICAST_ALL_ROUTERS(int) {
    return IPv6Constants::multicast_all_routers;
}

inline const IPv6& IPv6::DVMRP_ROUTERS(int) {
    return IPv6Constants::dvmrp_routers;
}

inline const IPv6& IPv6::OSPFIGP_ROUTERS(int) {
    return IPv6Constants::ospfigp_routers;
}

inline const IPv6& IPv6::OSPFIGP_DESIGNATED_ROUTERS(int) {
    return IPv6Constants::ospfigp_designated_routers;
}

inline const IPv6& IPv6::RIP2_ROUTERS(int) {
    return IPv6Constants::rip2_routers;
}

inline const IPv6& IPv6::PIM_ROUTERS(int) {
    return IPv6Constants::pim_routers;
}

inline IPv6 IPv6::operator~() const {
    uint32_t tmp_addr[4];
    tmp_addr[0] = ~_addr[0];
    tmp_addr[1] = ~_addr[1];
    tmp_addr[2] = ~_addr[2];
    tmp_addr[3] = ~_addr[3];
    return IPv6(tmp_addr);
}

inline IPv6 IPv6::operator|(const IPv6& other) const {
    uint32_t tmp_addr[4];
    tmp_addr[0] = _addr[0] | other._addr[0];
    tmp_addr[1] = _addr[1] | other._addr[1];
    tmp_addr[2] = _addr[2] | other._addr[2];
    tmp_addr[3] = _addr[3] | other._addr[3];
    return IPv6(tmp_addr);
}

inline IPv6 IPv6::operator&(const IPv6& other) const {
    uint32_t tmp_addr[4];
    tmp_addr[0] = _addr[0] & other._addr[0];
    tmp_addr[1] = _addr[1] & other._addr[1];
    tmp_addr[2] = _addr[2] & other._addr[2];
    tmp_addr[3] = _addr[3] & other._addr[3];
    return IPv6(tmp_addr);
}

inline IPv6 IPv6::operator^(const IPv6& other) const {
    uint32_t tmp_addr[4];
    tmp_addr[0] = _addr[0] ^ other._addr[0];
    tmp_addr[1] = _addr[1] ^ other._addr[1];
    tmp_addr[2] = _addr[2] ^ other._addr[2];
    tmp_addr[3] = _addr[3] ^ other._addr[3];
    return IPv6(tmp_addr);
}

#endif // __LIBXORP_IPV6_HH__
