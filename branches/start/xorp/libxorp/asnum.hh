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

// $XORP: xorp/libxorp/asnum.hh,v 1.11 2002/12/09 18:29:10 hodson Exp $

#ifndef __LIBXORP_ASNUM_HH__
#define __LIBXORP_ASNUM_HH__

#include "config.h"
#include <sys/types.h>
#include <inttypes.h>
#include <string>

/**
 * @short A class for storing an AS number used by protocols such as BGP
 * 
 * This class can be used to store an AS number that can be either
 * 16 or 32 bits.  Originally, the AS numbers were defined as 16-bit
 * unsigned numbers.  Later the "extended" AS numbers were introduced,
 * which are unsigned 32-bit numbers.
 */
class AsNum {
public:
    /**
     * Default constructor
     * 
     * The AS number is set to a value that is invalid: 0xffffffffU,
     * but defined as non-extended.
     */
    AsNum();
    
    /**
     * Constructor for a non-extended AS number.
     * 
     * @param value the value to assign to this AS number.
     */
    AsNum(uint16_t value);
    
    /**
     * Constructor for an extended AS number.
     * 
     * @param value the value to assign to this AS number.
     */
    AsNum(uint32_t value);
    
    /**
     * Set the AS number to a non-extended value.
     * 
     * @param value the value to assign to this AS number.
     */
    void set_asnum(uint16_t value);

    /**
     * Set the AS number to an extended value.
     * 
     * @param value the value to assign to this AS number.
     */
    void set_asnum(uint32_t value);

    /**
     * Get the non-extended AS number value.
     * 
     * @return the non-extended AS number value.
     */
    uint16_t as() const { force_valid(); return  (uint16_t)_asnum; }
    
    /**
     * Get the extended AS number value.
     * 
     * @return the extended AS number value.
     */
    uint32_t as_extended() const { force_valid(); return _asnum; }
    
    /**
     * Test if this is an extended AS number.
     * 
     * @return true if this is an extended AS number.
     */
    bool is_extended() const { return _is_extended; };
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const AsNum& other) const;
    
    /**
     * Less-Than Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const AsNum& other) const;
    
    /**
     * Test if this AS number is valid.
     * 
     * @return true if this AS number is valid.
     */
    inline bool is_valid() const {
	return ((_is_extended == true) || (_asnum <= 0xffff));
    }
    
    /**
     * Convert this AS number from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the AS number.
     */
    string str() const;
    
private:
    inline void force_valid() const { if (is_valid() == false ) abort(); }
    
    uint32_t _asnum;		// The value of the AS number
    bool _is_extended;		// True if the AS number is extended
};

#endif // __LIBXORP_ASNUM_HH__
