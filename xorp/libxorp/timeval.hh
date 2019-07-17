// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __LIBXORP_TIMEVAL_HH__
#define __LIBXORP_TIMEVAL_HH__

#include "xorp.h"
#include "random.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <math.h>


// TODO:  Move this out-of-line (let compiler make the decision)
// TODO:  Allow setting values, and remove return-by-value where possible.

/**
 * @short TimeVal class
 *
 * TimeVal class is used for storing time value. Similar to "struct timeval",
 * the time value is in seconds and microseconds.
 */
class TimeVal {
public:
    static const int32_t ONE_MILLION = 1000000;
    static const int32_t ONE_THOUSAND = 1000;
#ifdef HOST_OS_WINDOWS
    /*
     * The difference between the beginning of the Windows epoch
     * (1601-01-01-00-00) and the UNIX epoch (1970-01-01-00-00)
     * is 134,774 days on the Julian calendar. Compute this
     * difference in seconds and use it as a constant.
     * XXX: This does not take account of leap seconds.
     */
    static const int64_t UNIX_WIN32_EPOCH_DIFF = (134774LL * 24LL * 60LL * 60LL);
    static const int32_t UNITS_100NS_PER_1US = 10;
#endif

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
    TimeVal(int32_t sec, int32_t usec) : _sec(sec), _usec(usec) {}

#ifndef HOST_OS_WINDOWS

    /**
     * Constructor for given "struct timeval".
     *
     * @param timeval the "struct timeval" time value to initialize this
     * object with.
     */
    explicit TimeVal(const timeval& timeval)
	: _sec(timeval.tv_sec), _usec(timeval.tv_usec) {}

#ifdef HAVE_STRUCT_TIMESPEC
    /**
     * Constructor for given POSIX "struct timespec".
     *
     * @param timespec the "struct timespec" time value to initialize this
     * object with.
     */
    explicit TimeVal(const timespec& timespec)
	: _sec(timespec.tv_sec), _usec(timespec.tv_nsec / 1000) {}
#endif

#else /* HOST_OS_WINDOWS */

    /**
     * Constructor for given "FILETIME".
     *
     * @param ft the "FILETIME" time value to initialize this object with.
     */
    explicit TimeVal(const FILETIME& ft) { copy_in(ft); }

#endif /* !HOST_OS_WINDOWS */

    /**
     * Constructor for given double-float time value.
     *
     * @param d the double-float time value to initialize this object with.
     */
    explicit TimeVal(const double& d);

    /* copy-construtor */
    TimeVal(const TimeVal& tv);

    /**
     * Get the number of seconds.
     *
     * @return the number of seconds.
     */
    int32_t sec() const		{ return _sec; }

    /**
     * Get the number of microseconds.
     *
     * @return the number of microseconds.
     */
    int32_t usec() const	{ return _usec; }

    /**
     * @return seconds and microseconds as a string.
     */
    string str() const;

    /**
     * Pretty print the time
     *
     * @return the time as formated by ctime(3) without the newline.
     */
    string pretty_print() const;

    /**
     * Get zero value.
     */
    static TimeVal ZERO();

    /**
     * Get the maximum permitted value.
     */
    static TimeVal MAXIMUM();

    /**
     * Get the minimum permitted value.
     */
    static TimeVal MINIMUM();

    /**
     * Copy the time value from a timeval structure.
     *
     * @param timeval the storage to copy the time from.
     * @return the number of copied octets.
     */
    size_t copy_in(const timeval& timeval);

    /**
     * Copy the time value to a timeval structure.
     *
     * @param timeval the storage to copy the time to.
     * @return the number of copied octets.
     */
    size_t copy_out(timeval& timeval) const;

#ifdef HAVE_STRUCT_TIMESPEC
    /**
     * Copy the time value from a POSIX timespec structure.
     *
     * @param timespec the storage to copy the time from.
     * @return the number of copied octets.
     */
    size_t copy_in(const timespec& timespec);

    /**
     * Copy the time value to a POSIX timespec structure.
     *
     * @param timespec the storage to copy the time to.
     * @return the number of copied octets.
     */
    size_t copy_out(timespec& timespec) const;
#endif

    /**
     * Return an int64_t containing the total number of milliseconds
     * in the underlying structure.
     * This is intended for convenience when working with Win32 APIs.
     * XXX: This may overflow if _sec is too big.
     *
     * @return the number of milliseconds in total.
     */
    int64_t to_ms() const;

    void set_ms(int64_t ms);

#ifdef HOST_OS_WINDOWS
    /**
     * Copy the time value from a FILETIME structure.
     *
     * @param filetime the storage to copy the time from.
     * @return the number of copied octets.
     */
    size_t copy_in(const FILETIME& filetime);

