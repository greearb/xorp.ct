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

#ifndef __LIBXIPC_FP64_H__
#define __LIBXIPC_FP64_H__

#include <stdint.h>
#include <float.h>

/* This header chooses a native real type (float, double, long double)
   which best matches the IEEE754 binary64 capabilities. */



/* These are the target characteristics of various IEEE754 types,
   expressed in terms that make them comparable to the native real
   types. */

/* TODO: We only need the 64-bit details; the others could be in their
   own header.  */

#define XORP_IEEE754_BIN16_MANT_DIG 11
#define XORP_IEEE754_BIN16_MAX_EXP 16
#define XORP_IEEE754_BIN16_MIN_EXP -13

#define XORP_IEEE754_BIN32_MANT_DIG 24
#define XORP_IEEE754_BIN32_MAX_EXP 128
#define XORP_IEEE754_BIN32_MIN_EXP -125

#define XORP_IEEE754_BIN64_MANT_DIG 53
#define XORP_IEEE754_BIN64_MAX_EXP 1024
#define XORP_IEEE754_BIN64_MIN_EXP -1021

#define XORP_IEEE754_BIN128_MANT_DIG 113
#define XORP_IEEE754_BIN128_MAX_EXP 16384
#define XORP_IEEE754_BIN128_MIN_EXP -16381


/* TODO: These ratios and test macros are not binary64-specific.  If
   other types were to be supported, these macros should be factored
   out. */

/* These are 100000*log2(FLT_RADIX) for various possible FLT_RADIX
   values.  In comparing the capabilities of float, double and long
   double with binary64, we need to account for the slim possibility
   that our native base is not 2, while binary64's base is 2. */

#define XORP_IEEE754_RADIX_RATIO_2  1000000
#define XORP_IEEE754_RADIX_RATIO_3  1584963
#define XORP_IEEE754_RADIX_RATIO_4  2000000
#define XORP_IEEE754_RADIX_RATIO_5  2321928
#define XORP_IEEE754_RADIX_RATIO_6  2584963
#define XORP_IEEE754_RADIX_RATIO_7  2807354
#define XORP_IEEE754_RADIX_RATIO_8  3000000
#define XORP_IEEE754_RADIX_RATIO_9  3169925
#define XORP_IEEE754_RADIX_RATIO_10 3321928

#define XORP_IEEE754_RADIX_RATIO_N(B) (XORP_IEEE754_RADIX_RATIO_ ## B)
#define XORP_IEEE754_RADIX_RATIO(B) (XORP_IEEE754_RADIX_RATIO_N(B))


/* These preprocessor-safe macros test various native types'
   characteristics against IEEE754 types'. */

