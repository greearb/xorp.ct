// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.67 2008/10/02 21:58:23 bms Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#ifdef HAVE_GLOB_H
#include <glob.h>
#elif defined(HOST_OS_WINDOWS)
#include "glob_win32.h"
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "module_manager.hh"
#include "conf_tree_node.hh"
#include "master_conf_tree.hh"
#include "template_commands.hh"
#include "main_rtrmgr.hh"
#include "util.hh"


#ifdef HOST_OS_WINDOWS
#define stat _stat
#endif

const TimeVal Module::SHUTDOWN_TIMEOUT_TIMEVAL = TimeVal(2, 0);

Module::Module(ModuleManager& mmgr, const string& name, const string& path,
	       const string& expath, bool verbose)
    : GenericModule(name),
      _mmgr(mmgr),
      _path(path),
      _expath(expath),
      _do_exec(false),
      _verbose(verbose)
{
}

Module::~Module()
{
    XLOG_ASSERT(_status == MODULE_NOT_STARTED);
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
    string s = c_format("Module %s, path %s\n", _name.c_str(), _path.c_str());

    if (_status != MODULE_NOT_STARTED && _status != MODULE_FAILED)
	s += c_format("Module is running\n");
    else
	s += "Module is not running\n";
    return s;
}

int
Module::execute(bool do_exec, bool is_verification,
		XorpCallback1<void, bool>::RefPtr cb)
{
    string error_msg;

    _terminate_cb.release();

    if (! is_verification)
	XLOG_INFO("Executing module: %s (%s)", _name.c_str(), _path.c_str());

    _do_exec = do_exec;

    if (! _do_exec) {
	cb->dispatch(true);
	return XORP_OK;
    }

    if (_expath.empty()) {
	// No process to execute
	new_status(MODULE_STARTUP);
	cb->dispatch(true);
	return XORP_OK;
    }

    // Check if a process with the same path is running already
    if (_mmgr.find_process_by_path(_expath) == NULL) {
	if (_mmgr.execute_process(_expath, error_msg) != XORP_OK) {
	    cb->dispatch(false);
	    return XORP_ERROR;
	}
    }

    new_status(MODULE_STARTUP);
    cb->dispatch(true);
    return XORP_OK;
}

int
Module::restart()
{
    XLOG_INFO("Restarting module %s ...", name().c_str());

    XorpCallback1<void, bool>::RefPtr execute_cb
	= callback(this, &Module::module_restart_cb);
    if (execute(true, false, execute_cb) != XORP_OK) {
	XLOG_ERROR("Failed to restart module %s.", name().c_str());
	return (XORP_ERROR);
    }

    MasterConfigTree* mct = module_manager().master_config_tree();
    ConfigTreeNode* ctn = mct->find_config_module(name());
    if (ctn != NULL) {
	XLOG_INFO("Configuring module %s ...", name().c_str());
	ctn->mark_subtree_as_uncommitted();
	mct->execute();
    }

    return (XORP_OK);
}

void
Module::terminate(XorpCallback0<void>::RefPtr cb)
{
    debug_msg("Module::terminate() : %s\n", _name.c_str());

    _terminate_cb.release();

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
    list<Module*> module_list = _mmgr.find_running_modules_by_path(_expath);
    if (module_list.size() > 1) {
	// Not the last module within this process, hence we are done
	new_status(MODULE_NOT_STARTED);
	cb->dispatch();
	return;
    }

    //
    // Kill the process
    //
    XLOG_INFO("Killing module: %s", _name.c_str());
    new_status(MODULE_SHUTTING_DOWN);
    _terminate_cb = cb;
    ModuleManager::Process* process = _mmgr.find_process_by_path(_expath);
    XLOG_ASSERT(process != NULL);
    process->terminate();
    _shutdown_timer = _mmgr.eventloop().new_oneoff_after(
	Module::SHUTDOWN_TIMEOUT_TIMEVAL,
	callback(this, &Module::terminate_with_prejudice, cb));
}

void
Module::terminate_with_prejudice(XorpCallback0<void>::RefPtr cb)
{
    debug_msg("terminate_with_prejudice\n");

    _terminate_cb.release();

    if (_status == MODULE_NOT_STARTED) {
	cb->dispatch();
	return;
    }

    if (_status == MODULE_FAILED) {
	new_status(MODULE_NOT_STARTED);
	cb->dispatch();
	return;
    }

    if (_expath.empty()) {
	// Nothing to terminate
	new_status(MODULE_NOT_STARTED);
	cb->dispatch();
	return;
    }

    //
    // Kill the process with prejudice
    //
    XLOG_INFO("Killing module with prejudice: %s", _name.c_str());
    new_status(MODULE_NOT_STARTED);
    _terminate_cb = cb;
    ModuleManager::Process* process = _mmgr.find_process_by_path(_expath);
    XLOG_ASSERT(process != NULL);
    process->terminate_with_prejudice();
    // Give it a couple more seconds to really go away
    _shutdown_timer = _mmgr.eventloop().new_oneoff_after(
	Module::SHUTDOWN_TIMEOUT_TIMEVAL,
	callback(this, &Module::terminate_with_prejudice, cb));
}