    /**
     * Copy the time value to a FILETIME structure.
     *
     * @param filetime the storage to copy the time to.
     * @return the number of copied octets.
     */
    size_t copy_out(FILETIME& filetime) const;

#endif /* HOST_OS_WINDOWS */

    /**
     * Convert a TimeVal value to a double-float value.
     *
     * @return the double-float value of this TimeVal time.
     */
    double get_double() const { return (_sec * 1.0 + _usec * 1.0e-6); }

    /**
     * Assignment Operator
     */
    TimeVal& operator=(const TimeVal& other);

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const TimeVal& other) const;

    /**
     * Less-Than Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const TimeVal& other) const;

    /**
     * Assign-Sum Operator
     *
     * @param delta the TimeVal value to add to this TimeVal object.
     * @return the TimeVal value after the addition of @ref delta.
     */
    const TimeVal& operator+=(const TimeVal& delta);

    /**
     * Addition Operator
     *
     * @param other the TimeVal value to add to the value of this
     * TimeVal object.
     * @return the TimeVal value after the addition of @ref other.
     */
    TimeVal operator+(const TimeVal& other) const;

    /**
     * Assign-Difference Operator
     *
     * @param delta the TimeVal value to substract from this TimeVal object.
     * @return the TimeVal value after the substraction of @ref delta.
     */
    const TimeVal& operator-=(const TimeVal& delta);

    /**
     * Substraction Operator
     *
     * @param other the TimeVal value to substract from the value of this
     * TimeVal object.
     * @return the TimeVal value after the substraction of @ref other.
     */
    TimeVal operator-(const TimeVal& other) const;

    /**
     * Multiplication Operator for integer operand
     *
     * @param n the integer value used in multiplying the value of this
     * object with.
     * @return the TimeVal value of multiplying the value of this object
     * by @ref n.
     */
    TimeVal operator*(int n) const;

    /**
     * Multiplication Operator for unsigned integer operand
     *
     * @param n the unsigned integer value used in multiplying the value of
     * this object with.
     * @return the TimeVal value of multiplying the value of this object
     * by @ref n.
     */
    TimeVal operator*(unsigned int n) const;

    /**
     * Multiplication Operator for double float operand
     *
     * @param d the double float value used in multiplying the value of this
     * object with.
     * @return the TimeVal value of multiplying the value of this object
     * by @ref d.
     */
    TimeVal operator*(const double& d) const;

    /**
     * Division Operator for integer operand
     *
     * @param n the integer value used in dividing the value of this
     * object with.
     * @return the TimeVal value of dividing the value of this object
     * by @ref n.
     */
    TimeVal operator/(int n) const;

    /**
     * Division Operator for unsigned integer operand
     *
     * @param n the unsigned integer value used in dividing the value of this
     * object with.
     * @return the TimeVal value of dividing the value of this object
     * by @ref n.
     */
    TimeVal operator/(unsigned int n) const;

    /**
     * Division Operator for double-float operand
     *
     * @param d the double-float value used in dividing the value of this
     * object with.
     * @return the TimeVal value of dividing the value of this object
     * by @ref d.
     */
    TimeVal operator/(const double& d) const;

    /**
     * Test if this time value is numerically zero.
     *
     * @return true if the address is numerically zero.
     */
    bool is_zero() const	{ return ((_sec == 0) && (_usec == 0)); }

private:
    TimeVal(int i);		// Not implemented

    int32_t _sec;		// The number of seconds
    int32_t _usec;		// The number of microseconds
};

inline
TimeVal::TimeVal(const double& d)
    : _sec((int32_t)d),
      _usec((int32_t)((d - ((double)_sec)) * ONE_MILLION + 0.5e-6))
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
TimeVal::copy_in(const timeval& timeval)
{
    _sec = timeval.tv_sec;
    _usec = timeval.tv_usec;
    return (sizeof(_sec) + sizeof(_usec));
}

inline size_t
TimeVal::copy_out(timeval& timeval) const
{
    timeval.tv_sec = _sec;
    timeval.tv_usec = _usec;
    return (sizeof(_sec) + sizeof(_usec));
}

#ifdef HAVE_STRUCT_TIMESPEC

inline size_t
TimeVal::copy_in(const timespec& timespec)
{
    _sec = timespec.tv_sec;
    _usec = timespec.tv_nsec / 1000;
    return (sizeof(_sec) + sizeof(_usec));
}

inline size_t
TimeVal::copy_out(timespec& timespec) const
{
    timespec.tv_sec = _sec;
    timespec.tv_nsec = _usec * 1000;
    return (sizeof(_sec) + sizeof(_usec));
}

#endif

#ifdef HOST_OS_WINDOWS

