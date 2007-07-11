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

#ident "$XORP$"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/data_plane/ifconfig/ifconfig_get_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_observer_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_dummy.hh"

#include "fea_data_plane_manager_dummy.hh"


//
// FEA data plane manager class for Dummy FEA.
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerDummy(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerDummy::FeaDataPlaneManagerDummy(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "Dummy"),
      _ifconfig_get_dummy(NULL),
      _ifconfig_set_dummy(NULL),
      _ifconfig_observer_dummy(NULL),
      _fibconfig_entry_get_dummy(NULL),
      _fibconfig_entry_set_dummy(NULL),
      _fibconfig_entry_observer_dummy(NULL),
      _fibconfig_table_get_dummy(NULL),
      _fibconfig_table_set_dummy(NULL),
      _fibconfig_table_observer_dummy(NULL)
{
}

FeaDataPlaneManagerDummy::~FeaDataPlaneManagerDummy()
{
}

int
FeaDataPlaneManagerDummy::load_plugins(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_get_dummy == NULL);
    XLOG_ASSERT(_ifconfig_set_dummy == NULL);
    XLOG_ASSERT(_ifconfig_observer_dummy == NULL);
    XLOG_ASSERT(_fibconfig_entry_get_dummy == NULL);
    XLOG_ASSERT(_fibconfig_entry_set_dummy == NULL);
    XLOG_ASSERT(_fibconfig_entry_observer_dummy == NULL);
    XLOG_ASSERT(_fibconfig_table_get_dummy == NULL);
    XLOG_ASSERT(_fibconfig_table_set_dummy == NULL);
    XLOG_ASSERT(_fibconfig_table_observer_dummy == NULL);

    //
    // Load the plugins
    //
    _ifconfig_get_dummy = new IfConfigGetDummy(*this);
    _ifconfig_set_dummy = new IfConfigSetDummy(*this);
    _ifconfig_observer_dummy = new IfConfigObserverDummy(*this);
    _fibconfig_entry_get_dummy = new FibConfigEntryGetDummy(*this);
    _fibconfig_entry_set_dummy = new FibConfigEntrySetDummy(*this);
    _fibconfig_entry_observer_dummy = new FibConfigEntryObserverDummy(*this);
    _fibconfig_table_get_dummy = new FibConfigTableGetDummy(*this);
    _fibconfig_table_set_dummy = new FibConfigTableSetDummy(*this);
    _fibconfig_table_observer_dummy = new FibConfigTableObserverDummy(*this);
    _ifconfig_get = _ifconfig_get_dummy;
    _ifconfig_set = _ifconfig_set_dummy;
    _ifconfig_observer = _ifconfig_observer_dummy;
    _fibconfig_entry_get = _fibconfig_entry_get_dummy;
    _fibconfig_entry_set = _fibconfig_entry_set_dummy;
    _fibconfig_entry_observer = _fibconfig_entry_observer_dummy;
    _fibconfig_table_get = _fibconfig_table_get_dummy;
    _fibconfig_table_set = _fibconfig_table_set_dummy;
    _fibconfig_table_observer = _fibconfig_table_observer_dummy;

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerDummy::unload_plugins(string& error_msg)
{
    if (! _is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_get_dummy != NULL);
    XLOG_ASSERT(_ifconfig_set_dummy != NULL);
    XLOG_ASSERT(_ifconfig_observer_dummy != NULL);
    XLOG_ASSERT(_fibconfig_entry_get_dummy != NULL);
    XLOG_ASSERT(_fibconfig_entry_set_dummy != NULL);
    XLOG_ASSERT(_fibconfig_entry_observer_dummy != NULL);
    XLOG_ASSERT(_fibconfig_table_get_dummy != NULL);
    XLOG_ASSERT(_fibconfig_table_set_dummy != NULL);
    XLOG_ASSERT(_fibconfig_table_observer_dummy != NULL);

    //
    // Unload the plugins
    //
    if (FeaDataPlaneManager::unload_plugins(error_msg) != XORP_OK)
	return (XORP_ERROR);

    //
    // Reset the state
    //
    _ifconfig_get_dummy = NULL;
    _ifconfig_set_dummy = NULL;
    _ifconfig_observer_dummy = NULL;
    _fibconfig_entry_get_dummy = NULL;
    _fibconfig_entry_set_dummy = NULL;
    _fibconfig_entry_observer_dummy = NULL;
    _fibconfig_table_get_dummy = NULL;
    _fibconfig_table_set_dummy = NULL;
    _fibconfig_table_observer_dummy = NULL;

    return (XORP_OK);
}

int
FeaDataPlaneManagerDummy::register_plugins(string& error_msg)
{
    return (FeaDataPlaneManager::register_all_plugins(true, error_msg));
}
