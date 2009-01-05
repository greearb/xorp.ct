/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
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
 * $XORP: xorp/mrt/multicast_defs.h,v 1.11 2008/10/02 21:57:45 bms Exp $
 */

#ifndef __MRT_MULTICAST_DEFS_H__
#define __MRT_MULTICAST_DEFS_H__


/*
 * Various multicast-related definitions.
 */
/* XXX: everything here probably should go somewhere else. */


#include "libxorp/xorp.h"

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif


/*
 * Constants definitions
 */
enum action_jp_t {
    ACTION_JOIN	= 0,
    ACTION_PRUNE
};
#define ACTION_JP2ASCII(action_flag)			\
	(((action_flag) == ACTION_JOIN) ? 		\
		"JOIN" : "PRUNE")

#ifndef MINTTL
#define MINTTL		1
#endif
#ifndef IPDEFTTL
#define IPDEFTTL	64
#endif
#ifndef MAXTTL
#define MAXTTL		255
#endif


/*
 * Structures, typedefs and macros
 */

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

__END_DECLS


#endif /* __MRT_MULTICAST_DEFS_H__ */
