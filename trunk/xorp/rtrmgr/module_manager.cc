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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.23 2003/11/22 00:17:15 pavlin Exp $"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <glob.h>
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
    printf("child_handler: %d\n", x);
    pid_t pid;
    int child_wait_status;
    map<pid_t, string>::iterator pid_iter;
    multimap<string, Module*>::iterator path_iter, path_iter_next;

    pid = waitpid(-1, &child_wait_status, WUNTRACED);
    printf("pid=%d, wait status=%d\n", pid, child_wait_status);

    XLOG_ASSERT(pid > 0);
    pid_iter = module_pids.find(pid);
    XLOG_ASSERT(pid_iter != module_pids.end());

    if (WIFEXITED(child_wait_status)) {
	printf("process exited with value %d\n",
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
	printf("process was killed with signal %d\n",
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
	printf("process stopped\n");
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
    if (_verbose)
	printf("Shutting down %s\n", _name.c_str());

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

    printf("sending kill to pid %d\n", _pid);
    // We need to kill the process
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

    printf("terminate_with_prejudice\n");

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

    printf("sending kill -9\n");
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

    if (path[0] == '~') {
	// The path to the module starts from the user home directory
	_path = path;
    } else {
	// Add the XORP root path to the front
	_path = _mmgr.xorp_root_dir() + "/" + path;
    }
    if (_verbose) {
	printf("**********************************************************\n");
	printf("new module: %s path: %s\n", _name.c_str(), _path.c_str());
	printf("**********************************************************\n");
    }

    if (_path[0] != '/') {
	// we're going to call glob, but don't want to allow wildcard expansion
	for (size_t i = 0; i < _path.length(); i++) {
	    char c = _path[i];
	    if ((c == '*') || (c == '?') || (c == '[')) {
		string err = _path + ": bad filename";
		XLOG_ERROR(err.c_str());
		return XORP_ERROR;
	    }
	}
	glob_t pglob;
	glob(_path.c_str(), GLOB_TILDE, NULL, &pglob);
	if (pglob.gl_pathc != 1) {
	    string err(_path + ": File does not exist.");
	    XLOG_ERROR(err.c_str());
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
	XLOG_ERROR(err.c_str());
	return XORP_ERROR;
    }
    return XORP_OK;
}

int
Module::run(bool do_exec, XorpCallback1<void, bool>::RefPtr cb)
{
    bool is_process_running = false;

    if (_verbose) {
	printf("**********************************************************\n");
	printf("running module: %s path: %s\n", _name.c_str(), _path.c_str());
	printf("**********************************************************\n");
    }
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
	    // Detach from the controlling terminal.
	    setsid();
	    if (execl(_expath.c_str(), _expath.c_str(), NULL) < 0) {
		fprintf(stderr, "Execution of %s failed\n", _expath.c_str());
		exit(-1);
	    }
	}
	if (_verbose)
	    printf("New module has PID %d\n", _pid);

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
    printf("module_run_done\n");

    // Do we think we managed to start it?
    if (success == false) {
	new_status(MODULE_FAILED);
	return;
    }

    // Is it still running?
    if (_status == MODULE_FAILED) {
	string err = _expath + ": module startup failed";
	XLOG_ERROR(err.c_str());
	return;
    }
    if (_status == MODULE_STARTUP) {
	new_status(MODULE_INITIALIZING);
    }
}

void
Module::set_stalled()
{
    if (_verbose)
	printf("Module stalled: %s\n", _name.c_str());

    new_status(MODULE_STALLED);
}

void
Module::normal_exit()
{
    if (_verbose)
	printf("Module normal exit: %s\n", _name.c_str());

    new_status(MODULE_NOT_STARTED);
    _pid = 0;
}

void
Module::abnormal_exit(int child_wait_status)
{
    if (_verbose)
	printf("Module abnormal exit: %s status:%d\n",
	       _name.c_str(), child_wait_status);

    new_status(MODULE_FAILED);
    _pid = 0;
}

void
Module::killed()
{
    if (_status == MODULE_SHUTTING_DOWN) {
	// It may have been shutting down already, in which case this is OK.
	if (_verbose)
	    printf("Module killed during shutdown: %s\n", _name.c_str());
	new_status(MODULE_NOT_STARTED);
    } else {
	// We don't know why it was killed.
	if (_verbose)
	    printf("Module abnormally killed: %s\n", _name.c_str());
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
ModuleManager::new_module(const string& name, const string& path)
{
    if (_verbose)
	printf("ModuleManager::new_module %s\n", name.c_str());

    map<string, Module*>::iterator found_mod;
    found_mod = _modules.find(name);
    if (found_mod == _modules.end()) {
	Module* newmod;
	newmod = new Module(*this, name, _verbose);
	_modules[name] = newmod;
	if (newmod->set_execution_path(path) != XORP_OK)
	    return false;
	return true;
    } else {
	if (_verbose)
	    printf("module %s already exists\n", name.c_str());
	return true;
    }
}

int
ModuleManager::start_module(const string&name, bool do_exec,
			    XorpCallback1<void, bool>::RefPtr cb)
{
    Module* m = find_module(name);

    XLOG_ASSERT(m != NULL);
    return m->run(do_exec, cb);
}

int
ModuleManager::kill_module(const string&name, XorpCallback0<void>::RefPtr cb)
{
    Module* m = find_module(name);

    XLOG_ASSERT(m != NULL);
    m->terminate(cb);
    return XORP_OK;
}

bool
ModuleManager::module_is_running(const string& name) const
{
    const Module* module = const_find_module(name);

    if (module == NULL)
	return false;
    return (module->status() == Module::MODULE_RUNNING);
}

bool
ModuleManager::module_has_started(const string& name) const
{
    const Module* module = const_find_module(name);

    if (module == NULL)
	return false;
    return (module->status() != Module::MODULE_NOT_STARTED);
}

Module*
ModuleManager::find_module(const string& name)
{
    map<string, Module*>::iterator found;

    found = _modules.find(name);
    if (found == _modules.end()) {
	debug_msg("ModuleManager: Failed to find module %s\n", name.c_str());
	return NULL;
    } else {
	debug_msg("ModuleManager: Found module %s\n", name.c_str());
	return found->second;
    }
}

const Module*
ModuleManager::const_find_module(const string& name) const
{
    map<string, Module*>::const_iterator found;

    found = _modules.find(name);
    if (found == _modules.end()) {
	return NULL;
    } else {
	return found->second;
    }
}

bool
ModuleManager::module_exists(const string& name) const
{
    return _modules.find(name) != _modules.end();
}

void
ModuleManager::module_status_changed(const string& name,
				     Module::ModuleStatus old_status,
				     Module::ModuleStatus new_status)
{
    // XXX eventually we'll tell someone that things changed
    UNUSED(name);
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
					 &ModuleManager::module_shutdown_done));
    }
}

void
ModuleManager::module_shutdown_done()
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
