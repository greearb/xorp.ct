// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxorp/ipvx.hh,v 1.15 2004/02/24 23:50:52 hodson Exp $

#ifndef __LIBXORP_IPVX_HH__
#define __LIBXORP_IPVX_HH__

#include "ipv4.hh"
#include "ipv6.hh"


/**
 * @short Basic IPvX class (for both IPv4 and IPv6)
 */
class IPvX {
public:
    /**
     * Default contructor
     *
     * Creates an IPvX address with address family of AF_INET, and
     * address value of INADDR_ANY (i.e., containing IPv4(0)).
     */
    IPvX() : _af(AF_INET) { _addr[0] = 0; }

    /**
     * Constructor for a specified address family.
     *
     * Creates an address of specified family, and address value of
     * INADDR_ANY or IN6ADDR_ANY (for IPv4 and IPv6 respectively).
     *
     * @param family the address family.
     */
    explicit IPvX(int family) throw (InvalidFamily);

    /**
     * Constructor from a (uint8_t *) memory pointer.
     *
     * Creates an address of specified family, and address value
     * by using data from specified location.
     *
     * @param family the address family.
     * @param from_uint8 the pointer to the memory to copy the address value
     * from.
     */
    IPvX(int family, const uint8_t *from_uint8) throw (InvalidFamily);

    /**
     * Constructor from an IPv4 address.
     *
     * Create an IPvX address from an IPv4 address.
     *
     * @param ipv4 the IPv4 address to assign the address value from.
     */
    IPvX(const IPv4& ipv4);

    /**
     * Constructor from an IPv6 address.
     *
     * Create an IPvX address from an IPv6 address.
     *
     * @param ipv6 the IPv6 address to assign the address value from.
     */
    IPvX(const IPv6& ipv6);

    /**
     * Constructor from in_addr structure.
     *
     * Note that this address must be of AF_INET family.
     *
     * @param from_in_addr the storage to copy the address value from.
     */
    IPvX(const in_addr& from_in_addr);

    /**
     * Constructor from in6_addr structure.
     *
     * Note that this address must be of AF_INET6 family.
     *
     * @param from_in6_addr the storage to copy the address value from.
     */
    IPvX(const in6_addr& from_in6_addr);

    /**
     * Constructor from sockaddr structure.
     *
     * @param from_sockaddr the storage to copy the address from.
     */
    IPvX(const sockaddr& from_sockaddr) throw (InvalidFamily);

    /**
     * Constructor from sockaddr_in structure.
     *
     * @param from_sockaddr_in the storage to copy the address from.
     */
    IPvX(const sockaddr_in& from_sockaddr_in) throw (InvalidFamily);

    /**
     * Constructor from sockaddr_in6 structure.
     *
     * @param from_sockaddr_in6 the storage to copy the address from.
     */
    IPvX(const sockaddr_in6& from_sockaddr_in6) throw (InvalidFamily);

    /**
     * Constructor from a string.
     *
     * @param from_cstring C-style string in the IPv4 dotted decimal
     * or IPv6 canonical human-readable format used for initialization.
     */
    IPvX(const char *from_cstring) throw (InvalidString);

