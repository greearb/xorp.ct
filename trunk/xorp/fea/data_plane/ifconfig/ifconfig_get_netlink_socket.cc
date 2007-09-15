// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_netlink_socket.cc,v 1.12 2007/09/15 05:10:21 pavlin Exp $"

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
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_get_netlink_socket.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is netlink(7) sockets.
//

#ifdef HAVE_NETLINK_SOCKETS

IfConfigGetNetlinkSocket::IfConfigGetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop()),
      _ns_reader(*(NetlinkSocket *)this)
{
}

IfConfigGetNetlinkSocket::~IfConfigGetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigGetNetlinkSocket::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

int
IfConfigGetNetlinkSocket::read_config(IfTree& iftree)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + sizeof(struct ifaddrmsg) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    struct ifaddrmsg*	ifaddrmsg;
    NetlinkSocket&	ns = *this;
    
    //
    // Set the request. First the socket, then the request itself.
    //
    // Note that first we send a request for the network interfaces,
    // then a request for the IPv4 addresses, and then a request
    // for the IPv6 addresses.
    //
    
    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;
    
    //
    // Set the request for network interfaces
    //
    memset(&buffer, 0, sizeof(buffer));
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_GETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = 0;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;
    
    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl),
		  sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    //
    // XXX: setting the flag below is a work-around hack because of a
    // Linux kernel bug: when we read the whole table the kernel
    // may not set the NLM_F_MULTI flag for the multipart messages.
    //
    string error_msg;
    ns.set_multipart_message_read(true);
    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg) != XORP_OK) {
	ns.set_multipart_message_read(false);
	XLOG_ERROR("Error reading from netlink socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }
    // XXX: reset the multipart message read hackish flag
    ns.set_multipart_message_read(false);
    if (parse_buffer_netlink_socket(ifconfig(), iftree, _ns_reader.buffer())
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    //
    // Create a list with the interface indexes
    //
    list<uint32_t> if_index_list;
    uint32_t if_index;
    
    IfTree::IfMap::const_iterator if_iter;
    for (if_iter = iftree.interfaces().begin();
	 if_iter != iftree.interfaces().end();
	 ++if_iter) {
	const IfTreeInterface& iface = if_iter->second;
	IfTreeInterface::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfTreeVif& vif = vif_iter->second;
	    if_index = vif.pif_index();
	    if_index_list.push_back(if_index);
	}
    }
    
    //
    // Send requests for the addresses of each interface we just found
    //
    list<uint32_t>::const_iterator if_index_iter;
    for (if_index_iter = if_index_list.begin();
	 if_index_iter != if_index_list.end();
	 ++if_index_iter) {
	if_index = *if_index_iter;
	
	//
	// Set the request for IPv4 addresses
	//
	if (fea_data_plane_manager().have_ipv4()) {
	    memset(&buffer, 0, sizeof(buffer));
	    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	    nlh->nlmsg_type = RTM_GETADDR;
	    // Get the whole table
	    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	    nlh->nlmsg_seq = ns.seqno();
	    nlh->nlmsg_pid = ns.nl_pid();
	    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	    ifaddrmsg->ifa_family = AF_INET;
	    ifaddrmsg->ifa_prefixlen = 0;
	    ifaddrmsg->ifa_flags = 0;
	    ifaddrmsg->ifa_scope = 0;
	    ifaddrmsg->ifa_index = if_index;
	
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
	    // Linux kernel bug: when we read the whole table the kernel
	    // may not set the NLM_F_MULTI flag for the multipart messages.
	    //
	    string error_msg;
	    ns.set_multipart_message_read(true);
	    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
		!= XORP_OK) {
		ns.set_multipart_message_read(false);
		XLOG_ERROR("Error reading from netlink socket: %s",
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    // XXX: reset the multipart message read hackish flag
	    ns.set_multipart_message_read(false);
	    if (parse_buffer_netlink_socket(ifconfig(), iftree,
					    _ns_reader.buffer())
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}

#ifdef HAVE_IPV6
	//
	// Set the request for IPv6 addresses
	//
	if (fea_data_plane_manager().have_ipv6()) {
	    memset(&buffer, 0, sizeof(buffer));
	    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	    nlh->nlmsg_type = RTM_GETADDR;
	    // Get the whole table
	    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	    nlh->nlmsg_seq = ns.seqno();
	    nlh->nlmsg_pid = ns.nl_pid();
	    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	    ifaddrmsg->ifa_family = AF_INET6;
	    ifaddrmsg->ifa_prefixlen = 0;
	    ifaddrmsg->ifa_flags = 0;
	    ifaddrmsg->ifa_scope = 0;
	    ifaddrmsg->ifa_index = if_index;
	
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
	    // Linux kernel bug: when we read the whole table the kernel
	    // may not set the NLM_F_MULTI flag for the multipart messages.
	    //
	    string error_msg;
	    ns.set_multipart_message_read(true);
	    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
		!= XORP_OK) {
		ns.set_multipart_message_read(false);
		XLOG_ERROR("Error reading from netlink socket: %s",
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    // XXX: reset the multipart message read hackish flag
	    ns.set_multipart_message_read(false);
	    if (parse_buffer_netlink_socket(ifconfig(), iftree,
					    _ns_reader.buffer())
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
#endif // HAVE_IPV6
    }
    
    return (XORP_OK);
}
    
#endif // HAVE_NETLINK_SOCKETS
