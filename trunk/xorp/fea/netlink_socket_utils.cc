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

#ident "$XORP: xorp/fea/netlink_socket_utils.cc,v 1.9 2003/10/13 23:32:41 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ipvx.hh"

#include <net/if.h>

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "kernel_utils.hh"
#include "netlink_socket.hh"
#include "netlink_socket_utils.hh"

//
// NETLINK format related utilities for manipulating data
// (e.g., obtained by netlink(7) sockets mechanism).
//

#ifdef HAVE_NETLINK_SOCKETS

/**
 * @param m message type from netlink socket message
 * @return human readable form.
 */
string
NlmUtils::nlm_msg_type(uint32_t m)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } nlm_msg_types[] = {
#define RTM_MSG_ENTRY(X) { X, #X }
#ifdef RTM_NEWLINK
	RTM_MSG_ENTRY(RTM_NEWLINK),
#endif
#ifdef RTM_DELLINK
	RTM_MSG_ENTRY(RTM_DELLINK),
#endif
#ifdef RTM_GETLINK
	RTM_MSG_ENTRY(RTM_GETLINK),
#endif
#ifdef RTM_NEWADDR
	RTM_MSG_ENTRY(RTM_NEWADDR),
#endif
#ifdef RTM_DELADDR
	RTM_MSG_ENTRY(RTM_DELADDR),
#endif
#ifdef RTM_GETADDR
	RTM_MSG_ENTRY(RTM_GETADDR),
#endif
#ifdef RTM_NEWROUTE
	RTM_MSG_ENTRY(RTM_NEWROUTE),
#endif
#ifdef RTM_DELROUTE
	RTM_MSG_ENTRY(RTM_DELROUTE),
#endif
#ifdef RTM_GETROUTE
	RTM_MSG_ENTRY(RTM_GETROUTE),
#endif
#ifdef RTM_NEWNEIGH
	RTM_MSG_ENTRY(RTM_NEWNEIGH),
#endif
#ifdef RTM_DELNEIGH
	RTM_MSG_ENTRY(RTM_DELNEIGH),
#endif
#ifdef RTM_GETNEIGH
	RTM_MSG_ENTRY(RTM_GETNEIGH),
#endif
#ifdef RTM_NEWRULE
	RTM_MSG_ENTRY(RTM_NEWRULE),
#endif
#ifdef RTM_DELRULE
	RTM_MSG_ENTRY(RTM_DELRULE),
#endif
#ifdef RTM_GETRULE
	RTM_MSG_ENTRY(RTM_GETRULE),
#endif
#ifdef RTM_NEWQDISC
	RTM_MSG_ENTRY(RTM_NEWQDISC),
#endif
#ifdef RTM_DELQDISC
	RTM_MSG_ENTRY(RTM_DELQDISC),
#endif
#ifdef RTM_GETQDISC
	RTM_MSG_ENTRY(RTM_GETQDISC),
#endif
#ifdef RTM_NEWTCLASS
	RTM_MSG_ENTRY(RTM_NEWTCLASS),
#endif
#ifdef RTM_DELTCLASS
	RTM_MSG_ENTRY(RTM_DELTCLASS),
#endif
#ifdef RTM_GETTCLASS
	RTM_MSG_ENTRY(RTM_GETTCLASS),
#endif
#ifdef RTM_NEWTFILTER
	RTM_MSG_ENTRY(RTM_NEWTFILTER),
#endif
#ifdef RTM_DELTFILTER
	RTM_MSG_ENTRY(RTM_DELTFILTER),
#endif
#ifdef RTM_GETTFILTER
	RTM_MSG_ENTRY(RTM_GETTFILTER),
#endif
#ifdef RTM_MAX
	RTM_MSG_ENTRY(RTM_MAX),
#endif
	{ ~0, "Unknown" }
    };
    const size_t n_nlm_msgs = sizeof(nlm_msg_types) / sizeof(nlm_msg_types[0]);
    const char* ret = 0;
    for (size_t i = 0; i < n_nlm_msgs; i++) {
	ret = nlm_msg_types[i].name;
	if (nlm_msg_types[i].value == m)
	    break;
    }
    return ret;
}

void
NlmUtils::get_rtattr(const struct rtattr* rtattr, int rta_len,
		     const struct rtattr* rta_array[], size_t rta_array_n)
{
    while (RTA_OK(rtattr, rta_len)) {
	if (rtattr->rta_type < rta_array_n)
	    rta_array[rtattr->rta_type] = rtattr;
	rtattr = RTA_NEXT(const_cast<struct rtattr *>(rtattr), rta_len);
    }
    
    if (rta_len) {
	XLOG_WARNING("get_rtattr() failed: AF_NETLINK deficit in rtattr: "
		     "%d rta_len remaining",
		     rta_len);
    }
}

