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

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about the network interfaces from the underlying system.
//
// The mechanism to obtain the information is sysctl(3).
//


IfConfigGetSysctl::IfConfigGetSysctl(IfConfig& ifc)
    : IfConfigGet(ifc)
{
#ifdef HAVE_SYSCTL_NET_RT_IFLIST
    register_ifc();
#endif
}

IfConfigGetSysctl::~IfConfigGetSysctl()
{
    stop();
}

int
IfConfigGetSysctl::start()
{
    return (XORP_OK);
}

int
IfConfigGetSysctl::stop()
{
    return (XORP_OK);
}

void
IfConfigGetSysctl::receive_data(const uint8_t* data, size_t n_bytes)
{
    if (parse_buffer_rtm(ifc().live_config(), data, n_bytes) < 0)
	return;
    
    debug_msg("Start configuration read:\n");
    debug_msg_indent(4);
    debug_msg("%s\n", ifc().live_config().str().c_str());
    debug_msg_indent(0);
    debug_msg("\nEnd configuration read.\n");
    
    ifc().report_updates(ifc().live_config());
    ifc().live_config().finalize_state();
}

int
IfConfigGetSysctl::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HAVE_SYSCTL_NET_RT_IFLIST

int
IfConfigGetSysctl::read_config(IfTree& )
{
    return XORP_ERROR;
}

#else // HAVE_SYSCTL_NET_RT_IFLIST
int
IfConfigGetSysctl::read_config(IfTree& it)
{
    int mib[6];

    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;			// protocol number - always 0
    mib[3] = AF_UNSPEC;		// Address family: any (XXX: always true?)
    mib[4] = NET_RT_IFLIST;
    mib[5] = 0;			// no flags
    
    // Get the table size
    size_t sz;
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &sz, NULL, 0) != 0) {
	XLOG_ERROR("sysctl(NET_RT_IFLIST) failed: %s", strerror(errno));
	return XORP_ERROR;
    }
    
    //
    // XXX: we need to fetch the data in a loop, because its size
    // may increase between the two sysctl() calls.
    //
    for ( ; ; ) {
	uint8_t ifdata[sz];
	
	// Get the data
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), ifdata, &sz, NULL, 0)
	    == 0) {
	    // Parse the result
	    return (parse_buffer_rtm(it, ifdata, sz));
	}
	
	// Error
	if (errno == ENOMEM) {
	    // Buffer is too small. Try again.
	    continue;
	}
	XLOG_ERROR("sysctl(NET_RT_IFLIST) failed: %s", strerror(errno));
	return XORP_ERROR;
    }
}

#endif // HAVE_SYSCTL_NET_RT_IFLIST
