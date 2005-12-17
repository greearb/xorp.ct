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

// $XORP: xorp/rtrmgr/module_manager.hh,v 1.35 2005/11/03 17:27:51 pavlin Exp $

#ifndef __RTRMGR_MODULE_MANAGER_HH__
#define __RTRMGR_MODULE_MANAGER_HH__


#include <vector>
#include <map>
#include <list>

#include "libxorp/timer.hh"
#include "libxorp/callback.hh"

#include "generic_module_manager.hh"

class EventLoop;
class MasterConfigTree;
class ModuleCommand;
class ModuleManager;
class XorpClient;
class XrlAction;
//class XrlRtrmgrInterface;
class Rtrmgr;


class Module : public GenericModule {
public:
    Module(ModuleManager& mmgr, const string& name, bool verbose);
    ~Module();

    int set_execution_path(const string& path, string& error_msg);
    int run(bool do_exec, bool is_verification,
	    XorpCallback1<void, bool>::RefPtr cb);
    void module_run_done(bool success);
    void set_stalled();
    void normal_exit();
    void abnormal_exit(int child_wait_status);
    void killed();
    void terminate(XorpCallback0<void>::RefPtr cb);
    void terminate_with_prejudice(XorpCallback0<void>::RefPtr cb);
    ModuleManager& module_manager() const { return _mmgr; }
    string str() const;
    void new_status(ModuleStatus new_status);

private:
    ModuleManager& _mmgr;
    string	_path;		// relative path
    string	_expath;	// absolute path
    pid_t	_pid;
#ifdef HOST_OS_WINDOWS
    HANDLE	_phand;
#endif
    bool	_do_exec;	// false indicates we're running in test mode,
				// when we may not actually start any processes

    bool	_verbose;	// Set to true if output is verbose
    XorpTimer	_shutdown_timer;
};

class ModuleManager : public GenericModuleManager {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
    
public:
    ModuleManager(EventLoop& eventloop, Rtrmgr* rtrmgr,
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
    void module_shutdown_cb();
    bool module_is_running(const string& module_name) const;
    bool module_has_started(const string& module_name) const;
    void shutdown();
    bool shutdown_complete();
    void module_status_changed(const string& module_name, 
			       Module::ModuleStatus old_status,
			       Module::ModuleStatus new_status);

    void get_module_list(list <string>& modules);

    const string& xorp_root_dir() const { return _xorp_root_dir; }

    MasterConfigTree* master_config_tree() const { return _master_config_tree; }
    void set_master_config_tree(MasterConfigTree* v) { _master_config_tree = v; }
#if 0
    void set_xrl_interface(XrlRtrmgrInterface* xrl_interface) {
	_xrl_interface = xrl_interface;
    }
#endif

    /**
     * Test if processes that have failed should be restarted.
     *
     * @return true if failed processes should be restarted, otherwise false.
     */
    bool do_restart() const { return _do_restart; }

private:
    bool	_do_restart;
    bool	_verbose;	// Set to true if output is verbose
    string	_xorp_root_dir;	// The root of the XORP tree
    MasterConfigTree*	_master_config_tree;
    //XrlRtrmgrInterface* _xrl_interface;
    Rtrmgr *_rtrmgr;
};

#endif // __RTRMGR_MODULE_MANAGER_HH__
