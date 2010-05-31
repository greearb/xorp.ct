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
 * Checksum computations.
 */


#include "libproto_module.h"
#include "libxorp/xorp.h"

#include "checksum.h"


/*
 * inet_checksum extracted from:
 *			P I N G . C
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 * Modified at Uc Berkeley
 *
 * (ping.c) Status -
 *	Public Domain.  Distribution Unlimited.
 *
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
uint16_t
inet_checksum(const uint8_t *addr, size_t len)
{
	register size_t nleft = len;
	register const uint8_t *w = addr;
	uint16_t answer = 0;
	register uint32_t sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while (nleft > 1)  {
		sum += ((w[0] << 8) | w[1]);
		w += 2;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		/*
		 * XXX: If the number of bytes is odd, we assume a padding
		 * with a zero byte, hence we just "<< 8" the remaining
		 * odd byte.
		 */
		sum += (w[0] << 8);
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */

	/*
	 * XXX: We need to swap the bytes, because we implicitly performed
	 * network-to-host order swapping when we accessed the data.
	 */
	answer = htons(answer);			/* swap the bytes */
	return (answer);
}

/*
 * inet_checksum_add based on inet_cksum extracted from:
 *			P I N G . C
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 *
 *			I N _ C K S U M _ A D D
 *
 * Checksum routine for Internet Protocol family headers (C Version):
 *		adds two previously computed checksums.
 *
 */
uint16_t
inet_checksum_add(uint16_t sum1, uint16_t sum2)
{
	register uint32_t sum = (uint16_t)~sum1 + (uint16_t)~sum2;
	uint16_t answer;
	
	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}
