// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/mfea/mfea_unix_rtread_netlink.cc,v 1.21 2002/12/09 18:29:18 hodson Exp $"


//
// Implementation of Unicast Routing Table kernel read mechanism:
// AF_NETLINK (Linux-style raw socket).
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_RTREAD_METHOD) && (KERNEL_RTREAD_METHOD == KERNEL_RTREAD_NETLINK)

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
// Original guess for the number of unicast routing table entries
#define STARTUP_RTSIZE 65536

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//
static int max_table_n = STARTUP_RTSIZE;

//
// Local functions prototypes
//


/**
 * UnixComm::open_mrib_table_socket_osdep:
 * 
 * Open and initialize a routing socket (Linux-style NETLINK_ROUTE socket),
 * to read the whole unicast routing table from the kernel.
 * XXX: this socket should be open and closed every time it is needed.
 * (Linux-style NETLINK_ROUTE routing socket)
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_mrib_table_socket_osdep()
{
    int			sock = -1;
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
    if ((pid_t)sock_nl.nl_pid != _pid) {
	close(sock);
	XLOG_ERROR("Wrong nl_pid of AF_NETLINK socket: %d instead of %d",
		   sock_nl.nl_pid, _pid);
	return (XORP_ERROR);
    }
#endif // 0
    
    return (sock);
}

/**
 * UnixComm::get_mrib_table_osdep:
 * @return_mrib_table: A pointer to the routing table array composed
 * of #Mrib elements.
 * 
 * Return value: the number of entries in @return_mrib_table, or %XORP_ERROR
 * if there was an error.
 **/
