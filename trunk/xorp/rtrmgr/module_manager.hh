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

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.17 2003/12/10 22:31:29 pavlin Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__

#include <map>
#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/callback.hh"


class ModuleCommand;
class ModuleManager;
class XorpClient;
class XrlAction;

class Module {
public:
    Module(ModuleManager& mmgr, const string& name, bool verbose);
    ~Module();

    enum ModuleStatus {
	// The process has started, but we haven't started configuring it yet
	MODULE_STARTUP		= 0,

	// The process has started, and we're in the process of configuring it
	// (or it's finished configuration, but waiting for another process)
	MODULE_INITIALIZING	= 1,

	// The process is operating normally
	MODULE_RUNNING		= 2,

	// The process has failed, and is no longer runnning
	MODULE_FAILED		= 3,

	// The process has failed; it's running, but not responding anymore
	MODULE_STALLED		= 4,

	// The process has been signalled to shut down, but hasn't exitted yet
	MODULE_SHUTTING_DOWN	= 5,

	// The process has not been started
	MODULE_NOT_STARTED	= 6
    };

    int set_execution_path(const string& path);
    int run(bool do_exec, XorpCallback1<void, bool>::RefPtr cb);
    void module_run_done(bool success);
    void set_stalled();
    void normal_exit();
    void abnormal_exit(int child_wait_status);
    void killed();
    string str() const;
    ModuleStatus status() const { return _status; }
    void terminate(XorpCallback0<void>::RefPtr cb);
    void terminate_with_prejudice(XorpCallback0<void>::RefPtr cb);

private:
    void new_status(ModuleStatus new_status);
    
    ModuleManager& _mmgr;
    string	_name;
    string	_path;		// relative path
    string	_expath;	// absolute path
    pid_t	_pid;
    ModuleStatus _status;
    bool	_do_exec;	// false indicates we're running in debug mode,
				// so shouldn't actually start any processes

    bool	_verbose;	// verbose output of important events
    XorpTimer	_shutdown_timer;
};

class ModuleManager {
public:
    ModuleManager(EventLoop& eventloop, bool verbose,
		  const string& xorp_root_dir);
    ~ModuleManager();

    bool new_module(const string& module_name, const string& path);
    int start_module(const string& module_name, bool do_exec, 
		   XorpCallback1<void, bool>::RefPtr cb);
    int kill_module(const string& module_name, 
		   XorpCallback0<void>::RefPtr cb);
    void module_shutdown_cb();
    bool module_exists(const string& module_name) const;
    bool module_is_running(const string& module_name) const;
    bool module_has_started(const string& module_name) const;
    void shutdown();
    bool shutdown_complete();
    EventLoop& eventloop() { return _eventloop; }
    void module_status_changed(const string& module_name, 
			       Module::ModuleStatus old_status,
			       Module::ModuleStatus new_status);
    const string& xorp_root_dir() const { return _xorp_root_dir; }

private:
    Module* find_module(const string& module_name);
    const Module* const_find_module(const string& module_name) const;

    map<string, Module *> _modules;
    EventLoop&	_eventloop;
    bool	_verbose;	// verbose output of important events
    string	_xorp_root_dir;	// The root of the XORP tree
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
