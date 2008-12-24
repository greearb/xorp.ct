// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/fea/data_plane/control_socket/routing_socket_utilities.cc,v 1.19 2008/10/02 21:56:54 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#ifdef HOST_OS_WINDOWS
#include "windows_routing_socket.h"
#endif

#include "system_utilities.hh"
#include "routing_socket_utilities.hh"

//
// RTM format related utilities for manipulating data
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//

#if defined(HOST_OS_WINDOWS) || defined(HAVE_ROUTING_SOCKETS)

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
	{ ~0U, "Unknown" }
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

#ifdef HOST_OS_WINDOWS
    /*
     * XXX: XORP's modified BSD-style routing socket interface
     * to Router Manager V2 in Windows Longhorn always uses
     * a fixed sockaddr size of sockaddr_storage.
     */
    sa_size = sizeof(struct sockaddr_storage);
#else // ! HOST_OS_WINDOWS
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    sa_size = sa->sa_len ? round_up(sa->sa_len, min_size) : min_size;
#else // ! HAVE_STRUCT_SOCKADDR_SA_LEN
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
#endif // ! HAVE_STRUCT_SOCKADDR_SA_LEN
#endif // HOST_OS_WINDOWS

    // XXX: the sa_size offset is aligned, hence we can use a void pointer
    const void* p = reinterpret_cast<const uint8_t*>(sa) + sa_size;
    return reinterpret_cast<const struct sockaddr*>(p);
}

void
RtmUtils::get_rta_sockaddr(uint32_t amask, const struct sockaddr* sock,
			   const struct sockaddr* rti_info[])
{
    size_t sa_len = sizeof(*sock);

    for (uint32_t i = 0; i < RTAX_MAX; i++) {
	if (amask & (1 << i)) {
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	    sa_len = sock->sa_len;
	    debug_msg("\tPresent 0x%02x af %d size %u\n",
		      1 << i, sock->sa_family,
		      XORP_UINT_CAST(sa_len));
#else
	    UNUSED(sa_len);
#endif
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

#ifndef HAVE_STRUCT_SOCKADDR_SA_LEN
    switch (family) {
    case AF_INET:
    {
	// XXX: sock->sa_family is undefined
	const struct sockaddr_in* sin = sockaddr2sockaddr_in(sock);
	IPv4 netmask(sin->sin_addr);
	return (netmask.mask_len());
    }
#ifndef HOST_OS_WINDOWS // XXX not yet for windows
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// XXX: sock->sa_family is undefined
	const struct sockaddr_in6* sin6 = sockaddr2sockaddr_in6(sock);
	IPv6 netmask(sin6->sin6_addr);
	return (netmask.mask_len());
    }
#endif // HAVE_IPV6
#endif
    default:
	XLOG_FATAL("Invalid address family %d", family);
    }
    
#else // HAVE_STRUCT_SOCKADDR_SA_LEN
    
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
		const struct sockaddr_in* sin = sockaddr2sockaddr_in(sock);
		IPv4 netmask(sin->sin_addr);
		return (netmask.mask_len());
	    }
	}
    }
    
#ifndef HOST_OS_WINDOWS	// Not yet for Windows
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	if (sock->sa_len == 0) {
	    // XXX: the default /0 route
	    return (0);
	}
	// XXX: sock->sa_family is undefined
	struct sockaddr_in6 sin6;
	memset(&sin6, 0, sizeof(sin6));
	memcpy(&sin6, sock, sock->sa_len);
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_family = AF_INET6;
	IPv6 netmask(sin6.sin6_addr);
	return (netmask.mask_len());
    }
#endif // HAVE_IPV6
#endif
    
    default:
	XLOG_FATAL("Invalid address family %d", family);
    }
#endif // HAVE_STRUCT_SOCKADDR_SA_LEN
    
    return (-1);
}

