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
 * $XORP: xorp/mrt/include/ip_mroute.h,v 1.3 2003/04/15 18:55:37 pavlin Exp $
 */

#ifndef __MRT_INCLUDE_IP_MROUTE_H__
#define __MRT_INCLUDE_IP_MROUTE_H__

#include "config.h"

#if defined(ENABLE_ADVANCED_MCAST_API) && defined(HOST_OS_FREEBSD)
/* XXX: for now we use same header file for all FreeBSD versions */
#include "mrt/include/netinet/ip_mroute_adv_api_freebsd_4_5.h"
#else

#ifdef HOST_OS_LINUX
/* XXX: linux/mroute.h is broken for older Linux versions */
#include "mrt/include/linux/netinet/mroute.h"
#endif

#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
/*
 * XXX: in NetBSD and OpenBSD, struct igmpmsg definition can be seen
 * only by kernel...
 */
#define	_KERNEL
#endif
#ifndef HOST_OS_LINUX
#include <netinet/ip_mroute.h>
#endif
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
#undef _KERNEL
#endif

#endif /* ! (ENABLE_ADVANCED_MCAST_API && HOST_OS_FREEBSD) */


#ifdef HAVE_IPV6

/* XXX: on FreeBSD-4.3 including both ip_mroute.h and ip6_mroute.h is broken */
/* Save GET_TIME */
#ifdef GET_TIME
# define _SAVE_GET_TIME GET_TIME
# undef GET_TIME
#endif

#ifdef HAVE_NETINET6_IP6_MROUTE_H
#include <netinet6/ip6_mroute.h>
#endif

/* Restore GET_TIME */
#if defined(_SAVE_GET_TIME) && (! defined(GET_TIME))
# define GET_TIME _SAVE_GET_TIME
# undef _SAVE_GET_TIME
#endif

#endif /* HAVE_IPV6 */

#endif /* __MRT_INCLUDE_IP_MROUTE_H__ */
