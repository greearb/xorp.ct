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

// $XORP: xorp/libxorp/timeval.hh,v 1.9 2003/03/31 23:04:24 pavlin Exp $

#ifndef __LIBXORP_TIMEVAL_HH__
#define __LIBXORP_TIMEVAL_HH__

#include "xorp.h"

#include <math.h>
#include <sys/time.h>


#define ONE_MILLION	1000000

/**
 * @short TimeVal class
 *
 * TimeVal class is used for storing time value. Similar to "struct timeval",
 * the time value is in seconds and microseconds.
 */
class TimeVal {
public:
    /**
     * Default constructor
     */
    TimeVal() : _sec(0), _usec(0) {}
    
    /**
     * Constructor for given seconds and microseconds.
     * 
     * @param sec the number of seconds.
     * @param usec the number of microseconds.
     */
    TimeVal(uint32_t sec, uint32_t usec) : _sec(sec), _usec(usec) {}
    
    /**
     * Constructor for given "struct timeval".
     * 
     * @param timeval the "struct timeval" time value to initialize this
     * object with.
     */
    explicit TimeVal(const struct timeval& timeval)
	: _sec(timeval.tv_sec), _usec(timeval.tv_usec) {}
    
    /**
     * Constructor for given double-float time value.
     * 
     * @param d the double-float time value to initialize this object with.
     */
    explicit inline TimeVal(const double& d);
    
    /**
     * Get the number of seconds.
     * 
     * @return the number of seconds.
     */
    uint32_t sec() const	{ return _sec; }
    
    /**
     * Get the number of microseconds.
     * 
     * @return the number of microseconds.
     */
    uint32_t usec() const	{ return _usec; }
    
    /**
     * Set the time value.
     * 
     * @param sec the number of seconds.
     * @param usec the number of microseconds.
     */
    void set(uint32_t sec, uint32_t usec) { _sec = sec; _usec = usec; }
    
    /**
     * Set the time value to zero.
     */
    void clear() { _sec = 0; _usec = 0; }
    
    /**
     * Set the time value to its maximum possible value.
     */
    void set_max() { _sec = ~0, _usec = ONE_MILLION - 1; }
    
    /**
     * Copy the time value from a timeval structure.
     * 
     * @param timeval the storage to copy the time from.
     * @return the number of copied octets.
     */
    inline size_t copy_in(const struct timeval& timeval);
    
    /**
     * Copy the time value to a timeval structure.
     * 
     * @param timeval the storage to copy the time to.
     * @return the number of copied octets.
     */
    inline size_t copy_out(struct timeval& timeval) const;
    
    /**
     * Convert a TimeVal value to a double-float value.
     * 
     * @return the double-float value of this TimeVal time.
     */
    double get_double() const { return (_sec * 1.0 + _usec * 1.0e-6); }
    
    /**
     * Apply uniform randomization on the time value of this object.
     * 
     * The randomized time value is chosen randomly uniform in the interval
     * ( curr_value - factor*curr_value, curr_value + factor*curr_value ).
     * For example, if the current time value is 10 seconds,
     * and @param factor is 0.2, the randomized time value is uniformly
     * chosen in the interval (8, 12) seconds (10 +- 0.2*10).
     * If the value of @param factor is larger than 1.0, the minimum
     * time value after the randomization is zero (sec, usec), even though
     * the beginning of the interval to randomize within has a negative value.
     * 
     * @param factor the randomization factor to apply.
     * @return the randomized time value.
     */
    inline const TimeVal& randomize_uniform(const double& factor);
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    inline bool operator==(const TimeVal& other) const;
    
    /**
     * Less-Than Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    inline bool operator<(const TimeVal& other) const;
    
    /**
     * Assign-Sum Operator
     * 
     * @param delta the TimeVal value to add to this TimeVal object.
     * @return the TimeVal value after the addition of @ref delta.
     */
    inline const TimeVal& operator+=(const TimeVal& delta);
    
    /**
     * Addition Operator
     * 
     * @param other the TimeVal value to add to the value of this
     * TimeVal object.
     * @return the TimeVal value after the addition of @ref other. 
     */
    inline TimeVal operator+(const TimeVal& other) const;
    
    /**
     * Assign-Difference Operator
     * 
     * @param delta the TimeVal value to substract from this TimeVal object.
     * @return the TimeVal value after the substraction of @ref delta.
     */
    inline const TimeVal& operator-=(const TimeVal& delta);
    
    /**
     * Substraction Operator
     * 
     * @param other the TimeVal value to substract from the value of this
     * TimeVal object.
     * @return the TimeVal value after the substraction of @ref other.
     */
    inline TimeVal operator-(const TimeVal& other) const;

