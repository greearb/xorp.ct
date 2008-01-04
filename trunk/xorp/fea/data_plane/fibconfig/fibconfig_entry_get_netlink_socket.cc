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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_netlink_socket.cc,v 1.16 2007/12/28 09:13:36 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_get.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "fibconfig_entry_get_netlink_socket.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is netlink(7) sockets.
//

#ifdef HAVE_NETLINK_SOCKETS

FibConfigEntryGetNetlinkSocket::FibConfigEntryGetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntryGet(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop()),
      _ns_reader(*(NetlinkSocket *)this)
{
}

FibConfigEntryGetNetlinkSocket::~FibConfigEntryGetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryGetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntryGetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigEntryGetNetlinkSocket::lookup_route_by_dest4(const IPv4& dst,
						      Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

int
FibConfigEntryGetNetlinkSocket::lookup_route_by_network4(const IPv4Net& dst,
							 Fte4& fte)
{
    list<Fte4> fte_list4;

    if (fibconfig().get_table4(fte_list4) != XORP_OK)
	return (XORP_ERROR);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (fte4.net() == dst) {
	    fte = fte4;
	    return (XORP_OK);
	}
    }

    return (XORP_ERROR);
}

int
FibConfigEntryGetNetlinkSocket::lookup_route_by_dest6(const IPv6& dst,
						      Fte6& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

int
FibConfigEntryGetNetlinkSocket::lookup_route_by_network6(const IPv6Net& dst,
							 Fte6& fte)
{ 
    list<Fte6> fte_list6;

    if (fibconfig().get_table6(fte_list6) != XORP_OK)
	return (XORP_ERROR);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (fte6.net() == dst) {
	    fte = fte6;
	    return (XORP_OK);
	}
    }

    return (XORP_ERROR);
}

int
FibConfigEntryGetNetlinkSocket::lookup_route_by_dest(const IPvX& dst,
						     FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct rtmsg) + sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			family = dst.af();
    void*		rta_align_data;
    uint32_t		table_id = RT_TABLE_UNSPEC; // Default value for lookup

    UNUSED(data);
    UNUSED(rta_align_data);

    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);

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
    memset(&buffer, 0, sizeof(buffer));
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    rtmsg = reinterpret_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = IPvX::addr_bitlen(family);
    // Add the 'ipaddr' address as an attribute
    rta_len = RTA_LENGTH(IPvX::addr_bytelen(family));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    dst.copy_out(reinterpret_cast<uint8_t*>(RTA_DATA(rtattr)));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    rtmsg->rtm_tos = 0;			// XXX: what is this TOS?
    rtmsg->rtm_protocol = RTPROT_UNSPEC;
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type  = RTN_UNSPEC;
    rtmsg->rtm_flags = 0;

    //
    // Set the routing/forwarding table ID.
    // If the table ID is <= 0xff, then we set it in rtmsg->rtm_table,
    // otherwise we set rtmsg->rtm_table to RT_TABLE_UNSPEC and add the
    // real value as an RTA_TABLE attribute.
    //
    if (fibconfig().unicast_forwarding_table_id_is_configured(family))
	table_id = fibconfig().unicast_forwarding_table_id(family);
    if (table_id <= 0xff)
	rtmsg->rtm_table = table_id;
    else
	rtmsg->rtm_table = RT_TABLE_UNSPEC;

#ifdef HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE
    // Add the table ID as an attribute
    if (table_id > 0xff) {
	uint32_t uint32_table_id = table_id;
	rta_len = RTA_LENGTH(sizeof(uint32_table_id));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_TABLE;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	memcpy(data, &uint32_table_id, sizeof(uint32_table_id));
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }
#endif // HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    string error_msg;
    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg) != XORP_OK) {
	XLOG_ERROR("Error reading from netlink socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (parse_buffer_netlink_socket(fibconfig().live_config_iftree(), fte,
				    _ns_reader.buffer(), true)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