/*
 * Convert Windows time to a BSD struct timeval using 64-bit integer
 * arithmetic, by the following steps:
 * 1. Scale Windows' 100ns resolution to BSD's 1us resolution.
 * 2. Subtract the difference since the beginning of their respective epochs.
 * 3. Extract the appropriate fractional parts from the 64-bit
 *    integer used to represent Windows time.
 * Both UNIX and NT time systems correspond to UTC, therefore leap second
 * correction is NTP's problem.
 */
inline size_t
TimeVal::copy_in(const FILETIME& filetime)
{
    ULARGE_INTEGER ui;

    ui.LowPart = filetime.dwLowDateTime;
    ui.HighPart = filetime.dwHighDateTime;
    ui.QuadPart /= UNITS_100NS_PER_1US;
    ui.QuadPart -= UNIX_WIN32_EPOCH_DIFF * ONE_MILLION;
    _usec = ui.QuadPart % ONE_MILLION;
    _sec = ui.QuadPart / ONE_MILLION;
    return (sizeof(filetime.dwLowDateTime) + sizeof(filetime.dwHighDateTime));
}

inline size_t
TimeVal::copy_out(FILETIME& filetime) const
{
    ULARGE_INTEGER ui;

    ui.QuadPart = _sec + UNIX_WIN32_EPOCH_DIFF;
    ui.QuadPart *= ONE_MILLION;
    ui.QuadPart += _usec;
    ui.QuadPart *= UNITS_100NS_PER_1US;
    filetime.dwLowDateTime = ui.LowPart;
    filetime.dwHighDateTime = ui.HighPart;
    return (sizeof(filetime.dwLowDateTime) + sizeof(filetime.dwHighDateTime));
}

#endif /* HOST_OS_WINDOWS */

inline
TimeVal::TimeVal(const TimeVal& tv) {
    _sec = tv.sec();
    _usec = tv.usec();
}

inline TimeVal&
TimeVal::operator=(const TimeVal& other)
{
    _sec = other.sec();
    _usec = other.usec();
    return *this;
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
    tmp_tv += other;
    return tmp_tv;
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
TimeVal::operator*(unsigned int n) const
{
    return (*this)*(static_cast<int>(n));
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
TimeVal::operator/(unsigned int n) const
{
    return (*this)/(static_cast<int>(n));
}

inline TimeVal
TimeVal::operator/(const double& d) const
{
    return TimeVal(get_double() / d);
}

inline TimeVal
TimeVal::ZERO()
{
    return TimeVal(0, 0);
}

inline TimeVal
TimeVal::MAXIMUM()
{
    return TimeVal(0x7fffffff, ONE_MILLION - 1);
}

inline TimeVal
TimeVal::MINIMUM()
{
    return TimeVal(- 0x7fffffff - 1, - (ONE_MILLION - 1));
}

/**
 * Prefix unary minus.
 */
inline TimeVal
operator-(const TimeVal& v)
{
    return TimeVal(-v.sec(), -v.usec());
}

/**
 * Multiply TimeVal by integer.
 */
inline TimeVal
operator*(int n, const TimeVal& t)
{
    return t * n;
}

/**
 * Multiply TimeVal by double.
 */
inline TimeVal
operator*(const double& d, const TimeVal& t)
{
    return t * d;
}

/**
 * Generate a TimeVal value from a uniform random distribution between
 * specified bounds.
 * @param lower lower bound of generated value.
 * @param upper upper bound of generated value.
 * @return value chosen from uniform random distribution.
 */
inline TimeVal
random_uniform(const TimeVal& lower, const TimeVal& upper)
{
    double d = (upper - lower).get_double();
    d *= double(xorp_random()) / double(XORP_RANDOM_MAX);
    return lower + TimeVal(d);
}

/**
 * Generate a TimeVal value from a uniform random distribution between
 * zero and specified bound.
 * @param upper upper bound of generated value.
 * @return value chosen from uniform random distribution.
 */
inline TimeVal
random_uniform(const TimeVal& upper)
{
    double d = upper.get_double();
    d *= double(xorp_random()) / double(XORP_RANDOM_MAX);
    return TimeVal(d);
}

/**
 * Generate a TimeVal value from a uniform random distribution between
 * the bounds center - factor * center and center + factor * center.
 * If the lower bound is less than TimeVal::ZERO() it is rounded up to
 * TimeVal::ZERO().
 *
 * @param center mid-point of generated time value.
 * @param factor the spread of the uniform random distribution.
 * @return value chosen from uniform random distribution.
 */
inline TimeVal
random_uniform(const TimeVal& center, const double& factor)
{
    TimeVal l = max(center - center * factor, TimeVal::ZERO());
    TimeVal u = center + center * factor;
    return random_uniform(l, u);
}

#endif // __LIBXORP_TIMEVAL_HH__
