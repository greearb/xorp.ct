// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_entry_parse_rtm.cc,v 1.7 2004/11/05 01:27:53 bms Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <net/route.h>

#include "fticonfig.hh"
#include "fticonfig_entry_get.hh"
#include "routing_socket_utils.hh"


//
// Parse information about routing entry information received from
// the underlying system.
//
// The information to parse is in RTM format
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//
// Reading route(4) manual page is a good start for understanding this
//

#ifndef HAVE_ROUTING_SOCKETS
bool
FtiConfigEntryGet::parse_buffer_rtm(FteX& , const uint8_t* , size_t ,
				    FtiFibMsgSet)
{
    return false;
}

#else // HAVE_ROUTING_SOCKETS

bool
FtiConfigEntryGet::parse_buffer_rtm(FteX& fte, const uint8_t* buf,
				    size_t buf_bytes, FtiFibMsgSet filter)
{
    const struct rt_msghdr* rtm =
	reinterpret_cast<const struct rt_msghdr*>(buf);
    const uint8_t* last = buf + buf_bytes;

    for (const uint8_t* ptr = buf; ptr < last; ptr += rtm->rtm_msglen) {
	bool filter_match = false;

	rtm = reinterpret_cast<const struct rt_msghdr*>(ptr);
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
	if (filter & FtiFibMsg::GETS) {
	    if ((rtm->rtm_type == RTM_GET) &&
	        (rtm->rtm_flags & RTF_UP))
		filter_match = true;
	}

	// Caller wants route resolves to be parsed.
	// Resolves may not be supported in some implementations.
	if (filter & FtiFibMsg::RESOLVES) {
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
	if (filter & FtiFibMsg::UPDATES) {
	    if ((rtm->rtm_type == RTM_ADD) ||
		(rtm->rtm_type == RTM_DELETE) ||
		(rtm->rtm_type == RTM_CHANGE))
		    filter_match = true;
	}

	if (filter_match)
	    return (RtmUtils::rtm_get_to_fte_cfg(fte, ftic().iftree(), rtm));
    }

    return (false);
}

#endif // HAVE_ROUTING_SOCKETS
