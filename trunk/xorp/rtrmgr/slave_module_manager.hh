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

// $XORP: xorp/rtrmgr/slave_module_manager.hh,v 1.7 2003/04/25 03:39:02 mjh Exp $

#ifndef __RTRMGR_SLAVE_MODULE_MANAGER_HH__
#define __RTRMGR_SLAVE_MODULE_MANAGER_HH__

#include <map>
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"

#define MODULE_STARTUP 0
#define MODULE_INITIALIZING 1
#define MODULE_RUNNING 2
#define MODULE_STALLED 3
#define MODULE_FAILED 4
#define MODULE_SHUTDOWN 5

class ModuleCommand;

class ModuleManager {
public:
    ModuleManager(EventLoop& eventloop);
    bool new_module(const string& mod_name, const string& path);
    int start_module(const string& mod_name, bool do_exec, 
		   XorpCallback1<void, bool>::RefPtr cb);
    int stop_module(const string& mod_name, bool do_exec, 
		   XorpCallback1<void, bool>::RefPtr cb);
    bool module_exists(const string &name) const;
    bool module_running(const string &name) const;
    bool module_has_started(const string &name) const;
private:
};

#endif // __RTRMGR_SLAVE_MODULE_MANAGER_HH__

