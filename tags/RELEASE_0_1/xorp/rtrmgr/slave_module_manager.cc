// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/slave_module_manager.cc,v 1.5 2002/12/09 18:29:38 hodson Exp $"

#include "config.h"
#include "rtrmgr_module.h"
#include "slave_module_manager.hh"

/*this file merely provides dummy definitions for the module manager
  for use in xorpsh, as xorpsh never actually starts any modules, but
  the template commands need to know about a module manager.  This is
  an ugly hack */

int Module::run(bool /*no_execute*/) {return XORP_OK;}
void Module::module_run_done(bool /*success*/) {};
string Module::str() const {return string("");}


ModuleManager::ModuleManager(EventLoop */*event_loop*/) {}

Module* ModuleManager::new_module(const ModuleCommand */*cmd*/) {
    return NULL;
}

int ModuleManager::run_module(Module *, bool /*no_execute*/) {return XORP_OK;}
Module *ModuleManager::find_module(const string &/*name*/) {return NULL;}
bool ModuleManager::module_running(const string &/*name*/) const {return true;}
void ModuleManager::shutdown() {};

