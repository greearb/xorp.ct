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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.31 2004/05/28 22:27:57 pavlin Exp $"

#include <signal.h>
#include <glob.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <map>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "module_manager.hh"
#include "template_commands.hh"


static map<pid_t, string> module_pids;
static multimap<string, Module*> module_paths;

static void
child_handler(int x)
{
    debug_msg("child_handler: %d\n", x);
    pid_t pid;
    int child_wait_status;
    map<pid_t, string>::iterator pid_iter;
    multimap<string, Module*>::iterator path_iter, path_iter_next;

    pid = waitpid(-1, &child_wait_status, WUNTRACED);
    debug_msg("pid=%d, wait status=%d\n", pid, child_wait_status);

    XLOG_ASSERT(pid > 0);
    pid_iter = module_pids.find(pid);
    XLOG_ASSERT(pid_iter != module_pids.end());

    if (WIFEXITED(child_wait_status)) {
	debug_msg("process exited with value %d\n",
	       WEXITSTATUS(child_wait_status));
	//
	// Set the status for all appropriate modules, and at the same
	// time delete the entries from the "module_paths" multimap.
	//
	path_iter = module_paths.find(module_pids[pid]);
	XLOG_ASSERT(path_iter != module_paths.end());
	if (WEXITSTATUS(child_wait_status) == 0) {
	    while (path_iter != module_paths.end()) {
		path_iter_next = path_iter;
		++path_iter_next;
		if (path_iter->first != module_pids[pid])
		    break;
		Module* module = path_iter->second;
		module->normal_exit();
		module_paths.erase(path_iter);
		path_iter = path_iter_next;
	    }
	} else {
	    while (path_iter != module_paths.end()) {
		path_iter_next = path_iter;
		++path_iter_next;
		if (path_iter->first != module_pids[pid])
		    break;
		Module* module = path_iter->second;
		module->abnormal_exit(child_wait_status);
		module_paths.erase(path_iter);
		path_iter = path_iter_next;
	    }
	}
	module_pids.erase(pid_iter);
    } else if (WIFSIGNALED(child_wait_status)) {
	debug_msg("process was killed with signal %d\n",
	       WTERMSIG(child_wait_status));
	//
	// Set the status for all appropriate modules, and at the same
	// time delete the entries from the "module_paths" multimap.
	//
	path_iter = module_paths.find(module_pids[pid]);
	XLOG_ASSERT(path_iter != module_paths.end());
	while (path_iter != module_paths.end()) {
	    path_iter_next = path_iter;
	    ++path_iter_next;
	    if (path_iter->first != module_pids[pid])
		break;
	    Module* module = path_iter->second;
	    module->killed();
	    module_paths.erase(path_iter);
	    path_iter = path_iter_next;
	}
	module_pids.erase(pid_iter);
    } else if (WIFSTOPPED(child_wait_status)) {
	debug_msg("process stopped\n");
	//
	// Set the status for all appropriate modules.
	//
	path_iter = module_paths.find(module_pids[pid]);
	XLOG_ASSERT(path_iter != module_paths.end());
	while (path_iter != module_paths.end()) {
	    if (path_iter->first != module_pids[pid])
		break;
	    Module* module = path_iter->second;
	    module->set_stalled();
	    ++path_iter;
	}
    } else {
	XLOG_UNREACHABLE();
    }
}

Module::Module(ModuleManager& mmgr, const string& name, bool verbose)
    : _mmgr(mmgr),
      _name(name),
      _userid(NO_SETUID_ON_EXEC),
      _pid(0),
      _status(MODULE_NOT_STARTED),
      _do_exec(false),
      _verbose(verbose)
{

}

Module::~Module()
{
    XLOG_ASSERT(_status == MODULE_NOT_STARTED);
}