int
UnixComm::get_mrib_table_osdep(Mrib *return_mrib_table[])
{
    int			sock, ret_val;
    int			my_rtm_seq;
    struct sockaddr_nl	sock_nl;
    int			msglen;
    struct nlmsghdr	*nlh;
    struct rtgenmsg	*rtgenmsg;
#define RTMBUFSIZE 4096
    char		rtmbuf[RTMBUFSIZE];
    struct iovec	iov;
    struct msghdr	msg;
    struct rtmsg	*rtmsg;
    struct rtattr	*rtattr;
    struct rtattr	*rta_array[RTA_MAX + 1];
    int			rta_len, pif_index;
    Mrib		*mrib_table = NULL, *mrib;
    int			mrib_table_n;
    IPvX		dest_addr(family());
    int			dest_masklen;
    MfeaVif		*mfea_vif;
    
    *return_mrib_table = NULL;
    
    sock = open_mrib_table_socket_osdep();
    if (sock < 0)
	return (XORP_ERROR);
    ret_val = XORP_ERROR;		// XXX: default
    
    // Allocate memory for the new table
    mrib_table = new Mrib[max_table_n](family());
    mrib_table_n = 0;
    
    // Set the request. First the socket, then the request itself.
    
    // Set the socket
    memset(&sock_nl, 0, sizeof(sock_nl));
    sock_nl.nl_family = AF_NETLINK;
    sock_nl.nl_pid    = 0;	// nl_pid = 0 if destination is the kernel
    sock_nl.nl_groups = 0;
    
    // Set the request
    nlh = (struct nlmsghdr *)rtmbuf;
    rtgenmsg = (struct rtgenmsg *)NLMSG_DATA(nlh);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtgenmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT; // Get the whole table
    my_rtm_seq = _rtm_seq++;
    if (_rtm_seq == 0)
	_rtm_seq++;
    nlh->nlmsg_seq = my_rtm_seq;
    nlh->nlmsg_pid = _pid;
    memset(rtgenmsg, 0, sizeof(*rtgenmsg));
    rtgenmsg->rtgen_family = family();
    
    // Sent the request to the kernel
    if (sendto(sock, rtmbuf, nlh->nlmsg_len, 0,
	       (struct sockaddr *)&sock_nl, sizeof(sock_nl)) < 0) {
	XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
		   "rror writing to socket: %s",
		   strerror(errno));
	goto done_error_label;
    }
    
    // Get the responses from the kernel and parse them
    do {
	// Init the recvmsg() arguments
	iov.iov_base = rtmbuf;
	iov.iov_len  = sizeof(rtmbuf);
	msg.msg_name = &sock_nl;
	msg.msg_namelen = sizeof(sock_nl);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	
	msglen = recvmsg(sock, &msg, 0);
	
	// Error checks
	if (msglen < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
		       "error reading from socket: %s",
		       strerror(errno));
	    goto done_error_label;
	}
	if (msg.msg_namelen != sizeof(sock_nl)) {
	    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
		       "error reading from socket: "
		       "sender address length error: "
		       "length %d instead of %d",
		       msg.msg_namelen, sizeof(sock_nl));
	    goto done_error_label;
	}
	if (msglen < (int)sizeof(*nlh)) {
	    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
		       "error reading from socket: "
		       "message truncated: %d instead of (at least) %d",
		       msglen, sizeof(*nlh));
	    goto done_error_label;
	}
	
	for (nlh = (struct nlmsghdr *)rtmbuf; NLMSG_OK(nlh, (unsigned)msglen);
	     nlh = NLMSG_NEXT(nlh, msglen)) {
	    // sequence number and pid check
#if 0
	    if (((int)nlh->nlmsg_seq != my_rtm_seq)
		|| ((pid_t)nlh->nlmsg_pid != _pid))
		continue;
#endif
	    if ((int)nlh->nlmsg_seq != my_rtm_seq) // XXX
		continue;
	    if (nlh->nlmsg_type == NLMSG_DONE)
		goto done_label;
	    if (nlh->nlmsg_type == NLMSG_NOOP)
		continue;
	    if (nlh->nlmsg_type == NLMSG_ERROR) {
		struct nlmsgerr *nlmsgerr = (struct nlmsgerr *)NLMSG_DATA(nlh);
		if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*nlmsgerr)))
		    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
			       "AF_NETLINK ERROR message truncated");
		else {
		    errno = -nlmsgerr->error;
		    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
			       "AF_NETLINK ERROR message: %s",
			       strerror(errno));
		}
		goto done_error_label;
	    }
	    
	    // Parse the entry
	    rtmsg = (struct rtmsg *)NLMSG_DATA(nlh);
	    if (rtmsg->rtm_family != family())
		continue;
	    if (nlh->nlmsg_type != RTM_NEWROUTE)
		continue;
	    if (rtmsg->rtm_type != RTN_UNICAST)
		continue;
	    // TODO: XXX: what about RTN_LOCAL entries
	    rta_len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*rtmsg));
	    if (rta_len < 0) {
		XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
			   "AF_NETLINK rtmsg length error");
		goto done_error_label;
	    }
	    // The attributes
	    memset(rta_array, 0, sizeof(rta_array));
	    rtattr = RTM_RTA(rtmsg);
	    while (RTA_OK(rtattr, rta_len)) {
		if (rtattr->rta_type <= RTA_MAX)
		    rta_array[rtattr->rta_type] = rtattr;
		rtattr = RTA_NEXT(rtattr, rta_len);
	    }
	    if (rta_len) {
		XLOG_WARNING("get_mrib_table(AF_NETLINK) failed: "
			     "AF_NETLINK deficit in rtattr: "
			     "%d rta_len remaining",
			     rta_len);
	    }
	    
	    // Init the next entry
	    if (mrib_table_n >= max_table_n) {
		// The table is too small. Reallocate.
		Mrib *tmp_mrib_table = mrib_table;
		max_table_n = 3*max_table_n/2;	// XXX: 150 percent increase
		mrib_table = new Mrib[max_table_n](family());
		// TODO: this should be element-by-element copy!!
		memcpy(mrib_table, tmp_mrib_table, mrib_table_n*sizeof(*mrib_table));
		delete[] tmp_mrib_table;
	    }
	    mrib = &mrib_table[mrib_table_n];
	    
	    if (rta_array[RTA_DST] == NULL) {
		// The default entry
	    } else {
		// TODO: fix this!!
		dest_addr.copy_in(family(),
				  (uint8_t *)RTA_DATA(rta_array[RTA_DST]));
		if (! dest_addr.is_unicast())
		    continue;	// Invalid unicast address
	    }
	    
	    // The destination masklen
	    dest_masklen = rtmsg->rtm_dst_len;
	    
	    // The destination prefix
	    mrib->set_dest_prefix(IPvXNet(dest_addr, dest_masklen));
	    
	    // The vif
	    if (rta_array[RTA_OIF] != NULL) {
		pif_index = *(int *)RTA_DATA(rta_array[RTA_OIF]);
		mfea_vif = mfea_node().vif_find_by_pif_index(pif_index);
		if (mfea_vif != NULL) {
		    mrib->set_next_hop_vif_index(mfea_vif->vif_index());
		} else {
		    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
			       "pif_index = %d, but no vif", pif_index);
		    continue;
		}
	    } else {
		XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: no interface");
		goto done_error_label;
	    }
	    if (rta_array[RTA_GATEWAY] != NULL) {
		IPvX router_addr(family());
		router_addr.copy_in(family(),
				    (uint8_t *)RTA_DATA(rta_array[RTA_GATEWAY]));
		mrib->set_next_hop_router_addr(router_addr);
	    } else {
		mrib->set_next_hop_router_addr(IPvX::ZERO(family()));
	    }
	    
	    mrib_table_n++;		// Added one more entry
	}
	// Some more error checking
	if (msg.msg_flags & MSG_TRUNC) {
	    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
		       "NETLINK recvmsg() error: message truncated");
	    goto done_error_label;
	}
	if (msglen) {
	    XLOG_ERROR("get_mrib_table(AF_NETLINK) failed: "
		       "NETLINK recvmsg() error: data remain size %d", msglen);
	    goto done_error_label;
	}
    } while (true); 
    
 done_error_label:
    if (mrib_table != NULL)
	delete[] mrib_table;
    *return_mrib_table = NULL;
    if (sock >= 0) 
	close(sock);
    return (XORP_ERROR);
    
 done_label:
    if (mrib_table_n > 0) {
	// XXX: readjusting the table size for next time: increasing by 10%.
	max_table_n = 11*mrib_table_n/10;
    }
    
    close(sock);		// XXX    
    
    *return_mrib_table = mrib_table;
    return (mrib_table_n);
}
#endif // KERNEL_RTREAD_METHOD == KERNEL_RTREAD_NETLINK





