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

#ident "$XORP: xorp/fea/data_plane/managers/fea_data_plane_manager_windows.cc,v 1.1 2007/07/11 22:18:18 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_rras_support.hh"
#endif
#include "fea/data_plane/ifconfig/ifconfig_get_iphelper.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_iphelper.hh"
#include "fea/data_plane/ifconfig/ifconfig_observer_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_forwarding_windows.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_rtmv2.hh"

#include "fea_data_plane_manager_windows.hh"


//
// FEA data plane manager class for Windows FEA.
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerWindows(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerWindows::FeaDataPlaneManagerWindows(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "Windows")
{
}

FeaDataPlaneManagerWindows::~FeaDataPlaneManagerWindows()
{
}

int
FeaDataPlaneManagerWindows::load_plugins(string& error_msg)
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
#if defined(HOST_OS_WINDOWS)
    bool is_rras_running = WinSupport::is_rras_running();
    _ifconfig_get = new IfConfigGetIPHelper(*this);
    _ifconfig_set = new IfConfigSetIPHelper(*this);
    _ifconfig_observer = new IfConfigObserverIPHelper(*this);
    _fibconfig_forwarding = new FibConfigForwardingWindows(*this);
    _fibconfig_entry_get = new FibConfigEntryGetIPHelper(*this);
    // _fibconfig_entry_get = new FibConfigEntryGetRtmV2(*this);
    if (is_rras_running) {
	_fibconfig_entry_set = new FibConfigEntrySetRtmV2(*this);
    } else {
	_fibconfig_entry_set = new FibConfigEntrySetIPHelper(*this);
    }
    _fibconfig_entry_observer = new FibConfigEntryObserverIPHelper(*this);
    // _fibconfig_entry_observer = new FibConfigEntryObserverRtmV2(*this);
    _fibconfig_table_get = new FibConfigTableGetIPHelper(*this);
    _fibconfig_table_set = new FibConfigTableSetIPHelper(*this);
    // _fibconfig_table_set = new FibConfigTableSetRtmV2(*this);
    if (is_rras_running) {
	_fibconfig_table_observer = new FibConfigTableObserverRtmV2(*this);
    } else {
	_fibconfig_table_observer = new FibConfigTableObserverIPHelper(*this);
    }
#endif

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerWindows::register_plugins(string& error_msg)
{
    return (FeaDataPlaneManager::register_all_plugins(true, error_msg));
}