void
Module::terminate(XorpCallback0<void>::RefPtr cb)
{
    debug_msg("Module::terminate : %s\n", _name.c_str());

    if (! _do_exec) {
	cb->dispatch();
	return;
    }

    if (_status == MODULE_NOT_STARTED) {
	cb->dispatch();
	return;
    }

    if (_status == MODULE_FAILED) {
	new_status(MODULE_NOT_STARTED);
	cb->dispatch();
	return;
    }

    XLOG_INFO("Terminating module: %s\n", _name.c_str());

    //
    // Find whether this is the last module running within this process
    //
    map<pid_t, string>::iterator pid_iter;
    multimap<string, Module*>::iterator path_iter;
    XLOG_ASSERT(_pid != 0);
    pid_iter = module_pids.find(_pid);
    XLOG_ASSERT(pid_iter != module_pids.end());
    XLOG_ASSERT(pid_iter->second == _expath);
    path_iter = module_paths.find(_expath);
    XLOG_ASSERT(path_iter != module_paths.end());
    bool is_last_module = false;
    do {
	++path_iter;
	if (path_iter == module_paths.end()) {
	    is_last_module = true;
	    break;
	}
	if (path_iter->first != _expath) {
	    is_last_module = true;
	    break;
	}
    } while (false);

    if (! is_last_module) {
	// Not the last module within this process, hence we are done
	for (path_iter = module_paths.begin();
	     path_iter != module_paths.end();
	     ++path_iter) {
	    if (path_iter->second == this) {
		break;
	    }
	}
	XLOG_ASSERT(path_iter != module_paths.end());
	module_paths.erase(path_iter);
	new_status(MODULE_NOT_STARTED);
	_pid = 0;
	cb->dispatch();
	return;
    }

    //
    // We need to kill the process
    //
    XLOG_INFO("Killing module: %s (pid = %d)\n",
	      _name.c_str(), _pid);
    new_status(MODULE_SHUTTING_DOWN);
    kill(_pid, SIGTERM);

    _shutdown_timer = _mmgr.eventloop().new_oneoff_after_ms(2000,
			callback(this, &Module::terminate_with_prejudice, cb));
}

void
Module::terminate_with_prejudice(XorpCallback0<void>::RefPtr cb)
{
    map<pid_t, string>::iterator pid_iter;
    multimap<string, Module*>::iterator path_iter;

    debug_msg("terminate_with_prejudice\n");

    if (_status == MODULE_NOT_STARTED) {
	cb->dispatch();
	_pid = 0;
	return;
    }

    if (_status == MODULE_FAILED) {
	new_status(MODULE_NOT_STARTED);
	_pid = 0;
	cb->dispatch();
	return;
    }

    // Remove the module from the multimap with all running modules
    for (path_iter = module_paths.begin();
	 path_iter != module_paths.end();
	 ++path_iter) {
	if (path_iter->second == this) {
	    break;
	}
    }
    XLOG_ASSERT(path_iter != module_paths.end());
    module_paths.erase(path_iter);
    // XXX: this must be the last module for same path
    XLOG_ASSERT(module_paths.find(_expath) == module_paths.end());

    // Remove from the map with all pids
    XLOG_ASSERT(_pid != 0);
    pid_iter = module_pids.find(_pid);
    XLOG_ASSERT(pid_iter != module_pids.end());
    module_pids.erase(pid_iter);

    debug_msg("sending kill -9\n");
    // If it still hasn't exited, kill it properly.
    kill(_pid, SIGKILL);
    new_status(MODULE_NOT_STARTED);
    _pid = 0;

    // We'll give it a couple more seconds to really go away
    _shutdown_timer = _mmgr.eventloop().new_oneoff_after_ms(2000, cb);
}

int
Module::set_execution_path(const string& path)
{
    if (path.empty()) {
	// The path is missing.
	return XORP_ERROR;
    }

    if (path[0] == '~' || path[0] == '/') {
	// The path to the module starts from the user home directory,
	// or it's an absolute path
	_path = path;
    } else {
	// Add the XORP root path to the front
	_path = _mmgr.xorp_root_dir() + "/" + path;
    }
    XLOG_TRACE(_verbose, "New module: %s (%s)\n",
	       _name.c_str(), _path.c_str());

    if (_path[0] != '/') {
	// we're going to call glob, but don't want to allow wildcard expansion
	for (size_t i = 0; i < _path.length(); i++) {
	    char c = _path[i];
	    if ((c == '*') || (c == '?') || (c == '[')) {
		string err = _path + ": bad filename";
		XLOG_ERROR("%s", err.c_str());
		return XORP_ERROR;
	    }
	}
	glob_t pglob;
	glob(_path.c_str(), GLOB_TILDE, NULL, &pglob);
	if (pglob.gl_pathc != 1) {
	    string err(_path + ": File does not exist.");
	    XLOG_ERROR("%s", err.c_str());
	    return XORP_ERROR;
	}
	_expath = pglob.gl_pathv[0];
	globfree(&pglob);
    } else {
	_expath = _path;
    }

    struct stat sb;
    if (stat(_expath.c_str(), &sb) < 0) {
	string err = _expath + ": ";
	switch (errno) {
	case ENOTDIR:
	    err += "A component of the path prefix is not a directory.";
	    break;
	case ENOENT:
	    err += "File does not exist.";
	    break;
	case EACCES:
	    err += "Permission denied.";
	    break;
	case ELOOP:
	    err += "Too many symbolic links.";
	    break;
	default:
	    err += "Unknown error accessing file.";
	}
	XLOG_ERROR("%s", err.c_str());
	return XORP_ERROR;
    }
    return XORP_OK;
}

void 
Module::set_argv(const vector<string>& argv) {
    _argv = argv;
}

void
Module::set_userid(uid_t userid) {
    //don't call thiss if you don't want to setuid.
    XLOG_ASSERT(userid != NO_SETUID_ON_EXEC);

    _userid = userid;
}

