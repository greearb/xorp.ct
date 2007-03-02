// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/fea/kernel_utils.hh,v 1.7 2007/02/16 22:45:45 pavlin Exp $

#ifndef __FEA_KERNEL_UTILS_HH__
#define __FEA_KERNEL_UTILS_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "libproto/packet.hh"


//
// XXX: In case of KAME the local interface index (also the link-local
// scope_id) is encoded in the third and fourth octet of an IPv6
// address (for link-local unicast/multicast addresses or
// interface-local multicast addresses only).
// E.g., see the sa6_recoverscope() implementation inside file
// "sys/netinet6/scope6.c" on KAME-derived IPv6 stack.
//
#ifdef HAVE_IPV6
inline IPv6
kernel_adjust_ipv6_recv(const IPv6& ipv6)
{
#ifdef IPV6_STACK_KAME
    in6_addr in6_addr;
    ipv6.copy_out(in6_addr);
    if (IN6_IS_ADDR_LINKLOCAL(&in6_addr)
	|| IN6_IS_ADDR_MC_LINKLOCAL(&in6_addr)
	|| IN6_IS_ADDR_MC_NODELOCAL(&in6_addr)) {
	in6_addr.s6_addr[2] = 0;
	in6_addr.s6_addr[3] = 0;
	return IPv6(in6_addr);
    }
#endif // IPV6_STACK_KAME

    return ipv6;
}
#endif // HAVE_IPV6

inline IPvX
kernel_adjust_ipvx_recv(const IPvX& ipvx)
{
#ifndef HAVE_IPV6
    return ipvx;
#else
    if (! ipvx.is_ipv6())
	return ipvx;
    return (kernel_adjust_ipv6_recv(ipvx.get_ipv6()));
#endif // HAVE_IPV6
}

//
// XXX: In case of KAME the local interface index should be set as
// the link-local scope_id ((for link-local unicast/multicast addresses or
// interface-local multicast addresses only).
//
#ifdef HAVE_IPV6
inline void
kernel_adjust_sockaddr_in6_send(struct sockaddr_in6& sin6, uint16_t zone_id)
{
#ifdef HAVE_SIN6_SCOPE_ID
#ifdef IPV6_STACK_KAME
    if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)
	|| IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)
	|| IN6_IS_ADDR_MC_NODELOCAL(&sin6.sin6_addr)) {
	sin6.sin6_scope_id = zone_id;
    }
#endif // IPV6_STACK_KAME
#endif // HAVE_SIN6_SCOPE_ID

    UNUSED(sin6);
    UNUSED(zone_id);
}
#endif // HAVE_IPV6

//
// XXX: In case of KAME sometimes the local interface index is encoded
// in the third and fourth octet of an IPv6 address (for link-local
// unicast/multicast addresses) when we add an unicast route to the kernel.
// E.g., see /usr/src/sbin/route/route.c on FreeBSD-6.2 and search for
// the __KAME__ marker.
//
#ifdef HAVE_IPV6
inline void
kernel_adjust_sockaddr_in6_route(struct sockaddr_in6& sin6, uint16_t iface_id)
{
#ifdef IPV6_STACK_KAME
    if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)
	|| IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) {
	embed_16(&sin6.sin6_addr.s6_addr[2], iface_id);
#ifdef HAVE_SIN6_SCOPE_ID
	sin6.sin6_scope_id = 0;
#endif // HAVE_SIN6_SCOPE_ID
    }
#endif // IPV6_STACK_KAME

    UNUSED(sin6);
    UNUSED(iface_id);
}
#endif // HAVE_IPV6

#endif // __FEA_KERNEL_UTILS_HH__
