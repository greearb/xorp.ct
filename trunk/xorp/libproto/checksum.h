/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
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
 * $XORP: xorp/libproto/checksum.h,v 1.4 2008/01/04 03:16:19 pavlin Exp $
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