int
RtmUtils::rtm_get_to_fte_cfg(const IfTree& iftree, FteX& fte,
			     const struct rt_msghdr* rtm)
{
    const struct sockaddr *sa, *rti_info[RTAX_MAX];
    uint32_t if_index = rtm->rtm_index;
    string if_name;
    string vif_name;
    int family = fte.nexthop().af();
    bool is_family_match = false;
    bool is_deleted = false;
    bool is_unresolved = false;
    bool lookup_ifindex = true;
    bool xorp_route = false;
    bool is_recognized = false;
    
    if ((rtm->rtm_type == RTM_ADD)
	|| (rtm->rtm_type == RTM_DELETE)
	|| (rtm->rtm_type == RTM_CHANGE)
#ifdef RTM_GET
	|| (rtm->rtm_type == RTM_GET)
#endif
#ifdef RTM_MISS
	|| (rtm->rtm_type == RTM_MISS)
#endif
#ifdef RTM_RESOLVE
	|| (rtm->rtm_type == RTM_RESOLVE)
#endif
	) {
	is_recognized = true;
    }
    XLOG_ASSERT(is_recognized);

    debug_msg("%p index %d type %s\n", rtm, if_index,
	      rtm_msg_type(rtm->rtm_type).c_str());

    if (rtm->rtm_errno != 0)
	return (XORP_ERROR);		 // XXX: ignore entries with an error

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
	    dst_addr = system_adjust_ipvx_recv(dst_addr);
	    is_family_match = true;
	}
    }

    //
    // Deal with BSD upcalls. These only ever have RTAX_DST, and
    // only contain host addresses.
    //
#ifdef RTM_MISS
    if (rtm->rtm_type == RTM_MISS)
	is_unresolved = true;
#endif
#ifdef RTM_RESOLVE
    if (rtm->rtm_type == RTM_RESOLVE)
	is_unresolved = true;
#endif
    if (is_unresolved) {
	nexthop_addr = IPvX::ZERO(family);
	dst_mask_len = IPvX::addr_bitlen(family);
	if_name = "";
	vif_name = "";
	lookup_ifindex = false;
    }

    //
    // Get the next-hop router address
    // XXX: Windows does not include the 'gateway'.
    //
    if ( (sa = rti_info[RTAX_GATEWAY]) != NULL) {
	if (sa->sa_family == family) {
	    nexthop_addr.copy_in(*rti_info[RTAX_GATEWAY]);
	    nexthop_addr = system_adjust_ipvx_recv(nexthop_addr);
	    is_family_match = true;
	}
    }
