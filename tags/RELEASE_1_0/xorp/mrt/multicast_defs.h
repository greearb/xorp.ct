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
 * $XORP: xorp/mrt/multicast_defs.h,v 1.2 2003/03/10 23:20:45 hodson Exp $
 */

#ifndef __MRT_MULTICAST_DEFS_H__
#define __MRT_MULTICAST_DEFS_H__


/*
 * Various multicast-related definitions.
 */
/* XXX: everything here probably should go somewhere else. */


#include "libxorp/xorp.h"
#include <netinet/in_systm.h>
#include <netinet/ip.h>


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
