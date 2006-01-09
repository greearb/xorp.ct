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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.55 2005/12/21 20:08:44 pavlin Exp $"

#include "rtrmgr_module.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <signal.h>

#ifdef HAVE_GLOB_H
#include <glob.h>
#elif defined(HOST_OS_WINDOWS)
#include "glob_win32.h"
#endif

#include <map>

#include "module_manager.hh"
#include "conf_tree_node.hh"
#include "master_conf_tree.hh"
#include "template_commands.hh"
#include "main_rtrmgr.hh"
#include "util.hh"

#ifdef HOST_OS_WINDOWS
#define stat _stat
#endif

static map<pid_t, string> module_pids;
static multimap<string, Module*> module_paths;

#ifndef HOST_OS_WINDOWS
static int
restart_module(Module* module)
{
    XLOG_INFO("Restarting module %s ...", module->name().c_str());
    XorpCallback1<void, bool>::RefPtr run_cb
	= callback(module, &Module::module_run_done);
    if (module->run(true, false, run_cb) < 0) {
	XLOG_ERROR("Failed to restart module %s.", module->name().c_str());
	return (XORP_ERROR);
    }

    MasterConfigTree* mct = module->module_manager().master_config_tree();
    ConfigTreeNode* ctn = mct->find_config_module(module->name());
    if (ctn != NULL) {
	XLOG_INFO("Configuring module %s ...", module->name().c_str());
	ctn->mark_subtree_as_uncommitted();
	mct->execute();
    }

    return (XORP_OK);
}
#endif

//
// XXX: We need to figure out how to get an asynchronous notification
// of a 'child' process exiting in Win32.
//