void
Module::module_restart_cb(bool success)
{
    string error_msg;

    // Do we think we managed to start it?
    if (success == false) {
        new_status(MODULE_FAILED);
        return;
    }

    // Is it still running?
    if (_status == MODULE_FAILED) {
	error_msg = c_format("%s: module startup failed", _expath.c_str());
        XLOG_ERROR("%s", error_msg.c_str());
        return;
    }

    if (_status == MODULE_STARTUP) {
        new_status(MODULE_INITIALIZING);
    }
}

void
Module::module_exited(bool success, bool is_signal_terminated, int term_signal,
		      bool is_coredumped)
{
    UNUSED(term_signal);

    if (success) {
	XLOG_INFO("Module normal exit: %s", _name.c_str());
	new_status(MODULE_NOT_STARTED);
	return;
    }

    if (is_signal_terminated) {
	if (_status == MODULE_SHUTTING_DOWN) {
	    XLOG_INFO("Module killed during shutdown: %s", _name.c_str());
	    new_status(MODULE_NOT_STARTED);
	    if (! _terminate_cb.is_empty())
		_terminate_cb->dispatch();
	    _terminate_cb.release();
	    return;
	}

	if (is_coredumped) {
	    XLOG_INFO("Module coredumped: %s", _name.c_str());
	} else {
	    // We don't know why it was killed.
	    XLOG_INFO("Module abnormally killed: %s", _name.c_str());
	}
	new_status(MODULE_FAILED);

	if (module_manager().do_restart()) {
	    //
	    // Restart the module.
	    // XXX: if the child was killed by SIGTERM or SIGKILL, then
	    // DO NOT restart it, because probably it was killed for a reason.
	    //
	    switch (term_signal) {
#ifndef HOST_OS_WINDOWS
	    case SIGTERM:
	    case SIGKILL:
		break;
#endif // HOST_OS_WINDOWS
	    default:
		restart();
		break;
	    }
	}
	return;
    }

    XLOG_INFO("Module abnormal exit: %s", _name.c_str());
    new_status(MODULE_FAILED);
    if (module_manager().do_restart())
	restart();
}

void
Module::module_stopped(int stop_signal)
{
    UNUSED(stop_signal);

    XLOG_INFO("Module stalled: %s", _name.c_str());

    new_status(MODULE_STALLED);
}

ModuleManager::ModuleManager(EventLoop& eventloop, Rtrmgr& rtrmgr,
			     bool do_restart, bool verbose,
			     const string& xorp_root_dir)
    : GenericModuleManager(eventloop, verbose),
      _rtrmgr(rtrmgr),
      _master_config_tree(NULL),
      _do_restart(do_restart),
      _verbose(verbose),
      _xorp_root_dir(xorp_root_dir)
{
}

ModuleManager::~ModuleManager()
{
    shutdown();
}

int
ModuleManager::expand_execution_path(const string& path, string& expath,
				     string& error_msg)
{
    if (path.empty()) {
	// Empty path: no program should be executed
	expath = path;
	return XORP_OK;
    }

    if (! is_absolute_path(path, true)) {
	//
	// The path to the module doesn't starts from the user home directory
	// and is not an absolute path (in UNIX, DOS or NT UNC form).
	//
	// Add the XORP root path to the front
	expath = xorp_root_dir() + PATH_DELIMITER_STRING + path;
    }

#ifdef HOST_OS_WINDOWS
    // Assume the path is still in UNIX format at this point and needs to
    // be converted to the native format.
    expath = unix_path_to_native(expath);
#endif

    if (! is_absolute_path(expath, false)) {
	debug_msg("calling glob\n");

	// we're going to call glob, but don't want to allow wildcard expansion
	for (size_t i = 0; i < expath.length(); i++) {
	    char c = expath[i];
	    if ((c == '*') || (c == '?') || (c == '[')) {
		error_msg = c_format("%s: bad filename", expath.c_str());
		return XORP_ERROR;
	    }
	}
	glob_t pglob;
	glob(expath.c_str(), 0, NULL, &pglob);
	if (pglob.gl_pathc != 1) {
	    error_msg = c_format("%s: file does not exist", expath.c_str());
	    return XORP_ERROR;
	}
	expath = pglob.gl_pathv[0];
	globfree(&pglob);
    }
    expath += EXECUTABLE_SUFFIX;

    struct stat sb;
    if (stat(expath.c_str(), &sb) < 0) {
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
	error_msg = c_format("%s: %s", expath.c_str(), error_msg.c_str());
	return XORP_ERROR;
    }
    return XORP_OK;
}

