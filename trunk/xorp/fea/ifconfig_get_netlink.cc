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

#ident "$XORP: xorp/fea/ifconfig_get_netlink.cc,v 1.2 2003/10/01 22:49:47 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include <net/if.h>

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is netlink(7) sockets.
//

IfConfigGetNetlink::IfConfigGetNetlink(IfConfig& ifc)
    : IfConfigGet(ifc),
      NetlinkSocket4(ifc.eventloop()),
      NetlinkSocket6(ifc.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this),
      _cache_valid(false),
      _cache_seqno(0)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ifc();
#endif
}

IfConfigGetNetlink::~IfConfigGetNetlink()
{
    stop();
}

int
IfConfigGetNetlink::start()
{
    if (NetlinkSocket4::start() < 0)
	return (XORP_ERROR);
    
#ifdef HAVE_IPV6
    if (NetlinkSocket6::start() < 0) {
	NetlinkSocket4::stop();
	return (XORP_ERROR);
    }
#endif
    
    return (XORP_OK);
}

int
IfConfigGetNetlink::stop()
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;
    
    ret_value4 = NetlinkSocket4::stop();
    
#ifdef HAVE_IPV6
    ret_value6 = NetlinkSocket6::stop();
#endif
    
    if ((ret_value4 < 0) || (ret_value6 < 0))
	return (XORP_ERROR);
    
    return (XORP_OK);
}

bool
IfConfigGetNetlink::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HAVE_NETLINK_SOCKETS

bool
IfConfigGetNetlink::read_config(IfTree& )
{
    return false;
}

void
IfConfigGetNetlink::nlsock_data(const uint8_t* , size_t )
{
    
}

#else // HAVE_NETLINK_SOCKETS