    /**
     * Copy the IPvX raw address to specified memory location.
     *
     * @param: to_uint8 the pointer to the memory to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    /**
     * Copy the IPvX raw address to an in_addr structure.
     *
     * Note that this address must be of AF_INET family.
     *
     * @param to_in_addr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(in_addr& to_in_addr) const throw (InvalidFamily);

    /**
     * Copy the IPvX raw address to an in6_addr structure.
     *
     * Note that this address must be of AF_INET6 family.
     *
     * @param to_in6_addr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(in6_addr& to_in6_addr) const throw (InvalidFamily);

    /**
     * Copy the IPvX raw address to a sockaddr structure.
     *
     * Copy the raw address held within an IPvX instance to an sockaddr
     * structure and assign appropriately and set fields within sockaddr
     * appropriately. The underlying address representation may be either
     * IPv4 or IPv6.
     *
     * @param to_sockaddr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr& to_sockaddr) const throw (InvalidFamily);

    /**
     * Copy the IPvX raw address to a sockaddr_in structure.
     *
     * Copy the raw address held within an IPvX instance to an sockaddr_in
     * structure and assign appropriately and set fields within sockaddr_in
     * appropriately. The underlying address representation may be either
     * IPv4 or IPv6.
     *
     * @param to_sockaddr_in the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr_in& to_sockaddr_in) const throw (InvalidFamily);

    /**
     * Copy the IPvX raw address to a sockaddr_in6 structure.
     *
     * Copy the raw address held within an IPvX instance to a sockaddr_in6
     * structure and assign appropriately and set fields within sockaddr_in
     * appropriately. The underlying address representation may be either
     * IPv4 or IPv6.
     *
     * @param to_sockaddr_in6 the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(sockaddr_in6& to_sockaddr_in6) const throw (InvalidFamily);

    /**
     * Copy a raw address of specified family type from specified memory
     * location into IPvX structure.
     *
     * @param family the address family.
     * @param from_uint8 the memory address to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(int family, const uint8_t *from_uint8)
	throw (InvalidFamily);

    /**
     * Copy a raw IPv4 address from a in_addr structure into IPvX structure.
     *
     * @param from_in_addr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const in_addr& from_in_addr);

    /**
     * Copy a raw IPv6 address from a in6_addr structure into IPvX structure.
     *
     * @param from_in6_addr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const in6_addr& from_in6_addr);

    /**
     * Copy a raw address from a sockaddr structure into IPvX structure.
     *
     * Copy a raw address from a sockaddr structure, and set internal
     * address family appropriately.
     *
     * @param from_sockaddr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr& from_sockaddr) throw (InvalidFamily);

    /**
     * Copy a raw address from a sockaddr_in structure into IPvX structure.
     *
     * Copy a raw address from a sockaddr_in structure, and set internal
     * address family appropriately.
     *
     * @param from_sockaddr_in the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const sockaddr_in& from_sockaddr_in) throw (InvalidFamily);

    /**
     * Copy a raw address from sockaddr_in6 structure into IPvX structure.
     *
     * Copy a raw address from sockaddr_in6 structure, and set internal
     * address family appropriately.
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
    IPvX operator~() const;

    /**
     * OR Operator
     *
     * @param other the right-hand operand to OR with.
     * @return bitwise OR of two addresses.
     */
    IPvX operator|(const IPvX& other) const throw (InvalidCast);

    /**
     * AND Operator
     *
     * @param other the right-hand operand to AND with.
     * @return bitwise AND of two addresses.
     */
    IPvX operator&(const IPvX& other) const throw (InvalidCast);

    /**
     * XOR Operator
     *
     * @param other the right-hand operand to XOR with.
     * @return bitwize eXclusive-OR of two addresses.
     */
    IPvX operator^(const IPvX& other) const throw (InvalidCast);

    /**
     * Operator <<
     *
     * @param left_shift the number of bits to shift to the left.
     * @return IPvX address that is shift bitwise to the left.
     */
    IPvX operator<<(uint32_t left_shift) const;

    /**
     * Operator >>
     *
     * @param right_shift the number of bits to shift to the right.
     * @return IPvX address that is shift bitwise to the right.
     */
    IPvX operator>>(uint32_t right_shift) const;

    /**
     * Less-Than Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const IPvX& other) const;

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const IPvX& other) const;

    /**
     * Not-Equal Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically not same as the
     * right-hand operand.
     */
    bool operator!=(const IPvX& other) const;

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
    IPvX& operator--();

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
    IPvX& operator++();

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
    bool is_zero() const;

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
     * Test if this address is a valid loopback address.
     *
     * @return true if this address is a valid loopback address.
     */
    bool is_loopback() const;

    /**
     * Get the address octet-size.
     *
     * Note that this is a static function and is to be used without
     * a particular object. Example:
     *   size_t my_size = IPvX::addr_size(my_family);
     *
     * @param family the address family.
     * @return address size in number of octets for an address of
     * address family of @ref family.
     */
    static size_t addr_size(int family) throw (InvalidFamily);

