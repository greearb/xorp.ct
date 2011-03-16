// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.47 2008/10/02 21:58:23 bms Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__





#include "libxorp/timer.hh"
#include "libxorp/callback.hh"

#include "generic_module_manager.hh"

class EventLoop;
class MasterConfigTree;
class ModuleManager;
class Rtrmgr;
class RunCommand;

/**
 * @short A class for managing router manager modules.
 */
class Module : public GenericModule {
public:
    /**
     * Constructor.
     *
     * @param mmgr the module manager (@ref ModuleManager) to use.
     * @param name the module name.
     * @param path the path to the executable program for this module.
     * @param expath the expanded absolute path to the executable program for
     * this module.
     * @param verbose if true, then output trace messages.
     */
    Module(ModuleManager& mmgr, const string& name, const string& path,
	   const string& expath, bool verbose);

    /**
     * The default destructor.
     */
    ~Module();

    /**
     * Set new status for the module.
     *
     * @param new_status the new status for the module.
     */
    void new_status(ModuleStatus new_status);

    /**
     * Convert the module information to a string.
     *
     * @return a string with the module information.
     */
    string str() const;

    /**
     * Get a reference to the module manager (@ref ModuleManager).
     *
     * @return a reference to the module manager.
     */
    ModuleManager& module_manager() const { return _mmgr; }

    /**
     * Get the expanded absolute path to the executable program.
     *
     * @return the expanded absolute path to the executable program.
     */
    const string& expath() const { return _expath; }

    /**
     * Execute the module.
     *
     * @param do_exec if true then indeed execute the executable program,
     * otherwise just process the execution machinery.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int execute(bool do_exec, bool is_verification,
		XorpCallback1<void, bool>::RefPtr cb);

    /**
     * Restart the module.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int restart();

    /**
     * Terminate the module.
     *
     * @param cb the callback to execute after the module is terminated.
     */
    void terminate(XorpCallback0<void>::RefPtr cb);

    /**
     * Terminate the module with prejudice.
     *
     * @param cb the callback to execute after the module is terminated.
     */
    void terminate_with_prejudice(XorpCallback0<void>::RefPtr cb);

    /**
     * A method called when the corresponding process has exited.
     *
     * @param success if true, the exit status of the process indicates
     * success, otherwise failure.
     * @param is_signal_terminated if true the process has been terminated
     * by a signal.
     * @param term_signal if @ref is_signal_terminated is true, this
     * contains the terminating signal.
     * @param is_coredumped if true the process has generated a coredump.
     */
    void module_exited(bool success, bool is_signal_terminated,
		       int term_signal, bool is_coredumped);

    /**
     * A method called when the corresponding process has been stopped.
     *
     * @param stop_signal the signal that has stopped the process.
     */
    void module_stopped(int stop_signal);

private:
    void module_restart_cb(bool success);

    static const TimeVal SHUTDOWN_TIMEOUT_TIMEVAL;
    ModuleManager& _mmgr;	// The module manager to use
    string	_path;		// The path to the program
    string	_expath;	// The expanded absolute path to the program
    bool	_do_exec;	// false indicates we're running in test mode,
				// when we may not actually start any processes
    bool	_verbose;	// Set to true if output is verbose
    XorpTimer	_shutdown_timer; // A timer used during shutdown
    XorpCallback0<void>::RefPtr _terminate_cb; // The cb when module terminated
};

class ModuleManager : public GenericModuleManager {
public:
    class Process;

    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     * @param rtrmgr the router manager to use.
     * @param do_restart if true, then restart a module if it failed.
     * @param verbose if true, then output trace messages.
     * @param xorp_root_dir the XORP root directory.
     * @param xorp_module_dir the XORP module directory.
     */
    ModuleManager(EventLoop& eventloop, Rtrmgr& rtrmgr,
		  bool do_restart, bool verbose,
		  const string& xorp_root_dir,
		  const string& xorp_module_dir);

    /**
     * The default destructor.
     */
    ~ModuleManager();

    /**
     * Create a new module.
     *
     * @param module_name the module name.
     * @param path the path to the executable program for this module. It
     * could be either the relative or expanded absolute path.
     * @param error_msg the error message (if error).
     * @return true on success, otherwise false.
     */
    bool new_module(const string& module_name, const string& path,
		    string& error_msg);

    /**
     * Start a module.
     *
     * @param module_name the module name.
     * @param do_exec if true then indeed execute the executable program,
     * otherwise just process the execution machinery.
     * @param is_verification if true then this is verification of the
     * execution machinery.
     * @param cb the callback to dispatch at the end of the startup process.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start_module(const string& module_name, bool do_exec,
		     bool is_verification,
		     XorpCallback1<void, bool>::RefPtr cb);

    /**
     * Kill a module.
     *
     * @param module_name the module name.
     * @param cb the callback to dispatch when the module is terminated.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int kill_module(const string& module_name,
		   XorpCallback0<void>::RefPtr cb);

    /**
     * Test whether a module is running.
     *
     * @param module_name the module name.
     * @return true if the module is running, otherwise false.
     */
    bool module_is_running(const string& module_name) const;

