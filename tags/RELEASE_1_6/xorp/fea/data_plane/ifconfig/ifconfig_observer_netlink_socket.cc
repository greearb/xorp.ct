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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_observer_netlink_socket.cc,v 1.18 2008/10/02 21:57:06 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_get_netlink_socket.hh"
#include "ifconfig_observer_netlink_socket.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information is netlink(7) sockets.
//

#ifdef HAVE_NETLINK_SOCKETS

IfConfigObserverNetlinkSocket::IfConfigObserverNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigObserver(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket *)this)
{
}

IfConfigObserverNetlinkSocket::~IfConfigObserverNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to observe "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigObserverNetlinkSocket::start(string& error_msg)
{
    uint32_t nl_groups = 0;

    if (_is_running)
	return (XORP_OK);

    //
    // Listen to the netlink multicast group for network interfaces status
    // and IPv4 addresses.
    //
    if (fea_data_plane_manager().have_ipv4()) 
	nl_groups |= (RTMGRP_LINK | RTMGRP_IPV4_IFADDR);

#ifdef HAVE_IPV6
    //
    // Listen to the netlink multicast group for network interfaces status
    // and IPv6 addresses.
    //
    if (fea_data_plane_manager().have_ipv6())
	nl_groups |= (RTMGRP_LINK | RTMGRP_IPV6_IFADDR);
#endif // HAVE_IPV6

    //
    // Set the netlink multicast groups to listen for on the netlink socket
    //
    NetlinkSocket::set_nl_groups(nl_groups);

    if (NetlinkSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigObserverNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverNetlinkSocket::receive_data(const vector<uint8_t>& buffer)
{
    // Pre-processing cleanup
    ifconfig().system_config().finalize_state();

    if (IfConfigGetNetlinkSocket::parse_buffer_netlink_socket(
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
	if (ifconfig_vlan_get->pull_config(ifconfig().system_config())
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
IfConfigObserverNetlinkSocket::netlink_socket_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}

#endif // HAVE_NETLINK_SOCKETS
