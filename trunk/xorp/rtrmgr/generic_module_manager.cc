// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.34 2004/08/19 02:00:21 pavlin Exp $"

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
    if (new_status == _status)
	return;

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


GenericModuleManager::GenericModuleManager(EventLoop& eventloop) 
    : _eventloop(eventloop)
{
}

bool GenericModuleManager::new_module(const string& module_name, const string& path) 
{
    UNUSED(module_name);
    UNUSED(path);
    return true;
}

int 
GenericModuleManager::start_module(const string& module_name, bool do_exec, 
			  XorpCallback1<void, bool>::RefPtr cb)
{
    UNUSED(module_name);
    UNUSED(do_exec);
    UNUSED(cb);
    return XORP_OK;
}

int 
GenericModuleManager::kill_module(const string& module_name,
			   XorpCallback0<void>::RefPtr cb)
{
    UNUSED(module_name);
    UNUSED(cb);
    return XORP_OK;
}

bool 
GenericModuleManager::module_exists(const string& module_name) const
{
    UNUSED(module_name);
    return false;
}

bool 
GenericModuleManager::module_has_started(const string& module_name) const
{
    UNUSED(module_name);
    return false;
}

int 
GenericModuleManager::shell_execute(uid_t userid, const vector<string>& argv, 
			     GenericModuleManager::CallBack cb, bool do_exec)
{
    UNUSED(userid);
    UNUSED(argv);
    UNUSED(cb);
    UNUSED(do_exec);
    return XORP_ERROR;
}
