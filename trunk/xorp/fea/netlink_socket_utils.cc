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

#include "libxorp/ipvx.hh"

#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "kernel_utils.hh"
#include "netlink_socket_utils.hh"

//
// NETLINK format related utilities for manipulating data
// (e.g., obtained by netlink sockets mechanism).
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
#define NLM_MSG_ENTRY(X) { X, #X }
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
NlmUtils::get_rta_attr(const rtattr* rtattr, int rta_len,
		       const rtattr* rta_array[])
{
    while (RTA_OK(rtattr, rta_len)) {
	if (rtattr->rta_type <= RTA_MAX)
	    rta_array[rtattr->rta_type] = rtattr;
	rtattr = RTA_NEXT(rtattr, rta_len);
    }
    
    if (rta_len) {
	XLOG_WARNING("get_rta_attr() failed: AF_NETLINK deficit in rtattr: "
		     "%d rta_len remaining",
		     rta_len);
    }
}

int
NlmUtils::nlm_get_to_fte_cfg(FteX& fte, const struct rtmsg* rtmsg, int rta_len)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[RTA_MAX + 1];
    int rta_len, if_index;
    string if_name;
    int family = fte.gateway().af();
    
    // Reset the result
    fte.zero();
    
    IPvX gateway_addr(family);
    IPvX dst_addr(family);
    int dst_masklen = 0;
    
    // rtmsg check
    if (rtmsg->rtm_type == RTN_LOCAL) {
	// TODO: XXX: PAVPAVPAV: handle it, if needed!
	return (XORP_ERROR);	// TODO: is it really an error?
    }
    if (rtmsg->rtm_type != RTN_UNICAST) {
	XLOG_ERROR("nlm_get_to_fte_cfg() failed: "
		   "wrong AF_NETLINK route type: %d instead of %d",
		   rtmsg->rtm_type, RTN_UNICAST);
	return (XORP_ERROR);
    }
    if (rtmsg->rtm_family != family)
	return (XORP_ERROR);		// Invalid address family
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = RTM_RTA(rtmsg);
    NlmUtils::get_rta_attr(rtattr, rta_len, rta_array);

    //
    // Get the destination
    //
    if (rta_array[RTA_DST] == NULL) {
	// The default entry
    } else {
	// TODO: fix this!!
	dst_addr.copy_in(family, (uint8_t *)RTA_DATA(rta_array[RTA_DST]));
	dst_addr = kernel_ipvx_adjust(dst_addr);
	if (! dst_addr.is_unicast()) {
	    // TODO: should we make this check?
	    fte.zero();
	    return (XORP_ERROR);	// XXX: invalid unicast address
	}
    }
    
    //
    // Get the gateway
    //
    if (rta_array[RTA_GATEWAY] != NULL) {
	gateway_addr.copy_in(family,
			     (uint8_t *)RTA_DATA(rta_array[RTA_GATEWAY]));
    }
    
    //
    // Get the destination masklen
    //
    dest_masklen = rtmsg->rtm_dst_len;
    
    //
    // Get the interface index
    //
    if (rta_array[RTA_OIF] != NULL) {
	if_index = *(int *)RTA_DATA(rta_array[RTA_OIF]);
    } else {
	XLOG_ERROR("nlm_get_to_fte_cfg() failed: no interface found");
	return (XORP_ERROR);
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
    // TODO: define default routing metric and admin distance instead of ~0
    //
    fte = FteX(IPvXNet(dst_addr, dst_masklen), gateway_addr, if_name, if_name,
	       ~0, ~0);
    
    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
