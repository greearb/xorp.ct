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

// $XORP: xorp/libxorp/ipv4.hh,v 1.6 2003/04/18 04:52:08 pavlin Exp $

#ifndef __LIBXORP_IPV4_HH__
#define __LIBXORP_IPV4_HH__

#include <string.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "config.h"
#include "xorp.h"
#include "exceptions.hh"


class IPvX;
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
    /**
     * Default constrictor
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
    inline IPv4 operator~() const { return IPv4(~_addr); }

    /**
     * OR Operator
     * 
     * @param other the right-hand operand to OR with.
     * @return bitwise OR of two addresses.
     */
    inline IPv4 operator|(const IPv4& other) const {
	return IPv4((uint32_t)(_addr | other._addr));
    }

    /**
     * AND Operator
     * 
     * @param other the right-hand operand to AND with.
     * @return bitwise AND of two addresses.
     */
    inline IPv4 operator&(const IPv4& other) const {
	return IPv4((uint32_t)(_addr & other._addr));
    }

    /**
     * XOR Operator
     * 
     * @return eXclusive-OR of two addresses.
     */
    inline IPv4 operator^(const IPv4& other) const {
	return IPv4((uint32_t)(_addr ^ other._addr));
    }
    
    /**
     * Operator <<
     * 
     * @param left_shift the number of bits to shift to the left.
     * @return IPv4 address that is shift bitwise to the left.
     */
    IPv4 operator<<(size_t left_shift) const;

    /**
     * Operator >>
     * 
     * @param right_shift the number of bits to shift to the right.
     * @return IPv4 address that is shift bitwise to the right.
     */
    IPv4 operator>>(size_t right_shift) const;

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
     * Test if this address is a valid node-local multicast address.
     * 
     * @return true if the address is a valid multicast address,
     * and the scope of the address is node-local.
     * XXX: in IPv4 there is no node-local multicast scope, therefore
     * the return value is always false.
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
     * Get the address octet-size.
     * 
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_size = IPv4::addr_size();			OK
     *   size_t my_size = ipv4.addr_size();			OK
     * 
     * @return address size in number of octets.
     */
    inline static size_t addr_size() { 
	static_assert(sizeof(IPv4) == sizeof(uint32_t));
	return sizeof(IPv4); 
    }
    
    /**
     * Get the address bit-length.
     * 
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_bitlen = IPv4::addr_bitlen();		OK
     *   size_t my_bitlen = ipv4.addr_bitlen();			OK
     * 
     * @return address size in number of bits.
     */
    inline static size_t addr_bitlen() {
	return 8 * sizeof(uint8_t) * addr_size();
    }

    /**
     * Get the masklen for the multicast base address.
     * 
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   size_t my_len = IPv4::ip_multicast_base_address_masklen();	OK
     *   size_t my_len = ipv4.ip_multicast_base_address_masklen();	OK
     * 
     * @return the multicast base address masklen for family AF_INET.
     */
    static size_t ip_multicast_base_address_masklen() {
#define IP_MULTICAST_BASE_ADDRESS_MASKLEN_IPV4	4
	return (IP_MULTICAST_BASE_ADDRESS_MASKLEN_IPV4);
#undef IP_MULTICAST_BASE_ADDRESS_MASKLEN_IPV4
    }
    
    /**
     * Make an IPv4 mask prefix.
     * 
     * @param masklen the length of the mask to create.
     * @return a new IPv4 address that contains a mask of length @ref masklen.
     */
    static IPv4 make_prefix(size_t masklen) throw (InvalidNetmaskLength);

    /**
     * Make an IPv4 address prefix.
     * 
     * @param masklen the length of the mask of the prefix to create.
     * @return a new IPv4 address created by masking this address with a mask
     * of length @ref masklen.
     */
    inline IPv4 mask_by_prefix(size_t masklen) const
	throw (InvalidNetmaskLength) {
	return (*this) & make_prefix(masklen);
    }

    /**
     * Get the mask length.
     * 
     * @return the prefix length of the contiguous mask presumably stored
     * as an IPv4 address.
     */
    size_t prefix_length() const;
    
    /**
     * Get the uint32_t raw value of this address.
     * 
     * @return the value of this IPv4 address as an unsigned 32-bit integer.
     */
    uint32_t addr() const               { return _addr; }
    
    /**
     * Set the address value.
     * 
     * @param value unsigned 32-bit integer value to set the address to.
     */
    void set_addr(uint32_t value)    { _addr = value; }

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
    inline static const int af() { return AF; }

    /**
     * Get the IP protocol version.
     * 
     * @return the IP protocol version of this address.
     */
    inline static const uint32_t ip_version() { return IPV; }

    /**
     * Extract bits from an address.
     * 
     * @param lsb starting bit position (from the right) to extract.
     * @param len number of bits to extract. The maximum value is 32.
     * @return the first @ref len bits starting from the rightmost
     * position @ref lsb. The returned bits are in host order.
     */
    inline uint32_t bits(size_t lsb, size_t len) const;
    
    /**
     * Pre-defined IPv4 address constants.
     */
    inline static IPv4 ZERO(int af = AF_INET);
    inline static IPv4 ANY(int af = AF_INET);
    inline static IPv4 ALL_ONES(int af = AF_INET);
    inline static IPv4 MULTICAST_BASE(int af = AF_INET);
    inline static IPv4 MULTICAST_ALL_SYSTEMS(int af = AF_INET);
    inline static IPv4 MULTICAST_ALL_ROUTERS(int af = AF_INET);
    inline static IPv4 DVMRP_ROUTERS(int af = AF_INET);
    inline static IPv4 OSPFIGP_ROUTERS(int af = AF_INET);
    inline static IPv4 OSPFIGP_DESIGNATED_ROUTERS(int af = AF_INET);
    inline static IPv4 RIP2_ROUTERS(int af = AF_INET);
    inline static IPv4 PIM_ROUTERS(int af = AF_INET);
    
