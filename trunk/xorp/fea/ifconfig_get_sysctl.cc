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

#ident "$XORP: xorp/fea/ifconfig_get_sysctl.cc,v 1.5 2004/06/02 22:52:39 pavlin Exp $"


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
    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetSysctl::stop()
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigGetSysctl::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HAVE_SYSCTL_NET_RT_IFLIST

bool
IfConfigGetSysctl::read_config(IfTree& )
{
    return false;
}

#else // HAVE_SYSCTL_NET_RT_IFLIST
bool
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
	return false;
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
	return false;
    }
}

#endif // HAVE_SYSCTL_NET_RT_IFLIST
