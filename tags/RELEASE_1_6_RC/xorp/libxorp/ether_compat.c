/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/ether_compat.c,v 1.14 2008/07/23 05:10:51 pavlin Exp $"

/*
 * Part of this software is derived from the following file(s):
 *   tcpdump/addrtoname.c (from FreeBSD)
 *   lib/libc/net/ether_addr.c (from FreeBSD)
 * The copyright message(s) with the original file(s) is/are included below.
 */

/*
 * Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Internet, ethernet, port, and protocol string to address
 *  and address to string conversion routines
 *
 * $FreeBSD: src/contrib/tcpdump/addrtoname.c,v 1.12 2004/03/31 14:57:24 bms Exp $
 */

/*
 * Copyright (c) 1995
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ethernet address conversion and lookup routines
 *
 * Written by Bill Paul <wpaul@ctr.columbia.edu>
 * Center for Telecommunications Research
 * Columbia University, New York City
 */
#if 0
__FBSDID("$FreeBSD: /repoman/r/ncvs/src/lib/libc/net/ether_addr.c,v 1.15 2002/04/08 07:51:10 ru Exp $");
#endif

#include "libxorp/xorp.h"
#include "libxorp/utility.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <inttypes.h>
#include <ctype.h>


#ifndef HAVE_ETHER_NTOA
/* XXX: returns a pointer to static storage. */
char *
ether_ntoa(const struct ether_addr *e)
{
	static const char hex[] = "0123456789abcdef";
	static char buf[sizeof("00:00:00:00:00:00")];
	char *cp;
	const uint8_t *ep;
	unsigned int i, j;

	cp = buf;
	ep = (const uint8_t *)e->octet;
	if ((j = *ep >> 4) != 0)
		*cp++ = hex[j];
	*cp++ = hex[*ep++ & 0xf];
	for (i = 5; (int)--i >= 0;) {
		*cp++ = ':';
		if ((j = *ep >> 4) != 0)
		*cp++ = hex[j];
	*cp++ = hex[*ep++ & 0xf];
	}
	*cp = '\0';

	return (buf);
}
#endif /* !HAVE_ETHER_NTOA */

#ifndef HAVE_ETHER_ATON
/*
 * Convert 's' which has the form "x:x:x:x:x:x" into a new
 * ethernet address.
 * XXX: returns a pointer to static storage.
 */
struct ether_addr *
ether_aton(const char *s)
{
    int i;
    static struct ether_addr o;
    unsigned int od[6];

    i = sscanf(s, "%x:%x:%x:%x:%x:%x",
	       &od[0], &od[1], &od[2], &od[3], &od[4], &od[5]);
    if (i != 6)
	return (NULL);

    for (i = 0; i < 6; i++)
	o.octet[i] = od[i];

    return (&o);
}
#endif /* !HAVE_ETHER_ATON */
