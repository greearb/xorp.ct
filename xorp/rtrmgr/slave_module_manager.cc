// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net




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