bool
NlmUtils::nlm_get_to_fte_cfg(FteX& fte, const struct nlmsghdr* nlh,
			     const struct rtmsg* rtmsg, int rta_len)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[RTA_MAX + 1];
    int if_index;
    string if_name;
    int family = fte.gateway().af();
    bool is_deleted = false;
    
    // Reset the result
    fte.zero();
    
    // Test if this entry was deleted
    if (nlh->nlmsg_type == RTM_DELROUTE)
	is_deleted = true;

    IPvX gateway_addr(family);
    IPvX dst_addr(family);
    int dst_mask_len = 0;
    
    // rtmsg check
    if (rtmsg->rtm_type == RTN_LOCAL) {
	// TODO: XXX: PAVPAVPAV: handle it, if needed!
	return false;		// TODO: is it really an error?
    }
    if (rtmsg->rtm_type != RTN_UNICAST) {
	XLOG_ERROR("nlm_get_to_fte_cfg() failed: "
		   "wrong AF_NETLINK route type: %d instead of %d",
		   rtmsg->rtm_type, RTN_UNICAST);
	return false;
    }
    if (rtmsg->rtm_family != family)
	return false;		// Invalid address family
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = RTM_RTA(const_cast<struct rtmsg *>(rtmsg));
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
			 sizeof(rta_array) / sizeof(rta_array[0]));

    //
    // Get the destination
    //
    if (rta_array[RTA_DST] == NULL) {
	// The default entry
    } else {
	// TODO: fix this!!
	dst_addr.copy_in(family, (uint8_t *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_DST])));
	dst_addr = kernel_ipvx_adjust(dst_addr);
	if (! dst_addr.is_unicast()) {
	    // TODO: should we make this check?
	    fte.zero();
	    return false;	// XXX: invalid unicast address
	}
    }
    
    //
    // Get the gateway
    //
    if (rta_array[RTA_GATEWAY] != NULL) {
	gateway_addr.copy_in(family, (uint8_t *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_GATEWAY])));
    }
    
    //
    // Get the destination mask length
    //
    dst_mask_len = rtmsg->rtm_dst_len;

    //
    // Test whether we installed this route
    //
    bool xorp_route = false;
    if (rtmsg->rtm_protocol == RTPROT_XORP)
	xorp_route = true;

    //
    // Get the interface index
    //
    if (rta_array[RTA_OIF] != NULL) {
	if_index = *(int *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_OIF]));
    } else {
	XLOG_ERROR("nlm_get_to_fte_cfg() failed: no interface found");
	return false;
    }
    
    //
    // Get the interface name
    //
    {
	const char* name = NULL;
#ifdef HAVE_IF_INDEXTONAME
	char name_buf[IF_NAMESIZE];
	name = if_indextoname(if_index, name_buf);
#endif
	if (name == NULL) {
	    XLOG_FATAL("Could not find interface corresponding to index %d",
		       if_index);
	}
	if_name = string(name);
    }

    //
    // Get the route metric
    //
    uint32_t route_metric = ~0;
    if (rta_array[RTA_METRICS] != NULL) {
	int int_route_metric;
	int_route_metric = *(int *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_METRICS]));
	route_metric = int_route_metric;
    }

    //
    // Get the admin distance
    //
    uint32_t admin_distance = ~0;
    if (rta_array[RTA_PRIORITY] != NULL) {
	int int_admin_distance;
	int_admin_distance = *(int *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_PRIORITY]));
	admin_distance = int_admin_distance;
    }

    fte = FteX(IPvXNet(dst_addr, dst_mask_len), gateway_addr, if_name, if_name,
	       route_metric, admin_distance, xorp_route);
    if (is_deleted)
	fte.mark_deleted();
    
    return true;
}

/**
 * Check that a previous netlink request has succeeded.
 *
 * @param ns_reader the NetlinkSocketReader to use for reading data.
 * @param ns the NetlinkSocket to use for reading data.
 * @param seqno the sequence nomer of the netlink request to check for.
 * @param reason the human-readable reason for any failure.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
NlmUtils::check_netlink_request(NetlinkSocketReader& ns_reader,
				NetlinkSocket& ns,
				uint32_t seqno,
				string& reason)
{
    size_t buf_bytes;
    const struct nlmsghdr* nlh;

    //
    // Force to receive data from the kernel, and then parse it
    //
    ns_reader.receive_data(ns, seqno);

    buf_bytes = ns_reader.buffer_size();
    for (nlh = reinterpret_cast<const struct nlmsghdr*>(ns_reader.buffer());
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

#endif // HAVE_NETLINK_SOCKETS
