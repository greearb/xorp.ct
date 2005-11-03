// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/generic_module_manager.cc,v 1.5 2005/03/25 02:54:35 pavlin Exp $"

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
    XLOG_ASSERT(_status == MODULE_NOT_STARTED);
}

void
GenericModule::new_status(ModuleStatus new_status)
{
    _status = new_status;
}

string
GenericModule::str() const
{
    string s = "Module " + _name + "\n";

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
    : _eventloop(eventloop), _verbose(verbose)
{
}

bool
GenericModuleManager::store_new_module(GenericModule *module,
				       string& error_msg)
{
    map<string, GenericModule*>::iterator found_mod;
    found_mod = _modules.find(module->name());
    if (found_mod == _modules.end()) {
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
    map<string, GenericModule*>::iterator found;

    found = _modules.find(module_name);
    if (found == _modules.end()) {
	debug_msg("GenericModuleManager: Failed to find module %s\n",
		  module_name.c_str());
	return NULL;
    } else {
	debug_msg("GenericModuleManager: Found module %s\n", module_name.c_str());
	return found->second;
    }
}

const GenericModule*
GenericModuleManager::const_find_module(const string& module_name) const
{
    map<string, GenericModule*>::const_iterator found;

    found = _modules.find(module_name);
    if (found == _modules.end()) {
	return NULL;
    } else {
	return found->second;
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
    const GenericModule *m = const_find_module(module_name);
    if (m) {
	return m->status();
    } else {
	return GenericModule::NO_SUCH_MODULE;
    }
}

