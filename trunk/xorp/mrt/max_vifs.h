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
 * $XORP: xorp/mrt/max_vifs.h,v 1.2 2003/03/10 23:20:44 hodson Exp $
 */

#ifndef __MRT_MAX_VIFS_H__
#define __MRT_MAX_VIFS_H__


/*
 * Header file to define the maximum number of multicast-capable vifs
 * in the constant MAX_VIFS .
 */


#include "config.h"
#include "libxorp/xorp.h"

#include <sys/time.h>

#include "mrt/include/ip_mroute.h"


/*
 * Constants definitions
 */

/*
 * XXX: Define MAX_VIFS to be the largest of MAXVIFS, MAXMIFS, and 32
 */
#ifndef MAX_VIFS
#  define MAX_VIFS 32
#elif (32 > MAX_VIFS)
#  undef MAX_VIFS
#  define MAX_VIFS 32
#endif

#if defined(MAXVIFS) && (MAXVIFS > MAX_VIFS)
#  undef MAX_VIFS
#  define MAX_VIFS MAXVIFS
#endif // MAXVIFS > MAX_VIFS

#if defined(MAXMIFS) && (MAXMIFS > MAX_VIFS)
#  undef MAX_VIFS
#  define MAX_VIFS MAXMIFS
#endif // MAXMIFS > MAX_VIFS


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

#endif /* __MRT_MAX_VIFS_H__ */
