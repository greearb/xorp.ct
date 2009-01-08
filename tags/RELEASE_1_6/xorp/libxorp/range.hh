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

// $XORP: xorp/libxorp/range.hh,v 1.11 2008/10/02 21:57:32 bms Exp $

#ifndef __LIBXORP_RANGE_HH__
#define __LIBXORP_RANGE_HH__

#include <sstream>

class IPv4;
class IPv6;

/**
 * @short A template class for implementing linear ranges X..Y
 *
 * We define a linear range by its low and high inclusive boundaries.
 * It's the user's responisibility to ensure that the condition
 * (_low <= _high) always holds!
 */
template <class T>
class Range {
public:
    /**
     * Default constructor
     */
    Range() 			{}

    /**
     * Constructor from a single value.
     */
    explicit Range(T value)	{ _low = _high = value; }

    /**
     * Constructor from two values.
     */
    explicit Range(T low, T high) {
	_low = low;
	_high = high;
    }

    const T& low() const { return _low; }
    const T& high() const { return _high; }

protected:
    T _low;
    T _high;
};

/**
 * @short A linear range class (uint32_t low)..(uint32_t high)
 *
 * Inherits from templatized general Range<uint32_t> class.
 * Provides specialized constructor from string and str() method.
 */
class U32Range: public Range<uint32_t> {
public:
    /**
     * Default constructor
     */
    U32Range() 			{ Range<uint32_t>::_low = 
    				   Range<uint32_t>::_high = 0; }

    /**
     * Constructor from a string.
     */
    U32Range(const char *from_cstr) {
	string from_string = string(from_cstr);
	string::size_type delim = from_string.find("..", 0);
	if (delim == string::npos) {
	    _low = _high = strtoul(from_cstr, NULL, 10);
	} else if (delim > 0 && (from_string.length() - delim > 2)) {
	    _low = strtoul(from_string.substr(0, delim).c_str(), NULL, 10);
	    _high = strtoul(from_string.substr(delim + 2, from_string.length()).c_str(), NULL, 10);
	} else {
	    xorp_throw(InvalidString, "Syntax error");
	}
    }

    /**
     * Convert the range to a human-readable format.
     *
     * @return C++ string.
     */
    string str() const {
	ostringstream os;
	os << _low;
	if (_low < _high)
	    os << ".." << _high;
	return os.str();
    }
};

/**
 * Equality Operator for @ref uint32_t against @ref U32Range operand.
 *
 * @param lhs the left-hand @ref uint32_t type operand.
 * @param rhs the right-hand @ref U32Range operand.
 * @return true if the value of the left-hand operand falls inside
 * the range defined by the right-hand operand.
 */
inline bool operator==(const uint32_t& lhs, const U32Range& rhs) {
    return (lhs >= rhs.low() && lhs <= rhs.high());
}


/**
 * Non-equality Operator for @ref uint32_t against @ref U32Range operand.
 *
 * @param lhs the left-hand @ref uint32_t type operand.
 * @param rhs the right-hand @ref U32Range operand.
 * @return true if the value of the left-hand operand falls outside
 * the range defined by the right-hand operand.
 */
inline bool operator!=(const uint32_t& lhs, const U32Range& rhs) {
    return (lhs < rhs.low() || lhs > rhs.high());
}

/**
 * Less-than comparison for @ref uint32_t against @ref U32Range operand.
 *
 * @param lhs the left-hand @ref uint32_t type operand.
 * @param rhs the right-hand @ref U32Range operand.
 * @return true if the value of the left-hand operand is bellow
 * the range defined by the right-hand operand.
 */
inline bool operator<(const uint32_t& lhs, const U32Range& rhs) {
    return (lhs < rhs.low());
}

/**
 * Less-than or equal comparison for @ref uint32_t against @ref U32Range
 *
 * @param lhs the left-hand @ref uint32_t type operand.
 * @param rhs the right-hand @ref U32Range operand.
 * @return true if the value of the left-hand operand is bellow or within
 * the range defined by the right-hand operand.
 */
inline bool operator<=(const uint32_t& lhs, const U32Range& rhs) {
    return (lhs <= rhs.high());
}

/**
 * Greater-than comparison for @ref uint32_t against @ref U32Range operand.
 *
 * @param lhs the left-hand @ref uint32_t type operand.
 * @param rhs the right-hand @ref U32Range operand.
 * @return true if the value of the left-hand operand is above
 * the range defined by the right-hand operand.
 */
inline bool operator>(const uint32_t& lhs, const U32Range& rhs) {
    return (lhs > rhs.high());
}

/**
 * Greater-than or equal comparison for @ref uint32_t against @ref U32Range
 *
 * @param lhs the left-hand @ref uint32_t type operand.
 * @param rhs the right-hand @ref U32Range operand.
 * @return true if the value of the left-hand operand is above or within
 * the range defined by the right-hand operand.
 */
inline bool operator>=(const uint32_t& lhs, const U32Range& rhs) {
    return (lhs >= rhs.low());
}


/**
 * @short A linear IPvX class template (IPvX low)..(IPvX high)
 *
 * Inherits from templatized general Range<IPv4|IPv6> class.
 * Provides a specialized constructor from string and str() method.
 */
template <class T>
class IPvXRange: public Range<T> {
public:
    /**
     * Default constructor
     */
    IPvXRange()			{}

    /**
     * Constructor from a string.
     */
    IPvXRange(const char *from_cstr) {
	string from_string = string(from_cstr);
	string::size_type delim = from_string.find("..", 0);
	if (delim == string::npos)
	    Range<T>::_low = Range<T>::_high = T(from_cstr);
	else if (delim > 0 && (from_string.length() - delim > 2)) {
	    Range<T>::_low = T(from_string.substr(0, delim).c_str());
	    Range<T>::_high = T(from_string.substr(delim + 2, 
	    					   from_string.length())
						    .c_str());
	} else {
	    xorp_throw(InvalidString, "Syntax error");
	}
    }

    /**
     * Convert the range to a human-readable format.
     *
     * @return C++ string.
     */
    string str() const {
	ostringstream os;
	os << Range<T>::_low.str();
	if (Range<T>::_low < Range<T>::_high)
	    os << ".." << Range<T>::_high.str();
	return os.str();
    }
};

typedef IPvXRange<IPv4> IPv4Range;
typedef IPvXRange<IPv6> IPv6Range;

#endif // __LIBXORP_RANGE_HH__