bool
ModuleManager::new_module(const string& module_name, const string& path,
			  string& error_msg)
{
    string expath;
    debug_msg("ModuleManager::new_module %s\n", module_name.c_str());

    if (expand_execution_path(path, expath, error_msg) != XORP_OK) {
	return false;
    }
    Module* module = new Module(*this, module_name, path, expath, _verbose);
    if (store_new_module(module, error_msg) != true) {
	delete module;
	return false;
    }
    XLOG_TRACE(_verbose, "New module: %s (%s)",
	       module_name.c_str(), path.c_str());

    return true;
}

int
ModuleManager::start_module(const string& module_name, bool do_exec,
			    bool is_verification,
			    XorpCallback1<void, bool>::RefPtr cb)
{
    Module* module;

    module = dynamic_cast<Module *>(find_module(module_name));
    XLOG_ASSERT(module != NULL);

    return (module->execute(do_exec, is_verification, cb));
}

int
ModuleManager::kill_module(const string& module_name,
			   XorpCallback0<void>::RefPtr cb)
{
    Module* module;

    module = dynamic_cast<Module *>(find_module(module_name));
    XLOG_ASSERT(module != NULL);

    module->terminate(cb);
    return XORP_OK;
}

bool
ModuleManager::module_is_running(const string& module_name) const
{
    const Module* module;

    module = dynamic_cast<const Module *>(const_find_module(module_name));
    if (module == NULL)
	return false;
    return (module->status() == Module::MODULE_RUNNING);
}

bool
ModuleManager::module_has_started(const string& module_name) const
{
    const Module* module;

    module = dynamic_cast<const Module *>(const_find_module(module_name));
    if (module == NULL)
	return false;
    return (module->status() != Module::MODULE_NOT_STARTED);
}

void
ModuleManager::shutdown()
{
    debug_msg("ModuleManager::shutdown\n");

    map<string, GenericModule *>::iterator iter;
    for (iter = _modules.begin(); iter != _modules.end(); ++iter) {
	Module *module = dynamic_cast<Module *>(iter->second);
	XLOG_ASSERT(module != NULL);
	module->terminate(callback(this, &ModuleManager::module_shutdown_cb,
				   module->name()));
    }
}

void
ModuleManager::module_shutdown_cb(string module_name)
{
    // TODO: anything that needs to be done here?
    UNUSED(module_name);
}

bool
ModuleManager::is_shutdown_completed() const
{
    bool completed = true;

    map<string, GenericModule *>::const_iterator iter;
    for (iter = _modules.begin(); iter != _modules.end(); ++iter) {
	if (iter->second->status() != Module::MODULE_NOT_STARTED) {
	    completed = false;
	    break;
	}
    }

    return completed;
}

void
ModuleManager::module_status_changed(const string& module_name,
				     Module::ModuleStatus old_status,
				     Module::ModuleStatus new_status)
{
    UNUSED(old_status);
    _rtrmgr.module_status_changed(module_name, new_status);
}

list<string>
ModuleManager::get_module_names() const
{
    list<string> result;

    map<string, GenericModule *>::const_iterator iter;
    for (iter = _modules.begin(); iter != _modules.end(); ++iter) {
	result.push_back(iter->first);
    }

    return (result);
}

list<Module *>
ModuleManager::find_running_modules_by_path(const string& expath)
{
    list<Module *> modules_list;

    map<string, GenericModule *>::const_iterator iter;
    for (iter = _modules.begin(); iter != _modules.end(); ++iter) {
	Module* module = dynamic_cast<Module *>(iter->second);
	XLOG_ASSERT(module != NULL);
	if (module->expath() != expath)
	    continue;
	switch (module->status()) {
	case Module::MODULE_STARTUP:
	case Module::MODULE_INITIALIZING:
	case Module::MODULE_RUNNING:
	case Module::MODULE_STALLED:
	case Module::MODULE_SHUTTING_DOWN:
	    modules_list.push_back(module);
	    break;
	case Module::MODULE_FAILED:
	case Module::MODULE_NOT_STARTED:
	case Module::NO_SUCH_MODULE:
	    break;
	}
    }

    return (modules_list);
}

