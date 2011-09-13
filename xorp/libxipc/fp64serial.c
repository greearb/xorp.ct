/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
 * vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2011 XORP, Inc and Others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 *
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <inttypes.h>

#include "fp64serial.h"

/* How big are the fields of IEEE754 binary64? */
#define MANTISSA_BIT 52
#define EXPONENT_BIT 11
#define SIGN_BIT 1

/* What's the offset for each field? */
#define MANTISSA_SHIFT 0
#define EXPONENT_SHIFT (MANTISSA_SHIFT + MANTISSA_BIT)
#define SIGN_SHIFT (EXPONENT_SHIFT + EXPONENT_BIT)

/* Compute masks for each field. */
#define MANTISSA_MASK ((UINTMAX_C(1) << MANTISSA_BIT) - 1u)
#define EXPONENT_MASK ((UINTMAX_C(1) << EXPONENT_BIT) - 1u)
#define SIGN_MASK ((UINTMAX_C(1) << SIGN_BIT) - 1u)

/* How much is the exponent biased? */
#define EXPONENT_BIAS ((EXPONENT_MASK >> 1u) - 1u)

#if __STDC_VERSION__ >= 199901L || \
    _XOPEN_SOURCE >= 600 || \
    _ISOC99_SOURCE || \
    _POSIX_C_SOURCE >= 200112L

/* fpclassify is available. */
#else
/* We can't guarantee that fpclassify exists, so define a simple
   implementation that at least picks out zero. */
#undef FP_ZERO
#undef FP_INFINITE
#undef FP_NAN
#undef FP_NORMAL
#undef FP_SUBNORMAL
#undef fpclassify

#define FP_ZERO      0
#define FP_INFINITE  1
#define FP_NAN       2
#define FP_NORMAL    3
#define FP_SUBNORMAL 4

#define fpclassify(X) \
    ((X) == 0.0 ? FP_ZERO :			\
	(X) >= +XORP_FP64UC(HUGE_VAL) ? FP_INFINITE : \
	(X) <= -XORP_FP64UC(HUGE_VAL) ? FP_INFINITE : \
	(X) != (X) ? FP_NAN : \
     FP_NORMAL)
#endif


#if __STDC_VERSION__ >= 199901L || \
    _XOPEN_SOURCE >= 600 || _ISOC99_SOURCE || _POSIX_C_SOURCE >= 200112L
/* signbit is available. */
#else
/* We can't guarantee that signbit exists.  Define a simple
   implementation that probably won't do worse than fail to detect -ve
   zero. */
#undef signbit
#define signbit(X) ((X) < 0)

#endif

uint_fast64_t fp64enc(fp64_t input)
{
    fp64_t d_mant;
    unsigned int u_exp;
    uint_fast64_t u_mant;
    bool neg;
    int s_exp;
    uint_fast64_t bytes;

    switch (fpclassify(input)) {
    default:
	abort();
	break;

    case FP_ZERO:
	neg = signbit(input);
	u_mant = 0;
	u_exp = 0;
	break;

    case FP_INFINITE:
	neg = signbit(input);
	u_mant = 0;
	u_exp = EXPONENT_MASK;
	break;

    case FP_NAN:
	neg = false;
	u_mant = 1;
	u_exp = EXPONENT_MASK;
	break;

    case FP_SUBNORMAL:
    case FP_NORMAL:
	/* Handle normal and subnormal together.  The number might be
	   one class for double, but another for binary64. */

	/* Decompose the input into a significand (mantissa + 1) and
	   an exponent. */
	d_mant = XORP_FP64(frexp)(input, &s_exp);

	/* Extract the sign bit from the mantissa. */
	neg = signbit(input);
	d_mant = XORP_FP64(fabs)(d_mant);

	/* Offset the exponent so it can be represented as an unsigned
	   value. */
	s_exp += EXPONENT_BIAS;

	/* Now we find out whether the number we represent is normal,
	   subnormal, or overflows binary64. */
	if (s_exp >= (long) EXPONENT_MASK) {
	    /* The number is too big for binary64, so use the maximum
	       value. */
	    u_mant = MANTISSA_MASK;
	    u_exp = EXPONENT_MASK - 1u;
	} else if (s_exp <= 0) {
	    /* The number is subnormal in binary64. */

	    /* Shift the mantissa so that it's exponent would be 0. */
	    u_mant = XORP_FP64(ldexp)(d_mant, MANTISSA_BIT);

	    u_mant >>= -s_exp;
	    u_exp = 0;
	} else {
	    /* The number is normal in binary64. */

	    /* Use the suggested exponent. */
	    u_exp = s_exp;

	    /* Make the mantissa value into a positive integer. */
	    u_mant = XORP_FP64(ldexp)(d_mant, MANTISSA_BIT + 1);
	}

	break;
    }

    /* Transmit the bottom MANTISSA_BITs of u_mant.  The extra top bit
       will always be one when normalized. */

    bytes = ((uint_fast64_t) u_mant & MANTISSA_MASK) << MANTISSA_SHIFT;
    bytes |= ((uint_fast64_t) u_exp & EXPONENT_MASK) << EXPONENT_SHIFT;
    bytes |= ((uint_fast64_t) neg & SIGN_MASK) << SIGN_SHIFT;

    return bytes;
}

fp64_t fp64dec(uint_fast64_t bytes)
{
    int s_exp;
    unsigned int u_exp;
    uint_fast64_t u_mant;
    bool neg;
    fp64_t output;

    /* Extract the bit fields. */
    u_exp = (bytes >> EXPONENT_SHIFT) & EXPONENT_MASK;
    u_mant = (bytes >> MANTISSA_SHIFT) & MANTISSA_MASK;
    neg = (bytes >> SIGN_SHIFT) & SIGN_MASK;

    if (u_exp == EXPONENT_MASK) {
	if (u_mant == 0) {
#ifdef INFINITY
	    return neg ? -INFINITY : +INFINITY;
#else
	    return neg ? -XORP_FP64UC(HUGE_VAL) : +XORP_FP64UC(HUGE_VAL);
#endif
	}
#ifdef NAN
	return NAN;
#else
	return HUGE_VAL;
#endif
    }


    do {
	if (u_exp == 0) {
	    /* Positive or negative zero */
	    if (u_mant == 0)
		return XORP_FP64(copysign)(0.0, neg ? -1.0 : +1.0);

	    /* Subnormal value */

	    /* Multiply the mantissa by a power of two. */
	    output = XORP_FP64(ldexp)
		(u_mant, -(MANTISSA_BIT + (int) EXPONENT_BIAS));
	    break;
	}

	/* Recover the top bit of the mantissa. */
	u_mant |= MANTISSA_MASK + 1;

	/* Convert offset exponent back into a native signed value. */
	s_exp = (int) u_exp - EXPONENT_BIAS;

	/* Multiply the mantissa by a power of two. */
	output = XORP_FP64(ldexp)
	    (u_mant, s_exp - (MANTISSA_BIT + 1));
    } while (false);

    if (neg)
	output = -output;

    return output;
}
