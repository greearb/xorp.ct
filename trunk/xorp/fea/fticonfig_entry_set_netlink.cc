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

#ident "$XORP: xorp/fea/fticonfig_entry_set_netlink.cc,v 1.1 2003/10/13 02:23:19 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <net/if.h>

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"
#include "netlink_socket_utils.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is netlink(7) sockets.
//


FtiConfigEntrySetNetlink::FtiConfigEntrySetNetlink(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
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

FtiConfigEntrySetNetlink::~FtiConfigEntrySetNetlink()
{
    stop();
}

int
FtiConfigEntrySetNetlink::start()
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
FtiConfigEntrySetNetlink::stop()
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
FtiConfigEntrySetNetlink::add_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetNetlink::delete_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetNetlink::add_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetNetlink::delete_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.gateway()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

#ifndef HAVE_NETLINK_SOCKETS
bool
FtiConfigEntrySetNetlink::add_entry(const FteX& )
{
    return false;
}

bool
FtiConfigEntrySetNetlink::delete_entry(const FteX& )
{
    return false;
}

void
FtiConfigEntrySetNetlink::nlsock_data(const uint8_t* , size_t )
{
    
}

#else // HAVE_NETLINK_SOCKETS
bool
FtiConfigEntrySetNetlink::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rtmsg)
	+ 3*sizeof(struct rtattr) + sizeof(int) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket*	ns_ptr = NULL;
    int			family = fte.net().af();
    uint16_t		if_index = 0;

    debug_msg("add_entry "
	      "(network = %s gateway = %s)",
	      fte.net().str().c_str(), fte.gateway().str().c_str());

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    // Get the pointer to the NetlinkSocket
    switch(family) {
    case AF_INET:
    {
	NetlinkSocket4& ns4 = *this;
	ns_ptr = &ns4;
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	NetlinkSocket6& ns6 = *this;
	ns_ptr = &ns6;
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_NEWROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    rtmsg = reinterpret_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = fte.net().prefix_len(); // The destination mask length
    rtmsg->rtm_src_len = 0;
    rtmsg->rtm_tos = 0;
    rtmsg->rtm_table = RT_TABLE_MAIN;
    rtmsg->rtm_protocol = RTPROT_XORP;		// Mark this as a XORP route
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type = RTN_UNICAST;
    rtmsg->rtm_flags = RTM_F_NOTIFY;

    // Add the destination address as an attribute
    rta_len = RTA_LENGTH(fte.net().masked_addr().addr_size());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.net().masked_addr().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    // Add the gateway address as an attribute
    rta_len = RTA_LENGTH(fte.gateway().addr_size());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = (struct rtattr*)(((char*)(rtattr)) + RTA_ALIGN((rtattr)->rta_len));
    rtattr->rta_type = RTA_GATEWAY;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.gateway().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    // Get the interface index
    if_index = 0;
#ifdef HAVE_IF_NAMETOINDEX
    if_index = if_nametoindex(fte.vifname().c_str());
#endif // HAVE_IF_NAMETOINDEX
    if (if_index == 0) {
	XLOG_FATAL("Could not find interface index for name %s",
		   fte.vifname().c_str());
    }

    // Add the interface index as an attribute
    int int_if_index = if_index;
    rta_len = RTA_LENGTH(sizeof(int_if_index));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = (struct rtattr*)(((char*)(rtattr)) + RTA_ALIGN((rtattr)->rta_len));
    rtattr->rta_type = RTA_OIF;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    memcpy(data, &int_if_index, sizeof(int_if_index));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    // Add the route priority as an attribute
    int int_priority = fte.admin_distance();
    rta_len = RTA_LENGTH(sizeof(int_priority));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = (struct rtattr*)(((char*)(rtattr)) + RTA_ALIGN((rtattr)->rta_len));
    rtattr->rta_type = RTA_PRIORITY;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    memcpy(data, &int_priority, sizeof(int_priority));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    // Add the route metric as an attribute
    int int_metric = fte.metric();
    rta_len = RTA_LENGTH(sizeof(int_metric));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = (struct rtattr*)(((char*)(rtattr)) + RTA_ALIGN((rtattr)->rta_len));
    rtattr->rta_type = RTA_METRICS;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    memcpy(data, &int_metric, sizeof(int_metric));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    
    string reason;
    if (ns_ptr->sendto(buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	XLOG_ERROR(reason.c_str());
	return false;
    }
    if (check_netlink_request(*ns_ptr, nlh->nlmsg_seq, reason) < 0) {
	XLOG_ERROR(reason.c_str());
	return false;
    }

    return true;
}

bool
FtiConfigEntrySetNetlink::delete_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rtmsg)
	+ 3*sizeof(struct rtattr) + sizeof(int) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket*	ns_ptr = NULL;
    int			family = fte.net().af();

    debug_msg("delete_entry "
	      "(network = %s gateway = %s)",
	      fte.net().str().c_str(), fte.gateway().str().c_str());

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    // Get the pointer to the NetlinkSocket
    switch(family) {
    case AF_INET:
    {
	NetlinkSocket4& ns4 = *this;
	ns_ptr = &ns4;
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	NetlinkSocket6& ns6 = *this;
	ns_ptr = &ns6;
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_DELROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    rtmsg = reinterpret_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = fte.net().prefix_len(); // The destination mask length
    rtmsg->rtm_src_len = 0;
    rtmsg->rtm_tos = 0;
    rtmsg->rtm_table = RT_TABLE_MAIN;
    rtmsg->rtm_protocol = RTPROT_XORP;		// Mark this as a XORP route
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type = RTN_UNICAST;
    rtmsg->rtm_flags = RTM_F_NOTIFY;

    // Add the destination address as an attribute
    rta_len = RTA_LENGTH(fte.net().masked_addr().addr_size());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.net().masked_addr().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    string reason;
    if (ns_ptr->sendto(buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	XLOG_ERROR(reason.c_str());
	return false;
    }
    if (check_netlink_request(*ns_ptr, nlh->nlmsg_seq, reason) < 0) {
	XLOG_ERROR(reason.c_str());
	return false;
    }

    return true;
}

/**
 * Check that a previous netlink request has succeeded.
 * 
 * @param ns the NetlinkSocket to use for reading data.
 * @param seqno the sequence nomer of the netlink request to check for.
 * @param reason the human-readable reason for any failure.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfigEntrySetNetlink::check_netlink_request(NetlinkSocket& ns,
						uint32_t seqno,
						string& reason)
{
    struct sockaddr_nl		snl;
    socklen_t			snl_len;
    const struct nlmsghdr*	nlh;
    size_t buf_bytes;

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // We expect kernel to give us something back.  Force read until
    // data ripples up via nlsock_data() that corresponds to expected
    // sequence number and process id.
    //
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	ns.force_recvfrom(0, reinterpret_cast<struct sockaddr*>(&snl),
			  &snl_len);
    }

    buf_bytes = _cache_data.size();
    for (nlh = reinterpret_cast<const struct nlmsghdr*>(&_cache_data[0]);
	 NLMSG_OK(nlh, buf_bytes);
	 nlh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(nlh), buf_bytes)) {
	caddr_t nlmsg_data = reinterpret_cast<caddr_t>(NLMSG_DATA(const_cast<struct nlmsghdr*>(nlh)));
	
	switch (nlh->nlmsg_type) {
	case NLMSG_ERROR:
	{
	    const struct nlmsgerr* err;

	    err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		reason = "AF_NETLINK nlmsgerr length error";
		return (XORP_ERROR);
	    }
	    if (err->error == 0)
		return (XORP_OK);	// No error
	    errno = -err->error;
	    reason = c_format("AF_NETLINK NLMSG_ERROR message: %s",
			      strerror(errno));
	    return (XORP_ERROR);
	}
	break;
	
	case NLMSG_DONE:
	{
	    // End-of-message, and no ACK was received: error.
	    reason = "No ACK was received";
	    return (XORP_ERROR);
	}
	break;
	
	case NLMSG_NOOP:
	    break;

	default:
	    debug_msg("Unhandled type %s(%d) (%d bytes)\n",
		      NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
		      nlh->nlmsg_type, nlh->nlmsg_len);
	    break;
	}
    }

    reason = "No ACK was received";
    return (XORP_ERROR);		// No ACK was received: error.
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
FtiConfigEntrySetNetlink::nlsock_data(const uint8_t* data, size_t nbytes)
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
