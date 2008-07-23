/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
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
 * $XORP: xorp/libxorp/ether_compat.h,v 1.17 2008/01/04 03:16:35 pavlin Exp $
 */

/* Ethernet manipulation compatibility functions */

#ifndef __LIBXORP_ETHER_COMPAT_H__
#define __LIBXORP_ETHER_COMPAT_H__

#include "libxorp/xorp.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_ETHERNET_H
#include <sys/ethernet.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef HAVE_STRUCT_ETHER_ADDR
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif
struct ether_addr {
	char	octet[ETHER_ADDR_LEN];
};
#endif /* HAVE_STRUCT_ETHER_ADDR */

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
