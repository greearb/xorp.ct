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

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_get_getifaddrs.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is getifaddrs(3).
//

#ifdef HAVE_GETIFADDRS
IfConfigGetGetifaddrs::IfConfigGetGetifaddrs(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager)
{
}

IfConfigGetGetifaddrs::~IfConfigGetGetifaddrs()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the getifaddrs(3) mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetGetifaddrs::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetGetifaddrs::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigGetGetifaddrs::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

int
IfConfigGetGetifaddrs::read_config(IfTree& iftree)
{
    struct ifaddrs *ifap;
    
    if (getifaddrs(&ifap) != 0) {
	XLOG_ERROR("getifaddrs() failed: %s", strerror(errno));
	return (XORP_ERROR);
    }

    parse_buffer_getifaddrs(ifconfig(), iftree, ifap);
    freeifaddrs(ifap);

    //
    // Get the VLAN vif info
    //
    IfConfigVlanGet* ifconfig_vlan_get;
    ifconfig_vlan_get = fea_data_plane_manager().ifconfig_vlan_get();
    if (ifconfig_vlan_get != NULL) {
	if (ifconfig_vlan_get->pull_config(iftree) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_GETIFADDRS
