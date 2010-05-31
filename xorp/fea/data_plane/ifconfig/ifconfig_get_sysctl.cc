// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

int
IfConfigGetSysctl::pull_config(const IfTree* local_config, IfTree& iftree)
{
    UNUSED(local_config);
    return read_config(iftree);
}

int
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
	return (XORP_ERROR);
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
	    if (parse_buffer_routing_socket(ifconfig(), iftree, buffer)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	    //
	    // Get the VLAN vif info
	    //
	    IfConfigVlanGet* ifconfig_vlan_get;
	    ifconfig_vlan_get = fea_data_plane_manager().ifconfig_vlan_get();
	    if (ifconfig_vlan_get != NULL) {
		bool modified = false;
		if (ifconfig_vlan_get->pull_config(iftree, modified) != XORP_OK)
		    return (XORP_ERROR);
	    }
	    return (XORP_OK);
	}
	
	// Error
	if (errno == ENOMEM) {
	    // Buffer is too small. Try again.
	    continue;
	}
	XLOG_ERROR("sysctl(NET_RT_IFLIST) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
}

#endif // HAVE_SYSCTL_NET_RT_IFLIST
