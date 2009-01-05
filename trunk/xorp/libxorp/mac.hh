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

// $XORP: xorp/libxorp/mac.hh,v 1.32 2008/10/10 00:38:20 pavlin Exp $

#ifndef __LIBXORP_MAC_HH__
#define __LIBXORP_MAC_HH__

#include "libxorp/xorp.h"
#include "libxorp/ether_compat.h"
#include "libxorp/exceptions.hh"


/**
 * @short IEEE standard 48-bit address.
 */
class Mac {
public:
    /**
     * Default constructor.
     */
    Mac();

    /**
     * Constructor from a (uint8_t *) memory pointer.
     *
     * @param from_uint8 the pointer to the memory to copy the address value
     * from.
     */
    explicit Mac(const uint8_t* from_uint8);

    /**
     * Constructor from a string.
     *
     * @param from_cstring C-style string of the form XX:XX:XX:XX:XX:XX
     * where X represents a hex-digit.
     * @throws InvalidString if string passed does not match expected format.
     */
    Mac(const char* from_cstring) throw (InvalidString);

    /**
     * Constructor from ether_addr structure.
     *
     * @param from_ether_addr the ether_addr structure to construct the
     * Mac address from.
     */
    Mac(const struct ether_addr& from_ether_addr);

    /**
     * Constructor from sockaddr structure.
     *
     * @param from_sockaddr the sockaddr structure to construct the
     * Mac address from.
     */
    Mac(const struct sockaddr& from_sockaddr);

    /**
     * Copy the Mac raw address to specified memory location.
     *
     * @param: to_uint8 the pointer to the memory to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(uint8_t* to_uint8) const;

    /**
     * Copy the Mac raw address to ether_addr structure.
     *
     * @param to_ether_addr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(struct ether_addr& to_ether_addr) const;

    /**
     * Copy the Mac raw address to sockaddr structure.
     *
     * @param to_sockaddr the storage to copy the address to.
     * @return the number of copied octets.
     */
    size_t copy_out(struct sockaddr& to_sockaddr) const;

    /**
     * Copy a raw Mac address from specified memory location into
     * Mac container.
     *
     * @param from_uint8 the memory address to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const uint8_t* from_uint8);

    /**
     * Copy a raw Mac address from ether_addr structure into Mac container.
     *
     * @param from_ether_addr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const struct ether_addr& from_ether_addr);

    /**
     * Copy a raw Mac address from sockaddr structure into Mac container.
     *
     * @param from_sockaddr the storage to copy the address from.
     * @return the number of copied octets.
     */
    size_t copy_in(const struct sockaddr& from_sockaddr);

    /**
     * Copy a Mac address from a string.
     *
     * @param from_cstring C-style string of the form XX:XX:XX:XX:XX:XX
     * where X represents a hex-digit.
     * @throws InvalidString if string passed does not match expected format.
     */
    size_t copy_in(const char* from_cstring) throw (InvalidString);

    /**
     * Less-Than Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const Mac& other) const;

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const Mac& other) const;

    /**
     * Not-Equal Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically not same as the
     * right-hand operand.
     */
    bool operator!=(const Mac& other) const;

    /**
     * Convert this address from binary form to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the address.
     */
    string str() const;

    /**
     * Get the size of the raw Mac address (in octets).
     *
     * @return the size of the raw Mac address (in octets).
     */
    static size_t addr_bytelen() { return (ADDR_BYTELEN); }

    /**
     * Get the size of the raw Mac address (in number of bits).
     *
     * @return the size of the raw Mac address (in number of bits).
     */
    static uint32_t addr_bitlen() { return (ADDR_BITLEN); }

    /**
     * Test if this address is numerically zero.
     *
     * @return true if the address is numerically zero.
     */
    bool is_zero() const	{ return *this == ZERO(); }

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
     * Number of bits in address as a constant.
     */
    static const uint32_t ADDR_BITLEN = 48;

    /**
     * Number of bytes in address as a constant.
     */
    static const uint32_t ADDR_BYTELEN = ADDR_BITLEN / 8;

    /**
     * The multicast bit in the first octet of the address.
     */
    static const uint8_t MULTICAST_BIT = 0x1;

    /**
     * Pre-defined Mac address constants.
     */
    static const Mac& ZERO();
    static const Mac& ALL_ONES();
    static const Mac& BROADCAST();
    static const Mac& STP_MULTICAST();
    static const Mac& LLDP_MULTICAST();
    static const Mac& GMRP_MULTICAST();
    static const Mac& GVRP_MULTICAST();

private:
    uint8_t	_addr[ADDR_BYTELEN];	// The address value (in network-order)
};

struct MacConstants {
    static const Mac zero,
	all_ones,
	broadcast,
	stp_multicast,
	lldp_multicast,
	gmrp_multicast,
	gvrp_multicast;
};

inline const Mac& Mac::ZERO() {
    return MacConstants::zero;
}

inline const Mac& Mac::ALL_ONES() {
    return MacConstants::all_ones;
}

inline const Mac& Mac::BROADCAST() {
    return MacConstants::broadcast;
}

inline const Mac& Mac::STP_MULTICAST() {
    return MacConstants::stp_multicast;
}

inline const Mac& Mac::LLDP_MULTICAST() {
    return MacConstants::lldp_multicast;
}

inline const Mac& Mac::GMRP_MULTICAST() {
    return MacConstants::gmrp_multicast;
}

inline const Mac& Mac::GVRP_MULTICAST() {
    return MacConstants::gvrp_multicast;
}

#endif // __LIBXORP_MAC_HH__
