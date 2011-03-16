/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2011 XORP, Inc and Others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

/*
 * $XORP: xorp/mrt/netstream_access.h,v 1.10 2008/10/02 21:57:45 bms Exp $
 */


#ifndef __MRT_NETSTREAM_ACCESS_H__
#define __MRT_NETSTREAM_ACCESS_H__


/*
 * A set of macros to get/put data from/to a stream when the fields
 * are not 32-bit boundary aligned. Also useful to automatically
 * check the boundary of the input/output buffer while accessing it.
 */
/*
 * Idea taken from Eddy Rusty (eddy@isi.edu).
 */

#include "libxorp/xorp.h"

#include <sys/types.h>


/*
 * Constants definitions
 */

/*
 * Structures, typedefs and macros
 */
/*
 * XXX: the data in the stream is ALWAYS in "network order".
 *
 * The current stream pointer is (uint8_t *)(cp)
 *
 * XXX: the macros below assume that the code has the following
 * labels that can be used to jump and process the particular error:
 *	'rcvlen_error' 'buflen_error'
 *
 * PUT_NET_32  puts "network ordered" 32-bit data to the datastream.
 * PUT_HOST_32 puts "host ordered" 32-bit data to the datastream.
 * GET_NET_32  gets 32-bit data and keeps it in "network order" in the memory.
 * GET_HOST_32 gets 32-bit data, but in the memory it is in "host order".
 * The same applies for the 16-bit {PUT,GET}_{NET,HOST}_16
 * GET_OCTET and PUT_OCTET get and put a single octet from/to the datastream.
 * GET_SKIP(octets,...) skips the next 'octets' from the datastream.
 * PUT_SKIP(octets,...) leaves the next 'octets' untouched in the datastream.
 * GET_DATA(to, cp, rcvlen, datalen) copies 'datalen' octets from 'cp' to 'to'.
 * PUT_DATA(from, cp, buflen, datalen) copies 'datalen' octets from 'from'
 *                                     to 'cp'.
 */
#define GET_DATA(to, cp, rcvlen, datalen)				\
do {									\
	if ((size_t)(rcvlen) < (size_t)(datalen))			\
		goto rcvlen_error;					\
	memcpy((to), (cp), (datalen));					\
	(rcvlen) -= (datalen);						\
	(cp) += (datalen);						\
} while (0)
#define PUT_DATA(from, cp, buflen, datalen)				\
do {									\
	if ((size_t)(buflen) < (size_t)(datalen))			\
		goto buflen_error;					\
	memcpy((cp), (from), (datalen));				\
	(buflen) -= (datalen);						\
	(cp) += (datalen);						\
} while (0)

#define GET_SKIP(octets, cp, rcvlen)					\
do {									\
	if ((size_t)(rcvlen) < (size_t)(octets))			\
		goto rcvlen_error;					\
	(rcvlen) -= (octets);						\
	(cp) += (octets);						\
} while (0)
#define PUT_SKIP(octets, cp, buflen)					\
do {									\
	if ((size_t)(buflen) < (size_t)(octets))			\
		goto buflen_error;					\
	(buflen) -= (octets);						\
	(cp) += (octets);						\
} while (0)

#define GET_OCTET(val, cp, rcvlen)					\
do {									\
	if ((size_t)(rcvlen) < (size_t)1)				\
		goto rcvlen_error;					\
	(rcvlen)--;							\
	((val) = *(cp)++);						\
} while (0)
#define PUT_OCTET(val, cp, buflen)					\
do {									\
	if ((size_t)(buflen) < (size_t)1)				\
		goto buflen_error;					\
	(buflen)--;							\
	(*(cp)++ = (uint8_t)(val));					\
} while (0)

#define GET_HOST_16(val, cp, rcvlen)					\
do {									\
	register uint16_t v_;						\
									\
	if ((size_t)(rcvlen) < (size_t)2)				\
		goto rcvlen_error;					\
	(rcvlen) -= 2;							\
	v_ = (*(cp)++) << 8;						\
	v_ |= *(cp)++;							\
	(val) = v_;							\
} while (0)

