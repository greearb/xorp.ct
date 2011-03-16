// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#include <xorp_config.h>
#ifdef HAVE_ROUTING_SOCKETS


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"

#include "ifconfig_get_sysctl.hh"
#include "ifconfig_observer_routing_socket.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information is routing sockets.
//

IfConfigObserverRoutingSocket::IfConfigObserverRoutingSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigObserver(fea_data_plane_manager),
      RoutingSocket(fea_data_plane_manager.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
}

IfConfigObserverRoutingSocket::~IfConfigObserverRoutingSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the routing sockets mechanism to observe "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigObserverRoutingSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigObserverRoutingSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverRoutingSocket::receive_data(const vector<uint8_t>& buffer)
{
    // Pre-processing cleanup
    ifconfig().system_config().finalize_state();

    if (IfConfigGetSysctl::parse_buffer_routing_socket(
	    ifconfig(), ifconfig().system_config(), buffer)
	!= XORP_OK) {
	return;
    }

    //
    // Get the VLAN vif info
    //
    IfConfigVlanGet* ifconfig_vlan_get;
    ifconfig_vlan_get = fea_data_plane_manager().ifconfig_vlan_get();
    if (ifconfig_vlan_get != NULL) {
	bool modified = false; // Ignore this...assume it's always modified.
	if (ifconfig_vlan_get->pull_config(ifconfig().system_config(), modified)
	    != XORP_OK) {
	    XLOG_ERROR("Unknown error while pulling VLAN information");
	}
    }

    //
    // Propagate the changes from the system config to the merged config
    //
    IfTree& merged_config = ifconfig().merged_config();
    merged_config.align_with_observed_changes(ifconfig().system_config(),
					      ifconfig().user_config());
    ifconfig().report_updates(merged_config);
    merged_config.finalize_state();
}

void
IfConfigObserverRoutingSocket::routing_socket_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}

#endif // HAVE_ROUTING_SOCKETS
