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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_parse_netlink_socket.cc,v 1.8 2007/06/07 01:28:41 pavlin Exp $"

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

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_get.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "fibconfig_table_get_netlink_socket.hh"


//
// Parse information about routing table information received from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink(7) sockets mechanism).
//
// Reading netlink(3) manual page is a good start for understanding this
//

#ifdef HAVE_NETLINK_SOCKETS

bool
FibConfigTableGetNetlinkSocket::parse_buffer_netlink_socket(
    int family,
    const IfTree& iftree,
    list<FteX>& fte_list,
    const vector<uint8_t>& buffer,
    bool is_nlm_get_only)
{
    size_t buffer_bytes = buffer.size();
    AlignData<struct nlmsghdr> align_data(buffer);
    const struct nlmsghdr* nlh;

    for (nlh = align_data.payload();
	 NLMSG_OK(nlh, buffer_bytes);
	 nlh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(nlh), buffer_bytes)) {
	void* nlmsg_data = NLMSG_DATA(const_cast<struct nlmsghdr*>(nlh));
	
	switch (nlh->nlmsg_type) {
	case NLMSG_ERROR:
	{
	    const struct nlmsgerr* err;
	    
	    err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		XLOG_ERROR("AF_NETLINK nlmsgerr length error");
		break;
	    }
	    errno = -err->error;
	    XLOG_ERROR("AF_NETLINK NLMSG_ERROR message: %s", strerror(errno));
	}
	break;
	
	case NLMSG_DONE:
	{
	    return true;	// XXX: OK even if there were no entries
	}
	break;
	
	case NLMSG_NOOP:
	    break;
	    
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
	case RTM_GETROUTE:
	{
	    if (is_nlm_get_only) {
		//
		// Consider only the "get" entries returned by RTM_GETROUTE.
		// XXX: RTM_NEWROUTE below instead of RTM_GETROUTE is not
		// a mistake, but an artifact of Linux logistics.
		//
		if (nlh->nlmsg_type != RTM_NEWROUTE)
		    break;
	    }

	    const struct rtmsg* rtmsg;
	    int rta_len = RTM_PAYLOAD(nlh);
	    
	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK rtmsg length error");
		break;
	    }
	    rtmsg = reinterpret_cast<const struct rtmsg*>(nlmsg_data);
	    if (rtmsg->rtm_family != family)
		break;
	    if (rtmsg->rtm_flags & RTM_F_CLONED)
		break;		// XXX: ignore cloned entries
	    if (rtmsg->rtm_type == RTN_MULTICAST)
		break;		// XXX: ignore multicast entries
	    if (rtmsg->rtm_type == RTN_BROADCAST)
		break;		// XXX: ignore broadcast entries
	    
	    FteX fte(family);
	    if (NlmUtils::nlm_get_to_fte_cfg(iftree, fte, nlh, rtmsg, rta_len)
		== true) {
		fte_list.push_back(fte);
	    }
	}
	break;
	
	default:
	    debug_msg("Unhandled type %s(%d) (%d bytes)\n",
		      NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
		      nlh->nlmsg_type, nlh->nlmsg_len);
	}
    }
    
    return true;	// XXX: OK even if there were no entries
}

#endif // HAVE_NETLINK_SOCKETS
