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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_observer_routing_socket.cc,v 1.12 2007/10/12 07:53:48 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"

#include "fea/fibconfig_table_get.hh"
#include "fea/fibconfig_table_observer.hh"

#include "fibconfig_table_get_sysctl.hh"
#include "fibconfig_table_observer_routing_socket.hh"


//
// Observe whole-table information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would NOT specify the particular entry that
// has changed.
//
// The mechanism to observe the information is routing sockets.
//

#ifdef HAVE_ROUTING_SOCKETS

FibConfigTableObserverRoutingSocket::FibConfigTableObserverRoutingSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableObserver(fea_data_plane_manager),
      RoutingSocket(fea_data_plane_manager.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
}

FibConfigTableObserverRoutingSocket::~FibConfigTableObserverRoutingSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the routing sockets mechanism to observe "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableObserverRoutingSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigTableObserverRoutingSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
FibConfigTableObserverRoutingSocket::receive_data(const vector<uint8_t>& buffer)
{
    list<FteX> fte_list;
    FibConfigTableGetSysctl::FibMsgSet filter;
    filter = FibConfigTableGetSysctl::FibMsg::UPDATES | FibConfigTableGetSysctl::FibMsg::GETS | FibConfigTableGetSysctl::FibMsg::RESOLVES;

    //
    // Get the IPv4 routes
    //
    if (fea_data_plane_manager().have_ipv4()) {
	FibConfigTableGetSysctl::parse_buffer_routing_socket(AF_INET,
							     fibconfig().live_config_iftree(),
							     fte_list,
							     buffer,
							     filter);
	if (! fte_list.empty()) {
	    fibconfig().propagate_fib_changes(fte_list, this);
	    fte_list.clear();
	}
    }

#ifdef HAVE_IPV6
    //
    // Get the IPv6 routes
    //
    if (fea_data_plane_manager().have_ipv6()) {
	FibConfigTableGetSysctl::parse_buffer_routing_socket(AF_INET6,
							     fibconfig().live_config_iftree(),
							     fte_list,
							     buffer,
							     filter);
	if (! fte_list.empty()) {
	    fibconfig().propagate_fib_changes(fte_list, this);
	    fte_list.clear();
	}
    }
#endif // HAVE_IPV6
}

void
FibConfigTableObserverRoutingSocket::routing_socket_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}

#endif // HAVE_ROUTING_SOCKETS
