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
// The mechanism to obtain the information is netlink sockets.
//


FtiConfigTableGetNetlink::FtiConfigTableGetNetlink(FtiConfig& ftic)
    : FtiConfigTableGet(ftic),
      NetlinkSocket(ftic.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket *)this),
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
    return (NetlinkSocket::start());
}
    
int
FtiConfigTableGetNetlink::stop()
{
    return (NetlinkSocket::stop());
}

void
FtiConfigTableGetNetlink::receive_data(const uint8_t* data, size_t n_bytes)
{
    // TODO: use it?
    UNUSED(data);
    UNUSED(n_bytes);
}

int
FtiConfigTableGetNetlink::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET, ftex_list) < 0)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(Fte4(ftex.net().get_ipv4net(),
				ftex.gateway().get_ipv4(),
				ftex.ifname(), ftex.vifname(),
				ftex.metric(), ftex.admin_distance()));
    }
    
    return (XORP_OK);
}

int
FtiConfigTableGetNetlink::get_table6(list<Fte6>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET6, ftex_list) < 0)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(Fte6(ftex.net().get_ipv6net(),
				ftex.gateway().get_ipv6(),
				ftex.ifname(), ftex.vifname(),
				ftex.metric(), ftex.admin_distance()));
    }
    
    return (XORP_OK);
}

#ifndef HAVE_NETLINK_SOCKETS

int
FtiConfigTableGetNetlink::get_table(int , list<FteX>& )
{
    return (XORP_ERROR);
}

void
FtiConfigTableGetNetlink::nlsock_data(const uint8_t* , size_t )
{
    
}

#else // HAVE_NETLINK_SOCKETS

int
FtiConfigTableGetNetlink::get_table(int family, list<FteX>& fte_list)
{
#define RTMBUFSIZE (sizeof(struct nlmsghdr) + sizeof(struct rtmsg) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct nlmsghdr	*nlh;
    struct sockaddr_nl	snl;
    struct rtgenmsg	*rtgenmsg;
    NetlinkSocket&	ns = *this;
    
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
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtgenmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.pid();
    rtgenmsg = (struct rtgenmsg *)NLMSG_DATA(nlh);
    rtgenmsg->rtgen_family = family;
    
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
	ns.force_recvmsg(0);
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
