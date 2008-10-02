// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/generic_module_manager.cc,v 1.11 2008/07/23 05:11:41 pavlin Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "generic_module_manager.hh"

GenericModule::GenericModule(const string& name)
    : _name(name),
      _status(MODULE_NOT_STARTED)
{

}

GenericModule::~GenericModule()
{

}

void
GenericModule::new_status(ModuleStatus new_status)
{
    _status = new_status;
}

string
GenericModule::str() const
{
    string s;

    s = c_format("Module %s\n", _name.c_str());
    if (_status != MODULE_NOT_STARTED && _status != MODULE_FAILED)
	s += c_format("Module is running\n");
    else
	s += "Module is not running\n";
    return s;
}



//
// XXX: this file merely provides dummy definitions for the module manager
// for use in xorpsh, as xorpsh never actually starts any modules, but
// the template commands need to know about a module manager.
//


GenericModuleManager::GenericModuleManager(EventLoop& eventloop, bool verbose) 
    : _eventloop(eventloop),
      _verbose(verbose)
{
}

GenericModuleManager::~GenericModuleManager()
{
    while (! _modules.empty()) {
	GenericModule* module = _modules.begin()->second;
	delete module;
	_modules.erase(_modules.begin());
    }
}

bool
GenericModuleManager::store_new_module(GenericModule *module,
				       string& error_msg)
{
    map<string, GenericModule *>::iterator iter;

    iter = _modules.find(module->name());
    if (iter == _modules.end()) {
	_modules[module->name()] = module;
	return true;
    } else {
	error_msg = c_format("Module %s already exists",
			     module->name().c_str());
	XLOG_TRACE(_verbose, "%s", error_msg.c_str());
	return false;
    }
}

GenericModule*
GenericModuleManager::find_module(const string& module_name)
{
    map<string, GenericModule *>::iterator iter;

    iter = _modules.find(module_name);
    if (iter == _modules.end()) {
	debug_msg("GenericModuleManager: Failed to find module %s\n",
		  module_name.c_str());
	return NULL;
    } else {
	debug_msg("GenericModuleManager: Found module %s\n",
		  module_name.c_str());
	return iter->second;
    }
}

const GenericModule*
GenericModuleManager::const_find_module(const string& module_name) const
{
    map<string, GenericModule *>::const_iterator iter;

    iter = _modules.find(module_name);
    if (iter == _modules.end()) {
	return NULL;
    } else {
	return iter->second;
    }
}

bool
GenericModuleManager::module_exists(const string& module_name) const
{
    return _modules.find(module_name) != _modules.end();
}

GenericModule::ModuleStatus 
GenericModuleManager::module_status(const string& module_name) const
{
    const GenericModule* module = const_find_module(module_name);
    if (module != NULL) {
	return module->status();
    } else {
	return GenericModule::NO_SUCH_MODULE;
    }
}
