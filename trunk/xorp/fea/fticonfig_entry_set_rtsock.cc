// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_entry_set_rtsock.cc,v 1.32 2005/10/26 19:11:11 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is routing sockets.
//


FtiConfigEntrySetRtsock::FtiConfigEntrySetRtsock(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      RoutingSocket(ftic.eventloop())
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic_primary();
#endif
}

FtiConfigEntrySetRtsock::~FtiConfigEntrySetRtsock()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the routing sockets mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigEntrySetRtsock::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntrySetRtsock::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigEntrySetRtsock::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetRtsock::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetRtsock::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetRtsock::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

#ifndef HAVE_ROUTING_SOCKETS
bool
FtiConfigEntrySetRtsock::add_entry(const FteX& )
{
    return false;
}

bool
FtiConfigEntrySetRtsock::delete_entry(const FteX& )
{
    return false;
}

#else // HAVE_ROUTING_SOCKETS

//
// XXX: Define a number of constants with the size of the corresponding
// sockaddr structure rounded-up to the next sizeof(long) boundary.
// This is needed, because the routing socket expects the sockaddr
// structures to begin on the boundary of (long) words.
//
#define LONG_ROUNDUP_SIZEOF(type)					\
	((sizeof(type) % sizeof(long)) ?				\
	(sizeof(type) + sizeof(long) - (sizeof(type) % sizeof(long)))	\
	: (sizeof(type)))
static const size_t SOCKADDR_IN_ROUNDUP_LEN = LONG_ROUNDUP_SIZEOF(struct sockaddr_in);
#ifdef HAVE_IPV6
static const size_t SOCKADDR_IN6_ROUNDUP_LEN = LONG_ROUNDUP_SIZEOF(struct sockaddr_in6);
#endif
#ifdef AF_LINK
static const size_t SOCKADDR_DL_ROUNDUP_LEN = LONG_ROUNDUP_SIZEOF(struct sockaddr_dl);
#endif
#undef LONG_ROUNDUP_SIZEOF

bool
FtiConfigEntrySetRtsock::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    char		buffer[buffer_size];
    struct rt_msghdr	*rtm;
    struct sockaddr_in	*sin_dst, *sin_netmask;
    struct sockaddr_in	*sin_nexthop = NULL;
    RoutingSocket&	rs = *this;
    int			family = fte.net().af();
#ifdef AF_LINK
    struct sockaddr_dl*	sdl = NULL;
    size_t		sdl_len = 0;
