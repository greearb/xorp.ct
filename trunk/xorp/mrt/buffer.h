/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2004 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/mrt/buffer.h,v 1.2 2003/02/19 00:58:26 pavlin Exp $
 */


#ifndef __MRT_BUFFER_H__
#define __MRT_BUFFER_H__


/*
 * Buffer management header file.
 */


#include "netstream_access.h"


/*
 * Constants definitions
 */
#ifndef BUF_SIZE_DEFAULT
#define BUF_SIZE_DEFAULT (64*1024)
#endif

/*
 * Structures, typedefs and macros
 */
typedef struct buffer_ {
    uint8_t *_data;		/* The real data stream			     */
    uint8_t *_data_head;	/* The head of the data on the stream	     */
    uint8_t *_data_tail;	/* The tail of the data on the stream
				 * (actually, the first unused octet after the
				 * real data on the stream).
				 */
    size_t _buffer_size;	/* The allocated buffer size for the data    */
} buffer_t;

#define BUFFER_DATA_HEAD(buffer)	((buffer)->_data_head)
#define BUFFER_DATA_TAIL(buffer)	((buffer)->_data_tail)
#define BUFFER_DATA_SIZE(buffer)	((size_t)((buffer)->_data_tail	      \
						- (buffer)->_data_head))
#define BUFFER_AVAIL_HEAD(buffer)	((size_t)((buffer)->_data_head	      \
						- (buffer)->_data))
#define BUFFER_AVAIL_TAIL(buffer)	((size_t)((buffer)->_data	      \
						+ (buffer)->_buffer_size      \
						- (buffer)->_data_tail))
#define BUFFER_RESET_TAIL(buffer)					      \
do {									      \
	(buffer)->_data_tail = (buffer)->_data_head;			      \
} while (0)

#define BUFFER_COPYPUT_INET_CKSUM(cksum, buffer, offset)		      \
do {									      \
	BUFFER_COPYPUT_DATA_OFFSET(&(cksum), (buffer), (offset), 2);	      \
} while (0)

#define BUFFER_COPY(buffer_from, buffer_to)				      \
do {									      \
	size_t buffer_data_size_;					      \
									      \
	buffer_data_size_ = BUFFER_DATA_SIZE(buffer_from);		      \
	BUFFER_RESET(buffer_to);					      \
	BUFFER_PUT_DATA(BUFFER_DATA_HEAD(buffer_from),			      \
			(buffer_to),					      \
			buffer_data_size_);				      \
} while (0)

#define BUFFER_COMPARE(buffer1, buffer2)				      \
	((BUFFER_DATA_SIZE(buffer1) == BUFFER_DATA_SIZE(buffer2)) ?	      \
	 memcmp(BUFFER_DATA_HEAD(buffer1), BUFFER_DATA_HEAD(buffer2),	      \
		BUFFER_DATA_SIZE(buffer1))				      \
	 : ((BUFFER_DATA_SIZE(buffer1) < BUFFER_DATA_SIZE(buffer2)) ? -1 : 1))

#define BUFFER_COPYGET_DATA(to, buffer, datalen)			      \
do {									      \
	size_t rcvlen_;							      \
	uint8_t *cp_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	cp_ = BUFFER_DATA_HEAD(buffer);					      \
	GET_DATA((to), cp_, rcvlen_, (datalen));			      \
} while (0)
#define BUFFER_COPYPUT_DATA(from, buffer, datalen)			      \
do {									      \
	size_t buflen_;							      \
	uint8_t *cp_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	cp_ = BUFFER_DATA_TAIL(buffer);					      \
	PUT_DATA((from), cp_, buflen_, (datalen));			      \
} while (0)
#define BUFFER_COPYGET_DATA_OFFSET(to, buffer, offset, datalen)		      \
do {									      \
	size_t rcvlen_;							      \
	uint8_t *cp_;							      \
									      \
	if (BUFFER_DATA_SIZE(buffer) < (offset))			      \
		goto rcvlen_error;					      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer) - (offset);			      \
	cp_ = BUFFER_DATA_HEAD(buffer) + (offset);			      \
	GET_DATA((to), cp_, rcvlen_, (datalen));			      \
} while (0)
#define BUFFER_COPYPUT_DATA_OFFSET(from, buffer, offset, datalen)	      \
do {									      \
	size_t buflen_;							      \
	uint8_t *cp_;							      \
									      \
	if (BUFFER_DATA_SIZE(buffer) + BUFFER_AVAIL_TAIL(buffer) < (offset))  \
		goto buflen_error;					      \
	buflen_ = BUFFER_DATA_SIZE(buffer) + BUFFER_AVAIL_TAIL(buffer)	      \
			- (offset);					      \
	cp_ = BUFFER_DATA_HEAD(buffer) + (offset);			      \
	PUT_DATA((from), cp_, buflen_, (datalen));			      \
} while (0)

