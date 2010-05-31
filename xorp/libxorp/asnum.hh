// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/asnum.hh,v 1.20 2008/10/02 21:57:28 bms Exp $

#ifndef __LIBXORP_ASNUM_HH__
#define __LIBXORP_ASNUM_HH__

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "c_format.hh"


/**
 * @short A class for storing an AS number used by protocols such as BGP
 * 
 * This class can be used to store an AS number that can be either 16
 * or 32 bits.  Originally, the AS numbers were defined as 16-bit
 * unsigned numbers.  Later the "extended" AS numbers were introduced,
 * which are unsigned 32-bit numbers.  Conventional terminology refers
 * to the 32-bit version as 4-byte AS numbers rather than 32-bit AS
 * numbers, so we'll try and stick with that where it makes sense.
 *
 * 2-byte numbers are expanded to 32-bits by extending them with 0's in front.
 * 4-byte numbers are represented in a 2-byte AS path, by a special
 * 16-bit value, AS_TRAN, which will be allocated by IANA.
 * Together with any AsPath containing AS_TRAN, we will always see a
 * AS4_PATH attribute which contains the full 32-bit representation
 * of the path.  So there is no loss of information.
 *
 * IANA refers to NEW_AS_PATH, but the latest internet drafts refer to
 * AS4_PATH.  They're the same thing, but I the latter is preferred so
 * we'll use that.
 *
 * The internal representation of an AsNum is 32-bit in host order.
 *
 * The canonical string form of a 4-byte AS number is <high>.<low>, so
 * decimal 65536 ends up being printed as "1.0".
 *
 * An AsNum must always be initialized, so the default constructor
 * is never called.
 */
class AsNum {
public:
    static const uint16_t AS_INVALID = 0;	// XXX IANA-reserved
    static const uint16_t AS_TRAN = 23456;	// IANA

    /**
     * Constructor.
     * @param value the value to assign to this AS number.
     */
    explicit AsNum(const uint32_t value) : _as(value)	{
    }
 
    explicit AsNum(const uint16_t value) : _as(value)	{}

    explicit AsNum(int value)				{
	assert(value >= 0 && value <= 0xffff);
	_as = value;
    }

    /**
     * construct from a 2-byte buffer in memory
     */
    explicit AsNum(const uint8_t *d)			{
	_as = (d[0] << 8) + d[1];
    }

    /**
     * construct from a 2-byte buffer in memory or a 4 byte buffer (in
     * net byte order).
     *
     * The 4byte parameter is mostly to distinguish this from the
     * 2-byte constructor above.
     */
    explicit AsNum(const uint8_t *d, bool fourbyte)	{
	if (fourbyte) {
	    // 4 byte version
	    memcpy(&_as, d, 4);
	    _as = htonl(_as);
	} else {
	    // 2 byte version
	    _as = (d[0] << 8) + d[1];
	}
    }

    /**
     * construct from a string, either as a decimal number in the
     * range 1-65535, or as two decimal numbers x.y, where x and y are
     * in the range 0-65535 
     */
    explicit AsNum(const string& as_str) throw(InvalidString) {
	bool four_byte = false;
	bool seen_digit = false;
	for (uint32_t i = 0; i < as_str.size(); i++) {
	    if (as_str[i] == '.') {
		if (four_byte || seen_digit == false) {
		    // more than one dot, or no number before the first dot.
		    xorp_throw(InvalidString, c_format("Bad AS number \"%s\"",
						       as_str.c_str()));
		} else {
		    four_byte = true;
		    seen_digit = false;
		}
	    } else if (!isdigit(as_str[i])) {
		// got some disallowed character
		xorp_throw(InvalidString, c_format("Bad AS number \"%s\"",
						   as_str.c_str()));
	    } else {
		seen_digit = true;
	    }
	}
	if (seen_digit == false) {
	    // either no digit here, or no digit after the dot
	    xorp_throw(InvalidString, c_format("Bad AS number \"%s\"",
					       as_str.c_str()));
	}
	
	// got here, so the text is in the right format
	if (!four_byte) {
	    _as = atoi(as_str.c_str());
	    if (_as < 1 || _as > 65535) {
		// out of range
		xorp_throw(InvalidString, c_format("Bad AS number \"%s\"",
					       as_str.c_str()));
	    }
	} else {
	    uint32_t upper = strtoul(as_str.c_str(), NULL, 10);
	    uint32_t lower = strtoul(strchr(as_str.c_str(), '.') + 1, 
				     NULL, 10);
	    if  (upper > 65535 || lower > 65535) {
		// out of range
		xorp_throw(InvalidString, c_format("Bad AS number \"%s\"",
					       as_str.c_str()));
	    }
	    _as = (upper << 16) | lower;
	}
    }


    /**
     * Get the non-extended AS number value.
     * 
     * @return the non-extended AS number value.
     */
    uint16_t as() const					{
	return extended() ? AS_TRAN : _as;
    }

    /**
     * Get the extended AS number value.
     * 
     * @return the extended AS number value.
     */
    uint32_t as4() const				{ return _as; }

    /**
     * copy the 16-bit value into a 2-byte memory buffer
     */
    void copy_out(uint8_t *d) const			{
	uint16_t x = as();
	d[0] = (x >> 8) & 0xff;
	d[1] = x & 0xff;
    }

    /**
     * copy the 32-bit value into a 4-byte network byte order memory buffer
     */
    void copy_out4(uint8_t *d) const			{
	// note - buffer may be unaligned, so use memcpy
	uint32_t x = htonl(_as);
	memcpy(d, &x, 4);
    }

    /**
     * Test if this is an extended AS number.
     * 
     * @return true if this is an extended AS number.
     */
    bool extended() const				{ return _as>0xffff;};
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const AsNum& x) const		{ return _as == x._as; }

    /**
     * Less-Than Operator
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const AsNum& x) const		{ return _as < x._as; }
    
    /**
     * Convert this AS number from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the AS number.
     */
    string str() const					{
	if (extended())
	    return c_format("AS/%u.%u", (_as >> 16)&0xffff, _as&0xffff);
	else 
	    return c_format("AS/%u", XORP_UINT_CAST(_as));
    }

    string short_str() const					{
	if (extended())
	    return c_format("%u.%u", (_as >> 16)&0xffff, _as&0xffff);
	else
	    return c_format("%u", XORP_UINT_CAST(_as));
    }

    string fourbyte_str() const {
	// this version forces the long canonical format
	return c_format("%u.%u", (_as >> 16)&0xffff, _as&0xffff);
    }
    
private:
    uint32_t _as;		// The value of the AS number

    AsNum(); // forbidden
};
#endif // __LIBXORP_ASNUM_HH__
