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

#include "libxorp/ipvxnet.hh"

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_get.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is netlink sockets.
//


FtiConfigEntryGetNetlink::FtiConfigEntryGetNetlink(FtiConfig& ftic)
    : FtiConfigEntryGet(ftic),
      NetlinkSocket(ftic.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket *)this),
      _cache_valid(false),
      _cache_seqno(0)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ftic();
#endif
}

FtiConfigEntryGetNetlink::~FtiConfigEntryGetNetlink()
{
    stop();
}

int
FtiConfigEntryGetNetlink::start()
{
    return (NetlinkSocket::start());
}
    
int
FtiConfigEntryGetNetlink::stop()
{
    return (NetlinkSocket::stop());
}

void
FtiConfigEntryGetNetlink::receive_data(const uint8_t* data, size_t n_bytes)
{
    // TODO: use it?
    UNUSED(data);
    UNUSED(n_bytes);
}

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntryGetNetlink::lookup_route4(const IPv4& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;
    
    ret_value = lookup_route(IPvX(dst), ftex);
    
    fte = Fte4(ftex.net().get_ipv4net(), ftex.gateway().get_ipv4(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance());
    
    return (ret_value);
}

/**
 * Lookup entry.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntryGetNetlink::lookup_entry4(const IPv4Net& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;
    
    ret_value = lookup_entry(IPvXNet(dst), ftex);
    
    fte = Fte4(ftex.net().get_ipv4net(), ftex.gateway().get_ipv4(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance());
    
    return (ret_value);
}

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntryGetNetlink::lookup_route6(const IPv6& dst, Fte6& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;
    
    ret_value = lookup_route(IPvX(dst), ftex);
    
    fte = Fte6(ftex.net().get_ipv6net(), ftex.gateway().get_ipv6(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance());
    
    return (ret_value);
}

/**
 * Lookup entry.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntryGetNetlink::lookup_entry6(const IPv6Net& dst, Fte6& fte)
{ 
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;
    
    ret_value = lookup_entry(IPvXNet(dst), ftex);
    
    fte = Fte6(ftex.net().get_ipv6net(), ftex.gateway().get_ipv6(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance());
    
    return (ret_value);
}

#ifndef HAVE_NETLINK_SOCKETS

int
FtiConfigEntryGetNetlink::lookup_route(const IPvX& , FteX& )
{
    return (XORP_ERROR);
}
int
FtiConfigEntryGetNetlink::lookup_entry(const IPvXNet& , FteX& )
{
    return (XORP_ERROR);
}

void
FtiConfigEntryGetNetlink::nlsock_data(const uint8_t* , size_t )
{
    
}

#else // HAVE_NETLINK_SOCKETS

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntryGetNetlink::lookup_route(const IPvX& dst, FteX& fte)
{
#define RTMBUFSIZE (sizeof(struct nlmsghdr) + sizeof(struct rtmsg) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct nlmsghdr	*nlh, *nlh_answer;
    struct sockaddr_nl	snl;
    socklen_t		snl_len;
    struct rtmsg	*rtmsg;
    struct rtattr	*rtattr;
    int			rta_len;
    NetlinkSocket&	ns = *this;
    int			family = dst.af();
    
    // Zero the return information
    fte.zero();
    
    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return (XORP_ERROR);
    }
    
    //
    // Set the request. First the socket, then the request itself.
    //
    
    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;
    
    // Set the request
    memset(rtmbuf, 0, sizeof(rtmbuf));
    nlh = (struct nlmsghdr *)rtmbuf;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.pid();
    rtmsg = (struct rtmsg *)NLMSG_DATA(nlh);
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = IPvX::addr_bitlen(family);
    // Add the 'ipaddr' address as an attribute
    rta_len = RTA_LENGTH(IPvX::addr_size(family));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(rtmbuf)) {
	XLOG_ERROR("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(rtmbuf), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	return (XORP_ERROR);
    }
    rtattr = (struct rtattr *)(((uint8_t *)nlh) + NLMSG_ALIGN(nlh->nlmsg_len));
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    dst.copy_out((uint8_t *)RTA_DATA(rtattr));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    rtmsg->rtm_tos = 0;			// XXX: what is this TOS?
    rtmsg->rtm_table = RT_TABLE_UNSPEC; // Routing table ID
    rtmsg->rtm_protocol = RTPROT_UNSPEC;
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type  = RTN_UNSPEC;
    rtmsg->rtm_flags = 0;
    
    if (ns.sendto(rtmbuf, nlh->nlmsg_len, 0, (struct sockaddr *)&snl,
		  sizeof(snl)) != (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("error writing to netlink socket: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // We expect kernel to give us something back.  Force read until
    // data ripples up via nlsock_data() that corresponds to expected
    // sequence number and process id.
    //
    _cache_seqno = nlh->nlmsg_seq;
    _cache_valid = false;
    while (_cache_valid == false) {
	ns.force_recvfrom(0, (struct sockaddr *)&snl, &snl_len);
    }
    nlh_answer = (struct nlmsghdr *)(&_cache_data[0]);
    XLOG_ASSERT(nlh_answer->nlmsg_type == RTM_NEWROUTE);
    return (parse_buffer_nlm(fte, &_cache_data[0], _cache_data.size()));
}

/**
 * Lookup entry.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntryGetNetlink::lookup_entry(const IPvXNet& dst, FteX& fte)
{
    // TODO: XXX: PAVPAVPAV: implement it if Linux supports
    // lookup by network prefix, otherwise just use "lookup_route()"
    return (lookup_route(dst.masked_addr(), fte));
}

/**
 * Receive data from the netlink socket.
 *
 * Note that this method is called asynchronously when the netlink socket
 * has data to receive, therefore it should never be called directly by
 * anything else except the netlink socket facility itself.
 * 
 * @param data the buffer with the received data.
 * @param nbytes the number of bytes in the @param data buffer.
 */
void
FtiConfigEntryGetNetlink::nlsock_data(const uint8_t* data, size_t nbytes)
{
    NetlinkSocket& ns = *this;

    //
    // Copy data that has been requested to be cached by setting _cache_seqno.
    //
    size_t d = 0, off = 0;
    pid_t my_pid = ns.pid();
    
    _cache_data.resize(nbytes);
    
    while (d < nbytes) {
	const nlmsghdr* nlh = reinterpret_cast<const nlmsghdr*>(data + d);
	if (((pid_t)nlh->nlmsg_pid == my_pid)
	    && (nlh->nlmsg_seq == _cache_seqno)) {
#if 0	    // TODO: XXX: PAVPAVPAV: remove this assert?
	    XLOG_ASSERT(_cache_valid == false); // Do not overwrite cache data
#endif
	    XLOG_ASSERT(nbytes - d >= nlh->nlmsg_len);
	    memcpy(&_cache_data[off], nlh, nlh->nlmsg_len);
	    off += nlh->nlmsg_len;
	    _cache_valid = true;
	}
	d += nlh->nlmsg_len;
    }
}

#endif // HAVE_NETLINK_SOCKETS
