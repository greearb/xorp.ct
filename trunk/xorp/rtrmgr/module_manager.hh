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

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.22 2004/06/10 22:41:52 hodson Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__


#include <vector>
#include <map>

#include "libxorp/timer.hh"
#include "libxorp/callback.hh"


#define NO_SETUID_ON_EXEC 0

class EventLoop;
class MasterConfigTree;
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
    void set_argv(const vector<string>& argv);
    void set_userid(uid_t userid);
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
    ModuleManager& module_manager() const { return _mmgr; }
    const string& name() const { return _name; }

private:
    void new_status(ModuleStatus new_status);
    
    ModuleManager& _mmgr;
    string	_name;
    string	_path;		// relative path
    string	_expath;	// absolute path
    vector<string> _argv;	// command line arguments
    uid_t       _userid;        // userid to execute process as, zero
				// for current user
    pid_t	_pid;
    ModuleStatus _status;
    bool	_do_exec;	// false indicates we're running in test mode,
				// when we may not actually start any processes

    bool	_verbose;	// Set to true if output is verbose
    XorpTimer	_shutdown_timer;
};

class ModuleManager {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
    
public:
    ModuleManager(EventLoop& eventloop, bool do_restart, bool verbose,
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

    /**
     * @short shell_execute is used to start external processes.
     *
     * shell_execute is used to start external processes, running them
     * in a shell.  It is NOT used to start regular XORP processes,
     * but rather for background maintenance tasks.  This is
     * ModuleManager functionality primarily because it simplifies the
     * handling of SIGCHILD.
     *
     * @param userid the UID of the user to run the task as.
     * @param argv the command and arguements to run
     * @param callback callback to call when the child process terminates
     */
    int shell_execute(uid_t userid, const vector<string>& argv, 
		      ModuleManager::CallBack cb, bool do_exec);
    MasterConfigTree* master_config_tree() const { return _master_config_tree; }
    void set_master_config_tree(MasterConfigTree* v) { _master_config_tree = v; }

    /**
     * Test if processes that have failed should be restarted.
     *
     * @return true if failed processes should be restarted, otherwise false.
     */
    bool do_restart() const { return _do_restart; }

private:
    Module* find_module(const string& module_name);
    const Module* const_find_module(const string& module_name) const;

    map<string, Module *> _modules;
    EventLoop&	_eventloop;
    bool	_do_restart;
    bool	_verbose;	// Set to true if output is verbose
    string	_xorp_root_dir;	// The root of the XORP tree
    MasterConfigTree*	_master_config_tree;
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