int
Module::run(bool do_exec, XorpCallback1<void, bool>::RefPtr cb)
{
    bool is_process_running = false;

    XLOG_INFO("Running module: %s (%s)\n", _name.c_str(), _path.c_str());

    _do_exec = do_exec;

    if (!_do_exec) {
	cb->dispatch(true);
	return XORP_OK;
    }

    // Check if a process with the same path is running already
    multimap<string, Module*>::iterator path_iter;
    path_iter = module_paths.find(_expath);
    if (path_iter != module_paths.end()) {
	// The process with same path name is already running. Add this
	// module to the set of modules corresponding to that process.
	// However, first verify that this module is not yet in this set.
	for ( ; path_iter != module_paths.end(); ++path_iter) {
	    if (path_iter->first != _expath)
		break;
	    XLOG_ASSERT(path_iter->second != this);
	}
	// Find the pid of the running process
	XLOG_ASSERT(_pid == 0);
	map<pid_t, string>::iterator pid_iter;
	for (pid_iter = module_pids.begin();
	     pid_iter != module_pids.end();
	     ++pid_iter) {
	    if (pid_iter->second == _expath) {
		_pid = pid_iter->first;
		break;
	    }
	}
	XLOG_ASSERT(_pid != 0);
	is_process_running = true;
    }

    if (! is_process_running) {
	// We need to start a new process
	signal(SIGCHLD, child_handler);
	_pid = fork();
	if (_pid == 0) {

	    //set userid as required.
	    if (_userid != NO_SETUID_ON_EXEC) {
		if (setuid(_userid) != 0) {
		    XLOG_ERROR("Failed to setuid(%d) on exec\n", _userid);
		    exit(1);
		}
	    }

	    // Detach from the controlling terminal.
	    setsid();
	    if (_argv.empty()) {
		if (execl(_expath.c_str(), _expath.c_str(), NULL) < 0) {
		    XLOG_ERROR("Execution of %s failed\n", _expath.c_str());
		    exit(1);
		}
	    } else {
		//convert argv from strings to char*
		const char* argv[_argv.size() + 1];
		size_t i;
		for (i = 0; i < _argv.size(); i++) {
		    argv[i] = _argv[i].c_str();
		}
		argv[i] = NULL;
		
		if (execv(_expath.c_str(),const_cast<char*const*>(argv)) < 0) {
		    XLOG_ERROR("Execution of %s failed\n", _expath.c_str());
		    exit(1);
		}
	    }
	}
	debug_msg("New module has PID %d\n", _pid);

	// Insert the new process in the map of processes
	XLOG_ASSERT(module_pids.find(_pid) == module_pids.end());
	module_pids[_pid] = _expath;
    }

    // Insert the new module in the multimap of modules
    module_paths.insert(make_pair(_expath, this));

    new_status(MODULE_STARTUP);
    cb->dispatch(true);
    return XORP_OK;
}

void
Module::module_run_done(bool success)
{
    debug_msg("module_run_done\n");

    // Do we think we managed to start it?
    if (success == false) {
	new_status(MODULE_FAILED);
	return;
    }

    // Is it still running?
    if (_status == MODULE_FAILED) {
	string err = _expath + ": module startup failed";
	XLOG_ERROR("%s", err.c_str());
	return;
    }
    if (_status == MODULE_STARTUP) {
	new_status(MODULE_INITIALIZING);
    }
}

void
Module::set_stalled()
{
    XLOG_INFO("Module stalled: %s\n", _name.c_str());

    new_status(MODULE_STALLED);
}

void
Module::normal_exit()
{
    XLOG_INFO("Module normal exit: %s\n", _name.c_str());

    new_status(MODULE_NOT_STARTED);
    _pid = 0;
}

void
Module::abnormal_exit(int child_wait_status)
{
    XLOG_INFO("Module abnormal exit: %s status:%d\n",
	      _name.c_str(), child_wait_status);

    new_status(MODULE_FAILED);
    _pid = 0;
}

void
Module::killed()
{
    if (_status == MODULE_SHUTTING_DOWN) {
	// It may have been shutting down already, in which case this is OK.
	XLOG_INFO("Module killed during shutdown: %s\n",
		  _name.c_str());
	new_status(MODULE_NOT_STARTED);
    } else {
	// We don't know why it was killed.
	XLOG_INFO("Module abnormally killed: %s\n", _name.c_str());
	new_status(MODULE_FAILED);
    }
    _pid = 0;
}

void
Module::new_status(ModuleStatus new_status)
{
    if (new_status == _status)
	return;

    ModuleStatus old_status = _status;
    _status = new_status;
    _mmgr.module_status_changed(_name, old_status, new_status);
}

