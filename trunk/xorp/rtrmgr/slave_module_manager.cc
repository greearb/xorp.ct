// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/slave_module_manager.cc,v 1.24 2008/01/04 03:17:43 pavlin Exp $"


#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "slave_module_manager.hh"

SlaveModuleManager::SlaveModuleManager(EventLoop& eventloop) 
    : GenericModuleManager(eventloop, false)
{
}

GenericModule*
SlaveModuleManager::new_module(const string& module_name, string& error_msg)
{
    debug_msg("SlaveModuleManager::new_module %s\n", module_name.c_str());
    GenericModule* module = new GenericModule(module_name);
    if (store_new_module(module, error_msg) != true) {
	delete module;
	return NULL;
    }
    return module;
}

bool
SlaveModuleManager::module_is_active(const string& module_name) const
{
    const GenericModule *module = const_find_module(module_name);
    if (module == NULL)
	return false;
    debug_msg("module_is_active: %s %d\n", 
	      module_name.c_str(), module->status());
    switch (module->status()) {
    case GenericModule::MODULE_STARTUP:
    case GenericModule::MODULE_INITIALIZING:
    case GenericModule::MODULE_RUNNING:
	return true;
    case GenericModule::MODULE_FAILED:
    case GenericModule::MODULE_STALLED:
    case GenericModule::MODULE_SHUTTING_DOWN:
    case GenericModule::MODULE_NOT_STARTED:
    case GenericModule::NO_SUCH_MODULE:
	return false;
    }
    //keep stupid compiler happy
    return true;
}
