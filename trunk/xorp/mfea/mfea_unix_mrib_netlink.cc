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

#ident "$XORP: xorp/mfea/mfea_unix_mrib_netlink.cc,v 1.5 2003/03/10 23:20:40 hodson Exp $"


//
// Implementation of Reverse Path Forwarding kernel query mechanism,
// that would be used for Multicast Routing Information Base information:
// AF_NETLINK (Linux-style raw socket).
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_MRIB_METHOD) && (KERNEL_MRIB_METHOD == KERNEL_MRIB_NETLINK)

#include <fcntl.h>

#include "mrt/mrib_table.hh"
#include "mfea_node.hh"
#include "mfea_vif.hh"
#include "mfea_unix_comm.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


/**
 * UnixComm::open_mrib_socket_osdep:
 * 
 * Open and initialize a routing socket (Linux-style NETLINK_ROUTE)
 * to access/lookup the unicast forwarding table in the kernel.
 * (Linux-style NETLINK_ROUTE routing socket)
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_mrib_socket_osdep()
{
    int			sock;
    int			socket_protocol = -1;
    struct sockaddr_nl	sock_nl;
    socklen_t		sock_nl_len;
    
    switch (family()) {
    case AF_INET:
	socket_protocol = NETLINK_ROUTE;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	socket_protocol = NETLINK_ROUTE6;
	break;
#endif // HAVE_IPV6
    default:
	break;
    }
    
    if ( (sock = socket(AF_NETLINK, SOCK_RAW, socket_protocol)) < 0) {
	XLOG_ERROR("Cannot open AF_NETLINK raw socket stream: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
    
    // Don't block
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
	close(sock);
	XLOG_ERROR("fcntl(AF_NETLINK, O_NONBLOCK) failed: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
    
    // Bind the socket
    memset(&sock_nl, 0, sizeof(sock_nl));
    sock_nl.nl_family = AF_NETLINK;
    sock_nl.nl_pid    = 0;	// nl_pid = 0 if destination is the kernel
    sock_nl.nl_groups = 0;
    if (bind(sock, (struct sockaddr *)&sock_nl, sizeof(sock_nl)) < 0) {
	close(sock);
	XLOG_ERROR("bind(AF_NETLINK) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    // Double-check the result socket is AF_NETLINK
    sock_nl_len = sizeof(sock_nl);
    if (getsockname(sock, (struct sockaddr *)&sock_nl, &sock_nl_len) < 0) {
	close(sock);
	XLOG_ERROR("getsockname(AF_NETLINK) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    if (sock_nl_len != sizeof(sock_nl)) {
	close(sock);
	XLOG_ERROR("Wrong address length of AF_NETLINK socket: "
		   "%d instead of %d",
		   sock_nl_len, sizeof(sock_nl));
	return (XORP_ERROR);
    }
    if (sock_nl.nl_family != AF_NETLINK) {
	close(sock);
	XLOG_ERROR("Wrong address family of AF_NETLINK socket: "
		   "%d instead of %d",
		   sock_nl.nl_family, AF_NETLINK);
	return (XORP_ERROR);
    }
#if 0				// XXX
    // XXX: 'nl_pid' is supposed to be defined as 'pid_t'
    if ( (pid_t)sock_nl.nl_pid != _pid) {
	close(sock);
	XLOG_ERROR("Wrong nl_pid of AF_NETLINK socket: %d instead of %d",
		   sock_nl.nl_pid, _pid);
	return (XORP_ERROR);
    }
#endif // 0
    
    return (sock);
}


/**
 * UnixComm::get_mrib_osdep:
 * @dest_addr: The destination address to lookup.
 * @mrib: A reference to a #Mrib structure to return the MRIB information.
 * 
 * Get the Multicast Routing Information Base (MRIB) information
 * (the next hop router and the vif to send out packets toward a given
 * destination) for a given address.
 * (Linux-style NETLINK_ROUTE routing socket)
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mrib_osdep(const IPvX& dest_addr, Mrib& mrib)
{
    int			my_rtm_seq;
    struct sockaddr_nl	sock_nl;
    socklen_t		sock_nl_len;
    int			msglen;
    struct nlmsghdr	*nlh;
    struct rtmsg	*rtmsg;
    struct rtattr	*rtattr;
#define RTMBUFSIZE (sizeof(struct nlmsghdr) + sizeof(struct rtmsg) + 512)
    char		rtmbuf[RTMBUFSIZE];
    struct rtattr	*rta_array[RTA_MAX + 1];
    int			rta_len, pif_index;
    MfeaVif		*mfea_vif;
    
    if (dest_addr.af() != family())
	return (XORP_ERROR);
    
    // Zero the MRIB information
    mrib.set_dest_prefix(IPvXNet(dest_addr, dest_addr.addr_bitlen()));
    mrib.set_next_hop_router_addr(IPvX::ZERO(family()));
    mrib.set_next_hop_vif_index(Vif::VIF_INDEX_INVALID);
    // XXX: the UNIX kernel doesn't provide reasonable metrics
    mrib.set_metric_preference(mfea_node().mrib_table_default_metric_preference().get());
    mrib.set_metric(mfea_node().mrib_table_default_metric().get());
    
    if (! dest_addr.is_unicast())
	return (XORP_ERROR);
    
    // Check if the lookup address is one of my own interfaces or a neighbor
    mfea_vif = mfea_node().vif_find_same_subnet_op_p2p(dest_addr);
    if ((mfea_vif != NULL) && (mfea_vif->is_underlying_vif_up())) {
	mrib.set_next_hop_router_addr(dest_addr);
	mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	return (XORP_OK);
    } else {
	mfea_vif = NULL;
    }
    
    // Set the request. First the socket, then the request itself.
    
    // Set the socket
    memset(&sock_nl, 0, sizeof(sock_nl));
    sock_nl.nl_family = AF_NETLINK;
    sock_nl.nl_pid    = 0;	// nl_pid = 0 if destination is the kernel
    sock_nl.nl_groups = 0;
    
    // Set the request
    nlh = (struct nlmsghdr *)rtmbuf;
    rtmsg = (struct rtmsg *)NLMSG_DATA(nlh);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    my_rtm_seq = _rtm_seq++;
    if (_rtm_seq == 0)
	_rtm_seq++;
    nlh->nlmsg_seq = my_rtm_seq;
    nlh->nlmsg_pid = _pid;
    memset(rtmsg, 0, sizeof(*rtmsg));
    rtmsg->rtm_family = family();
    rtmsg->rtm_dst_len = IPvX::addr_bitlen(family());
    // Add the 'ipaddr' address as an attribute
    rta_len = RTA_LENGTH(IPvX::addr_size(family()));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(rtmbuf)) {
	XLOG_ERROR("get_mrib(AF_NETLINK) buffer size error: %d instead of %d",
		   sizeof(rtmbuf), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	return (XORP_ERROR);
    }
    rtattr = (struct rtattr *)(((uint8_t *)nlh) + NLMSG_ALIGN(nlh->nlmsg_len));
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    mrib.dest_prefix().masked_addr().copy_out((uint8_t *)RTA_DATA(rtattr));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    rtmsg->rtm_tos = 0;			// XXX: what is this TOS?
    rtmsg->rtm_table = RT_TABLE_UNSPEC; // Routing table ID
    rtmsg->rtm_protocol = RTPROT_UNSPEC;
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type  = RTN_UNSPEC;
    rtmsg->rtm_flags = 0;
    
    // Sent the request to the kernel
    if (sendto(_mrib_socket, rtmbuf, nlh->nlmsg_len, 0,
	       (struct sockaddr *)&sock_nl, sizeof(sock_nl)) < 0) {
	XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
		   "error writing to socket: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
    // Get the response from the kernel
    do {
	sock_nl_len = sizeof(sock_nl);
	msglen = recvfrom(_mrib_socket, rtmbuf, sizeof(rtmbuf), 0,
			   (struct sockaddr *)&sock_nl, &sock_nl_len);
	if (msglen < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
		       "error reading from socket: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	if (msglen < (int)sizeof(*nlh)) {
	    XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
		       "error reading from socket: "
		       "message truncated: %d instead of (at least) %d",
		       msglen, sizeof(*nlh));
	    return (XORP_ERROR);
	}
    } while (((int)nlh->nlmsg_seq != my_rtm_seq)
	     || ((pid_t)nlh->nlmsg_pid != _pid));
    
    // Message type check
    if (nlh->nlmsg_type != RTM_NEWROUTE) {
	if (nlh->nlmsg_type != NLMSG_ERROR) {
	    XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
		       "AF_NETLINK wrong answer type: %d instead of %d",
		       nlh->nlmsg_type, RTM_NEWROUTE);
	} else {
	    struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(nlh);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
			   "AF_NETLINK ERROR message truncated");
	    } else {
		errno = -err->error;
		XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
			   "AF_NETLINK ERROR message: %s",
			   strerror(errno));
	    }
	}
	return (XORP_ERROR);
    }
    msglen -= sizeof(*nlh);
    
    // rtmsg check
    mfea_vif = NULL;
    if (rtmsg->rtm_type == RTN_LOCAL) {
	// XXX: weird, we already called vif_find_same_subnet_or_p2p().
	// Anyway...
	mfea_vif = mfea_node().vif_find_same_subnet_or_p2p(dest_addr);
	if ((mfea_vif != NULL) && (mfea_vif->is_underlying_vif_up())) {
	    mrib.set_next_hop_router_addr(dest_addr);
	    mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	    return (XORP_OK);
	}
	mfea_vif = NULL;
	XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
		   "cannot find the vif for local interface route to %s",
		   cstring(mrib.dest_prefix()));
	return (false);
    }
    if (rtmsg->rtm_type != RTN_UNICAST) {
	XLOG_ERROR("get_mrib(AF_NETLINK) failed: "
		   "wrong AF_NETLINK route type to %s: %d instead of %d",
		   cstring(mrib.dest_prefix()), rtmsg->rtm_type, RTN_UNICAST);
	return (false);
    }
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = RTM_RTA(rtmsg);
    rta_len = msglen - sizeof(*rtmsg);
    while (RTA_OK(rtattr, rta_len)) {
        if (rtattr->rta_type <= RTA_MAX)
            rta_array[rtattr->rta_type] = rtattr;
        rtattr = RTA_NEXT(rtattr, rta_len);
    }
    if (rta_len) {
	XLOG_WARNING("get_mrib(AF_NETLINK) failed: "
		     "AF_NETLINK deficit in rtattr: "
		     "%d rta_len remaining",
		     rta_len);
    }
    
    mfea_vif = NULL;
    if (rta_array[RTA_OIF]) {
	pif_index = *(int *)RTA_DATA(rta_array[RTA_OIF]);
	mfea_vif = mfea_node().vif_find_by_pif_index(pif_index);
	if (mfea_vif == NULL) {
	    XLOG_ERROR("get_mrib(AF_NETLINN) failed: "
		       "pif_index = %d, but no vif", pif_index);
	    return (false);
	}
    } else {
	XLOG_ERROR("get_mrib(AF_NETLINK) failed: no interface");
	return (false);
    }
    if (rta_array[RTA_GATEWAY]) {
	IPvX mem_addr(family());
	mem_addr.copy_in(family(),
			 (uint8_t *)RTA_DATA(rta_array[RTA_GATEWAY]));
	mrib.set_next_hop_router_addr(mem_addr);
    } else {
	mrib.set_next_hop_router_addr(dest_addr);
    }
    
    if (mfea_vif != NULL) {
	mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	return (XORP_OK);		// XXX
    } else {
	return (XORP_ERROR);
    }
}

#endif // KERNEL_MRIB_METHOD == KERNEL_MRIB_NETLINK
