// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/routing_socket_utils.cc,v 1.16 2004/06/10 22:40:56 hodson Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#include <net/route.h>
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#include "kernel_utils.hh"
#include "routing_socket_utils.hh"

//
// RTM format related utilities for manipulating data
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//

#ifdef HAVE_ROUTING_SOCKETS

/**
 * @param m message type from routing socket message
 * @return human readable form.
 */
string
RtmUtils::rtm_msg_type(uint32_t m)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } rtm_msg_types[] = {
#define RTM_MSG_ENTRY(X) { X, #X }
#ifdef RTM_ADD
	RTM_MSG_ENTRY(RTM_ADD),
#endif
#ifdef RTM_DELETE
	RTM_MSG_ENTRY(RTM_DELETE),
#endif
#ifdef RTM_CHANGE
	RTM_MSG_ENTRY(RTM_CHANGE),
#endif
#ifdef RTM_GET
	RTM_MSG_ENTRY(RTM_GET),
#endif
#ifdef RTM_LOSING
	RTM_MSG_ENTRY(RTM_LOSING),
#endif
#ifdef RTM_REDIRECT
	RTM_MSG_ENTRY(RTM_REDIRECT),
#endif
#ifdef RTM_MISS
	RTM_MSG_ENTRY(RTM_MISS),
#endif
#ifdef RTM_LOCK
	RTM_MSG_ENTRY(RTM_LOCK),
#endif
#ifdef RTM_OLDADD
	RTM_MSG_ENTRY(RTM_OLDADD),
#endif
#ifdef RTM_OLDDEL
	RTM_MSG_ENTRY(RTM_OLDDEL),
#endif
#ifdef RTM_RESOLVE
	RTM_MSG_ENTRY(RTM_RESOLVE),
#endif
#ifdef RTM_NEWADDR
	RTM_MSG_ENTRY(RTM_NEWADDR),
#endif
#ifdef RTM_DELADDR
	RTM_MSG_ENTRY(RTM_DELADDR),
#endif
#ifdef RTM_IFINFO
	RTM_MSG_ENTRY(RTM_IFINFO),
#endif
#ifdef RTM_NEWMADDR
	RTM_MSG_ENTRY(RTM_NEWMADDR),
#endif
#ifdef RTM_DELMADDR
	RTM_MSG_ENTRY(RTM_DELMADDR),
#endif
#ifdef RTM_IFANNOUNCE
	RTM_MSG_ENTRY(RTM_IFANNOUNCE),
#endif
	{ ~0, "Unknown" }
    };
    const size_t n_rtm_msgs = sizeof(rtm_msg_types) / sizeof(rtm_msg_types[0]);
    const char* ret = 0;
    for (size_t i = 0; i < n_rtm_msgs; i++) {
	ret = rtm_msg_types[i].name;
	if (rtm_msg_types[i].value == m)
	    break;
    }
    return ret;
}

/*
 * Round up to nearest integer value of step.
 * Taken from ROUND_UP macro in Stevens.
 */
static inline size_t
round_up(size_t val, size_t step)
{
    return (val & (step - 1)) ? (1 + (val | (step - 1))) : val;
}

/*
 * Step to next socket address pointer.
 * Taken from NEXT_SA macro in Stevens.
 */
static inline const struct sockaddr*
next_sa(const struct sockaddr* sa)
{
    const size_t min_size = sizeof(u_long);
    size_t sa_size = min_size;
    
#ifdef HAVE_SA_LEN
    sa_size = sa->sa_len ? round_up(sa->sa_len, min_size) : min_size;
#else // ! HAVE_SA_LEN
    switch (sa->sa_family) {
    case AF_INET:
	sa_size = sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	sa_size = sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	sa_size = sizeof(struct sockaddr_in);
	break;
    }
#endif // ! HAVE_SA_LEN
    
    const uint8_t* p = reinterpret_cast<const uint8_t*>(sa) + sa_size;
    return reinterpret_cast<const struct sockaddr*>(p);
}

void
RtmUtils::get_rta_sockaddr(uint32_t amask, const struct sockaddr* sock,
			   const struct sockaddr* rti_info[])
{
    for (uint32_t i = 0; i < RTAX_MAX; i++) {
	if (amask & (1 << i)) {
	    debug_msg("\tPresent 0x%02x af %d size %d\n",
		      1 << i, sock->sa_family, sock->sa_len);
	    rti_info[i] = sock;
	    sock = next_sa(sock);
	} else {
	    rti_info[i] = 0;
	}
    }
}

/*
 * Return the mask length on success, otherwise -1
 */
int
RtmUtils::get_sock_mask_len(int family, const struct sockaddr* sock)
{

#ifndef HAVE_SA_LEN
    switch (family) {
    case AF_INET:
    {
	// XXX: sock->sa_family is undefined
	const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(sock);
	IPv4 netmask(sin->sin_addr);
	return (netmask.mask_len());
    }
#ifdef HAVE_IPV6
    case AF_INET:
    {
	// XXX: sock->sa_family is undefined
	const struct sockaddr_in6* sin6 = reinterpret_cast<const struct sockaddr_in6*>(sock);
	IPv6 netmask(sin6->sin6_addr);
	return (netmask.mask_len());
    }
#endif // HAVE_IPV6
    default:
	XLOG_FATAL("Invalid address family %d", family);
    }
    
#else // HAVE_SA_LEN
    
    switch (family) {
    case AF_INET:
    {
	uint8_t buf[4];
	buf[0] = buf[1] = buf[2] = buf[3] = 0;
	const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&sock->sa_data[2]);
	
	switch (sock->sa_len) {
	case 0:
	    return (0);
	case 8:
	    buf[3] = *(ptr + 3);
	    // FALLTHROUGH
	case 7:
	    buf[2] = *(ptr + 2);
	    // FALLTHROUGH
	case 6:
	    buf[1] = *(ptr + 1);
	    // FALLTHROUGH
	case 5:
	    buf[0] = *(ptr + 0);
	    {
		IPv4 netmask(buf);
		return (netmask.mask_len());
	    }
	default:
	    // XXX: assume that the whole mask is stored
	    {
		// XXX: sock->sa_family is undefined
		const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(sock);
		IPv4 netmask(sin->sin_addr);
		return (netmask.mask_len());
	    }
	}
    }
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	if (sock->sa_len == 0) {
	    // XXX: the default /0 route
	    return (0);
	}
	// XXX: sock->sa_family is undefined
	const struct sockaddr_in6* sin6 = reinterpret_cast<const struct sockaddr_in6*>(sock);
	IPv6 netmask(sin6->sin6_addr);
	return (netmask.mask_len());
    }
