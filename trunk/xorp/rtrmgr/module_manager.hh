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

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.4 2003/03/10 23:21:00 hodson Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__

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
class XorpClient;
class XrlAction;

class ExecutionError {
public:
    ExecutionError(const string &reason) {err = reason;}
    string err;
};

class Module {
public:
    Module(const ModuleCommand* cmd);
    int set_execution_path(const string &path);
    ~Module();
    int run(bool no_execute);
    void module_run_done(bool success);
    int start_transaction(XorpClient* _xclient, uint tid, 
			  bool no_execute, bool no_commit) const;
    int end_transaction(XorpClient* _xclient, uint tid, 
			bool no_execute, bool no_commit) const;
    void failed();
    bool is_running() const {return _running;}
    string str() const;
private:
    int do_transaction(XrlAction* xa, XorpClient* _xclient, uint tid, 
		       bool no_execute, bool no_commit) const;
    string _name;
    string _path; //relative path
    string _expath; //absolute path
    bool _running;
    pid_t _pid;
    int _status;
    bool _no_execute; //indicates we're running in debug mode, so
                      //shouldn't actually start any processes

    const ModuleCommand* _cmd;
};

class ModuleManager {
public:
    ModuleManager(EventLoop& eventloop);

    ~ModuleManager();
    Module *new_module(const ModuleCommand* cmd);
    int run_module(Module *m, bool no_execute);
    Module *find_module(const string &name);
    const Module *const_find_module(const string &name) const;
    bool module_running(const string &name) const;
    bool module_starting(const string &name) const;
    void shutdown();
    EventLoop& eventloop() {return _eventloop;}
private:
    map <string, Module *> _modules;
    EventLoop& _eventloop;
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
