/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2003 International Computer Science Institute
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
 * $XORP: xorp/libxorp/status_codes.h,v 1.2 2003/05/07 23:49:59 pavlin Exp $
 */

#ifndef __LIBXORP_STATUS_CODES_H__
#define __LIBXORP_STATUS_CODES_H__

typedef enum {
    PROC_NULL		= 0,
    PROC_STARTUP	= 1,
    PROC_NOT_READY	= 2,
    PROC_READY		= 3,
    PROC_SHUTDOWN	= 4,
    PROC_FAILED		= 5
} ProcessStatus;

#endif /* __LIBXORP_STATUS_CODES_H__ */
