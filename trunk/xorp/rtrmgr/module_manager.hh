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

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.38 2006/02/09 01:24:29 pavlin Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__


#include <list>
#include <map>

#include "libxorp/timer.hh"
#include "libxorp/callback.hh"

#include "generic_module_manager.hh"

class EventLoop;
class MasterConfigTree;
class ModuleManager;
class Rtrmgr;
class RunCommand;


class Module : public GenericModule {
public:
    Module(ModuleManager& mmgr, const string& name, const string& path,
	   const string& expath, bool verbose);
    ~Module();

    void new_status(ModuleStatus new_status);
    string str() const;
    ModuleManager& module_manager() const { return _mmgr; }
    const string& expath() const { return _expath; }

    int execute(bool do_exec, bool is_verification,
		XorpCallback1<void, bool>::RefPtr cb);
    int restart();
    void terminate(XorpCallback0<void>::RefPtr cb);
    void terminate_with_prejudice(XorpCallback0<void>::RefPtr cb);

    void module_restart_cb(bool success);
    void module_exited(bool success, bool is_signal_terminated,
		       int term_signal, bool is_coredumped);
    void module_stopped(int stop_signal);

private:
    static const TimeVal SHUTDOWN_TIMEOUT_TIMEVAL;
    ModuleManager& _mmgr;
    string	_path;
    string	_expath;
    bool	_do_exec;	// false indicates we're running in test mode,
				// when we may not actually start any processes
    bool	_verbose;	// Set to true if output is verbose
    XorpTimer	_shutdown_timer;
    XorpCallback0<void>::RefPtr _terminate_cb; // The cb when module terminated
};

class ModuleManager : public GenericModuleManager {
public:
    class Process;

    ModuleManager(EventLoop& eventloop, Rtrmgr& rtrmgr,
		  bool do_restart, bool verbose,
		  const string& xorp_root_dir);
    ~ModuleManager();

    bool new_module(const string& module_name, const string& path,
		    string& error_msg);
    int start_module(const string& module_name, bool do_exec,
		     bool is_verification,
		     XorpCallback1<void, bool>::RefPtr cb);
    int kill_module(const string& module_name,
		   XorpCallback0<void>::RefPtr cb);
    bool module_is_running(const string& module_name) const;
    bool module_has_started(const string& module_name) const;
    void shutdown();
    void module_shutdown_cb(string module_name);
    bool is_shutdown_completed() const;
    void module_status_changed(const string& module_name, 
			       Module::ModuleStatus old_status,
			       Module::ModuleStatus new_status);

    list<string> get_module_names() const;
    list<Module *> find_running_modules_by_path(const string& expath);
    int execute_process(const string& expath, string& error_msg);
    void process_exited(const string& expath, bool success,
			bool is_signal_terminated, int term_signal,
			bool is_coredumped);
    void process_stopped(const string& expath, int stop_signal);
    ModuleManager::Process* find_process_by_path(const string& expath);

    const string& xorp_root_dir() const { return _xorp_root_dir; }
    MasterConfigTree* master_config_tree() const { return _master_config_tree; }
    void set_master_config_tree(MasterConfigTree* v) { _master_config_tree = v; }

    /**
     * Test if processes that have failed should be restarted.
     *
     * @return true if failed processes should be restarted, otherwise false.
     */
    bool do_restart() const { return _do_restart; }

    class Process {
    public:
	Process(ModuleManager& mmgr, const string& expath);
	~Process();

	int startup(string& error_msg);
	void terminate();
	void terminate_with_prejudice();

    private:
	void stdout_cb(RunCommand* run_command, const string& output);
	void stderr_cb(RunCommand* run_command, const string& output);
	void done_cb(RunCommand* run_command, bool success,
		     const string& error_msg);
	void stopped_cb(RunCommand* run_command, int stop_signal);

	ModuleManager&	_mmgr;
	string		_expath;
	RunCommand* _run_command;
    };

private:
    int expand_execution_path(const string& path, string& expath,
			      string& error_msg);

    Rtrmgr&		_rtrmgr;
    MasterConfigTree*	_master_config_tree;
    map<string, Process *> _expath2process; // Map exec path to running process
    bool		_do_restart;	// Set to true to enable module restart
    bool		_verbose;	// Set to true if output is verbose
    string		_xorp_root_dir;	// The root of the XORP tree
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
