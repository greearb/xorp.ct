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

#ident "$XORP: xorp/fea/fticonfig_entry_get_netlink.cc,v 1.14 2004/03/17 07:22:52 pavlin Exp $"


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
// The mechanism to obtain the information is netlink(7) sockets.
//


FtiConfigEntryGetNetlink::FtiConfigEntryGetNetlink(FtiConfig& ftic)
    : FtiConfigEntryGet(ftic),
      NetlinkSocket4(ftic.eventloop()),
      NetlinkSocket6(ftic.eventloop()),
      _ns_reader(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this)
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
    if (NetlinkSocket4::start() < 0)
	return (XORP_ERROR);
    
#ifdef HAVE_IPV6
    if (NetlinkSocket6::start() < 0) {
	NetlinkSocket4::stop();
	return (XORP_ERROR);
    }
#endif

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntryGetNetlink::stop()
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value4 = NetlinkSocket4::stop();
    
#ifdef HAVE_IPV6
    ret_value6 = NetlinkSocket6::stop();
#endif
    
    if ((ret_value4 < 0) || (ret_value6 < 0))
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetNetlink::lookup_route4(const IPv4& dst, Fte4& fte)
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
FtiConfigEntryGetNetlink::lookup_entry4(const IPv4Net& dst, Fte4& fte)
{
    list<Fte4> fte_list4;

    if (ftic().get_table4(fte_list4) != true)
	return (false);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (fte4.net() == dst) {
	    fte = fte4;
	    return (true);
	}
    }

    return (false);
}

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetNetlink::lookup_route6(const IPv6& dst, Fte6& fte)
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
FtiConfigEntryGetNetlink::lookup_entry6(const IPv6Net& dst, Fte6& fte)
{ 
    list<Fte6> fte_list6;

    if (ftic().get_table6(fte_list6) != true)
	return (false);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (fte6.net() == dst) {
	    fte = fte6;
	    return (true);
	}
    }

    return (false);
}

#ifndef HAVE_NETLINK_SOCKETS

bool
FtiConfigEntryGetNetlink::lookup_route(const IPvX& , FteX& )
{
    return false;
}

#else // HAVE_NETLINK_SOCKETS

/**
 * Lookup a route.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetNetlink::lookup_route(const IPvX& dst, FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr) + sizeof(struct rtmsg) + sizeof(struct rtattr) + 512;
    char		buffer[buffer_size];
    struct nlmsghdr	*nlh;
    struct sockaddr_nl	snl;
    struct rtmsg	*rtmsg;
    struct rtattr	*rtattr;
    int			rta_len;
    NetlinkSocket	*ns_ptr = NULL;
    int			family = dst.af();
    
    // Zero the return information
    fte.zero();
    
    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return false;
    }
    
    // Get the pointer to the NetlinkSocket
    do {
	if (dst.is_ipv4()) {
	    NetlinkSocket4& ns4 = *this;
	    ns_ptr = &ns4;
	    break;
	}
	if (dst.is_ipv6()) {
	    NetlinkSocket6& ns6 = *this;
	    ns_ptr = &ns6;
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while(false);
    
    //
    // Set the request. First the socket, then the request itself.
    //
    
    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;
    
    // Set the request
    memset(buffer, 0, sizeof(buffer));
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    rtmsg = reinterpret_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = IPvX::addr_bitlen(family);
    // Add the 'ipaddr' address as an attribute
    rta_len = RTA_LENGTH(IPvX::addr_size(family));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    dst.copy_out(reinterpret_cast<uint8_t*>(RTA_DATA(rtattr)));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    rtmsg->rtm_tos = 0;			// XXX: what is this TOS?
    rtmsg->rtm_table = RT_TABLE_UNSPEC; // Routing table ID
    rtmsg->rtm_protocol = RTPROT_UNSPEC;
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type  = RTN_UNSPEC;
    rtmsg->rtm_flags = 0;
    
    if (ns_ptr->sendto(buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl),
		       sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("error writing to netlink socket: %s",
		   strerror(errno));
	return false;
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    _ns_reader.receive_data(*ns_ptr, nlh->nlmsg_seq);
    if (parse_buffer_nlm(fte, _ns_reader.buffer(), _ns_reader.buffer_size(),
			 true)
	!= true) {
	return (false);
    }
    return (true);
}

#endif // HAVE_NETLINK_SOCKETS
