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
 * $XORP: xorp/libproto/checksum.h,v 1.6 2008/10/02 21:57:17 bms Exp $
 */

#ifndef __LIBPROTO_CHECKSUM_H__
#define __LIBPROTO_CHECKSUM_H__

/*
 * Header file for checksum computations.
 */

# ifdef __cplusplus
extern "C" {
# endif


/**
 * Checksum computation for Internet Protocol family headers.
 *
 * @param addr the address with the data.
 * @param len the length of the data.
 * @return the calculated checksum (in network order).
 */
extern uint16_t inet_checksum(const uint8_t *addr, size_t len);

/**
 * Add two previously computed checksums for Internet Protocol family header.
 *
 * Note that if both checksums to add are in host order, the result is also in
 * host order. Similarly, if both checksums to add are in network order, the
 * result is also in network order.
 *
 * @param sum1 the first sum to add.
 * @param sum2 the second sum to add.
 * @return the sum of the two checksums.
 */
extern uint16_t inet_checksum_add(uint16_t sum1, uint16_t sum2);

# ifdef __cplusplus
}
# endif

#endif /* __LIBPROTO_CHECKSUM_H__ */