#endif // HAVE_IPV6
    
    default:
	XLOG_FATAL("Invalid address family %d", family);
    }
#endif // HAVE_SA_LEN
    
    return (-1);
}

bool
RtmUtils::rtm_get_to_fte_cfg(FteX& fte, const struct rt_msghdr* rtm)
{
    const struct sockaddr *sa, *rti_info[RTAX_MAX];
    u_short if_index = rtm->rtm_index;
    string if_name;
    int family = fte.nexthop().af();
    bool is_family_match = false;
    bool is_deleted = false;
    
    XLOG_ASSERT((rtm->rtm_type == RTM_ADD)
		|| (rtm->rtm_type == RTM_DELETE)
		|| (rtm->rtm_type == RTM_CHANGE)
		|| (rtm->rtm_type == RTM_GET));
    debug_msg("%p index %d type %s\n", rtm, if_index,
	      rtm_msg_type(rtm->rtm_type).c_str());

    if (rtm->rtm_errno != 0)
	return false;		 // XXX: ignore entries with an error

    // Reset the result
    fte.zero();

    // Test if this entry was deleted
    if (rtm->rtm_type == RTM_DELETE)
	is_deleted = true;

    // Get the pointers to the corresponding data structures    
    sa = reinterpret_cast<const struct sockaddr*>(rtm + 1);
    RtmUtils::get_rta_sockaddr(rtm->rtm_addrs, sa, rti_info);
    
    IPvX dst_addr(family);
    IPvX nexthop_addr(family);
    int dst_mask_len = 0;
    
    //
    // Get the destination
    //
    if ( (sa = rti_info[RTAX_DST]) != NULL) {
	if (sa->sa_family == family) {
	    dst_addr.copy_in(*rti_info[RTAX_DST]);
	    dst_addr = kernel_ipvx_adjust(dst_addr);
	    is_family_match = true;
	}
    }
    
    //
    // Get the next-hop router address
    //
    if ( (sa = rti_info[RTAX_GATEWAY]) != NULL) {
	if (sa->sa_family == family) {
	    nexthop_addr.copy_in(*rti_info[RTAX_GATEWAY]);
	    nexthop_addr = kernel_ipvx_adjust(nexthop_addr);
	    is_family_match = true;
	}
    }
    if ((rtm->rtm_flags & RTF_LLINFO)
	&& (nexthop_addr == IPvX::ZERO(family))) {
	// Link-local entry (could be the broadcast address as well)
	bool not_bcast_addr = true;
#ifdef RTF_BROADCAST
	if (rtm->rtm_flags & RTF_BROADCAST)
	    not_bcast_addr = false;
#endif
	if (not_bcast_addr)
	    nexthop_addr = dst_addr;
    }
    if (! is_family_match)
	return false;
    
    //
    // Get the destination mask length
    //
    if ( (sa = rti_info[RTAX_NETMASK]) != NULL) {
	dst_mask_len = RtmUtils::get_sock_mask_len(family, sa);
    }
    
    //
    // Patch the destination mask length (if necessary)
    //
    if (rtm->rtm_flags & RTF_HOST) {
	if (dst_mask_len == 0)
	    dst_mask_len = IPvX::addr_bitlen(family);
    }
    
    //
    // Test whether we installed this route
    //
    bool xorp_route = false;
    if (rtm->rtm_flags & RTF_PROTO1)
	xorp_route = true;

    //
    // Get the interface name and index
    //
    if ( (sa = rti_info[RTAX_IFP]) != NULL) {
	if (sa->sa_family != AF_LINK) {
	    // TODO: verify whether this is really an error.
	    XLOG_ERROR("Ignoring RTM_GET for RTAX_IFP with sa_family = %d",
		       sa->sa_family);
	    return false;
	}
	const struct sockaddr_dl* sdl = reinterpret_cast<const struct sockaddr_dl*>(sa);
	
	if (sdl->sdl_nlen > 0) {
	    if_name = string(sdl->sdl_data, sdl->sdl_nlen);
	} else {
	    if (if_index == 0) {
		XLOG_FATAL("Interface with no name and index");
	    }
	    const char* name = NULL;
#ifdef HAVE_IF_INDEXTONAME
	    char name_buf[IF_NAMESIZE];
	    name = if_indextoname(if_index, name_buf);
#endif
	    if (name == NULL) {
		XLOG_FATAL("Could not find interface corresponding to index %d",
			   if_index);
	    }
	    if_name = string(name);
	}
    }
    
    //
    // TODO: define default routing metric and admin distance instead of ~0
    //
    fte = FteX(IPvXNet(dst_addr, dst_mask_len), nexthop_addr, if_name, if_name,
	       ~0, ~0, xorp_route);
    if (is_deleted)
	fte.mark_deleted();
    
    return true;
}

#endif // HAVE_ROUTING_SOCKETS
