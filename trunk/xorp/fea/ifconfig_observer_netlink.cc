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

#ident "$XORP: xorp/fea/ifconfig_observer_netlink.cc,v 1.3 2003/10/14 20:22:13 pavlin Exp $"

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
      NetlinkSocket4(ifc.eventloop()),
      NetlinkSocket6(ifc.eventloop()),
      NetlinkSocketObserver(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ifc();
#endif
}

IfConfigObserverNetlink::~IfConfigObserverNetlink()
{
    stop();
}

int
IfConfigObserverNetlink::start()
{
#ifndef HAVE_NETLINK_SOCKETS
    XLOG_UNREACHABLE();
    return (XORP_ERROR);

#else // HAVE_NETLINK_SOCKETS

    //
    // Listen to the netlink multicast group for network interfaces status
    // and IPv4 addresses.
    //
    NetlinkSocket4::set_nl_groups(RTMGRP_LINK | RTMGRP_IPV4_IFADDR);

    if (NetlinkSocket4::start() < 0)
	return (XORP_ERROR);

#ifdef HAVE_IPV6
    //
    // Listen to the netlink multicast group for network interfaces status
    // and IPv6 addresses.
    //
    NetlinkSocket6::set_nl_groups(RTMGRP_LINK | RTMGRP_IPV6_IFADDR);

    if (NetlinkSocket6::start() < 0) {
	NetlinkSocket4::stop();
	return (XORP_ERROR);
    }
#endif // HAVE_IPV6
    
    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS
}

int
IfConfigObserverNetlink::stop()
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
IfConfigObserverNetlink::receive_data(const uint8_t* data, size_t nbytes)
{
    if (ifc().ifc_get().parse_buffer_nlm(ifc().live_config(), data, nbytes)
	!= true)
	return;
    ifc().report_updates(ifc().live_config(), true);
    ifc().live_config().finalize_state();
}

void
IfConfigObserverNetlink::nlsock_data(const uint8_t* data, size_t nbytes)
{
    receive_data(data, nbytes);
}