    /**
     * Multiplication Operator for integer operand
     * 
     * @param n the integer value used in multiplying the value of this
     * object with.
     * @return the TimeVal value of multiplying the value of this object
     * by @ref n.
     */
    inline TimeVal operator*(int n) const;

    /**
     * Multiplication Operator for double float operand
     * 
     * @param d the double float value used in multiplying the value of this
     * object with.
     * @return the TimeVal value of multiplying the value of this object
     * by @ref d.
     */
    inline TimeVal operator*(const double& d) const;
    
    /**
     * Division Operator for integer operand
     * 
     * @param n the integer value used in dividing the value of this
     * object with. 
     * @return the TimeVal value of dividing the value of this object
     * by @ref n.
     */
    inline TimeVal operator/(int n) const;

    /**
     * Division Operator for double-float operand
     * 
     * @param d the double-float value used in dividing the value of this
     * object with. 
     * @return the TimeVal value of dividing the value of this object
     * by @ref d.
     */
    inline TimeVal operator/(const double& d) const;
    
private:
    uint32_t _sec;		// The number of seconds
    uint32_t _usec;		// The number of microseconds
};

inline
TimeVal::TimeVal(const double& d)
    : _sec((uint32_t)d),
      _usec((uint32_t)((d - ((double)_sec)) * ONE_MILLION + 0.5e-6))
{
    //
    // Adjust
    //
    if (_usec >= ONE_MILLION) {
	_sec += _usec / ONE_MILLION;
	_usec %= ONE_MILLION;
    }
}

inline size_t
TimeVal::copy_in(const struct timeval& timeval)
{
    _sec = timeval.tv_sec;
    _usec = timeval.tv_usec;
    return (sizeof(_sec) + sizeof(_usec));
}

inline size_t
TimeVal::copy_out(struct timeval& timeval) const
{
    timeval.tv_sec = _sec;
    timeval.tv_usec = _usec;
    return (sizeof(_sec) + sizeof(_usec));
}

inline const TimeVal&
TimeVal::randomize_uniform(const double& factor)
{
    TimeVal delta(factor * get_double());
    uint32_t random_sec = delta.sec();
    uint32_t random_usec = delta.usec();
    
    // Randomize the offset
    if (random_sec != 0)
	random_sec = random() % random_sec;
    if (random_usec != 0)
	random_usec = random() % random_usec;
    TimeVal random_delta(random_sec, random_usec);
    
    // Either add or substract the randomized offset
    if (random() % 2) {
	// Add
	*this += random_delta;
    } else {
	// Substract
	if (random_delta < *this) {
	    *this -= random_delta;
	} else {
	    // The offset is too large. Set the result to zero.
	    _sec = 0;
	    _usec = 0;
	}
    }
    
    return (*this);
}

inline bool
TimeVal::operator==(const TimeVal& other) const
{
    return (_sec == other.sec()) && (_usec == other.usec());
}

inline bool
TimeVal::operator<(const TimeVal& other) const
{
    return (_sec == other.sec()) ? _usec < other.usec() : _sec < other.sec();
}

inline const TimeVal&
TimeVal::operator+=(const TimeVal& delta)
{
    _sec += delta.sec();
    _usec += delta.usec();
    if (_usec >= ONE_MILLION) {
	_sec++;
	_usec -= ONE_MILLION;
    }
    return (*this);
}

inline TimeVal
TimeVal::operator+(const TimeVal& other) const
{
    TimeVal tmp_tv(*this);
    return tmp_tv += other;
}

inline const TimeVal&
TimeVal::operator-=(const TimeVal& delta)
{
    _sec -= delta.sec();
    if (_usec < delta.usec()) {
	// Compensate
	_sec--;
	_usec += ONE_MILLION;
    }
    _usec -= delta.usec();
    
    return (*this);
}

inline TimeVal
TimeVal::operator-(const TimeVal& other) const
{
    TimeVal tmp_tv(*this);
    return tmp_tv -= other;
}

inline TimeVal
TimeVal::operator*(int n) const
{
    uint32_t tmp_sec, tmp_usec;
    
    tmp_usec = _usec * n;
    tmp_sec = _sec * n + tmp_usec / ONE_MILLION;
    tmp_usec %= ONE_MILLION;
    return TimeVal(tmp_sec, tmp_usec);
}

inline TimeVal
TimeVal::operator*(const double& d) const
{
    return TimeVal(get_double() * d);
}

inline TimeVal
TimeVal::operator/(int n) const
{
    return TimeVal(_sec / n, ((_sec % n) * ONE_MILLION + _usec) / n);
}

inline TimeVal
TimeVal::operator/(const double& d) const
{
    return TimeVal(get_double() / d);
}

#endif // __LIBXORP_TIMEVAL_HH__
