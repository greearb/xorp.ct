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

#ident "$XORP: xorp/fea/fticonfig_entry_set_rtsock.cc,v 1.17 2004/08/17 02:20:07 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

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
    stop();
}

int
FtiConfigEntrySetRtsock::start()
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start() < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntrySetRtsock::stop()
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop() < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigEntrySetRtsock::add_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetRtsock::delete_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetRtsock::add_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetRtsock::delete_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
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
bool
FtiConfigEntrySetRtsock::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    char		buffer[buffer_size];
    struct rt_msghdr	*rtm;
    struct sockaddr_in	*sin_dst, *sin_nexthop, *sin_netmask;
    RoutingSocket&	rs = *this;
    int			family = fte.net().af();

    debug_msg("add_entry "
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
    
    //
    // Set the request
    //
    memset(buffer, 0, sizeof(buffer));
    rtm = (struct rt_msghdr *)buffer;
    
    switch (family) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + 3 * sizeof(struct sockaddr_in);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_nexthop = ADD_POINTER(sin_dst, sizeof(struct sockaddr_in),
				  struct sockaddr_in *);
	sin_netmask = ADD_POINTER(sin_nexthop, sizeof(struct sockaddr_in),
				  struct sockaddr_in *);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + 3 * sizeof(struct sockaddr_in6);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_nexthop = ADD_POINTER(sin_dst, sizeof(struct sockaddr_in6),
				  struct sockaddr_in *);
	sin_netmask = ADD_POINTER(sin_nexthop, sizeof(struct sockaddr_in6),
				  struct sockaddr_in *);
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
    if (fte.is_host_route())
	rtm->rtm_flags |= RTF_HOST;
    if (fte.nexthop() != IPvX::ZERO(family))
	rtm->rtm_flags |= RTF_GATEWAY;
    rtm->rtm_flags |= RTF_PROTO1;	// XXX: mark this as a XORP route
    rtm->rtm_flags |= RTF_UP;		// XXX: mark this route as UP
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();

    // Copy the destination, the nexthop, and the netmask addresses
    fte.net().masked_addr().copy_out(*sin_dst);
    fte.nexthop().copy_out(*sin_nexthop);
    if (fte.nexthop() == IPvX::ZERO(family)) {
	// TODO: XXX: PAVPAVPAV: set the interface index, etc:
	// nexthop_dl.sdl_family = AF_LINK;
	// nexthop_dl.sdl_index = ifindex(fte.vifname());
    }
    fte.net().netmask().copy_out(*sin_netmask);

    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("error writing to routing socket: %s", strerror(errno));
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
    
    //
    // Set the request
    //
    memset(buffer, 0, sizeof(buffer));
    rtm = (struct rt_msghdr *)buffer;
    
    switch (family) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	if (! fte.is_host_route()) {
	    rtm->rtm_msglen += sizeof(struct sockaddr_in);
	    sin_netmask = ADD_POINTER(sin_dst, sizeof(struct sockaddr_in),
				      struct sockaddr_in *);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in6);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	if (! fte.is_host_route()) {
	    rtm->rtm_msglen += sizeof(struct sockaddr_in6);
	    sin_netmask = ADD_POINTER(sin_dst, sizeof(struct sockaddr_in6),
				      struct sockaddr_in *);
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
    if (! fte.is_host_route())
	rtm->rtm_addrs |= RTA_NETMASK;
    rtm->rtm_flags = 0;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();
    
    // Copy the destination, and the netmask addresses (if needed)
    fte.net().masked_addr().copy_out(*sin_dst);
    if (! fte.is_host_route())
	fte.net().netmask().copy_out(*sin_netmask);
    
    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("error writing to routing socket: %s", strerror(errno));
	return false;
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return true;
}
#endif // HAVE_ROUTING_SOCKETS
