// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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
#include "libxorp/ether_compat.h"

#include "libcomm/comm_api.h"

#include "fea/ifconfig.hh"

#include "ifconfig_vlan_set_dummy.hh"


//
// Set VLAN information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is Dummy (for testing purpose).
//

IfConfigVlanSetDummy::IfConfigVlanSetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigVlanSet(fea_data_plane_manager)
{
}

IfConfigVlanSetDummy::~IfConfigVlanSetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to set "
		   "information about VLAN network interfaces into the "
		   "underlying system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigVlanSetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigVlanSetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigVlanSetDummy::config_add_vlan(const IfTreeInterface* pulled_ifp,
				      const IfTreeVif* pulled_vifp,
				      const IfTreeInterface& config_iface,
				      const IfTreeVif& config_vif,
				      string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigVlanSetDummy::config_delete_vlan(const IfTreeInterface* pulled_ifp,
					 const IfTreeVif* pulled_vifp,
					 const IfTreeInterface& config_iface,
					 const IfTreeVif& config_vif,
					 string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(error_msg);

    return (XORP_OK);
}