#define XORP_IEEE754_TEST_MANT_DIG(T,W) \
    ( T ## _MANT_DIG * XORP_IEEE754_RADIX_RATIO(FLT_RADIX) >=		\
      XORP_IEEE754_BIN ## W ## _MANT_DIG * XORP_IEEE754_RADIX_RATIO_2 )

#define XORP_IEEE754_TEST_MAX_EXP(T,W) \
    ( T ## _MAX_EXP * XORP_IEEE754_RADIX_RATIO(FLT_RADIX) >=		\
      XORP_IEEE754_BIN ## W ## _MAX_EXP * XORP_IEEE754_RADIX_RATIO_2 )

#define XORP_IEEE754_TEST_MIN_EXP(T,W) \
    ( T ## _MIN_EXP * XORP_IEEE754_RADIX_RATIO(FLT_RADIX) <=		\
      XORP_IEEE754_BIN ## W ## _MIN_EXP * XORP_IEEE754_RADIX_RATIO_2)

#define XORP_IEEE754_TEST(T,W)	    \
    ( XORP_IEEE754_TEST_MANT_DIG(T,W) && \
      XORP_IEEE754_TEST_MAX_EXP(T,W) &&  \
      XORP_IEEE754_TEST_MIN_EXP(T,W) )

/* Now we choose a native type to fulfil binary64. */

#if XORP_IEEE754_TEST(DBL,64)
/* double is sufficient, and is probably the optimal FP type used by
   this system. */
#define XORP_IEEE754_BIN64_DBL 1
#else
/* We'll have to use long double, and hope that it is adequate.  What
   other choice do we have? */
#define XORP_IEEE754_BIN64_LDBL 1
#endif


/* Declare a type alias according to what we've chosen.  Also define
   some macros (like those in <inttypes.h>) to print and scan the
   type. */

#if XORP_IEEE754_BIN64_FLT
typedef float fp64_t;
#define XORP_PRIaFP64 "a"
#define XORP_PRIeFP64 "e"
#define XORP_PRIfFP64 "f"
#define XORP_PRIgFP64 "g"
#define XORP_PRIAFP64 "A"
#define XORP_PRIEFP64 "E"
#define XORP_PRIFFP64 "F"
#define XORP_PRIGFP64 "G"
#define XORP_SCNaFP64 "a"
#define XORP_SCNeFP64 "e"
#define XORP_SCNfFP64 "f"
#define XORP_SCNgFP64 "g"
#define XORP_FP64(F) F ## f
#define XORP_FP64_DIG FLT_FP64_DIG
#define XORP_FP64_EPSILON FLT_FP64_EPSILON
#define XORP_FP64_MANT_DIG FLT_FP64_MANT_DIG
#define XORP_FP64_MAX FLT_FP64_MAX
#define XORP_FP64_MAX_10_EXP FLT_FP64_MAX_10_EXP
#define XORP_FP64_MAX_EXP FLT_FP64_MAX_EXP
#define XORP_FP64_MIN FLT_FP64_MIN
#define XORP_FP64_MIN_10_EXP FLT_FP64_MIN_10_EXP
#define XORP_FP64_MIN_EXP FLT_FP64_MIN_EXP
#endif

#if XORP_IEEE754_BIN64_DBL
typedef double fp64_t;
#define XORP_PRIaFP64 "a"
#define XORP_PRIeFP64 "e"
#define XORP_PRIfFP64 "f"
#define XORP_PRIgFP64 "g"
#define XORP_PRIAFP64 "A"
#define XORP_PRIEFP64 "E"
#define XORP_PRIFFP64 "F"
#define XORP_PRIGFP64 "G"
#define XORP_SCNaFP64 "la"
#define XORP_SCNeFP64 "le"
#define XORP_SCNfFP64 "lf"
#define XORP_SCNgFP64 "lg"
#define XORP_FP64(F) F
#define XORP_FP64_DIG DBL_FP64_DIG
#define XORP_FP64_EPSILON DBL_FP64_EPSILON
#define XORP_FP64_MANT_DIG DBL_FP64_MANT_DIG
#define XORP_FP64_MAX DBL_FP64_MAX
#define XORP_FP64_MAX_10_EXP DBL_FP64_MAX_10_EXP
#define XORP_FP64_MAX_EXP DBL_FP64_MAX_EXP
#define XORP_FP64_MIN DBL_FP64_MIN
#define XORP_FP64_MIN_10_EXP DBL_FP64_MIN_10_EXP
#define XORP_FP64_MIN_EXP DBL_FP64_MIN_EXP
#endif

#if XORP_IEEE754_BIN64_LDBL
typedef long double fp64_t;
#define XORP_PRIaFP64 "La"
#define XORP_PRIeFP64 "Le"
#define XORP_PRIfFP64 "Lf"
#define XORP_PRIgFP64 "Lg"
#define XORP_PRIAFP64 "LA"
#define XORP_PRIEFP64 "LE"
#define XORP_PRIFFP64 "LF"
#define XORP_PRIGFP64 "LG"
#define XORP_SCNaFP64 "La"
#define XORP_SCNeFP64 "Le"
#define XORP_SCNfFP64 "Lf"
#define XORP_SCNgFP64 "Lg"
#define XORP_FP64(F) F ## l
#define XORP_FP64_DIG LDBL_FP64_DIG
#define XORP_FP64_EPSILON LDBL_FP64_EPSILON
#define XORP_FP64_MANT_DIG LDBL_FP64_MANT_DIG
#define XORP_FP64_MAX LDBL_FP64_MAX
#define XORP_FP64_MAX_10_EXP LDBL_FP64_MAX_10_EXP
#define XORP_FP64_MAX_EXP LDBL_FP64_MAX_EXP
#define XORP_FP64_MIN LDBL_FP64_MIN
#define XORP_FP64_MIN_10_EXP LDBL_FP64_MIN_10_EXP
#define XORP_FP64_MIN_EXP LDBL_FP64_MIN_EXP
#endif


#endif
