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

#ident "$XORP: xorp/fea/fticonfig_table_observer_rtsock.cc,v 1.17 2006/08/29 22:42:20 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"

#include "fticonfig_table_get.hh"
#include "fticonfig_table_observer.hh"

//
// Observe whole-table information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would NOT specify the particular entry that
// has changed.
//
// The mechanism to observe the information is routing sockets.
//


FtiConfigTableObserverRtsock::FtiConfigTableObserverRtsock(FtiConfig& ftic)
    : FtiConfigTableObserver(ftic),
      RoutingSocket(ftic.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic_primary();
#endif
}

FtiConfigTableObserverRtsock::~FtiConfigTableObserverRtsock()
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
FtiConfigTableObserverRtsock::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}
    
int
FtiConfigTableObserverRtsock::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
FtiConfigTableObserverRtsock::receive_data(const vector<uint8_t>& buffer)
{
    using namespace FtiFibMsg;
    list<FteX> fte_list;

    if (_fib_table_observers.empty())
	return;		// Nobody is interested in the routes

    //
    // Get the IPv4 routes
    //
    if (ftic().have_ipv4()) {
	ftic().ftic_table_get_primary().parse_buffer_rtm(AF_INET, fte_list,
				buffer,
				UPDATES | GETS | RESOLVES);
	if (! fte_list.empty()) {
	    propagate_fib_changes(fte_list);
	    fte_list.clear();
	}
    }

#ifdef HAVE_IPV6
    //
    // Get the IPv6 routes
    //
    if (ftic().have_ipv6()) {
	ftic().ftic_table_get_primary().parse_buffer_rtm(AF_INET6, fte_list,
				buffer,
				UPDATES | GETS | RESOLVES);
	if (! fte_list.empty()) {
	    propagate_fib_changes(fte_list);
	    fte_list.clear();
	}
    }
#endif // HAVE_IPV6
}

void
FtiConfigTableObserverRtsock::rtsock_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
