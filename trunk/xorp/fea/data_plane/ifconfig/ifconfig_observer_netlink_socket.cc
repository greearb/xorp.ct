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

#ident "$XORP: xorp/fea/ifconfig_observer_netlink.cc,v 1.19 2007/04/18 06:20:57 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_observer.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information is netlink(7) sockets.
//


IfConfigObserverNetlink::IfConfigObserverNetlink(IfConfig& ifc)
    : IfConfigObserver(ifc),
      NetlinkSocket(ifc.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ifc_primary();
#endif
}

IfConfigObserverNetlink::~IfConfigObserverNetlink()
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
IfConfigObserverNetlink::start(string& error_msg)
{
#ifndef HAVE_NETLINK_SOCKETS
    error_msg = c_format("The netlink(7) sockets mechanism to observe "
			 "information about network interfaces from the "
			 "underlying system is not supported");
    XLOG_UNREACHABLE();
    return (XORP_ERROR);

#else // HAVE_NETLINK_SOCKETS

    uint32_t nl_groups = 0;

    if (_is_running)
	return (XORP_OK);

    //
    // Listen to the netlink multicast group for network interfaces status
    // and IPv4 addresses.
    //
    if (ifc().have_ipv4()) 
	nl_groups |= (RTMGRP_LINK | RTMGRP_IPV4_IFADDR);

#ifdef HAVE_IPV6
    //
    // Listen to the netlink multicast group for network interfaces status
    // and IPv6 addresses.
    //
    if (ifc().have_ipv6())
	nl_groups |= (RTMGRP_LINK | RTMGRP_IPV6_IFADDR);
#endif // HAVE_IPV6

    //
    // Set the netlink multicast groups to listen for on the netlink socket
    //
    NetlinkSocket::set_nl_groups(nl_groups);

    if (NetlinkSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS
}

int
IfConfigObserverNetlink::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverNetlink::receive_data(const vector<uint8_t>& buffer)
{
    if (ifc().ifc_get_primary().parse_buffer_nlm(ifc().live_config(), buffer)
	!= true) {
	return;
    }

    ifc().report_updates(ifc().live_config(), true);

    // Propagate the changes from the live config to the local config
    IfTree& local_config = ifc().local_config();
    local_config.track_live_config_state(ifc().live_config());
    ifc().report_updates(local_config, false);
    local_config.finalize_state();
    ifc().live_config().finalize_state();
}

void
IfConfigObserverNetlink::nlsock_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
