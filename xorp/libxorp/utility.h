/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
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

/*
 * $XORP: xorp/libxorp/utility.h,v 1.21 2008/10/02 21:57:36 bms Exp $
 */

#ifndef __LIBXORP_UTILITY_H__
#define __LIBXORP_UTILITY_H__

/*
 * Compile time assertion.
 */
#ifndef static_assert
// +0 is to work around clang bug.
#define static_assert(a) switch ((a) + 0) case 0: case ((a) + 0):
#endif /* static_assert */

/*
 * A macro to avoid compilation warnings about unused functions arguments.
 * XXX: this should be used only in C. In C++ just remove the argument name
 * in the function definition.
 */
#ifndef UNUSED
#define UNUSED(var)	static_assert(sizeof(var) != 0)
#endif /* UNUSED */

#ifdef __cplusplus
#define cstring(s) (s).str().c_str()
#endif

/*
 * XORP code uses typedefs (e.g. uint32_t, int32_t) rather than using
 * the base types, because the 'C' language allows a compiler to use a
 * natural size for base type. The XORP code is internally consistent
 * in this usage, one problem arises with format strings in printf
 * style functions.
 *
 * In order to print a size_t or uint32_t with "%u" it is necessary to
 * cast to an unsigned int. On Mac OS X a size_t is not an unsigned
 * int. On windows uint32_t is not an unsigned int.
 *
 * In order to print a int32_t with a "%d" it is necessary to cast to
 * a signed int. On windows int32_t is not a signed int.
 *
 * The two convenience macros are provided to perform the cast.
 */
#ifdef __cplusplus
#define	XORP_UINT_CAST(x)	static_cast<unsigned int>(x)
#define	XORP_INT_CAST(x)	static_cast<int>(x)
#else
#define	XORP_UINT_CAST(x)	(unsigned int)(x)
#define	XORP_INT_CAST(x)	(int)(x)
#endif

/*
 * On some architectures casting a "(struct sockaddr *)" pointer to
 * "(struct sockaddr_in *)" or "(struct sockaddr_in6 *)" pointer generates
 * a warning like:
 *     warning: cast from 'sockaddr*' to 'sockaddr_in*' increases required
 *     alignment of target type
 *
 * In general such casting shouldn't create any alignment issues and
 * shouldn't generate such warning.
 * To get around the problem we use the help of a "void" pointer.
 *
 * If the casting actually creates an alignment problem, then we need
 * to copy the "struct sockaddr" content to "struct sockaddr_in" or
 * "struct sockaddr_in6" placeholder.
 * Doing this (without using local static storage) might requite changing
 * the semantics hence we don't provide the implementation.
 */
#ifdef __cplusplus
inline const struct sockaddr_in *
sockaddr2sockaddr_in(const struct sockaddr* sa)
{
    const void* v = sa;
    return (reinterpret_cast<const struct sockaddr_in*>(v));
}

inline struct sockaddr_in *
sockaddr2sockaddr_in(struct sockaddr* sa)
{
    void* v = sa;
    return (reinterpret_cast<struct sockaddr_in*>(v));
}

inline const struct sockaddr_in6 *
sockaddr2sockaddr_in6(const struct sockaddr* sa)
{
    const void* v = sa;
    return (reinterpret_cast<const struct sockaddr_in6*>(v));
}

inline struct sockaddr_in6 *
sockaddr2sockaddr_in6(struct sockaddr* sa)
{
    void* v = sa;
    return (reinterpret_cast<struct sockaddr_in6*>(v));
}

inline const struct sockaddr *
sockaddr_storage2sockaddr(const struct sockaddr_storage* ss)
{
    const void* v = ss;
    return (reinterpret_cast<const struct sockaddr*>(v));
}

inline struct sockaddr *
sockaddr_storage2sockaddr(struct sockaddr_storage* ss)
{
    void* v = ss;
    return (reinterpret_cast<struct sockaddr*>(v));
}

#endif /* __cplusplus */

#define ADD_POINTER(pointer, size, type)				\
	((type)(void *)(((uint8_t *)(pointer)) + (size)))

/*
 * Micro-optimization: byte ordering fix for constants.  htonl uses
 * CPU instructions, whereas the macro below can be handled by the
 * compiler front-end for literal values.
 */
#if defined(WORDS_BIGENDIAN)
#  define htonl_literal(x) (x)
#elif defined(WORDS_SMALLENDIAN)
#  define htonl_literal(x) 						      \
		((((x) & 0x000000ffU) << 24) | (((x) & 0x0000ff00U) << 8) |   \
		 (((x) & 0x00ff0000U) >> 8) | (((x) & 0xff000000U) >> 24))
#else
#  error "Missing endian definition from config.h"
#endif

/*
 * Various ctype(3) wrappers that work properly even if the value of the int
 * argument is not representable as an unsigned char and doesn't have the
 * value of EOF.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern int xorp_isalnum(int c);
extern int xorp_isalpha(int c);
/*
 * TODO: for now comment-out xorp_isblank(), because isblank(3) is introduced
 * with ISO C99, and may not always be available on the system.
 */
/* extern int xorp_isblank(int c); */
extern int xorp_iscntrl(int c);
extern int xorp_isdigit(int c);
extern int xorp_isgraph(int c);
extern int xorp_islower(int c);
extern int xorp_isprint(int c);
extern int xorp_ispunct(int c);
extern int xorp_isspace(int c);
extern int xorp_isupper(int c);
extern int xorp_isxdigit(int c);
extern int xorp_tolower(int c);
extern int xorp_toupper(int c);

/*
 * A strptime(3) wrapper that uses a local implementation of strptime(3)
 * if the operating system doesn't have one. Note that the particular
 * implementation is inside file "libxorp/strptime.c".
 */
extern char *xorp_strptime(const char *buf, const char *fmt, struct tm *tm);

/*
 * Function to return C-string representation of a boolean: "true" of "false".
 */
extern const char *bool_c_str(int v);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXORP_UTILITY_H__ */
