// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#include <xorp_config.h>
#ifdef HAVE_ROUTING_SOCKETS


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
	if (rtm->rtm_version != RTM_VERSION) {
#if defined(RTM_OVERSION) && defined(HOST_OS_OPENBSD)
	    //
	    // XXX: Silently ignore old messages.
	    // The OpenBSD kernel sends each message twice, once as
	    // RTM_VERSION and once as RTM_OVERSION, hence we need to ignore
	    // the RTM_OVERSION duplicates.
	    //
	    if (rtm->rtm_version == RTM_OVERSION)
		continue;
#endif // RTM_OVERSION && HOST_OS_OPENBSD
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
