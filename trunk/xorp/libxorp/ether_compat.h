/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2006 International Computer Science Institute
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
 * $XORP: xorp/libxorp/ether_compat.h,v 1.11 2005/08/18 15:28:39 bms Exp $
 */

/* Ethernet manipulation compatibility functions */

#ifndef __LIBXORP_ETHER_COMPAT_H__
#define __LIBXORP_ETHER_COMPAT_H__

#ifndef __XORP_CONFIG_H__
#error "config.h must be included before ether_compat.h"
#endif /* __XORP_CONFIG_H__ */

#ifdef HAVE_NET_ETHERNET_H
# include <net/ethernet.h>
#elif defined(HAVE_NET_IF_ETHER_H)
# ifdef HAVE_NET_IF_H
#  include <net/if.h>
# endif
# include <net/if_ether.h>
#elif defined(HAVE_SYS_ETHERNET_H)
# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif
# include <sys/ethernet.h>
#endif

#if defined(HAVE_NETINET_ETHER_H)
# ifdef HAVE_NET_IF_H
#  include <net/if.h>
# endif
# include <netinet/ether.h>
#elif defined(HAVE_NETINET_IF_ETHER_H)
# ifdef HAVE_NET_IF_H
#  include <net/if.h>
# endif
# include <netinet/if_ether.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
struct ether_addr {
	char	octet[ETHER_ADDR_LEN];
};
#endif

#ifndef HAVE_ETHER_ATON
struct ether_addr* ether_aton(const char *a);
#endif

#ifndef HAVE_ETHER_NTOA
char* ether_ntoa(const struct ether_addr* ea);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LIBXORP_ETHER_COMPAT_H__ */