#define PUT_HOST_16(val, cp, buflen)					\
do {									\
	register uint16_t v_;						\
									\
	if ((size_t)(buflen) < (size_t)2)				\
		goto buflen_error;					\
	(buflen) -= 2;							\
	v_ = (uint16_t)(val);						\
	*(cp)++ = (uint8_t)(v_ >> 8);					\
	*(cp)++ = (uint8_t)v_;						\
} while (0)

#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
#define GET_NET_16(val, cp, rcvlen)					\
do {									\
	register uint16_t v_;						\
									\
	if ((size_t)(rcvlen) < (size_t)2)				\
		goto rcvlen_error;					\
	(rcvlen) -= 2;							\
	v_ = *(cp)++;							\
	v_ |= (*(cp)++) << 8;						\
	(val) = v_;							\
} while (0)
#define PUT_NET_16(val, cp, buflen)					\
do {									\
	register uint16_t v_;						\
									\
	if ((size_t)(buflen) < (size_t)2)				\
		goto buflen_error;					\
	(buflen) -= 2;							\
	v_ = (uint16_t)(val);						\
	*(cp)++ = (uint8_t)v_;						\
	*(cp)++ = (uint8_t)(v_ >> 8);					\
} while (0)
#endif /* LITTLE_ENDIAN {GET,PUT}_NET_16 */

#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
#define GET_NET_16(val, cp, rcvlen) GET_HOST_16(val, cp, rcvlen)
#define PUT_NET_16(val, cp, buflen) PUT_HOST_16(val, cp, buflen)
#endif /* BIG_ENDIAN {GET,PUT}_NET_16 */

#define GET_HOST_32(val, cp, rcvlen)					\
do {									\
	register uint32_t v_;						\
									\
	if ((size_t)(rcvlen) < (size_t)4)				\
		goto rcvlen_error;					\
	(rcvlen) -= 4;							\
	v_  = (*(cp)++) << 24;						\
	v_ |= (*(cp)++) << 16;						\
	v_ |= (*(cp)++) <<  8;						\
	v_ |= *(cp)++;							\
	(val) = v_;							\
} while (0)

#define PUT_HOST_32(val, cp, buflen)					\
do {									\
	register uint32_t v_;						\
									\
	if ((size_t)(buflen) < (size_t)4)				\
		goto buflen_error;					\
	(buflen) -= 4;							\
	v_ = (uint32_t)(val);						\
	*(cp)++ = (uint8_t)(v_ >> 24);					\
	*(cp)++ = (uint8_t)(v_ >> 16);					\
	*(cp)++ = (uint8_t)(v_ >>  8);					\
	*(cp)++ = (uint8_t)v_;						\
} while (0)

#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
#define GET_NET_32(val, cp, rcvlen)					\
do {									\
	register uint32_t v_;						\
									\
	if ((size_t)(rcvlen) < (size_t)4)				\
		goto rcvlen_error;					\
	(rcvlen) -= 4;							\
	v_  = *(cp)++;							\
	v_ |= (*(cp)++) <<  8;						\
	v_ |= (*(cp)++) << 16;						\
	v_ |= (*(cp)++) << 24;						\
	(val) = v_;							\
} while (0)

#define PUT_NET_32(val, cp, buflen)					\
do {									\
	register uint32_t v_;						\
									\
	if ((size_t)(buflen) < (size_t)4)				\
		goto buflen_error;					\
	(buflen) -= 4;							\
	v_ = (uint32_t)(val);						\
	*(cp)++ = (uint8_t)v_;						\
	*(cp)++ = (uint8_t)(v_ >>  8);					\
	*(cp)++ = (uint8_t)(v_ >> 16);					\
	*(cp)++ = (uint8_t)(v_ >> 24);					\
} while (0)
#endif /* LITTLE_ENDIAN {GET,PUT}_HOST_32 */

#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
#define GET_NET_32(val, cp, rcvlen) GET_HOST_32(val, cp, rcvlen)
#define PUT_NET_32(val, cp, buflen) PUT_HOST_32(val, cp, buflen)
#endif /* BIG_ENDIAN {GET,PUT}_HOST_32 */

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */

__BEGIN_DECLS

__END_DECLS

#endif /* __MRT_NETSTREAM_ACCESS_H__ */
