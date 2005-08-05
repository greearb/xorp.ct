/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/ether_compat.c,v 1.4 2005/03/25 02:53:40 pavlin Exp $"

/*
 * Part of this software is derived from the following file(s):
 *   tcpdump/addrtoname.c (from FreeBSD)
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
#if 0
static const char rcsid[] _U_ =
    "@(#) $Header$ (LBL)";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp/xorp.h"
#include "libxorp/utility.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <inttypes.h>
#include <ctype.h>

#ifdef NEED_ETHER_NTOA
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
#endif /* NEED_ETHER_NTOA */

#ifdef NEED_ETHER_ATON

/* Hex digit to integer. */
#ifdef __STDC__
inline
#endif
static int
xdtoi(int c)
{
	if (isdigit(c))
		return (c - '0');
	else if (islower(c))
		return (c - 'a' + 10);
	else
		return (c - 'A' + 10);
}

/*
 * Convert 's' which has the form "xx:xx:xx:xx:xx:xx" into a new
 * ethernet address.  Assumes 's' is well formed.
 * XXX: returns a pointer to static storage.
 */
struct ether_addr *
ether_aton(const char *s)
{
	static struct ether_addr o;
	uint8_t *ep, *e;
	unsigned int d;

	e = ep = (uint8_t *)&o;

	while (*s) {
		if (*s == ':')
			s += 1;
		d = xdtoi(*s++);
		if (isxdigit((uint8_t)*s)) {
			d <<= 4;
			d |= xdtoi(*s++);
		}
		*ep++ = d;
	}

	return (&o);
}
#endif /* NEED_ETHER_ATON */
