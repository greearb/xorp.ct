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

// $XORP: xorp/libxorp/timeval.hh,v 1.10 2002/12/09 18:29:15 hodson Exp $

#ifndef __LIBXORP_TIMEVAL_HH__
#define __LIBXORP_TIMEVAL_HH__

#include <sys/time.h>

#include <math.h>

#include <stl_config.h>
#include <stl_relops.h>


#define ONE_MILLION	1000000

/**
 * Make a timeval value.
 * 
 * @param sec the number of seconds.
 * @param usec the number of microseconds.
 * @return the timeval value of @ref sec and @ref usec.
 */
static inline timeval
mk_timeval(int sec, int usec)
{
    timeval t;
    t.tv_sec = sec;
    t.tv_usec = usec;
    return t;
}

/**
 * Adjust the timeval value so it is valid.
 * 
 * A timeval value is valid if the number of microseconds is not negative,
 * and is smaller than 1000000 (i.e., one million).
 * 
 * @param t the timeval value to adjust.
 * @return the adjusted timeval value. Note that the original @ref t value
 * is also adjusted.
 */
static inline timeval& 
round(timeval& t)
{
    if (t.tv_usec < 0) {
	t.tv_usec += ONE_MILLION ;
	t.tv_sec -- ;
    } else if (t.tv_usec >= ONE_MILLION) {
	t.tv_usec -= ONE_MILLION ;
	t.tv_sec ++ ;
    }
    return t;
}

/**
 * Convert a timeval value to a double-float value.
 * 
 * @param t the timeval value to convert to a double-float value.
 * @return the double-float value of @ref t.
 */
static inline double
timeval_to_double(const timeval& t)
{
    return t.tv_sec * 1.0 + t.tv_usec * 1.0e-6;
}

/**
 * Convert a double-float value to a timeval value.
 * 
 * @param d the double-float value to convert to a timeval value.
 * @return the timeval value of @ref.
 */
static inline timeval
double_to_timeval(const double& d)
{
    timeval t;
    t.tv_sec = (int)d;
    t.tv_usec = (int)(d - ((double)t.tv_sec)) * ONE_MILLION;
    return t;
}


// ----------------------------------------------------------------------------
// The following allow the templates in stl_relops.h to constitute set of
// comparison operators for timeval's (>=, <=, !=, >)
//

/**
 * Timeval Equality Operator
 * 
 * @param t the first timeval value to compare.
 * @param u the second timeval value to compare.
 * @return true if @ref t and @ref u have same timeval value.
 */
static inline bool
operator==(const timeval& t, const timeval& u)
{
    return (t.tv_sec == u.tv_sec && t.tv_usec == u.tv_usec);
}

/**
 * Timeval Less-Than Operator
 * 
 * @param t the first timeval value to compare.
 * @param u the second timeval value to compare.
 * @return true if @ref t has numerically smaller timeval value
 * than @ref u.
 */
static inline bool 
operator<(const timeval& t, const timeval& u)
{
    return (t.tv_sec == u.tv_sec) ?
	t.tv_usec < u.tv_usec : (unsigned)t.tv_sec < (unsigned)u.tv_sec;
}


// ----------------------------------------------------------------------------
// Arithmetic operations (+,-,+=,-=,*,/,%)
//

/**
 * Timeval Assign-Sum Operator
 * 
 * @param t the timeval value to assign-sum.
 * @param delta the timeval value to add to @ref t.
 * @return the timeval value of adding @ref delta to @ref t.
 */
static inline timeval&
operator+=(timeval& t, const timeval& delta)
{
    t.tv_sec  += delta.tv_sec;
    t.tv_usec += delta.tv_usec;
    return round(t);
}

/**
 * Timeval Addition Operator
 * 
 * @param t the first timeval value to add.
 * @param u the second timeval value to add.
 * @return the timeval value of adding @ref t and @ref u.
 */
static inline timeval 
operator+(const timeval& t, const timeval& u)
{
    timeval v = t;
    return  v += u;
}

/**
 * Timeval Assign-Difference Operator
 * 
 * @param t the timeval value to assign-difference.
 * @param delta the timeval value to substract from @ref t.
 * @return the timeval value of substracting @ref delta from @ref t.
 */
static inline timeval& 
operator-=(timeval& t, const timeval& delta)
{
    t.tv_sec  -= delta.tv_sec;
    t.tv_usec -= delta.tv_usec;
    return round(t);
}

/**
 * Timeval Substraction Operator
 * 
 * @param t the timeval value of the first operand (the one to substract from).
 * @param u the timeval value of the second operand (the one to substract
 * from @ref t).
 * @return the timeval value of substracting @ref u from @ref t.
 */
static inline timeval
operator-(const timeval& t, const timeval& u)
{
    timeval v = t;
    return  v -= u;
}

/**
 * Timeval Division Operator
 * 
 * @param t the timeval value of the first operand (the one to be divided).
 * @param n the integer value of the second operand (the one to divide @ref t).
 * @return the timeval value of dividing @ref t by @ref n.
 */
static inline timeval
operator/(const timeval & t, int n)
{
    timeval v;
    v.tv_sec = t.tv_sec / n;
    v.tv_usec = ((t.tv_sec % n) * ONE_MILLION + t.tv_usec) / n;
    return v;
}

/**
 * Timeval Modulo Operator
 * 
 * @param t the timeval value of the first operand (the one to be divided).
 * @param u the timeval value of the second operand (the one to divide @ref t).
 * @return the timeval value of the remainder of dividing @ref t by @ref u.
 */
static inline timeval
operator%(const timeval& t, const timeval& u)
{
    double d = fmod(timeval_to_double(t), timeval_to_double(u));
    return double_to_timeval(d);
}

/**
 * Timeval Division Operator
 * 
 * @param t the timeval value of the first operand (the one to be divided).
 * @param u the timeval value of the second operand (the one to divide @ref t).
 * @return the timeval value of dividing @ref t by @ref u.
 */
static inline double
operator/(const timeval& t, const timeval& u)
{
    return timeval_to_double(t) / timeval_to_double(u);
}

/**
 * Timeval Multiplication Operator
 * 
 * @param t the timeval value of the first operand to multiply.
 * @param n the integer value of the second operand to multiply.
 * @return the timeval value of multiplying @ref t and @ref n.
 */
static inline timeval
operator*(const timeval& t, int n)
{
    timeval u;
    u.tv_usec = t.tv_usec * n;
    u.tv_sec = t.tv_sec * n + (u.tv_usec) / ONE_MILLION;
    u.tv_usec = u.tv_usec % ONE_MILLION;
    return u;
}

#endif // __LIBXORP_TIMEVAL_HH__