#define BUFFER_GET_DATA(to, buffer, datalen)				      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_DATA((to), BUFFER_DATA_HEAD(buffer), rcvlen_, (datalen));	      \
} while (0)
#define BUFFER_PUT_DATA(from, buffer, datalen)				      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_DATA((from), BUFFER_DATA_TAIL(buffer), buflen_, (datalen));	      \
} while (0)

#define BUFFER_GET_SKIP(octets, buffer)					      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_SKIP((octets), BUFFER_DATA_HEAD(buffer), rcvlen_);		      \
} while (0)
#define BUFFER_PUT_SKIP(octets, buffer)					      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_SKIP((octets), BUFFER_DATA_TAIL(buffer), buflen_);		      \
} while (0)

#define BUFFER_GET_SKIP_REVERSE(octets, buffer)				      \
do {									      \
	size_t avail_head_;						      \
									      \
	avail_head_ = BUFFER_AVAIL_HEAD(buffer);			      \
	if (avail_head_ < (octets))					      \
		goto rcvlen_error;					      \
	(buffer)->_data_head -= (octets);				      \
} while (0)
#define BUFFER_PUT_SKIP_REVERSE(octets, buffer)				      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	if (rcvlen_ < (octets))						      \
		goto buflen_error;					      \
	(buffer)->_data_tail -= (octets);				      \
} while (0)

#define BUFFER_GET_OCTET(val, buffer)					      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_OCTET((val), BUFFER_DATA_HEAD(buffer), rcvlen_);		      \
} while (0)
#define BUFFER_PUT_OCTET(val, buffer)					      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_OCTET((val), BUFFER_DATA_TAIL(buffer), buflen_);		      \
} while (0)

#define BUFFER_GET_HOST_16(val, buffer)					      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_HOST_16((val), BUFFER_DATA_HEAD(buffer), rcvlen_);		      \
} while (0)
#define BUFFER_PUT_HOST_16(val, buffer)					      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_HOST_16((val), BUFFER_DATA_TAIL(buffer), buflen_);		      \
} while (0)

#define BUFFER_GET_NET_16(val, buffer)					      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_NET_16((val), BUFFER_DATA_HEAD(buffer), rcvlen_);		      \
} while (0)
#define BUFFER_PUT_NET_16(val, buffer)					      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_NET_16((val), BUFFER_DATA_TAIL(buffer), buflen_);		      \
} while (0)

#define BUFFER_GET_HOST_32(val, buffer)					      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_HOST_32((val), BUFFER_DATA_HEAD(buffer), rcvlen_);		      \
} while (0)
#define BUFFER_PUT_HOST_32(val, buffer)					      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_HOST_32((val), BUFFER_DATA_TAIL(buffer), buflen_);		      \
} while (0)

#define BUFFER_GET_NET_32(val, buffer)					      \
do {									      \
	size_t rcvlen_;							      \
									      \
	rcvlen_ = BUFFER_DATA_SIZE(buffer);				      \
	GET_NET_32((val), BUFFER_DATA_HEAD(buffer), rcvlen_);		      \
} while (0)
#define BUFFER_PUT_NET_32(val, buffer)					      \
do {									      \
	size_t buflen_;							      \
									      \
	buflen_ = BUFFER_AVAIL_TAIL(buffer);				      \
	PUT_NET_32((val), BUFFER_DATA_TAIL(buffer), buflen_);		      \
} while (0)

