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

#ident "$XORP: xorp/fea/fticonfig_table_get_netlink.cc,v 1.8 2003/09/20 06:51:52 pavlin Exp $"


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

#include "fticonfig.hh"
#include "fticonfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is netlink(7) sockets.
//


FtiConfigTableGetNetlink::FtiConfigTableGetNetlink(FtiConfig& ftic)
    : FtiConfigTableGet(ftic),
      NetlinkSocket4(ftic.eventloop()),
      NetlinkSocket6(ftic.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this),
      _cache_valid(false),
      _cache_seqno(0)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ftic();
#endif
}

FtiConfigTableGetNetlink::~FtiConfigTableGetNetlink()
{
    stop();
}

int
FtiConfigTableGetNetlink::start()
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
FtiConfigTableGetNetlink::stop()
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
FtiConfigTableGetNetlink::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET, ftex_list) != true)
	return false;
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(Fte4(ftex.net().get_ipv4net(),
				ftex.gateway().get_ipv4(),
				ftex.ifname(), ftex.vifname(),
				ftex.metric(), ftex.admin_distance(),
				ftex.xorp_route()));
    }
    
    return true;
}

bool
FtiConfigTableGetNetlink::get_table6(list<Fte6>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET6, ftex_list) != true)
	return false;
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(Fte6(ftex.net().get_ipv6net(),
				ftex.gateway().get_ipv6(),
				ftex.ifname(), ftex.vifname(),
				ftex.metric(), ftex.admin_distance(),
				ftex.xorp_route()));
    }
    
    return true;
}

#ifndef HAVE_NETLINK_SOCKETS

bool
FtiConfigTableGetNetlink::get_table(int , list<FteX>& )
{
    return false;
}

void
FtiConfigTableGetNetlink::nlsock_data(const uint8_t* , size_t )
{
    
}

#else // HAVE_NETLINK_SOCKETS

bool
FtiConfigTableGetNetlink::get_table(int family, list<FteX>& fte_list)
{
#define RTMBUFSIZE (sizeof(struct nlmsghdr) + sizeof(struct rtmsg) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct nlmsghdr	*nlh;
    struct sockaddr_nl	snl;
    struct rtgenmsg	*rtgenmsg;
    NetlinkSocket	*ns_ptr = NULL;
    
    // Get the pointer to the NetlinkSocket
    switch(family) {
    case AF_INET:
    {
	NetlinkSocket4&	ns4 = *this;
	ns_ptr = &ns4;
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	NetlinkSocket6&	ns6 = *this;
	ns_ptr = &ns6;
	break;
    }
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
    memset(rtmbuf, 0, sizeof(rtmbuf));
    nlh = reinterpret_cast<struct nlmsghdr*>(rtmbuf);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtgenmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    rtgenmsg = reinterpret_cast<struct rtgenmsg*>(NLMSG_DATA(nlh));
    rtgenmsg->rtgen_family = family;
    
    if (ns_ptr->sendto(rtmbuf, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl),
		       sizeof(snl)) != (ssize_t)nlh->nlmsg_len) {
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
	ns_ptr->force_recvmsg(0);
    }
    return (parse_buffer_nlm(family, fte_list, &_cache_data[0],
			     _cache_data.size()));
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
FtiConfigTableGetNetlink::nlsock_data(const uint8_t* data, size_t nbytes)
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
	const struct nlmsghdr* nlh = reinterpret_cast<const struct nlmsghdr *>(data + d);
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