#endif
    size_t		sin_dst_len = 0;
    size_t		sin_nexthop_len = 0;
    size_t		sin_netmask_len = 0;
    bool		is_host_route = fte.is_host_route();
    bool		is_interface_route = false;
    bool		is_nexthop_sockaddr_dl = false;
    bool		is_discard_route = false;
    IPvX		fte_nexthop = fte.nexthop();

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte_nexthop.str().c_str());

    // Check that the family is supported
    do {
	if (fte_nexthop.is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte_nexthop.is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);
    
    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes

    if (! fte.ifname().empty())
	is_interface_route = true;

    do {
	//
	// Check for a discard route; the referenced ifname must have the
	// discard property. The next-hop is forcibly rewritten to be the
	// loopback address, in order for the RTF_BLACKHOLE flag to take
	// effect on BSD platforms.
	//
	if (fte.ifname().empty())
	    break;
	IfTree& it = ftic().iftree();
	IfTree::IfMap::const_iterator ii = it.get_if(fte.ifname());
	if (ii == it.ifs().end()) {
	    XLOG_ERROR("Invalid interface name: %s", fte.ifname().c_str());
	    return false;
	}
	if (ii->second.discard()) {
	    is_discard_route = true;
	    fte_nexthop = IPvX::LOOPBACK(family);
	}
	break;
    } while (false);

#ifdef AF_LINK
    //
    // XXX: If we want to add an interface-specific route, and if
    // there is no nexthop IP address, then the nexthop (RTA_GATEWAY)
    // must be "struct sockaddr_dl" with the interface information.
    // Note: this check must ba after the discard route check, because
    // the discard route check may overwrite fte_nexthop.
    //
    if (is_interface_route && (fte_nexthop == IPvX::ZERO(family))) {
	is_nexthop_sockaddr_dl = true;
    }
#endif // AF_LINK

    //
    // Set the request
    //
    memset(buffer, 0, sizeof(buffer));
    rtm = reinterpret_cast<struct rt_msghdr *>(buffer);
    rtm->rtm_msglen = sizeof(*rtm);
    
    switch (family) {
    case AF_INET:
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_dst_len = SOCKADDR_IN_ROUNDUP_LEN;
	rtm->rtm_msglen += sin_dst_len;
	if (is_nexthop_sockaddr_dl) {
#ifdef AF_LINK
	    sdl = ADD_POINTER(sin_dst, sin_dst_len, struct sockaddr_dl *);
	    sdl_len = SOCKADDR_DL_ROUNDUP_LEN;
	    sin_netmask = ADD_POINTER(sdl, sdl_len, struct sockaddr_in *);
	    sin_netmask_len = SOCKADDR_IN_ROUNDUP_LEN;
	    rtm->rtm_msglen += sdl_len + sin_netmask_len;
#endif // AF_LINK
	} else {
	    sin_nexthop = ADD_POINTER(sin_dst, sin_dst_len,
				      struct sockaddr_in *);
	    sin_nexthop_len = SOCKADDR_IN_ROUNDUP_LEN;
	    sin_netmask = ADD_POINTER(sin_nexthop, sin_nexthop_len,
				      struct sockaddr_in *);
	    sin_netmask_len = SOCKADDR_IN_ROUNDUP_LEN;
	    rtm->rtm_msglen += sin_nexthop_len + sin_netmask_len;
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_dst_len = SOCKADDR_IN6_ROUNDUP_LEN;
	rtm->rtm_msglen += sin_dst_len;
	if (is_nexthop_sockaddr_dl) {
#ifdef AF_LINK
	    sdl = ADD_POINTER(sin_dst, sin_dst_len, struct sockaddr_dl *);
	    sdl_len = SOCKADDR_DL_ROUNDUP_LEN;
	    sin_netmask = ADD_POINTER(sdl, sdl_len, struct sockaddr_in *);
	    sin_netmask_len = SOCKADDR_IN6_ROUNDUP_LEN;
	    rtm->rtm_msglen += sdl_len + sin_netmask_len;
#endif // AF_LINK
	} else {
	    sin_nexthop = ADD_POINTER(sin_dst, sin_dst_len,
				      struct sockaddr_in *);
	    sin_nexthop_len = SOCKADDR_IN6_ROUNDUP_LEN;
	    sin_netmask = ADD_POINTER(sin_nexthop, sin_nexthop_len,
				      struct sockaddr_in *);
	    sin_netmask_len = SOCKADDR_IN6_ROUNDUP_LEN;
	    rtm->rtm_msglen += sin_nexthop_len + sin_netmask_len;
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_ADD;
    rtm->rtm_index = 0;			// XXX: not used by the kernel (?)
    rtm->rtm_addrs = (RTA_DST | RTA_GATEWAY | RTA_NETMASK);
    if (is_host_route)
	rtm->rtm_flags |= RTF_HOST;
    if (is_discard_route)
	rtm->rtm_flags |= RTF_BLACKHOLE;
    if ((fte_nexthop != IPvX::ZERO(family)) && (! is_nexthop_sockaddr_dl))
	rtm->rtm_flags |= RTF_GATEWAY;
    rtm->rtm_flags |= RTF_PROTO1;	// XXX: mark this as a XORP route
    rtm->rtm_flags |= RTF_UP;		// XXX: mark this route as UP
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();

    // Copy the destination, the nexthop, and the netmask addresses
    fte.net().masked_addr().copy_out(*sin_dst);
#ifdef HAVE_SIN_LEN
    sin_dst->sin_len = sin_dst_len;
#endif
    if (sin_nexthop != NULL) {
	fte_nexthop.copy_out(*sin_nexthop);
#ifdef HAVE_SIN_LEN
	sin_nexthop->sin_len = sin_nexthop_len;
#endif
    }
    fte.net().netmask().copy_out(*sin_netmask);
#ifdef HAVE_SIN_LEN
    sin_netmask->sin_len = sin_netmask_len;
#endif

    if (is_interface_route) {
	//
	// This is an interface-specific route.
	// Set the interface-related information.
	//

#ifdef AF_LINK
	if (sdl == NULL) {
	    //
	    // Add extra space for data-link sockaddr_dl for the RTA_IFP flag
	    //
	    sdl = ADD_POINTER(sin_netmask, sin_netmask_len,
			      struct sockaddr_dl *);
	    sdl_len = SOCKADDR_DL_ROUNDUP_LEN;
	    rtm->rtm_msglen += sdl_len;
	    rtm->rtm_addrs |= RTA_IFP;
	}

	sdl->sdl_family = AF_LINK;
	sdl->sdl_len = sdl_len;
	IfTree& it = ftic().iftree();
	IfTree::IfMap::const_iterator ii = it.get_if(fte.ifname());
	if (ii == it.ifs().end()) {
	    XLOG_ERROR("Invalid interface name: %s", fte.ifname().c_str());
	    return false;
	}
	sdl->sdl_index = ii->second.pif_index();
	strncpy(sdl->sdl_data, fte.ifname().c_str(), sizeof(sdl->sdl_data));
	if (fte.ifname().size() < sizeof(sdl->sdl_data)) {
	    sdl->sdl_nlen = fte.ifname().size();
	    sdl->sdl_data[sizeof(sdl->sdl_data) - 1] = '\0';
	} else {
	    sdl->sdl_nlen = sizeof(sdl->sdl_data);
	}
#endif // AF_LINK
    }

    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to routing socket: %s", strerror(errno));
	return false;
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return true;
}

bool
FtiConfigEntrySetRtsock::delete_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    char		buffer[buffer_size];
    struct rt_msghdr	*rtm;
    struct sockaddr_in	*sin_dst, *sin_netmask = NULL;
    RoutingSocket&	rs = *this;
    int			family = fte.net().af();
    size_t		sin_dst_len = 0;
    size_t		sin_netmask_len = 0;
    bool		is_host_route = fte.is_host_route();

    debug_msg("delete_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes
    
    //
    // Set the request
    //
    memset(buffer, 0, sizeof(buffer));
    rtm = reinterpret_cast<struct rt_msghdr *>(buffer);
    rtm->rtm_msglen = sizeof(*rtm);
    
    switch (family) {
    case AF_INET:
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_dst_len = SOCKADDR_IN_ROUNDUP_LEN;
	rtm->rtm_msglen += sin_dst_len;
	if (! is_host_route) {
	    sin_netmask = ADD_POINTER(sin_dst, sin_dst_len,
				      struct sockaddr_in *);
	    sin_netmask_len = SOCKADDR_IN_ROUNDUP_LEN;
	    rtm->rtm_msglen += sin_netmask_len;
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_dst_len = SOCKADDR_IN6_ROUNDUP_LEN;
	rtm->rtm_msglen += sin_dst_len;
	if (! is_host_route) {
	    sin_netmask = ADD_POINTER(sin_dst, sin_dst_len,
				      struct sockaddr_in *);
	    sin_netmask_len = SOCKADDR_IN6_ROUNDUP_LEN;
	    rtm->rtm_msglen += sin_netmask_len;
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_DELETE;
    rtm->rtm_index = 0;			// XXX: not used by the kernel (?)
    rtm->rtm_addrs = RTA_DST;
    if (! is_host_route)
	rtm->rtm_addrs |= RTA_NETMASK;
    rtm->rtm_flags = 0;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();
    
    // Copy the destination, and the netmask addresses (if needed)
    fte.net().masked_addr().copy_out(*sin_dst);
#ifdef HAVE_SIN_LEN
    sin_dst->sin_len = sin_dst_len;
#endif

    if (! is_host_route) {
	fte.net().netmask().copy_out(*sin_netmask);
#ifdef HAVE_SIN_LEN
	sin_netmask->sin_len = sin_netmask_len;
#endif
    }
    
    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to routing socket: %s", strerror(errno));
	return false;
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return true;
}
#endif // HAVE_ROUTING_SOCKETS
