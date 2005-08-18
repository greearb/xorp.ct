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

/*
 * $XORP: xorp/mrt/include/ip_mroute.h,v 1.9 2005/03/25 02:53:58 pavlin Exp $
 */

#ifndef __MRT_INCLUDE_IP_MROUTE_H__
#define __MRT_INCLUDE_IP_MROUTE_H__

#include "config.h"

#ifdef HAVE_NET_ROUTE_H
# include <net/route.h>
#endif

/*
 * FreeBSD (all versions)
 */
#if defined(HOST_OS_FREEBSD)
# include <netinet/ip_mroute.h>
#endif

/*
 * NetBSD (all versions)
 * OpenBSD (all versions)
 *
 * Prologue.
 *
 * The definition of 'struct igmpmsg' is wrapped under
 * an #ifdef _KERNEL conditional on these platforms.
 */
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
# define	_KERNEL
#endif

/*
 * Non-Linux platforms with the <netinet/ip_mroute.h>
 * header available.
 */
#if defined(HAVE_NETINET_IP_MROUTE_H) && !defined(HOST_OS_LINUX)
# include <netinet/ip_mroute.h>
#endif

/*
 * NetBSD (all versions)
 * OpenBSD (all versions)
 *
 * Epilogue.
 */
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
# undef _KERNEL
#endif

/*
 * Linux (Older versions)
 *
 * We ship our own Linux <netinet/mroute.h> header to
 * workaround broken headers on certain Linux variants.
 */
#if defined(HOST_OS_LINUX)
# include "mrt/include/linux/netinet/mroute.h"
#endif

/*
 * FreeBSD 4.3
 *
 * On this platform, attempting to include both the ip_mroute.h
 * and ip6_mroute.h headers results in undefined behavior.
 * Therefore we implement a preprocessor workaround here.
 */
#ifdef HAVE_IPV6

/* Save GET_TIME. */
# ifdef GET_TIME
#  define _SAVE_GET_TIME GET_TIME
#  undef GET_TIME
# endif

# ifdef HAVE_NETINET6_IP6_MROUTE_H
#  include <netinet6/ip6_mroute.h>
# endif

/* Restore GET_TIME. */
# if defined(_SAVE_GET_TIME) && !defined(GET_TIME)
#  define GET_TIME _SAVE_GET_TIME
#  undef _SAVE_GET_TIME
# endif

#endif /* HAVE_IPV6 */

#endif /* __MRT_INCLUDE_IP_MROUTE_H__ */
