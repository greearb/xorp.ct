// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/fibconfig.hh"

#include "fibconfig_table_get_netlink_socket.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is netlink(7) sockets.
//

#ifdef HAVE_NETLINK_SOCKETS

FibConfigTableGetNetlinkSocket::FibConfigTableGetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableGet(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop()),
      _ns_reader(*(NetlinkSocket *)this)
{
}

FibConfigTableGetNetlinkSocket::~FibConfigTableGetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableGetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigTableGetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigTableGetNetlinkSocket::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;

    // Get the table
    if (get_table(AF_INET, ftex_list) != XORP_OK)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(ftex.get_fte4());
    }
    
    return (XORP_OK);
}

int
FibConfigTableGetNetlinkSocket::get_table6(list<Fte6>& fte_list)
{
#ifndef HAVE_IPV6
    UNUSED(fte_list);
    
    return (XORP_ERROR);
#else
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET6, ftex_list) != XORP_OK)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(ftex.get_fte6());
    }
    
    return (XORP_OK);
#endif // HAVE_IPV6
}

int
FibConfigTableGetNetlinkSocket::get_table(int family, list<FteX>& fte_list)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct rtmsg) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct rtgenmsg*	rtgenmsg;
    NetlinkSocket&	ns = *this;

    // Check that the family is supported
    switch(family) {
    case AF_INET:
	if (! fea_data_plane_manager().have_ipv4())
	    return (XORP_ERROR);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (! fea_data_plane_manager().have_ipv6())
	    return (XORP_ERROR);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
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
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtgenmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    rtgenmsg = reinterpret_cast<struct rtgenmsg*>(NLMSG_DATA(nlh));
    rtgenmsg->rtgen_family = family;

    struct rtmsg* rtmsg = reinterpret_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    uint32_t table_id = RT_TABLE_UNSPEC; // Default value for lookup

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
    struct rtattr* rtattr = RTM_RTA(rtmsg);
    // Add the table ID as an attribute
    if (table_id > 0xff) {
	uint8_t* data;
	uint32_t uint32_table_id = table_id;
	int rta_len = RTA_LENGTH(sizeof(uint32_table_id));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	void* rta_align_data = reinterpret_cast<char*>(rtattr)
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
		  reinterpret_cast<struct sockaddr*>(&snl),
		  sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    //
    // XXX: setting the flag below is a work-around hack because of a
    // Linux kernel bug: when we read the forwarding table the kernel
    // doesn't set the NLM_F_MULTI flag for the multipart messages.
    //
    ns.set_multipart_message_read(true);
    string error_msg;
    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
	!= XORP_OK) {
	ns.set_multipart_message_read(false);
	XLOG_ERROR("Error reading from netlink socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }
    // XXX: reset the multipart message read hackish flag
    ns.set_multipart_message_read(false);

    if (parse_buffer_netlink_socket(family, fibconfig().system_config_iftree(),
				    fte_list, _ns_reader.buffer(), true, fibconfig())
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
