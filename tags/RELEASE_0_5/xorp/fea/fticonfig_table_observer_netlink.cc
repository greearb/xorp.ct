// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_table_observer_rtsock.cc,v 1.2 2003/05/28 21:50:54 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
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
      NetlinkSocket4(ftic.eventloop()),
      NetlinkSocket6(ftic.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ftic();
#endif
}

FtiConfigTableObserverNetlink::~FtiConfigTableObserverNetlink()
{
    stop();
}

int
FtiConfigTableObserverNetlink::start()
{
#ifndef HAVE_NETLINK_SOCKETS
    XLOG_UNREACHABLE();
    return (XORP_ERROR);

#else // HAVE_NETLINK_SOCKETS

    //
    // Listen to the netlink multicast group for IPv4 routing entries.
    //
    NetlinkSocket4::set_nl_groups(RTMGRP_IPV4_ROUTE);

    if (NetlinkSocket4::start() < 0)
	return (XORP_ERROR);

#ifdef HAVE_IPV6
    //
    // Listen to the netlink multicast group for IPv6 routing entries.
    //
    NetlinkSocket6::set_nl_groups(RTMGRP_IPV6_ROUTE);

    if (NetlinkSocket6::start() < 0) {
	NetlinkSocket4::stop();
	return (XORP_ERROR);
    }
#endif // HAVE_IPV6
    
    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS
}
    
int
FtiConfigTableObserverNetlink::stop()
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;
    
    ret_value4 = NetlinkSocket4::stop();
    
#ifdef HAVE_IPV6
    ret_value6 = NetlinkSocket6::stop();
#endif
    
    if ((ret_value4 < 0) || (ret_value6 < 0))
	return (XORP_ERROR);
    
    return (XORP_OK);
}

void
FtiConfigTableObserverNetlink::receive_data(const uint8_t* data, size_t nbytes)
{
    // TODO: XXX: PAVPAVPAV: use it?
    UNUSED(data);
    UNUSED(nbytes);
}

void
FtiConfigTableObserverNetlink::nlsock_data(const uint8_t* data, size_t nbytes)
{
    receive_data(data, nbytes);
}
