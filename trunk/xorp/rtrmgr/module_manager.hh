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

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.8 2003/04/24 23:43:47 mjh Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__

#include <map>
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/callback.hh"

#define MODULE_STARTUP 0
#define MODULE_INITIALIZING 1
#define MODULE_RUNNING 2
#define MODULE_STALLED 3
#define MODULE_FAILED 4
#define MODULE_SHUTDOWN 5

class ModuleCommand;
class XorpClient;
class XrlAction;

class ExecutionError {
public:
    ExecutionError(const string &reason) {err = reason;}
    string err;
};

class Module {
public:
    Module(const string& name, bool verbose);
    int set_execution_path(const string &path);
    ~Module();
    int run(bool do_exec, XorpCallback1<void, bool>::RefPtr cb);
    void module_run_done(bool success);
    void failed();
    bool is_running() const {return _running;}
    bool is_starting() const {return _status == MODULE_STARTUP;}
    string str() const;
private:
    string _name;
    string _path; //relative path
    string _expath; //absolute path
    bool _running;
    pid_t _pid;
    int _status;
    bool _do_exec; //false indicates we're running in debug mode, so
                   //shouldn't actually start any processes

    bool _verbose; //verbose output of important events
};

class ModuleManager {
public:
    ModuleManager(EventLoop& eventloop, bool verbose);

    ~ModuleManager();
    bool new_module(const string& mod_name, const string& path);
    int run_module(const string& mod_name, bool do_exec, 
		   XorpCallback1<void, bool>::RefPtr cb);
    bool module_exists(const string &name) const;
    bool module_running(const string &name) const;
    bool module_starting(const string &name) const;
    void shutdown();
    EventLoop& eventloop() {return _eventloop;}
private:
    Module *find_module(const string &name);
    const Module *const_find_module(const string &name) const;

    map <string, Module *> _modules;
    EventLoop& _eventloop;
    bool _verbose; //verbose output of important events
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
