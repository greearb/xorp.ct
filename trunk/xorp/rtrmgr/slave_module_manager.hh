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

// $XORP: xorp/rtrmgr/slave_module_manager.hh,v 1.1.1.1 2002/12/11 23:56:16 hodson Exp $

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

#ifdef NOTDEF
class ExecutionError {
public:
    ExecutionError(const string &reason) {err = reason;}
    string err;
};

class Module {
public:
    Module(const string &name, const string &path, EventLoop *event_loop,
	   bool no_execute) 
	throw (ExecutionError);
    ~Module();
    void failed();
private:
    string _name;
    string _path;
    pid_t _pid;
    int _status;
    bool _no_execute; //indicates we're running in debug mode, so
                      //shouldn't actually start any processes
};
#endif

class Module {
public:
    int run(bool /*no_execute*/);
    void module_run_done(bool /*success*/);
    string str() const;
};

class ModuleManager {
public:
    ModuleManager(EventLoop */*event_loop*/);
    Module* new_module(const ModuleCommand */*cmd*/);
    int run_module(Module *m, bool no_execute);
    Module *find_module(const string &name);
    bool module_running(const string &name) const;
    void shutdown();
private:
};

#endif // __RTRMGR_SLAVE_MODULE_MANAGER_HH__
