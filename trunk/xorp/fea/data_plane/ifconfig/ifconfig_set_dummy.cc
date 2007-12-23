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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_dummy.cc,v 1.13 2007/12/22 21:23:42 pavlin Exp $"

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
    ifconfig().report_updates(iftree, true);
    ifconfig().set_live_config(iftree);

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