    /**
     * Test whether a module has been started.
     *
     * @param module_name the module name.
     * @return true if the module has been started, otherwise false.
     */
    bool module_has_started(const string& module_name) const;

    /**
     * Shutdown the module manager.
     */
    void shutdown();

    /**
     * Test whether the shutdown has been completed.
     *
     * @return true if the shutdown has been completed, otherwise false.
     */
    bool is_shutdown_completed() const;

    /**
     * Change the status of a module.
     *
     * @param module_name the module name.
     * @param old_status the old status.
     * @param new_status the new status.
     */
    void module_status_changed(const string& module_name, 
			       Module::ModuleStatus old_status,
			       Module::ModuleStatus new_status);

    /**
     * Get the module names.
     *
     * @return a list with the module names.
     */
    list<string> get_module_names() const;

    /**
     * Get the running modules that match an executional path.
     *
     * @param expath the path to match.
     * @return a list of modules that are running and match @ref expath.
     */
    list<Module *> find_running_modules_by_path(const string& expath);

    /**
     * Execute a process.
     *
     * @param expath the expanded path for the process to execute.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int execute_process(const string& expath, string& error_msg);

    /**
     * A method called when a process has exited.
     *
     * @param expath the expanded path for the process that has exited.
     * @param success if true, the exit status of the process indicates
     * success, otherwise failure.
     * @param is_signal_terminated if true the process has been terminated
     * by a signal.
     * @param term_signal if @ref is_signal_terminated is true, this
     * contains the terminating signal.
     * @param is_coredumped if true the process has generated a coredump.
     */
    void process_exited(const string& expath, bool success,
			bool is_signal_terminated, int term_signal,
			bool is_coredumped);

    /**
     * A method called when a process has been stopped.
     *
     * @param expath the expanded path for the process that has been stopped.
     * @param stop_signal the signal that has stopped the process.
     */
    void process_stopped(const string& expath, int stop_signal);

    /**
     * Find a process by its expanded path.
     *
     * @param expath the expanded path for the process to find.
     * @return the corresponding process if found, otherwise NULL.
     */
    ModuleManager::Process* find_process_by_path(const string& expath);

    /**
     * Get the XORP root directory.
     *
     * @return the XORP root directory.
     */
    const string& xorp_root_dir() const { return _xorp_root_dir; }

    /**
     * Get the XORP module directory.
     *
     * @return the XORP module directory.
     */
    const string& xorp_module_dir() const { return _xorp_module_dir; }

    /**
     * Get the master configuration tree.
     *
     * @return the master configuration tree.
     */
    MasterConfigTree* master_config_tree() const { return _master_config_tree; }

    /**
     * Set the master configuration tree.
     *
     * @param v the master configuration tree to set.
     */
    void set_master_config_tree(MasterConfigTree* v) { _master_config_tree = v; }

    /**
     * Test if processes that have failed should be restarted.
     *
     * @return true if failed processes should be restarted, otherwise false.
     */
    bool do_restart() const { return _do_restart; }

    class Process {
    public:
	/**
	 * Constructor.
	 *
	 * @param mmgr the module manager (@ref ModuleManager) to use.
	 * @param expath the expanded absolute path to the executable program.
	 */
	Process(ModuleManager& mmgr, const string& expath);

	/**
	 * The default constructor.
	 */
	~Process();

	/**
	 * Startup the process.
	 *
	 * @param error_msg the error message (if error).
	 * @return XORP_OK on success, otherwise XORP_ERROR.
	 */
	int startup(string& error_msg);

	/**
	 * Terminate the process.
	 */
	void terminate();

	/**
	 * Terminate the process with prejudice.
	 */
	void terminate_with_prejudice();

    private:
	void stdout_cb(RunCommand* run_command, const string& output);
	void stderr_cb(RunCommand* run_command, const string& output);
	void done_cb(RunCommand* run_command, bool success,
		     const string& error_msg);
	void stopped_cb(RunCommand* run_command, int stop_signal);

	ModuleManager&	_mmgr;		// The module manager to use
	string		_expath;	// The absolute expanded path
	RunCommand*	_run_command;	// The result running command
    };

private:
    void module_shutdown_cb(string module_name);
    int expand_execution_path(const string& path, string& expath,
			      string& error_msg);

    Rtrmgr&		_rtrmgr;	// The router manager to use
    MasterConfigTree*	_master_config_tree; // The master configuration tree
    map<string, Process *> _expath2process; // Map exec path to running process
    bool		_do_restart;	// Set to true to enable module restart
    bool		_verbose;	// Set to true if output is verbose
    string		_xorp_root_dir;	// The root of the XORP tree
    string		_xorp_module_dir; // Module directory
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