    /**
     * Get the address octet-size for this address.
     *
     * Note that this is not a static function, hence it has to be used with
     * a particular object. Example:
     *   size_t my_size = ipvx.addr_size();
     *
     * @param family the address family.
     * @return address size in number of octets for this IPvX address.
     */
    size_t addr_size() const { return IPvX::addr_size(_af); }

    /**
     * Get the address bit-length.
     *
     * Note that this is a static function and is to be used without
     * a particular object. Example:
     *   uint32_t my_bitlen = IPvX::addr_bitlen(my_family);
     *
     * @param family the address family.
     * @return address size in number of bits for an address of
     * address family of @ref family.
     */
    inline static uint32_t addr_bitlen(int family) throw (InvalidFamily) {
	return uint32_t(8 * sizeof(uint8_t) * addr_size(family));
    }

    /**
     * Get the address bit-length for this address.
     *
     * Note that this is not a static function, hence it has to be used with
     * a particular object. Example:
     *   uint32_t my_bitlen = ipvx.addr_bitlen();
     *
     * @param family the address family.
     * @return address size in number of bits for this IPvX address.
     */
    inline uint32_t addr_bitlen() const {
	return uint32_t(8 * sizeof(uint8_t) * addr_size());
    }

    /**
     * Get the mask length for the multicast base address.
     *
     * Note that this is a static function and can be used without
     * a particular object. Example:
     *   uint32_t my_len = IPvX::ip_multicast_base_address_mask_len(my_family);
     *
     * @param family the address family.
     * @return the multicast base address mask length for an address of
     * address family of @ref family.
     */
    static uint32_t ip_multicast_base_address_mask_len(int family)
	throw (InvalidFamily);

    /**
     * Get the mask length for the multicast base address for this address.
     *
     * Note that this is not a static function, hence it has to be used with
     * a particular object. Example:
     *   size_t my_len = ipvx.ip_multicast_base_address_mask_len();
     *
     * @param family the address family.
     * @return the multicast base address mask length for this IPvX address.
     */
    uint32_t ip_multicast_base_address_mask_len() const {
	return IPvX::ip_multicast_base_address_mask_len(_af);
    }

    /**
     * Make an IPvX mask prefix.
     *
     * @param family the address family.
     * @param mask_len the length of the mask to create.
     * @return a new IPvX address that contains a mask of length @ref mask_len.
     */
    static IPvX make_prefix(int family, uint32_t mask_len)
	throw (InvalidFamily, InvalidNetmaskLength);

    /**
     * Make an IPvX mask prefix for the address family of this address.
     *
     * @param mask_len the length of the mask to create.
     * @return a new IPvX address that contains a mask of length @ref mask_len.
     */
    IPvX make_prefix(uint32_t mask_len) const throw (InvalidNetmaskLength) {
	return IPvX::make_prefix(_af, mask_len);
    }

    /**
     * Make an IPvX address prefix.
     *
     * @param prefix_len the length of the mask of the prefix to create.
     * @return a new IPvX address created by masking this address with a mask
     * of length @ref prefix_len.
     */
    IPvX mask_by_prefix_len(uint32_t prefix_len) const throw (InvalidNetmaskLength);

    /**
     * Get the mask length.
     *
     * @return the prefix length of the contiguous mask presumably stored
     * as an IPvX address.
     */
    uint32_t mask_len() const;

    /**
     * Test if this address is IPv4 address.
     *
     * @return true if the address is IPv4.
     */
    inline bool is_ipv4() const { return (_af == AF_INET); }

    /**
     * Test if this address is IPv6 address.
     *
     * @return true if the address is IPv6.
     */
    inline bool is_ipv6() const { return (_af == AF_INET6); }

    /**
     * Get IPv4 address.
     *
     * @return IPv4 address contained with IPvX structure.
     */
    inline IPv4 get_ipv4() const throw (InvalidCast);

    /**
     * Get IPv6 address.
     *
     * @return IPv6 address contained with IPvX structure.
     */
    inline IPv6 get_ipv6() const throw (InvalidCast);

