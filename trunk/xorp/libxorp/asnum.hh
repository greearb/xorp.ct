// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/libxorp/asnum.hh,v 1.3 2003/01/28 03:21:52 rizzo Exp $

#ifndef __LIBXORP_ASNUM_HH__
#define __LIBXORP_ASNUM_HH__

#include "config.h"
#include <sys/types.h>
#include <inttypes.h>
#include <string>
#include "c_format.hh"

/**
 * @short A class for storing an AS number used by protocols such as BGP
 * 
 * This class can be used to store an AS number that can be either
 * 16 or 32 bits.  Originally, the AS numbers were defined as 16-bit
 * unsigned numbers.  Later the "extended" AS numbers were introduced,
 * which are unsigned 32-bit numbers.
 *
 * 16-bit numbers are expanded to 32-bit by extending them with 0's in front.
 * 32-bit numbers are represented in a 16-bit path, by a special 16-bit value,
 * AS_TRAN, which will be allocated by IANA.
 * Together with any AsPath containing AS_TRAN, we will always see a NEW_AS_PATH
 * attribute which contains the full 32-bit representation of the path.
 * So there is no loss of information.
 *
 * The internal representation of an AsNum is 32-bit in host order.
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
	// XXX remove when we support 32-bit AS
	assert(value < 0xffff);
    }
 
    explicit AsNum(const uint16_t value) : _as(value)	{}

    /**
     * construct from a 2-byte buffer in memory
     */
    explicit AsNum(const uint8_t *d)			{
	_as = (d[0] << 8) + d[1];
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
    uint32_t as32() const				{ return _as; }

    /**
     * copy the 16-bit value into a 2-byte memory buffer
     */
    void copy_out(uint8_t *d) const			{
	uint16_t x = as();
	d[0] = (x >> 8) & 0xff;
	d[1] = x & 0xff;
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
	return c_format("AS/%d", _as);
    }
    
private:
    uint32_t _as;		// The value of the AS number

    AsNum(); // forbidden
    AsNum(int); // forbidden

};
#endif // __LIBXORP_ASNUM_HH__