#ifdef RTF_LLINFO
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
#endif /* RTF_LLINFO */
    if (! is_family_match)
	return (XORP_ERROR);
    
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
    // XXX: Windows does not set this flag currently.
    //
    if (rtm->rtm_flags & RTF_PROTO1)
	xorp_route = true;

    //
    // Test whether this route is a discard route.
    // Older BSD kernels do not implement a software discard interface,
    // so map this back to a reference to the FEA's notion of one.
    //
    // XXX: Windows does not currently support blackhole/reject routes.
    //
    if (rtm->rtm_flags & RTF_BLACKHOLE) {
	//
        // Try to map discard routes back to the first software discard
        // interface in the tree. If we don't have one, then ignore this route.
        // We have to scan all interfaces because IfTree elements
        // are held in a map, and we don't key on this property.
        //
	const IfTreeInterface* pi = NULL;
	for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	     ii != iftree.interfaces().end(); ++ii) {
		if (ii->second->discard()) {
		    pi = ii->second;
		    break;
		}
	}
	if (pi == NULL) {
	    //
	    // XXX: Cannot map a discard route back to an FEA soft discard
	    // interface.
	    //
	    return (XORP_ERROR);
	}

	if_name = pi->ifname();
	vif_name = if_name;		// XXX: ifname == vifname
	// XXX: Do we need to change nexthop_addr?
	lookup_ifindex = false;
    }

    //
    // Test whether this route is an unreachable route.
    // Older BSD kernels do not implement a software unreachable interface,
    // so map this back to a reference to the FEA's notion of one.
    //
    // XXX: Windows does not currently support blackhole/reject routes.
    //
    if (rtm->rtm_flags & RTF_REJECT) {
	//
        // Try to map unreachable routes back to the first software
        // unreachable interface in the tree. If we don't have one, then 
	// ignore this route.
        // We have to scan all interfaces because IfTree elements
        // are held in a map, and we don't key on this property.
        //
	const IfTreeInterface* pi = NULL;
	for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	     ii != iftree.interfaces().end(); ++ii) {
		if (ii->second->unreachable()) {
		    pi = ii->second;
		    break;
		}
	}
	if (pi == NULL) {
	    //
	    // XXX: Cannot map an unreachable route back to an FEA soft
	    // unreachable interface.
	    //
	    return (XORP_ERROR);
	}

	if_name = pi->ifname();
	vif_name = if_name;		// XXX: ifname == vifname
	// XXX: Do we need to change nexthop_addr?
	lookup_ifindex = false;
    }

    //
    // Get the interface/vif name and index
    //
    if (lookup_ifindex) {
	if (if_index != 0) {
	    const IfTreeVif* vifp = iftree.find_vif(if_index);
	    if (vifp != NULL) {
		if_name = vifp->ifname();
		vif_name = vifp->vifname();
	    }
	}

	if (if_name.empty()) {
#ifdef AF_LINK
	    sa = rti_info[RTAX_IFP];
	    if (sa != NULL) {
		// Use the RTAX_IFP info to get the interface name
		if (sa->sa_family != AF_LINK) {
		    // TODO: verify whether this is really an error.
		    XLOG_ERROR("Ignoring RTM_GET for RTAX_IFP with sa_family = %d",
			       sa->sa_family);
		    return (XORP_ERROR);
		}
		const struct sockaddr_dl* sdl;
		sdl = reinterpret_cast<const struct sockaddr_dl*>(sa);
		if (sdl->sdl_nlen > 0) {
		    if_name = string(sdl->sdl_data, sdl->sdl_nlen);
		    vif_name = if_name;		// TODO: XXX: not true for VLAN
		}
	    }
#endif // AF_LINK
	}

	// Test whether the interface/vif name was found
	if (if_name.empty() || vif_name.empty()) {
	    if (is_deleted) {
		//
		// XXX: If the route is deleted and we cannot find
		// the corresponding interface/vif, this could be because
		// an interface/vif is deleted from the kernel, and
		// the kernel might send first the upcall message
		// that deletes the interface from user space.
		// Hence the upcall message that followed to delete the
		// corresponding route for the connected subnet
		// won't find the interface/vif.
		// Propagate the route deletion with empty interface
		// and vif name.
		//
	    } else {
		//
		// XXX: A route was added, but we cannot find the corresponding
		// interface/vif. This might happen because of a race
		// condition. E.g., an interface was added and then
		// immediately deleted, but the processing for the addition of
		// the corresponding connected route was delayed.
		// Note that the misorder in the processing might happen
		// because the interface and routing control messages are
		// received on different control sockets.
		// For the time being make it a fatal error until there is
		// enough evidence and the issue is understood.
		//
		IPvXNet dst_subnet(dst_addr, dst_mask_len);
		XLOG_FATAL("Decoding for route %s next hop %s failed: "
			   "could not find interface and vif for index %d",
			   dst_subnet.str().c_str(),
			   nexthop_addr.str().c_str(),
			   if_index);
	    }
	}
    }

    //
    // TODO: define default routing metric and admin distance instead of 0xffff
    //
    fte = FteX(IPvXNet(dst_addr, dst_mask_len), nexthop_addr,
	       if_name, vif_name, 0xffff, 0xffff, xorp_route);
    if (is_deleted)
	fte.mark_deleted();
    if (is_unresolved)
	fte.mark_unresolved();
    
    return (XORP_OK);
}

#endif // HAVE_ROUTING_SOCKETS
