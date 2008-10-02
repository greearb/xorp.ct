// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_dummy.cc,v 1.19 2008/07/23 05:10:29 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#include "fea/ifconfig.hh"

#include "ifconfig_set_dummy.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is Dummy (for testing purpose).
//

IfConfigSetDummy::IfConfigSetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigSet(fea_data_plane_manager)
{
}

IfConfigSetDummy::~IfConfigSetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigSetDummy::push_config(IfTree& iftree)
{
    _iftree = iftree;

    return (XORP_OK);
}

bool
IfConfigSetDummy::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);	// TODO: return appropriate value.
}

bool
IfConfigSetDummy::is_unreachable_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);	// TODO: return appropriate value.
}

int
IfConfigSetDummy::config_begin(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_end(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_interface_begin(const IfTreeInterface* pulled_ifp,
					 IfTreeInterface& config_iface,
					 string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(config_iface);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_interface_end(const IfTreeInterface* pulled_ifp,
				       const IfTreeInterface& config_iface,
				       string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(config_iface);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_vif_begin(const IfTreeInterface* pulled_ifp,
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
IfConfigSetDummy::config_vif_end(const IfTreeInterface* pulled_ifp,
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
IfConfigSetDummy::config_add_address(const IfTreeInterface* pulled_ifp,
				     const IfTreeVif* pulled_vifp,
				     const IfTreeAddr4* pulled_addrp,
				     const IfTreeInterface& config_iface,
				     const IfTreeVif& config_vif,
				     const IfTreeAddr4& config_addr,
				     string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(config_addr);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_delete_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr4* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr4& config_addr,
					string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(config_addr);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_add_address(const IfTreeInterface* pulled_ifp,
				     const IfTreeVif* pulled_vifp,
				     const IfTreeAddr6* pulled_addrp,
				     const IfTreeInterface& config_iface,
				     const IfTreeVif& config_vif,
				     const IfTreeAddr6& config_addr,
				     string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(config_addr);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_delete_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr6* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr6& config_addr,
					string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(config_addr);
    UNUSED(error_msg);

    return (XORP_OK);
}
