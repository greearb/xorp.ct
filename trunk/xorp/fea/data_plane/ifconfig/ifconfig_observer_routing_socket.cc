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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_observer_routing_socket.cc,v 1.6 2007/04/30 23:40:34 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"
#include "ifconfig_observer_routing_socket.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information is routing sockets.
//


IfConfigObserverRoutingSocket::IfConfigObserverRoutingSocket(IfConfig& ifconfig)
    : IfConfigObserver(ifconfig),
      RoutingSocket(ifconfig.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
#ifdef HAVE_ROUTING_SOCKETS
    ifconfig.register_ifconfig_observer_primary(this);
#endif
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

    if (RoutingSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigObserverRoutingSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverRoutingSocket::receive_data(const vector<uint8_t>& buffer)
{
    if (IfConfigGetSysctl::parse_buffer_routing_socket(
	    ifconfig(), ifconfig().live_config(), buffer)
	!= true) {
	return;
    }

    ifconfig().report_updates(ifconfig().live_config(), true);

    // Propagate the changes from the live config to the local config
    IfTree& local_config = ifconfig().local_config();
    local_config.track_live_config_state(ifconfig().live_config());
    ifconfig().report_updates(local_config, false);
    local_config.finalize_state();
    ifconfig().live_config().finalize_state();
}

void
IfConfigObserverRoutingSocket::rtsock_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