int
ModuleManager::execute_process(const string& expath, string& error_msg)
{
    if (expath.empty())
	return (XORP_OK);	// XXX: nothing ro execute

    if (_expath2process.find(expath) != _expath2process.end())
	return (XORP_OK);	// XXX: process is already executed

    ModuleManager::Process* process;
    process = new ModuleManager::Process(*this, expath);
    _expath2process.insert(make_pair(expath, process));
    if (process->startup(error_msg) != XORP_OK) {
	_expath2process.erase(expath);
	delete process;
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
ModuleManager::process_exited(const string& expath, bool success,
			      bool is_signal_terminated, int term_signal,
			      bool is_coredumped)
{
    list<Module *> modules_list;
    list<Module *>::iterator module_iter;
    map<string, Process *>::iterator process_iter;

    //
    // Inform all modules that the corresponding process has exited
    //
    modules_list = find_running_modules_by_path(expath);
    for (module_iter = modules_list.begin();
	 module_iter != modules_list.end();
	 ++module_iter) {
	Module* module = *module_iter;
	module->module_exited(success, is_signal_terminated, term_signal,
			      is_coredumped);
    }

    //
    // Remove the process-related state
    //
    process_iter = _expath2process.find(expath);
    XLOG_ASSERT(process_iter != _expath2process.end());
    Process* process = process_iter->second;
    _expath2process.erase(process_iter);
    delete process;
}

void
ModuleManager::process_stopped(const string& expath, int stop_signal)
{
    list<Module *> modules_list;
    list<Module *>::iterator iter;

    //
    // Inform all modules that the corresponding process has stopped
    //
    modules_list = find_running_modules_by_path(expath);
    for (iter = modules_list.begin(); iter != modules_list.end(); ++iter) {
	Module* module = *iter;
	module->module_stopped(stop_signal);
    }
}

ModuleManager::Process*
ModuleManager::find_process_by_path(const string& expath)
{
    map<string, Process *>::iterator iter;

    iter = _expath2process.find(expath);
    if (iter == _expath2process.end())
	return (NULL);

    return (iter->second);
}

ModuleManager::Process::Process(ModuleManager& mmgr, const string& expath)
    : _mmgr(mmgr),
      _expath(expath),
      _run_command(NULL)
{
}

ModuleManager::Process::~Process()
{
    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
}

int
ModuleManager::Process::startup(string& error_msg)
{
    list<string> empty_args;

    XLOG_ASSERT(_run_command == NULL);

    _run_command = new RunCommand(
	_mmgr.eventloop(),
	_expath,
	empty_args,	// XXX: no arguments allowed
	callback(this, &ModuleManager::Process::stdout_cb),
	callback(this, &ModuleManager::Process::stderr_cb),
	callback(this, &ModuleManager::Process::done_cb),
	true);
    _run_command->set_stopped_cb(
	callback(this, &ModuleManager::Process::stopped_cb));
    if (_run_command->execute() != XORP_OK) {
	delete _run_command;
	_run_command = NULL;
	error_msg = c_format("Could not execute program %s",
			     _expath.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
ModuleManager::Process::terminate()
{
    if (_run_command != NULL)
	_run_command->terminate();
}

void
ModuleManager::Process::terminate_with_prejudice()
{
    if (_run_command != NULL)
	_run_command->terminate_with_prejudice();
}

void
ModuleManager::Process::stdout_cb(RunCommand* run_command,
				  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    // XXX: output the message from the child process to stdout as-is
    // XXX: temp, to be removed; see Bugzilla entry 795 */
    XLOG_RTRMGR_ONLY_NO_PREAMBLE("%s", output.c_str());
}

void
ModuleManager::Process::stderr_cb(RunCommand* run_command,
				  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    // XXX: output the message from the child process to stderr as-is
    // XXX: temp, to be removed; see Bugzilla entry 795 */
    XLOG_RTRMGR_ONLY_NO_PREAMBLE("%s", output.c_str());
}

void
ModuleManager::Process::done_cb(RunCommand* run_command, bool success,
				const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);

    bool is_signal_terminated = run_command->is_signal_terminated();
    int term_signal = run_command->term_signal();
    bool is_coredumped = run_command->is_coredumped();

    if (! success)
	XLOG_ERROR("%s", error_msg.c_str());

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }

    //
    // XXX: Must be the last action because it will delete the object itself.
    //
    _mmgr.process_exited(_expath, success, is_signal_terminated, term_signal,
			 is_coredumped);
}

void
ModuleManager::Process::stopped_cb(RunCommand* run_command,
				   int stop_signal)
{
    XLOG_ASSERT(run_command == _run_command);

    _mmgr.process_stopped(_expath, stop_signal);
}
