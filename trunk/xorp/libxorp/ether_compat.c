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

#ident "$XORP: xorp/libxorp/ether_compat.c,v 1.2 2004/06/22 23:46:35 pavlin Exp $"

/*
 * Part of this software is derived from the following file(s):
 *   - OpenBSD's "src/lib/libc/net/ethers.c"
 *
 * The copyright message(s) with the original file(s) is/are included below.
 */

/*	$OpenBSD: ethers.c,v 1.17 2004/02/16 19:41:12 otto Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* 
 * ethers(3) a la Sun.
 * Originally Written by Roland McGrath <roland@frob.com> 10/14/93.
 * Substantially modified by Todd C. Miller <Todd.Miller@courtesan.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "libxorp_module.h"
#include "xorp.h"
#include "ether_compat.h"
#include "utility.h"


#ifdef NEED_ETHER_ATON
/*
 * Convert an ASCII representation of an ethernet address to
 * binary form.
 */
struct ether_addr *
ether_aton(const char *a)
{
	int i;
	long l;
	static struct ether_addr e;
	const char *s = a;
	char *pp;

	while (xorp_isspace(*s))
		s++;

	/* expect 6 hex octets separated by ':' or space/NUL if last octet */
	for (i = 0; i < 6; i++) {
		l = strtol(s, &pp, 16);
		if (pp == s || l > 0xFF || l < 0)
			return (NULL);
		if (!(*pp == ':' ||
			(i == 5 && (xorp_isspace(*pp) || *pp == '\0'))))
			return (NULL);
		e.ether_addr_octet[i] = (u_char)l;
		s = pp + 1;
	}

	return (&e);
}
#endif /* NEED_ETHER_ATON */

#ifdef NEED_ETHER_NTOA
/*
 * Convert a binary representation of an ethernet address to
 * an ASCII string.
 */
char *
ether_ntoa(const struct ether_addr *e)
{
	static char a[] = "xx:xx:xx:xx:xx:xx";

	(void)snprintf(a, sizeof a, "%02x:%02x:%02x:%02x:%02x:%02x",
	    e->ether_addr_octet[0], e->ether_addr_octet[1],
	    e->ether_addr_octet[2], e->ether_addr_octet[3],
	    e->ether_addr_octet[4], e->ether_addr_octet[5]);

	return (a);
}
#endif /* NEED_ETHER_NTOA */
