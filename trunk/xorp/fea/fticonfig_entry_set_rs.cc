// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_rtsock.cc,v 1.5 2003/03/10 23:20:15 hodson Exp $"


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


FtiConfigEntrySetRs::FtiConfigEntrySetRs(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      RoutingSocket(ftic.eventloop())
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic();
#endif
}

FtiConfigEntrySetRs::~FtiConfigEntrySetRs()
{
    stop();
}

int
FtiConfigEntrySetRs::start()
{
    return (RoutingSocket::start());
}
    
int
FtiConfigEntrySetRs::stop()
{
    return (RoutingSocket::stop());
}


int
FtiConfigEntrySetRs::add_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance());
    
    return (add_entry(ftex));
}

int
FtiConfigEntrySetRs::delete_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance());
    
    return (delete_entry(ftex));
}

int
FtiConfigEntrySetRs::add_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance());
    
    return (add_entry(ftex));
}

int
FtiConfigEntrySetRs::delete_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance());
    
    return (delete_entry(ftex));
}

#ifndef HAVE_ROUTING_SOCKETS
int
FtiConfigEntrySetRs::add_entry(const FteX& )
{
    return (XORP_ERROR);
}

int
FtiConfigEntrySetRs::delete_entry(const FteX& )
{
    return (XORP_ERROR);
}

#else // HAVE_ROUTING_SOCKETS
int
FtiConfigEntrySetRs::add_entry(const FteX& fte)
{
#define RTMBUFSIZE (sizeof(struct rt_msghdr) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct rt_msghdr	*rtm;
    struct sockaddr_in	*sin_dst, *sin_gateway, *sin_netmask;
    RoutingSocket&	rs = *this;
    int			family = fte.net().af();

    //
    // Set the request
    //
    memset(rtmbuf, 0, sizeof(rtmbuf));
    rtm = (struct rt_msghdr *)rtmbuf;
    
    switch (family) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + 3 * sizeof(struct sockaddr_in);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_gateway = sin_dst + sizeof(struct sockaddr_in);
	sin_netmask = sin_gateway + sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + 3 * sizeof(struct sockaddr_in6);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	sin_gateway = sin_dst + sizeof(struct sockaddr_in6);
	sin_netmask = sin_gateway + sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_ADD;
    rtm->rtm_index = 0;			// XXX: not used by the kernel (?)
    rtm->rtm_addrs = (RTA_DST | RTA_GATEWAY | RTA_NETMASK);
    if (fte.gateway() != IPvX::ZERO(family)) {
	if (fte.is_host_route())
	    rtm->rtm_flags = RTF_HOST;
	else
	    rtm->rtm_flags = RTF_GATEWAY;
    }
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();
    
    // Copy the destination, the gateway, and the netmask addresses
    fte.net().masked_addr().copy_out(*sin_dst);
    fte.gateway().copy_out(*sin_gateway);
    if (fte.gateway() == IPvX::ZERO(family)) {
	// TODO: XXX: PAVPAVPAV: set the interface index, etc:
	// gateway_dl.sdl_family = AF_LINK;
	// gateway_dl.sdl_index = ifindex(fte.vifname());
    }
    fte.net().netmask().copy_out(*sin_netmask);
    
    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("error writing to routing socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return (XORP_OK);
}

int
FtiConfigEntrySetRs::delete_entry(const FteX& fte)
{
#define RTMBUFSIZE (sizeof(struct rt_msghdr) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct rt_msghdr	*rtm;
    struct sockaddr_in	*sin_dst, *sin_netmask = NULL;
    RoutingSocket&	rs = *this;
    int			family = fte.net().af();
    
    //
    // Set the request
    //
    memset(rtmbuf, 0, sizeof(rtmbuf));
    rtm = (struct rt_msghdr *)rtmbuf;
    
    switch (family) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	if (fte.is_host_route()) {
	    rtm->rtm_msglen += sizeof(struct sockaddr_in);
	    sin_netmask = sin_dst + sizeof(struct sockaddr_in);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in6);
	sin_dst = (struct sockaddr_in *)(rtm + 1);
	if (fte.is_host_route()) {
	    rtm->rtm_msglen += sizeof(struct sockaddr_in6);
	    sin_netmask = sin_dst + sizeof(struct sockaddr_in6);
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
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
	return (XORP_ERROR);
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return (XORP_OK);
}
#endif // HAVE_ROUTING_SOCKETS
