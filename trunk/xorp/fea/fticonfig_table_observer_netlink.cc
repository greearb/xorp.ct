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

#ident "$XORP: xorp/fea/fticonfig_table_observer_netlink.cc,v 1.15 2006/08/29 22:42:19 pavlin Exp $"

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

#include "fticonfig.hh"
#include "fticonfig_table_observer.hh"


//
// Observe whole-table information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would NOT specify the particular entry that
// has changed.
//
// The mechanism to observe the information is netlink(7) sockets.
//


FtiConfigTableObserverNetlink::FtiConfigTableObserverNetlink(FtiConfig& ftic)
    : FtiConfigTableObserver(ftic),
      NetlinkSocket(ftic.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ftic_primary();
#endif
}

FtiConfigTableObserverNetlink::~FtiConfigTableObserverNetlink()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to observe "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigTableObserverNetlink::start(string& error_msg)
{
#ifndef HAVE_NETLINK_SOCKETS
    error_msg = c_format("The netlink(7) mechanism to observe "
			 "whole forwarding table from the "
			 "underlying system is not supported");
    XLOG_UNREACHABLE();
    return (XORP_ERROR);

#else // HAVE_NETLINK_SOCKETS

    uint32_t nl_groups = 0;

    if (_is_running)
	return (XORP_OK);

    //
    // Listen to the netlink multicast group for IPv4 routing entries
    //
    if (ftic().have_ipv4())
	nl_groups |= RTMGRP_IPV4_ROUTE;

#ifdef HAVE_IPV6
    //
    // Listen to the netlink multicast group for IPv6 routing entries
    //
    if (ftic().have_ipv6())
	nl_groups |= RTMGRP_IPV6_ROUTE;
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
FtiConfigTableObserverNetlink::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
FtiConfigTableObserverNetlink::receive_data(const vector<uint8_t>& buffer)
{
    list<FteX> fte_list;

    //
    // Get the IPv4 routes
    //
    if (ftic().have_ipv4()) {
	ftic().ftic_table_get_primary().parse_buffer_nlm(AF_INET, fte_list,
							 buffer, false);
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
	ftic().ftic_table_get_primary().parse_buffer_nlm(AF_INET6, fte_list,
							 buffer, false);
	if (! fte_list.empty()) {
	    propagate_fib_changes(fte_list);
	    fte_list.clear();
	}
    }
#endif // HAVE_IPV6
}

void
FtiConfigTableObserverNetlink::nlsock_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
