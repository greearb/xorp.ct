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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_sysctl.cc,v 1.8 2007/06/13 00:15:52 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_get_sysctl.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is sysctl(3).
//

#ifdef HAVE_SYSCTL_NET_RT_IFLIST

IfConfigGetSysctl::IfConfigGetSysctl(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager)
{
}

IfConfigGetSysctl::~IfConfigGetSysctl()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the sysctl(3) mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetSysctl::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetSysctl::stop(string& error_msg)
{
    UNUSED(error_msg);

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

bool
IfConfigGetSysctl::read_config(IfTree& iftree)
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
	vector<uint8_t> buffer(sz);
	
	// Get the data
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &buffer[0], &sz, NULL, 0)
	    == 0) {
	    // Check the new size
	    if (buffer.size() < sz)
		continue;	// XXX: shouldn't happen, but just in case
	    if (buffer.size() > sz)
		buffer.resize(sz);
	    // Parse the result
	    return (parse_buffer_routing_socket(ifconfig(), iftree, buffer));
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