private:
    uint32_t _addr;		// The address value (in network-order)
};

//
// Micro-optimization: byte ordering fix for constants.  htonl uses
// CPU instructions, whereas the macro below can be handled by the
// compiler front-end for literal values. 
//
#if defined(WORDS_BIGENDIAN)
#  define htonl_literal(x) (x)
#elif defined(WORDS_SMALLENDIAN)
#  define htonl_literal(x) 						      \
		((((x) & 0x000000ffU) << 24) | (((x) & 0x0000ff00U) << 8) |   \
		 (((x) & 0x00ff0000U) >> 8) | (((x) & 0xff000000U) >> 24))
#else
#  error "Missing endian definition from config.h"
#endif

inline uint32_t
IPv4::bits(size_t lsb, size_t len) const
{
    uint32_t mask = ~(0xffffffffU << len);
    
    if (len >= 32)
	mask = 0xffffffffU;	// XXX: shifting with >= 32 bits is undefined
    return (ntohl(_addr) >> lsb) & mask;
}

IPv4 IPv4::ZERO(int) { 
    return IPv4(0x0U);
}

IPv4 IPv4::ANY(int) { 
    return ZERO(); 
}

IPv4 IPv4::ALL_ONES(int) {
    return IPv4(0xffffffffU);
}

IPv4 IPv4::MULTICAST_BASE(int) { 
    return IPv4(htonl_literal(0xe0000000U));
}

IPv4 IPv4::MULTICAST_ALL_SYSTEMS(int) { 
    return IPv4(htonl_literal(0xe0000001U));
}

IPv4 IPv4::MULTICAST_ALL_ROUTERS(int) { 
    return IPv4(htonl_literal(0xe0000002U));
}

IPv4 IPv4::DVMRP_ROUTERS(int) { 
    return IPv4(htonl_literal(0xe0000004U));
}

IPv4 IPv4::OSPFIGP_ROUTERS(int) {
    return IPv4(htonl_literal(0xe0000005U));
}

IPv4 IPv4::OSPFIGP_DESIGNATED_ROUTERS(int) { 
    return IPv4(htonl_literal(0xe0000006U));
}

IPv4 IPv4::RIP2_ROUTERS(int) { 
    return IPv4(htonl_literal(0xe0000009U));
}

IPv4 IPv4::PIM_ROUTERS(int) {
    return IPv4(htonl_literal(0xe000000dU));
}

#endif // __LIBXORP_IPV4_HH__
