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
 * $XORP: xorp/mrt/include/ip_mroute.h,v 1.21 2008/10/02 21:57:46 bms Exp $
 */

#ifndef __MRT_INCLUDE_IP_MROUTE_H__
#define __MRT_INCLUDE_IP_MROUTE_H__

#include "libxorp/xorp.h"

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
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
 * XXX: On these platforms the definition of 'struct igmpmsg'
 * and IGMPMSG_* is wrapped inside #ifdef _KERNEL hence we need
 * to define _KERNEL before including <netinet/ip_mroute.h>.
 */
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
#define	_KERNEL
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
#undef _KERNEL
#endif

/*
 * DragonFlyBSD
 *
 * DragonFlyBSD (as per version 1.4) has moved <netinet/ip_mroute.h> to
 * <net/ip_mroute/ip_mroute.h>. Hopefully, in the future it will be back
 * to its appropriate location.
 */
#ifdef HAVE_NET_IP_MROUTE_IP_MROUTE_H
# include <net/ip_mroute/ip_mroute.h>
#endif

/*
 * Linux hacks because of broken Linux header files
 */
#if defined(HOST_OS_LINUX)
#  include <linux/types.h>
#  ifndef _LINUX_IN_H
#    define _LINUX_IN_H		/*  XXX: a hack to exclude <linux/in.h> */
#    define EXCLUDE_LINUX_IN_H
#  endif
#  ifndef __LINUX_PIM_H
#    define __LINUX_PIM_H	/*  XXX: a hack to exclude <linux/pim.h> */
#    define EXCLUDE_LINUX_PIM_H
#  endif
#    include <linux/mroute.h>
#  ifdef EXCLUDE_LINUX_IN_H
#    undef _LINUX_IN_H
#    undef EXCLUDE_LINUX_IN_H
#  endif
#  ifdef EXCLUDE_LINUX_PIM_H
#    undef __LINUX_PIM_H
#    undef EXCLUDE_LINUX_PIM_H
#  endif
/*
 * XXX: Conditionally add missing definitions from the <linux/mroute.h>
 * header file.
 */
#  ifndef IGMPMSG_NOCACHE
#    define IGMPMSG_NOCACHE 1
#   endif
#  ifndef IGMPMSG_WRONGVIF
#    define IGMPMSG_WRONGVIF 2
#  endif
#  ifndef IGMPMSG_WHOLEPKT
#    define IGMPMSG_WHOLEPKT 3
#  endif
#endif /* HOST_OS_LINUX */

#ifdef HAVE_LINUX_MROUTE6_H
#include <linux/mroute6.h>
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
#  ifdef GET_TIME
#    define _SAVE_GET_TIME GET_TIME
#    undef GET_TIME
#  endif

#  ifdef HAVE_NETINET6_IP6_MROUTE_H
#    include <netinet6/ip6_mroute.h>
#  endif

/* Restore GET_TIME. */
#  if defined(_SAVE_GET_TIME) && !defined(GET_TIME)
#    define GET_TIME _SAVE_GET_TIME
#    undef _SAVE_GET_TIME
#  endif

#endif /* HAVE_IPV6 */

#endif /* __MRT_INCLUDE_IP_MROUTE_H__ */