#ifndef HOST_OS_WINDOWS
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
    if (pid_iter == module_pids.end()) {
	// XXX: the program that has exited is not a module process
	return;
    }

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
		//
		// Restart the module because it exited abnormally.
		// It probably crashed.
		//
		if (module->status() == Module::MODULE_FAILED) {
		    if (module->module_manager().do_restart())
			restart_module(module);
		}
	    }
	}
	module_pids.erase(pid_iter);
    } else if (WIFSIGNALED(child_wait_status)) {
	int sig = WTERMSIG(child_wait_status);

	debug_msg("process was killed with signal %d\n", sig);

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
	    //
	    // XXX: if the child was killed by SIGTERM or SIGKILL, then
	    // DO NOT restart it, because probably it was killed for a reason.
	    //
	    if (module->status() == Module::MODULE_FAILED) {
		switch (sig) {
		case SIGTERM:
		case SIGKILL:
		    break;
		default:
		    if (module->module_manager().do_restart())
			restart_module(module);
		    break;
		}
	    }
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
#endif // ! HOST_OS_WINDOWS

Module::Module(ModuleManager& mmgr, const string& name, bool verbose)
    : GenericModule(name), _mmgr(mmgr),
      _pid(0),
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

    XLOG_INFO("Terminating module: %s", _name.c_str());

    if (_expath.empty()) {
	// Nothing to terminate
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

    //
    // We need to kill the process
    //
    XLOG_INFO("Killing module: %s (pid = %u)", _name.c_str(),
	      XORP_UINT_CAST(_pid));
    new_status(MODULE_SHUTTING_DOWN);
#ifdef HOST_OS_WINDOWS
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, _pid);
#else
    kill(_pid, SIGTERM);
#endif

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

    if (_expath.empty()) {
	// Nothing to terminate
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

    // If it still hasn't exited, kill it properly.
#ifdef HOST_OS_WINDOWS
    TerminateProcess(_phand, 1);
    CloseHandle(_phand);
    _pid = 0;
    _phand = INVALID_HANDLE_VALUE;
#else
    kill(_pid, SIGKILL);
    _pid = 0;
#endif
    new_status(MODULE_NOT_STARTED);

    // We'll give it a couple more seconds to really go away
    _shutdown_timer = _mmgr.eventloop().new_oneoff_after_ms(2000, cb);
}

int
Module::set_execution_path(const string& path, string& error_msg)
{
    if (path.empty()) {
	// Empty path: no program should be executed
	_path = path;
	_expath = path;
	XLOG_TRACE(_verbose, "New module: %s (%s)",
		   _name.c_str(), _path.c_str());
	return XORP_OK;
    }

    if (is_absolute_path(_path, true)) {
	// The path to the module starts from the user home directory,
	// or it's an absolute path (in UNIX, DOS or NT UNC form).
	_path = path;
    } else {
	// Add the XORP root path to the front
	_path = _mmgr.xorp_root_dir() + PATH_DELIMITER_STRING + path;
    }

#ifdef HOST_OS_WINDOWS
    // Assume the path is still in UNIX format at this point and needs to
    // be converted to the native format.
    _path = unix_path_to_native(_path);
#endif

    XLOG_TRACE(_verbose, "New module: %s (%s)", _name.c_str(), _path.c_str());

    if (!is_absolute_path(_path, false)) {
	debug_msg("calling glob\n");

	// we're going to call glob, but don't want to allow wildcard expansion
	for (size_t i = 0; i < _path.length(); i++) {
	    char c = _path[i];
	    if ((c == '*') || (c == '?') || (c == '[')) {
		error_msg = c_format("%s: bad filename", _path.c_str());
		return XORP_ERROR;
	    }
	}
	glob_t pglob;
	glob(_path.c_str(), GLOB_TILDE, NULL, &pglob);
	if (pglob.gl_pathc != 1) {
	    error_msg = c_format("%s: file does not exist", _path.c_str());
	    return XORP_ERROR;
	}
	_expath = pglob.gl_pathv[0];
	globfree(&pglob);
    } else {
	_expath = _path;
    }

    _path += EXECUTABLE_SUFFIX;
    _expath += EXECUTABLE_SUFFIX;

    struct stat sb;
    if (stat(_expath.c_str(), &sb) < 0) {
	switch (errno) {
	case ENOTDIR:
	    error_msg = "a component of the path prefix is not a directory";
	    break;
	case ENOENT:
	    error_msg = "file does not exist";
	    break;
	case EACCES:
	    error_msg = "permission denied";
	    break;
#ifdef ELOOP
	case ELOOP:
	    error_msg = "too many symbolic links";
	    break;
#endif
	default:
	    error_msg = "unknown error accessing file";
	}
	error_msg = c_format("%s: %s", _expath.c_str(), error_msg.c_str());
	return XORP_ERROR;
    }
    return XORP_OK;
}

int
Module::run(bool do_exec, bool is_verification,
	    XorpCallback1<void, bool>::RefPtr cb)
{
    bool is_process_running = false;

    if (! is_verification)
	XLOG_INFO("Running module: %s (%s)", _name.c_str(), _path.c_str());

    _do_exec = do_exec;

    if (!_do_exec) {
	cb->dispatch(true);
	return XORP_OK;
    }

    if (_expath.empty()) {
	// Nothing to run
	new_status(MODULE_STARTUP);
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
#ifdef HOST_OS_WINDOWS
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	GetStartupInfoA(&si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = INVALID_HANDLE_VALUE;
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	if (CreateProcessA(NULL,
			   const_cast<char *>(_expath.c_str()),
			   NULL, NULL, TRUE,
			   CREATE_DEFAULT_ERROR_MODE,
			   NULL, NULL, &si, &pi) == 0) {
	    XLOG_ERROR("Execution of %s failed", _expath.c_str());
	    _pid = 0;
	    _phand = INVALID_HANDLE_VALUE;
	} else {
	    _pid = pi.dwProcessId;
	    _phand = pi.hProcess;
	}

#else // ! HOST_OS_WINDOWS
	// We need to start a new process
	signal(SIGCHLD, child_handler);
	_pid = fork();
	if (_pid == 0) {
	    // Detach from the controlling terminal.
	    setsid();
	    if (execl(_expath.c_str(), _expath.c_str(),
		      static_cast<const char*>(NULL)) < 0) {
		XLOG_ERROR("Execution of %s failed", _expath.c_str());
		exit(1);
	    }
	}
#endif // ! HOST_OS_WINDOWS
	debug_msg("New module has PID %u\n", XORP_UINT_CAST(_pid));

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
    XLOG_INFO("Module stalled: %s", _name.c_str());

    new_status(MODULE_STALLED);
}

void
Module::normal_exit()
{
    XLOG_INFO("Module normal exit: %s", _name.c_str());

    new_status(MODULE_NOT_STARTED);
    _pid = 0;
}

void
Module::abnormal_exit(int child_wait_status)
{
    XLOG_INFO("Module abnormal exit: %s status:%d",
	      _name.c_str(), child_wait_status);

    new_status(MODULE_FAILED);
    _pid = 0;
}

void
Module::killed()
{
    if (_status == MODULE_SHUTTING_DOWN) {
	// It may have been shutting down already, in which case this is OK.
	XLOG_INFO("Module killed during shutdown: %s",
		  _name.c_str());
	new_status(MODULE_NOT_STARTED);
    } else {
	// We don't know why it was killed.
	XLOG_INFO("Module abnormally killed: %s", _name.c_str());
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
	s += c_format("Module is running, pid=%u\n", XORP_UINT_CAST(_pid));
    else
	s += "Module is not running\n";
    return s;
}

ModuleManager::ModuleManager(EventLoop& eventloop, 
			     Rtrmgr *rtrmgr, bool do_restart,
			     bool verbose, const string& xorp_root_dir)
    : GenericModuleManager(eventloop, verbose),
      _do_restart(do_restart),
      _verbose(verbose),
      _xorp_root_dir(xorp_root_dir),
      _master_config_tree(NULL),
      _rtrmgr(rtrmgr)
{
}

ModuleManager::~ModuleManager()
{
    shutdown();
}

bool
ModuleManager::new_module(const string& module_name, const string& path,
			  string& error_msg)
{
    debug_msg("ModuleManager::new_module %s\n", module_name.c_str());
    Module* module = new Module(*this, module_name, _verbose);
    if (module->set_execution_path(path, error_msg) != XORP_OK) {
	delete module;
	return false;
    }

    if (store_new_module(module, error_msg) != true) {
	delete module;
	return false;
    }
    return true;
}

int
ModuleManager::start_module(const string& module_name, bool do_exec,
			    bool is_verification,
			    XorpCallback1<void, bool>::RefPtr cb)
{
    Module* module = (Module*)find_module(module_name);

    XLOG_ASSERT(module != NULL);
    return module->run(do_exec, is_verification, cb);
}

int
ModuleManager::kill_module(const string& module_name,
			   XorpCallback0<void>::RefPtr cb)
{
    Module* module = (Module*)find_module(module_name);

    XLOG_ASSERT(module != NULL);
    module->terminate(cb);
    return XORP_OK;
}

bool
ModuleManager::module_is_running(const string& module_name) const
{
    const Module* module = (const Module*)const_find_module(module_name);

    if (module == NULL)
	return false;
    return (module->status() == Module::MODULE_RUNNING);
}

bool
ModuleManager::module_has_started(const string& module_name) const
{
    const Module* module = (const Module*)const_find_module(module_name);

    if (module == NULL)
	return false;
    return (module->status() != Module::MODULE_NOT_STARTED);
}

void
ModuleManager::module_status_changed(const string& module_name,
				     Module::ModuleStatus old_status,
				     Module::ModuleStatus new_status)
{
    UNUSED(old_status);
    _rtrmgr->module_status_changed(module_name, new_status);
}

void
ModuleManager::get_module_list(list <string>& modules)
{
    map<string, GenericModule *>::iterator i;
    for (i = _modules.begin(); i != _modules.end(); i++) {
	modules.push_back(i->first);
    }
}

void
ModuleManager::shutdown()
{
    debug_msg("ModuleManager::shutdown\n");

    map<string, GenericModule*>::iterator iter;
    for (iter = _modules.begin(); iter != _modules.end(); ++iter) {
	Module *module = (Module*)iter->second;
	module->terminate(callback(this, &ModuleManager::module_shutdown_cb));
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

    map<string, GenericModule*>::iterator iter, iter2;
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
