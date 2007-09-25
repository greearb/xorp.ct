// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_vlan_set_dummy.cc,v 1.3 2007/09/15 01:22:36 pavlin Exp $"

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
IfConfigVlanSetDummy::config_vlan(const IfTreeInterface* pulled_ifp,
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
