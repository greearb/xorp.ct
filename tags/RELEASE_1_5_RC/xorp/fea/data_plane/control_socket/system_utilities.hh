// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/data_plane/control_socket/system_utilities.hh,v 1.3 2007/06/04 23:17:33 pavlin Exp $

#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_SYSTEM_UTILITIES_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_SYSTEM_UTILITIES_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "libproto/packet.hh"


//
// Conditionally re-define some of the system macros that are not
// defined properly and might generate alignment-related compilation
// warning on some architectures (e.g, ARM/XScale) if we use
// "-Wcast-align" compilation flag.
//
#ifdef HAVE_BROKEN_MACRO_CMSG_NXTHDR
#if ((! defined(__CMSG_ALIGN)) && (! defined(_ALIGN)))
#error "The system has broken CMSG_NXTHDR, but we don't know how to re-define it. Fix the alignment compilation error of the CMSG_NXTHDR macro in your system header files."
#endif

#ifndef __CMSG_ALIGN
#define __CMSG_ALIGN(n)		_ALIGN(n)
#endif

#undef CMSG_NXTHDR
#define	CMSG_NXTHDR(mhdr, cmsg)	\
	(((caddr_t)(cmsg) + __CMSG_ALIGN((cmsg)->cmsg_len) + \
			    __CMSG_ALIGN(sizeof(struct cmsghdr)) > \
	    ((caddr_t)(mhdr)->msg_control) + (mhdr)->msg_controllen) ? \
	    (struct cmsghdr *)NULL : \
	    (struct cmsghdr *)(void *)((caddr_t)(cmsg) + __CMSG_ALIGN((cmsg)->cmsg_len)))
#endif // HAVE_BROKEN_MACRO_CMSG_NXTHDR

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
system_adjust_ipv6_recv(const IPv6& ipv6)
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
system_adjust_ipvx_recv(const IPvX& ipvx)
{
#ifndef HAVE_IPV6
    return ipvx;
#else
    if (! ipvx.is_ipv6())
	return ipvx;
    return (system_adjust_ipv6_recv(ipvx.get_ipv6()));
#endif // HAVE_IPV6
}

//
// XXX: In case of KAME the local interface index should be set as
// the link-local scope_id ((for link-local unicast/multicast addresses or
// interface-local multicast addresses only).
//
#ifdef HAVE_IPV6
inline void
system_adjust_sockaddr_in6_send(struct sockaddr_in6& sin6, uint16_t zone_id)
{
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
#ifdef IPV6_STACK_KAME
    if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)
	|| IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)
	|| IN6_IS_ADDR_MC_NODELOCAL(&sin6.sin6_addr)) {
	sin6.sin6_scope_id = zone_id;
    }
#endif // IPV6_STACK_KAME
#endif // HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID

    UNUSED(sin6);
    UNUSED(zone_id);
}
#endif // HAVE_IPV6

//
// XXX: In case of KAME sometimes the local interface index is encoded
// in the third and fourth octet of an IPv6 address (for link-local
// unicast/multicast addresses) when we add an unicast route to the system.
// E.g., see /usr/src/sbin/route/route.c on FreeBSD-6.2 and search for
// the __KAME__ marker.
//
#ifdef HAVE_IPV6
inline void
system_adjust_sockaddr_in6_route(struct sockaddr_in6& sin6, uint16_t iface_id)
{
#ifdef IPV6_STACK_KAME
    if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)
	|| IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) {
	embed_16(&sin6.sin6_addr.s6_addr[2], iface_id);
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
	sin6.sin6_scope_id = 0;
#endif
    }
#endif // IPV6_STACK_KAME

    UNUSED(sin6);
    UNUSED(iface_id);
}
#endif // HAVE_IPV6

#endif // _FEA_DATA_PLANE_CONTROL_SOCKET_SYSTEM_UTILITIES_HH__
