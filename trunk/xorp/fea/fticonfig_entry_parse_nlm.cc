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

#ident "$XORP: xorp/fea/fticonfig_entry_parse_nlm.cc,v 1.1 2003/05/02 07:50:44 pavlin Exp $"


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
#include "fticonfig_entry_get.hh"
#include "netlink_socket_utils.hh"


//
// Parse information about routing entry information received from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink sockets mechanism).
//

#ifndef HAVE_NETLINK_SOCKETS
bool
FtiConfigEntryGet::parse_buffer_nlm(FteX& , const uint8_t* , size_t )
{
    return false;
}

#else // HAVE_NETLINK_SOCKETS


// Reading netlink(3) manual page is a good start for understanding this
bool
FtiConfigEntryGet::parse_buffer_nlm(FteX& fte, const uint8_t* buf,
				    size_t buf_bytes)
{
    const nlmsghdr* nlh = reinterpret_cast<const nlmsghdr *>(buf);
    const uint8_t* last = buf + buf_bytes;
    
    for (const uint8_t* ptr = buf; ptr < last; ptr += nlh->nlmsg_len) {
    	nlh = reinterpret_cast<const nlmsghdr*>(ptr);
	const struct rtmsg *rtmsg;
	int rta_len;
	
	// Message type check
	if (nlh->nlmsg_type == NLMSG_ERROR) {
	    const struct nlmsgerr *err = (const struct nlmsgerr*)NLMSG_DATA(const_cast<nlmsghdr *>(nlh));
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		XLOG_ERROR("AF_NETLINK ERROR message truncated");
	    } else {
		errno = -err->error;
		XLOG_ERROR("AF_NETLINK ERROR message: %s", strerror(errno));
	    }
	    continue;
	}
	if (nlh->nlmsg_type != RTM_NEWROUTE)
	    continue;
	
	rtmsg = (const struct rtmsg *)NLMSG_DATA(const_cast<nlmsghdr *>(nlh));
	rta_len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*rtmsg));
	
	return (NlmUtils::nlm_get_to_fte_cfg(fte, rtmsg, rta_len));
    }
    
    return false;
}

#endif // HAVE_NETLINK_SOCKETS
