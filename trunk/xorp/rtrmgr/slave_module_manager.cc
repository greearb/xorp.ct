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

#ident "$XORP: xorp/rtrmgr/slave_module_manager.cc,v 1.12 2003/12/02 09:38:57 pavlin Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "slave_module_manager.hh"


//
// XXX: this file merely provides dummy definitions for the module manager
// for use in xorpsh, as xorpsh never actually starts any modules, but
// the template commands need to know about a module manager.
// This is an ugly hack.
//


ModuleManager::ModuleManager(EventLoop& eventloop) 
{
    UNUSED(eventloop);
}

bool ModuleManager::new_module(const string& module_name, const string& path) 
{
    UNUSED(module_name);
    UNUSED(path);
    return true;
}

int 
ModuleManager::start_module(const string& module_name, bool do_exec, 
			  XorpCallback1<void, bool>::RefPtr cb)
{
    UNUSED(module_name);
    UNUSED(do_exec);
    UNUSED(cb);
    return XORP_OK;
}

int 
ModuleManager::kill_module(const string& module_name,
			   XorpCallback0<void>::RefPtr cb)
{
    UNUSED(module_name);
    UNUSED(cb);
    return XORP_OK;
}

int
ModuleManager::module_shutdown_completed(const string& module_name,
					 bool success)
{
    UNUSED(module_name);
    UNUSED(success);
    return XORP_OK;
}

bool 
ModuleManager::module_exists(const string& module_name) const
{
    UNUSED(module_name);
    return false;
}

bool 
ModuleManager::module_has_started(const string& module_name) const
{
    UNUSED(module_name);
    return false;
}
