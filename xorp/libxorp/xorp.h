/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2010 XORP, Inc and Others
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
 * $XORP: xorp/libxorp/xorp.h,v 1.20 2009/01/05 18:30:58 jtc Exp $
 */


#ifndef __LIBXORP_XORP_H__
#define __LIBXORP_XORP_H__

#include "xorp_config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if defined (__cplusplus)
#if defined (USE_BOOST) && defined (HAS_BOOST_NONCOPYABLE_INC)
#  include <boost/noncopyable.hpp>
#  define NONCOPYABLE boost::noncopyable
#else
#  define NONCOPYABLE xorp_noncopyable
#  include "xorp_noncopyable.hh"
#endif
#endif

#if defined (__cplusplus) && !defined(__STL_NO_NAMESPACES)
using namespace std;
#endif



/*
 * Include sys/cdefs.h to define __BEGIN_DECLS and __END_DECLS.  Even if
 * this file exists, not all platforms define these macros.
 */
#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

/*
 * Define C++ decls wrappers if not previously defined.
 */
#ifndef __BEGIN_DECLS
#  if defined(__cplusplus)
#    define __BEGIN_DECLS	extern "C" {
#    define __END_DECLS		};    
#  else /* __BEGIN_DECLS */
#    define __BEGIN_DECLS
#    define __END_DECLS
#  endif /* __BEGIN_DECLS */
#endif /* __BEGIN_DECLS */

#include "xorp_osdep_begin.h"

#ifdef __cplusplus
#include <new>
#include <iostream>
#include <string>
#include <algorithm>
#include <utility>
#include <cstdarg>
#include <cstdio>
#endif /* __cplusplus */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
/*
 * XXX: Include <net/if.h> upfront so we can create a work-around because
 * of a problematic name collusion in the Solaris-10 header file.
 * That file contains "struct map *" pointer that prevents us from
 * including the STL <map> file.
 */
#ifdef HAVE_NET_IF_H
#define map solaris_class_map
#include <net/if.h>
#undef map
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <stdarg.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include <time.h>

#include "utility.h"

#include "xorp_osdep_mid.h"

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

#include "xorp_osdep_end.h"

#endif /* __LIBXORP_XORP_H__ */
