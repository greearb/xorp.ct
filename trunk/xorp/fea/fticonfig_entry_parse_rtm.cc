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

#ifndef HAVE_ROUTING_SOCKETS
int
FtiConfigEntryGet::parse_buffer_rtm(FteX& , const uint8_t* , size_t )
{
    return (XORP_ERROR);
}

#else // HAVE_ROUTING_SOCKETS


// Reading route(4) manual page is a good start for understanding this
int
FtiConfigEntryGet::parse_buffer_rtm(FteX& fte, const uint8_t* buf,
				    size_t buf_bytes)
{
    const rt_msghdr* rtm = reinterpret_cast<const rt_msghdr *>(buf);
    const uint8_t* last = buf + buf_bytes;
    
    for (const uint8_t* ptr = buf; ptr < last; ptr += rtm->rtm_msglen) {
    	rtm = reinterpret_cast<const rt_msghdr*>(ptr);
	if (RTM_VERSION != rtm->rtm_version) {
	    XLOG_ERROR("RTM version mismatch: expected %d got %d",
		       RTM_VERSION,
		       rtm->rtm_version);
	    continue;
	}
	
	if (rtm->rtm_type != RTM_GET)
	    continue;
	if (! (rtm->rtm_flags & RTF_UP))
	    continue;
	
	return (RtmUtils::rtm_get_to_fte_cfg(fte, rtm));
    }
    
    return (XORP_ERROR);
}

#endif // HAVE_ROUTING_SOCKETS
