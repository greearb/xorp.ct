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

#ident "$XORP: xorp/fea/fticonfig_table_get_netlink.cc,v 1.12 2003/10/13 23:32:41 pavlin Exp $"


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
      _ns_reader(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this)
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
#ifndef HAVE_IPV6
    UNUSED(fte_list);
    
    return false;
#else
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
#endif // HAVE_IPV6
}

#ifndef HAVE_NETLINK_SOCKETS

bool
FtiConfigTableGetNetlink::get_table(int , list<FteX>& )
{
    return false;
}

#else // HAVE_NETLINK_SOCKETS

bool
FtiConfigTableGetNetlink::get_table(int family, list<FteX>& fte_list)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr) + sizeof(struct rtmsg) + 512;
    char		buffer[buffer_size];
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
    memset(buffer, 0, sizeof(buffer));
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtgenmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    rtgenmsg = reinterpret_cast<struct rtgenmsg*>(NLMSG_DATA(nlh));
    rtgenmsg->rtgen_family = family;
    
    if (ns_ptr->sendto(buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl),
		       sizeof(snl)) != (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("error writing to netlink socket: %s",
		   strerror(errno));
	return false;
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    //
    // XXX: setting the flag below is a work-around hack because of a
    // Linux kernel bug: when we read the forwarding table the kernel
    // doesn't set the NLM_F_MULTI flag for the multipart messages.
    //
    ns_ptr->set_multipart_message_read(true);
    _ns_reader.receive_data(*ns_ptr, nlh->nlmsg_seq);
    // XXX: reset the multipart message read hackish flag
    ns_ptr->set_multipart_message_read(false);
    if (parse_buffer_nlm(family, fte_list, _ns_reader.buffer(),
			 _ns_reader.buffer_size())
	!= true) {
	return (false);
    }
    return (true);
}

#endif // HAVE_NETLINK_SOCKETS
