// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/managers/fea_data_plane_manager_bsd.cc,v 1.1 2007/07/11 22:18:17 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/data_plane/ifconfig/ifconfig_get_getifaddrs.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_ioctl.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_sysctl.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_ioctl.hh"
#include "fea/data_plane/ifconfig/ifconfig_observer_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_forwarding_sysctl.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_sysctl.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_routing_socket.hh"

#include "fea_data_plane_manager_bsd.hh"


//
// FEA data plane manager class for BSD FEA.
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerBsd(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerBsd::FeaDataPlaneManagerBsd(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "BSD")
{
}

FeaDataPlaneManagerBsd::~FeaDataPlaneManagerBsd()
{
}

int
FeaDataPlaneManagerBsd::load_plugins(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_get == NULL);
    XLOG_ASSERT(_ifconfig_set == NULL);
    XLOG_ASSERT(_ifconfig_observer == NULL);
    XLOG_ASSERT(_fibconfig_forwarding == NULL);
    XLOG_ASSERT(_fibconfig_entry_get == NULL);
    XLOG_ASSERT(_fibconfig_entry_set == NULL);
    XLOG_ASSERT(_fibconfig_entry_observer == NULL);
    XLOG_ASSERT(_fibconfig_table_get == NULL);
    XLOG_ASSERT(_fibconfig_table_set == NULL);
    XLOG_ASSERT(_fibconfig_table_observer == NULL);

    //
    // Load the plugins
    //
#if defined(HAVE_GETIFADDRS)
    _ifconfig_get = new IfConfigGetGetifaddrs(*this);
#elif defined(HAVE_SYSCTL_NET_RT_IFLIST)
    _ifconfig_get = new IfConfigGetSysctl(*this);
#elif defined(HAVE_IOCTL_SIOCGIFCONF)
    _ifconfig_get = new IfConfigGetIoctl(*this);
#endif

#if defined(HAVE_IOCTL_SIOCGIFCONF)
    _ifconfig_set = new IfConfigSetIoctl(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _ifconfig_observer = new IfConfigObserverRoutingSocket(*this);
#endif

#if defined(HAVE_SYSCTL_IPCTL_FORWARDING) || defined(HAVE_SYSCTL_IPV6CTL_FORWARDING) || defined(HAVE_SYSCTL_IPV6CTL_ACCEPT_RTADV)
    _fibconfig_forwarding = new FibConfigForwardingSysctl(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_entry_get = new FibConfigEntryGetRoutingSocket(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_entry_set = new FibConfigEntrySetRoutingSocket(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_entry_observer = new FibConfigEntryObserverRoutingSocket(*this);
#endif

#if defined(HAVE_SYSCTL_NET_RT_DUMP)
    _fibconfig_table_get = new FibConfigTableGetSysctl(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_table_set = new FibConfigTableSetRoutingSocket(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_table_observer = new FibConfigTableObserverRoutingSocket(*this);
#endif

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerBsd::register_plugins(string& error_msg)
{
    return (FeaDataPlaneManager::register_all_plugins(true, error_msg));
}
