// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_parse_routing_socket.cc,v 1.13 2008/01/04 03:15:59 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_routing_socket.h"
#endif

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_get.hh"
#include "fea/data_plane/control_socket/routing_socket_utilities.hh"

#include "fibconfig_entry_get_routing_socket.hh"


//
// Parse information about forwarding entry information received from
// the underlying system.
//
// The information to parse is in RTM format
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//
// Reading route(4) manual page is a good start for understanding this
//

#ifdef HAVE_ROUTING_SOCKETS

int
FibConfigEntryGetRoutingSocket::parse_buffer_routing_socket(
    const IfTree& iftree,
    FteX& fte,
    const vector<uint8_t>& buffer,
    FibMsgSet filter)
{
    AlignData<struct rt_msghdr> align_data(buffer);
    const struct rt_msghdr* rtm;
    size_t offset;

    rtm = align_data.payload();
    for (offset = 0; offset < buffer.size(); offset += rtm->rtm_msglen) {
	bool filter_match = false;

	rtm = align_data.payload_by_offset(offset);
	if (RTM_VERSION != rtm->rtm_version) {
	    XLOG_ERROR("RTM version mismatch: expected %d got %d",
		       RTM_VERSION,
		       rtm->rtm_version);
	    continue;
	}

	// XXX: ignore entries with an error
	if (rtm->rtm_errno != 0)
	    continue;

	// Caller wants route gets to be parsed.
	if (filter & FibMsg::GETS) {
#ifdef RTM_GET
	    if ((rtm->rtm_type == RTM_GET) &&
	        (rtm->rtm_flags & RTF_UP))
		filter_match = true;
#endif
	}

	// Caller wants route resolves to be parsed.
	// Resolves may not be supported in some implementations.
	if (filter & FibMsg::RESOLVES) {
#ifdef RTM_MISS
	    if (rtm->rtm_type == RTM_MISS)
		filter_match = true;
#endif
#ifdef RTM_RESOLVE
	    if (rtm->rtm_type == RTM_RESOLVE)
		filter_match = true;
#endif
	}

	// Caller wants routing table updates to be parsed.
	if (filter & FibMsg::UPDATES) {
	    if ((rtm->rtm_type == RTM_ADD) ||
		(rtm->rtm_type == RTM_DELETE) ||
		(rtm->rtm_type == RTM_CHANGE))
		    filter_match = true;
	}

	if (filter_match)
	    return (RtmUtils::rtm_get_to_fte_cfg(iftree, fte, rtm));
    }

    return (XORP_ERROR);
}

#endif // HAVE_ROUTING_SOCKETS