string
Module::str() const
{
    string s = "Module " + _name + ", path " + _path + "\n";

    if (_status != MODULE_NOT_STARTED && _status != MODULE_FAILED)
	s += c_format("Module is running, pid=%d\n", _pid);
    else
	s += "Module is not running\n";
    return s;
}

ModuleManager::ModuleManager(EventLoop& eventloop, bool verbose,
			     const string& xorp_root_dir)
    : _eventloop(eventloop),
      _verbose(verbose),
      _xorp_root_dir(xorp_root_dir)
{
}

ModuleManager::~ModuleManager()
{
    shutdown();
}

bool
ModuleManager::new_module(const string& module_name, const string& path)
{
    debug_msg("ModuleManager::new_module %s\n", module_name.c_str());

    map<string, Module*>::iterator found_mod;
    found_mod = _modules.find(module_name);
    if (found_mod == _modules.end()) {
	Module* new_module;
	new_module = new Module(*this, module_name, _verbose);
	_modules[module_name] = new_module;
	if (new_module->set_execution_path(path) != XORP_OK)
	    return false;
	return true;
    } else {
	XLOG_TRACE(_verbose, "Module %s already exists\n",
		   module_name.c_str());
	return true;
    }
}

int
ModuleManager::start_module(const string& module_name, bool do_exec,
			    XorpCallback1<void, bool>::RefPtr cb)
{
    Module* module = find_module(module_name);

    XLOG_ASSERT(module != NULL);
    return module->run(do_exec, cb);
}

int
ModuleManager::kill_module(const string& module_name,
			   XorpCallback0<void>::RefPtr cb)
{
    Module* module = find_module(module_name);

    XLOG_ASSERT(module != NULL);
    module->terminate(cb);
    return XORP_OK;
}

bool
ModuleManager::module_is_running(const string& module_name) const
{
    const Module* module = const_find_module(module_name);

    if (module == NULL)
	return false;
    return (module->status() == Module::MODULE_RUNNING);
}

bool
ModuleManager::module_has_started(const string& module_name) const
{
    const Module* module = const_find_module(module_name);

    if (module == NULL)
	return false;
    return (module->status() != Module::MODULE_NOT_STARTED);
}

Module*
ModuleManager::find_module(const string& module_name)
{
    map<string, Module*>::iterator found;

    found = _modules.find(module_name);
    if (found == _modules.end()) {
	debug_msg("ModuleManager: Failed to find module %s\n",
		  module_name.c_str());
	return NULL;
    } else {
	debug_msg("ModuleManager: Found module %s\n", module_name.c_str());
	return found->second;
    }
}

const Module*
ModuleManager::const_find_module(const string& module_name) const
{
    map<string, Module*>::const_iterator found;

    found = _modules.find(module_name);
    if (found == _modules.end()) {
	return NULL;
    } else {
	return found->second;
    }
}

bool
ModuleManager::module_exists(const string& module_name) const
{
    return _modules.find(module_name) != _modules.end();
}

void
ModuleManager::module_status_changed(const string& module_name,
				     Module::ModuleStatus old_status,
				     Module::ModuleStatus new_status)
{
    // XXX eventually we'll tell someone that things changed
    UNUSED(module_name);
    UNUSED(old_status);
    UNUSED(new_status);
}

void
ModuleManager::shutdown()
{
    debug_msg("ModuleManager::shutdown\n");

    map<string, Module*>::iterator iter;
    for (iter = _modules.begin(); iter != _modules.end(); ++iter) {
	iter->second->terminate(callback(this,
					 &ModuleManager::module_shutdown_cb));
    }
}

void
ModuleManager::module_shutdown_cb()
{
    // XXX: TODO: implement it?
}

bool
ModuleManager::shutdown_complete()
{
    bool complete = true;

    map<string, Module*>::iterator iter, iter2;
    for (iter = _modules.begin(); iter != _modules.end(); ) {
	if (iter->second->status() == Module::MODULE_NOT_STARTED) {
	    delete iter->second;
	    iter2 = iter;
	    ++iter;
	    _modules.erase(iter2);
	} else {
	    complete = false;
	    ++iter;
	}
    }
    return complete;
}


int 
ModuleManager::shell_execute(uid_t userid, const vector<string>& argv, 
			     ModuleManager::CallBack cb, bool do_exec)
{
    if (!new_module(argv[0], argv[0])) {
	return XORP_ERROR;
    }
    Module *module = find_module(argv[0]);
    module->set_userid(userid);
    module->set_argv(argv);
    XorpCallback1<void, bool>::RefPtr run_cb 
	= callback(module, &Module::module_run_done);
    if (module->run(do_exec, run_cb)<0) {
	cb->dispatch(false, "Failed to execute" + argv[0]);
	return XORP_ERROR;
    }
    cb->dispatch(true, "");
    return XORP_OK;
}
