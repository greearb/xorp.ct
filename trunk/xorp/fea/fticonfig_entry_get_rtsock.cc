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

#ident "$XORP: xorp/fea/fticonfig_entry_get_rtsock.cc,v 1.7 2003/09/20 06:41:01 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ipvxnet.hh"

#include "fticonfig.hh"
#include "fticonfig_entry_get.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is routing sockets.
//


FtiConfigEntryGetRtsock::FtiConfigEntryGetRtsock(FtiConfig& ftic)
    : FtiConfigEntryGet(ftic),
      RoutingSocket(ftic.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this),
      _cache_valid(false),
      _cache_seqno(0)
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic();
#endif
}

FtiConfigEntryGetRtsock::~FtiConfigEntryGetRtsock()
{
    stop();
}

int
FtiConfigEntryGetRtsock::start()
{
    return (RoutingSocket::start());
}
    
int
FtiConfigEntryGetRtsock::stop()
{
    return (RoutingSocket::stop());
}

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false
 */
bool
FtiConfigEntryGetRtsock::lookup_route4(const IPv4& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;
    
    ret_value = lookup_route(IPvX(dst), ftex);
    
    fte = Fte4(ftex.net().get_ipv4net(), ftex.gateway().get_ipv4(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance(), ftex.xorp_route());
    
    return (ret_value);
}

/**
 * Lookup entry.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtsock::lookup_entry4(const IPv4Net& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;
    
    ret_value = lookup_entry(IPvXNet(dst), ftex);
    
    fte = Fte4(ftex.net().get_ipv4net(), ftex.gateway().get_ipv4(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance(), ftex.xorp_route());
    
    return (ret_value);
}

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false;
 */
bool
FtiConfigEntryGetRtsock::lookup_route6(const IPv6& dst, Fte6& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;
    
    ret_value = lookup_route(IPvX(dst), ftex);
    
    fte = Fte6(ftex.net().get_ipv6net(), ftex.gateway().get_ipv6(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance(), ftex.xorp_route());
    
    return (ret_value);
}

/**
 * Lookup entry.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtsock::lookup_entry6(const IPv6Net& dst, Fte6& fte)
{ 
    FteX ftex(dst.af());
    bool ret_value = false;
    
    ret_value = lookup_entry(IPvXNet(dst), ftex);
    
    fte = Fte6(ftex.net().get_ipv6net(), ftex.gateway().get_ipv6(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance(), ftex.xorp_route());
    
    return (ret_value);
}

#ifndef HAVE_ROUTING_SOCKETS
bool
FtiConfigEntryGetRtsock::lookup_route(const IPvX& , FteX& )
{
    return false;
}

bool
FtiConfigEntryGetRtsock::lookup_entry(const IPvXNet& , FteX& )
{
    return false;
}

void
FtiConfigEntryGetRtsock::rtsock_data(const uint8_t* , size_t )
{
    
}

#else // HAVE_ROUTING_SOCKETS

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtsock::lookup_route(const IPvX& dst, FteX& fte)
{
#define RTMBUFSIZE (sizeof(struct rt_msghdr) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct rt_msghdr	*rtm, *rtm_answer;
    struct sockaddr_in	*sin;
    RoutingSocket&	rs = *this;
    
    // Zero the return information
    fte.zero();
    
    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return false;
    }
    
    //
    // Set the request
    //
    memset(rtmbuf, 0, sizeof(rtmbuf));
    rtm = reinterpret_cast<struct rt_msghdr*>(rtmbuf);
    
    switch (dst.af()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    // TODO: XXX: PAVPAVPAV: do we need RTA_IFP ??
    rtm->rtm_addrs = (RTA_DST | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();
    
    // Copy the destination address
    sin = reinterpret_cast<struct sockaddr_in*>(rtm + 1);
    dst.copy_out(*sin);
    
    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("error writing to routing socket: %s", strerror(errno));
	return false;
    }
    
    //
    // We expect kernel to give us something back.  Force read until
    // data ripples up via rtsock_data() that corresponds to expected
    // sequence number and process id.
    //
    _cache_seqno = rtm->rtm_seq;
    _cache_valid = false;
    while (_cache_valid == false) {
	rs.force_read();
    }
    rtm_answer = reinterpret_cast<struct rt_msghdr*>(&_cache_data[0]);
    XLOG_ASSERT(rtm_answer->rtm_type == RTM_GET);
    
    return (parse_buffer_rtm(fte, &_cache_data[0], _cache_data.size()));
}

/**
 * Lookup entry.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtsock::lookup_entry(const IPvXNet& dst, FteX& fte)
{
#define RTMBUFSIZE (sizeof(struct rt_msghdr) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct rt_msghdr	*rtm, *rtm_answer;
    struct sockaddr_in	*sin;
    RoutingSocket&	rs = *this;
    
    // Zero the return information
    fte.zero();

    // Check that the destination address is valid    
    if (! (dst.masked_addr().is_unicast()
	   || (dst == IPvXNet(IPvX::ZERO(dst.af()), 0)))) {
	return false;
    }

    //
    // Set the request
    //
    memset(rtmbuf, 0, sizeof(rtmbuf));
    rtm = reinterpret_cast<struct rt_msghdr*>(rtmbuf);
    
    switch (dst.af()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + 2 * sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + 2 * sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    // TODO: XXX: PAVPAVPAV: do we need RTA_IFP ??
    rtm->rtm_addrs = (RTA_DST | RTA_NETMASK | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();

    // Copy the destination address    
    sin = reinterpret_cast<struct sockaddr_in*>(rtm + 1);
    dst.masked_addr().copy_out(*sin);
    
    // Copy the network mask
    switch (dst.af()) {
    case AF_INET:
	sin = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			  struct sockaddr_in*);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	sin = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			  struct sockaddr_in*);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    IPvX netmask = IPvX::make_prefix(dst.af(), dst.prefix_len());
    netmask.copy_out(*sin);
    
    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("error writing to routing socket: %s", strerror(errno));
	return false;
    }
    
    //
    // We expect kernel to give us something back.  Force read until
    // data ripples up via rtsock_data() that corresponds to expected
    // sequence number and process id.
    //
    _cache_seqno = rtm->rtm_seq;
    _cache_valid = false;
    while (_cache_valid == false) {
	rs.force_read();
    }
    rtm_answer = reinterpret_cast<struct rt_msghdr*>(&_cache_data[0]);
    XLOG_ASSERT(rtm_answer->rtm_type == RTM_GET);
    
    return (parse_buffer_rtm(fte, &_cache_data[0], _cache_data.size()));
}

/**
 * Receive data from the routing socket.
 *
 * Note that this method is called asynchronously when the routing socket
 * has data to receive, therefore it should never be called directly by
 * anything else except the routing socket facility itself.
 * 
 * @param data the buffer with the received data.
 * @param nbytes the number of bytes in the @param data buffer.
 */
void
FtiConfigEntryGetRtsock::rtsock_data(const uint8_t* data, size_t nbytes)
{
    RoutingSocket& rs = *this;
    
    //
    // Copy data that has been requested to be cached by setting _cache_seqno.
    //
    size_t d = 0;
    pid_t my_pid = rs.pid();
    
    while (d < nbytes) {
	const struct rt_msghdr* rh = reinterpret_cast<const struct rt_msghdr*>(data + d);
	if ((rh->rtm_pid == my_pid)
	    && (rh->rtm_seq == (signed)_cache_seqno)) {
#if 0	    // TODO: XXX: PAVPAVPAV: remove this assert?
	    XLOG_ASSERT(_cache_valid == false); // Do not overwrite cache data
#endif
	    XLOG_ASSERT(nbytes - d >= rh->rtm_msglen);
	    _cache_data.resize(rh->rtm_msglen);
	    memcpy(&_cache_data[0], rh, rh->rtm_msglen);
	    _cache_valid = true;
	    return;
	}
	d += rh->rtm_msglen;
    }
}
#endif // HAVE_ROUTING_SOCKETS
