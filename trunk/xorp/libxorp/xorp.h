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
 * $XORP: xorp/libxorp/xorp.h,v 1.4 2003/04/15 17:09:24 hodson Exp $
 */


#ifndef __LIBXORP_XORP_H__
#define __LIBXORP_XORP_H__

#include "config.h"

#ifdef __cplusplus
#include <new>
#include <iostream>
#include <string>
#include <algorithm>
#include <utility>
#include <cstdarg>
#include <cstdio>
#endif /* __cplusplus */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/route.h>
#include <arpa/inet.h>  

#ifdef HAVE_MD5GLOBAL_H
#include <md5global.h>
#endif

#include "utility.h"

#if defined (__cplusplus) && !defined(__STL_NO_NAMESPACES)
using namespace std::rel_ops;
#endif

/*
 * Misc. definitions that may be missing from the system header files.
 * TODO: this should go to a different header file.
 */

#ifndef NBBY
#define NBBY	(8)	/* number of bits in a byte */
#endif

/*
 * Redefine NULL as per Stroustrup to get additional error checking.
 */
#ifdef NULL
#   undef NULL
#   if defined __GNUG__ && \
	(__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#	define NULL (__null)
#   else
#	if !defined(__cplusplus)
#	    define NULL ((void*)0)
#	else
#	    define NULL (0)
#	endif
#   endif
#endif /* NULL */

/*
 * Boolean values
 */
#if (!defined(TRUE)) || (!defined(FALSE))
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define FALSE (0)
#define TRUE (!FALSE)
#endif /* TRUE, FALSE */
#ifndef __cplusplus
typedef enum { true = TRUE, false = FALSE } bool;
#endif
typedef bool bool_t;

/*
 * Generic error return codes
 */
#ifdef XORP_OK
#undef XORP_OK
#endif
#ifdef XORP_ERROR
#undef XORP_ERROR
#endif
#define XORP_OK		 (0)
#define XORP_ERROR	(-1)

#endif /* __LIBXORP_XORP_H__ */