bool
IfConfigGetNetlink::read_config(IfTree& it)
{
#define IFIBUFSIZE (sizeof(struct nlmsghdr) + sizeof(struct ifinfomsg) + sizeof(struct ifaddrmsg) + 512)
    char		ifibuf[IFIBUFSIZE];
    struct nlmsghdr	*nlh, *nlh_answer;
    struct sockaddr_nl	snl;
    socklen_t		snl_len;
    struct ifinfomsg	*ifinfomsg;
    struct ifaddrmsg	*ifaddrmsg;
    NetlinkSocket4&	ns4 = *this;
#ifdef HAVE_IPV6
    NetlinkSocket6&	ns6 = *this;
#endif
    
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
    memset(ifibuf, 0, sizeof(ifibuf));
    nlh = reinterpret_cast<struct nlmsghdr*>(ifibuf);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_GETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns4.seqno();
    nlh->nlmsg_pid = ns4.pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = 0;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;
    
    if (ns4.sendto(ifibuf, nlh->nlmsg_len, 0,
		   reinterpret_cast<struct sockaddr*>(&snl),
		   sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("error writing to netlink socket: %s",
		   strerror(errno));
	return false;
    }
    
    //
    // We expect kernel to give us something back.  Force read until
    // data ripples up via nlsock_data() that corresponds to expected
    // sequence number and process id.
    //
    _cache_seqno = nlh->nlmsg_seq;
    _cache_valid = false;
    while (_cache_valid == false) {
	ns4.force_recvfrom(0, reinterpret_cast<struct sockaddr*>(&snl),
			   &snl_len);
    }
    nlh_answer = reinterpret_cast<struct nlmsghdr*>(&_cache_data[0]);
    if (parse_buffer_nlm(it, &_cache_data[0], _cache_data.size()) != true)
	return (false);
    
    //
    // Create a list with the interface indexes
    //
    list<uint16_t> if_index_list;
    uint16_t if_index;
    
    IfTree::IfMap::const_iterator if_iter;
    for (if_iter = it.ifs().begin(); if_iter != it.ifs().end(); ++if_iter) {
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
    list<uint16_t>::const_iterator if_index_iter;
    for (if_index_iter = if_index_list.begin();
	 if_index_iter != if_index_list.end();
	 ++if_index_iter) {
	if_index = *if_index_iter;
	
	//
	// Set the request for IPv4 addresses
	//
	memset(ifibuf, 0, sizeof(ifibuf));
	nlh = reinterpret_cast<struct nlmsghdr*>(ifibuf);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	nlh->nlmsg_type = RTM_GETADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
	nlh->nlmsg_seq = ns4.seqno();
	nlh->nlmsg_pid = ns4.pid();
	ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	ifaddrmsg->ifa_family = AF_INET;
	ifaddrmsg->ifa_prefixlen = 0;
	ifaddrmsg->ifa_flags = 0;
	ifaddrmsg->ifa_scope = 0;
	ifaddrmsg->ifa_index = if_index;
	
	if (ns4.sendto(ifibuf, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl),
		       sizeof(snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
	    return false;
	}
	
	//
	// We expect kernel to give us something back.  Force read until
	// data ripples up via nlsock_data() that corresponds to expected
	// sequence number and process id.
	//
	_cache_seqno = nlh->nlmsg_seq;
	_cache_valid = false;
	while (_cache_valid == false) {
	    ns4.force_recvfrom(0, reinterpret_cast<struct sockaddr*>(&snl),
			       &snl_len);
	}
	nlh_answer = reinterpret_cast<struct nlmsghdr*>(&_cache_data[0]);
	if (parse_buffer_nlm(it, &_cache_data[0], _cache_data.size()) != true)
	    return (false);

#ifdef HAVE_IPV6
	//
	// Set the request for IPv6 addresses
	//
	memset(ifibuf, 0, sizeof(ifibuf));
	nlh = reinterpret_cast<struct nlmsghdr*>(ifibuf);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	nlh->nlmsg_type = RTM_GETADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
	nlh->nlmsg_seq = ns6.seqno();
	nlh->nlmsg_pid = ns6.pid();
	ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	ifaddrmsg->ifa_family = AF_INET6;
	ifaddrmsg->ifa_prefixlen = 0;
	ifaddrmsg->ifa_flags = 0;
	ifaddrmsg->ifa_scope = 0;
	ifaddrmsg->ifa_index = if_index;
	
	if (ns6.sendto(ifibuf, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl),
		       sizeof(snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
	    return false;
	}
	
	//
	// We expect kernel to give us something back.  Force read until
	// data ripples up via nlsock_data() that corresponds to expected
	// sequence number and process id.
	//
	_cache_seqno = nlh->nlmsg_seq;
	_cache_valid = false;
	while (_cache_valid == false) {
	    ns6.force_recvfrom(0, reinterpret_cast<struct sockaddr*>(&snl),
			       &snl_len);
	}
	nlh_answer = reinterpret_cast<struct nlmsghdr*>(&_cache_data[0]);
	if (parse_buffer_nlm(it, &_cache_data[0], _cache_data.size()) != true)
	    return (false);
#endif // HAVE_IPV6
    }
    
    return (true);
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
IfConfigGetNetlink::nlsock_data(const uint8_t* data, size_t nbytes)
{
    NetlinkSocket4& ns4 = *this;	// XXX: needed only to get the pid
    
    //
    // Copy data that has been requested to be cached by setting _cache_seqno.
    //
    size_t d = 0, off = 0;
    pid_t my_pid = ns4.pid();
    
    UNUSED(my_pid);	// XXX: (see below)
    
    _cache_data.resize(nbytes);
    
    while (d < nbytes) {
	const struct nlmsghdr* nlh = reinterpret_cast<const struct nlmsghdr*>(data + d);
	if (nlh->nlmsg_seq == _cache_seqno) {
	    //
	    // TODO: XXX: here we should add the following check as well:
	    //       ((pid_t)nlh->nlmsg_pid == my_pid)
	    // However, it appears that on return Linux doesn't fill-in
	    // nlh->nlmsg_pid to our pid (e.g., it may be set to 0xffffefff).
	    // Unfortunately, Linux's netlink(7) is not helpful on the
	    // subject, hence we ignore this additional test.
	    //
	    
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