#ifndef __cplusplus
#define BUFFER_GET_IPADDR(family, ipaddr, buffer)			      \
				_BUFFER_GET_IPADDR_C(family, ipaddr, buffer)
#define BUFFER_PUT_IPADDR(ipaddr, buffer)				      \
				_BUFFER_PUT_IPADDR_C(ipaddr, buffer)
#else	/* C++ */
#define BUFFER_GET_IPADDR(family, ipaddr, buffer)			      \
				_BUFFER_GET_IPADDR_CPP(family, ipaddr, buffer)
#define BUFFER_PUT_IPADDR(ipaddr, buffer)				      \
				_BUFFER_PUT_IPADDR_CPP(ipaddr, buffer)
#define BUFFER_GET_IPVX(family, ipaddr, buffer)				      \
				BUFFER_GET_IPADDR(family, ipaddr, buffer)
#define BUFFER_PUT_IPVX(ipaddr, buffer)					      \
				BUFFER_PUT_IPADDR(ipaddr, buffer)
#endif /* __cplusplus */

/* C version of IP addresses */
#define _BUFFER_GET_IPADDR_C(family, ipaddr, buffer)			      \
do {									      \
	if (BUFFER_DATA_SIZE(buffer) < FAMILY2ADDRSIZE((family)))	      \
		goto rcvlen_error;					      \
	MEMORY2IPADDR((family), BUFFER_DATA_HEAD((buffer)), (ipaddr));	      \
	BUFFER_GET_SKIP(FAMILY2ADDRSIZE((family)), (buffer));		      \
} while (0)
#define _BUFFER_PUT_IPADDR_C(ipaddr, buffer)				      \
do {									      \
	if (BUFFER_AVAIL_TAIL((buffer))					      \
		< FAMILY2ADDRSIZE(IPADDR2FAMILY((ipaddr))))		      \
		goto buflen_error;					      \
	IPADDR2MEMORY((ipaddr), BUFFER_DATA_TAIL((buffer)));		      \
	BUFFER_PUT_SKIP(FAMILY2ADDRSIZE(IPADDR2FAMILY((ipaddr))), (buffer));  \
} while (0)

/* C++ version of IP addresses */
#define _BUFFER_GET_IPADDR_CPP(family, ipvx, buffer)			      \
do {									      \
	if (BUFFER_DATA_SIZE(buffer) < family2addr_size((family)))	      \
		goto rcvlen_error;					      \
	(ipvx).copy_in(family, BUFFER_DATA_HEAD((buffer)));		      \
	BUFFER_GET_SKIP(family2addr_size((family)), (buffer));		      \
} while (0)
#define _BUFFER_PUT_IPADDR_CPP(ipvx, buffer)				      \
do {									      \
	if (BUFFER_AVAIL_TAIL((buffer))	< (ipvx).addr_size())		      \
		goto buflen_error;					      \
	(ipvx).copy_out(BUFFER_DATA_TAIL((buffer)));			      \
	BUFFER_PUT_SKIP((ipvx).addr_size(), (buffer));			      \
} while (0)

/*
 * Wrappers for the buffer functions.
 */
#define BUFFER_MALLOC(buffer_size)	(buffer_malloc_utils_((buffer_size)))
#define BUFFER_FREE(buffer)						      \
do {									      \
	buffer_free_utils_((buffer));					      \
	(buffer) = NULL;						      \
} while (0)
#define BUFFER_RESET(buffer)		(buffer_reset_utils_((buffer)))

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS
extern buffer_t *	buffer_malloc_utils_(size_t buffer_size);
extern void		buffer_free_utils_(buffer_t *buffer);
extern buffer_t *	buffer_reset_utils_(buffer_t *buffer);
__END_DECLS

#endif /* __MRT_BUFFER_H__ */