    /**
     * Assign address value to an IPv4 address.
     *
     * @param to_ipv4 IPv4 address to be assigned IPv4 value contained
     * within this address.
     */
    inline void get(IPv4& to_ipv4) const throw (InvalidCast) {
	to_ipv4 = get_ipv4();
    }

    /**
     * Assign address value to an IPv6 address.
     *
     * @param to_ipv6 IPv6 address to be assigned IPv4 value contained
     * within this address.
     */
    inline void get(IPv6& to_ipv6) const throw (InvalidCast) {
	to_ipv6 = get_ipv6();
    }

    /**
     * Get the address family.
     *
     * @return the address family of this address (AF_INET or AF_INET6).
     */
    inline int af() const { return (_af); }

    /**
     * Get the IP protocol version.
     *
     * @return the IP protocol version of this address.
     */
    uint32_t ip_version() const throw (InvalidFamily);

    /**
     * Get the human-readable string with the IP protocol version.
     *
     * @return the human-readable string with the IP protocol version of
     * this address.
     */
    const string& ip_version_str() const throw (InvalidFamily);

    /**
     * Extract bits from an address.
     *
     * @param lsb starting bit position (from the right) to extract.
     * @param len number of bits to extract. The maximum value is 32.
     * @return the first @ref len bits starting from the rightmost
     * position @ref lsb. The returned bits are in host order.
     */
    inline uint32_t bits(uint32_t lsb, uint32_t len) const throw (InvalidFamily);

    /**
     * Pre-defined IPvX address constants.
     */
    static const IPvX& ZERO(int family)
	throw (InvalidFamily);
    static const IPvX& ANY(int family)
	throw (InvalidFamily);
    static const IPvX& ALL_ONES(int family)
	throw (InvalidFamily);
    static const IPvX& MULTICAST_BASE(int family)
	throw (InvalidFamily);
    static const IPvX& MULTICAST_ALL_SYSTEMS(int family)
	throw (InvalidFamily);
    static const IPvX& MULTICAST_ALL_ROUTERS(int family)
	throw (InvalidFamily);
    static const IPvX& DVMRP_ROUTERS(int family)
	throw (InvalidFamily);
    static const IPvX& RIP2_ROUTERS(int family)
	throw (InvalidFamily);
    static const IPvX& PIM_ROUTERS(int family)
	throw (InvalidFamily);
    static const IPvX& OSPFIGP_ROUTERS(int family)
	throw (InvalidFamily);
    static const IPvX& OSPFIGP_DESIGNATED_ROUTERS(int family)
	throw (InvalidFamily);

private:
    friend class IPv4;
    friend class IPv6;

    uint32_t _addr[4];	// Underlay address value for casting to IPv4 and IPv6
    int _af;		// The address family AF_INET or AF_INET6
};

inline IPv4
IPvX::get_ipv4() const throw (InvalidCast)
{
    if (_af == AF_INET)
	return IPv4(_addr[0]);
    xorp_throw(InvalidCast, "Miscast as IPv4");
}

inline IPv6
IPvX::get_ipv6() const throw (InvalidCast)
{
    if (_af == AF_INET6)
	return IPv6(&_addr[0]);
    xorp_throw(InvalidCast, "Miscast as IPv6");
}

inline uint32_t
IPvX::bits(uint32_t lsb, uint32_t len) const throw (InvalidFamily)
{
    uint32_t mask = ~(0xffffffffU << len);

    if (len >= 32)
	mask = 0xffffffffU;	// XXX: shifting with >= 32 bits is undefined

    if (_af == AF_INET)
	return ntohl((*this >> lsb)._addr[0]) & mask;
    if (_af == AF_INET6)
	return ntohl((*this >> lsb)._addr[3]) & mask;

    xorp_throw(InvalidFamily, _af);
    return (0x0U);
}

//
// Front-end functions that can be used by C programs.
// TODO: this is not the right place for this.
// We need a system for exporting API to C programs.
//
inline size_t
family2addr_size(const int family)
{
    return IPvX::addr_size(family);
}

inline uint32_t
family2addr_bitlen(const int family)
{
    return IPvX::addr_bitlen(family);
}

#endif // __LIBXORP_IPVX_HH__
