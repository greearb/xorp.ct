/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
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
 * $XORP: xorp/libxorp/ether_compat.h,v 1.20 2008/10/28 14:08:18 bms Exp $
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

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP	0x0800
#endif

#ifndef ETHERTYPE_ARP
#define ETHERTYPE_ARP	0x0806
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LIBXORP_ETHER_COMPAT_H__ */
