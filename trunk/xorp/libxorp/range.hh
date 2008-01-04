// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxorp/range.hh,v 1.8 2007/02/16 22:46:22 pavlin Exp $

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
