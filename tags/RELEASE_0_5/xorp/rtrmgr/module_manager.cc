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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.21 2003/10/01 21:18:50 hodson Exp $"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <glob.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <map>

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "module_manager.hh"
#include "template_commands.hh"

extern "C" int errno;

static map<pid_t, Module*> module_pids;

static void
childhandler(int x)
{
    printf("childhandler: %d\n", x);
    pid_t pid;
    int status;
    pid = wait(&status);
    printf("pid=%d, status=%d\n", pid, status);
    if (WIFEXITED(status)) {
	printf("process exited normally with value %d\n", WEXITSTATUS(status));
	if (status == 0)
	    module_pids[pid]->normal_exit();
	else
	    module_pids[pid]->abnormal_exit(status);
    } else if (WIFSIGNALED(status)) {
	printf("process was killed with signal %d\n", WTERMSIG(status));
	module_pids[pid]->killed();
    } else if (WIFSTOPPED(status)) {
	printf("process stopped\n");
	module_pids[pid]->set_stalled();
    } else {
	XLOG_UNREACHABLE();
    }
}

Module::Module(ModuleManager& mmgr, const string& name, bool verbose)
    : _mmgr(mmgr), _name(name),
    _pid(0),
    _status(MODULE_NOT_STARTED),
    _do_exec(false),
    _verbose(verbose)
{}

Module::~Module()
{
    assert(_status == MODULE_NOT_STARTED);
}

void
Module::terminate(XorpCallback0<void>::RefPtr cb)
{
    if (_verbose)
	printf("Shutting down %s\n", _name.c_str());

    if (!_do_exec) {
	cb->dispatch();
	return;
    }

    if (_status == MODULE_NOT_STARTED) {
	cb->dispatch();
	return;
    }

    if (_status == MODULE_FAILED) {
	_status = MODULE_NOT_STARTED;
	cb->dispatch();
	return;
    }

    printf("sending kill\n");
    // We need to kill the process
    new_status(MODULE_SHUTTING_DOWN);
    kill(_pid, SIGTERM);

    _shutdown_timer =
	_mmgr.eventloop().new_oneoff_after_ms(2000,
             callback(this, &Module::terminate_with_prejudice, cb));
}

void
Module::terminate_with_prejudice(XorpCallback0<void>::RefPtr cb)
{
    printf("terminate_with_prejudice\n");
    if (_status == MODULE_FAILED) {
	_status = MODULE_NOT_STARTED;
	cb->dispatch();
	return;
    }
    if (_status == MODULE_NOT_STARTED) {
	cb->dispatch();
	return;
    }

    printf("sending kill -9\n");
    // if it still hasn't exited, kill it properly.
    kill(_pid, SIGKILL);
    _status = MODULE_NOT_STARTED;

    //we'll give it a couple more seconds to really go away
    _shutdown_timer =
	_mmgr.eventloop().new_oneoff_after_ms(2000, cb);
}


int
Module::set_execution_path(const string& path)
{
    if ((path.size() >= 1) && (path[0] == '~')) {
	// The path to the module starts from the user home directory
	_path = path;
    } else {
	// Add the XORP root path to the front
	_path = _mmgr.xorp_root_dir() + "/" + path;
    }
    struct stat sb;
    if (_verbose) {
	printf("**********************************************************\n");
	printf("new module: %s path: %s\n", _name.c_str(), _path.c_str());
	printf("**********************************************************\n");
    }

    if (_path[0] != '/') {
	// we're going to call glob, but don't want to allow wildcard expansion
	for (u_int i = 0; i < _path.length(); i++) {
	    char c = _path[i];
	    if ((c == '*') || (c == '?') || (c == '[')) {
		string err = _path + ": bad filename";
		throw ExecutionError(err);
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

    signal(SIGCHLD, childhandler);
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
    module_pids[_pid] = this;
    new_status(MODULE_STARTUP);
    cb->dispatch(true);
    return XORP_OK;
}

void
Module::module_run_done(bool success)
{
    printf("module_run_done\n");
    // do we think we managed to start it?
    if (success == false) {
	new_status(MODULE_FAILED);
	return;
    }

    // is it still running?
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
}

void
Module::abnormal_exit(int status)
{
    if (_verbose)
	printf("Module abnormal exit: %s status:%d\n", _name.c_str(), status);
    new_status(MODULE_FAILED);
}

void
Module::killed()
{
    if (_status == MODULE_SHUTTING_DOWN) {
	// it may have been shutting down already, in which case this is OK.
	if (_verbose)
	    printf("Module killed during shutdown: %s\n", _name.c_str());
	new_status(MODULE_NOT_STARTED);
    } else {
	// we don't know why it was killed.
	if (_verbose)
	    printf("Module abnormally killed: %s\n", _name.c_str());
	new_status(MODULE_FAILED);
    }
}

void
Module::new_status(int new_status)
{
    if (new_status == _status)
	return;

    int old_status = _status;
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
    : _eventloop(eventloop), _verbose(verbose), _xorp_root_dir(xorp_root_dir)
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
    Module* m =  find_module(name);
    assert(m != NULL);
    return m->run(do_exec, cb);
}

int
ModuleManager::kill_module(const string&name,
			   XorpCallback0<void>::RefPtr cb)
{
    Module* m =  find_module(name);
    assert(m != NULL);
    m->terminate(cb);
    return XORP_OK;
}

bool
ModuleManager::module_is_running(const string& name) const
{
    const Module* module = const_find_module(name);
    if (module == NULL)
	return false;
    return (module->status() == MODULE_RUNNING);
}

bool
ModuleManager::module_has_started(const string& name) const
{
    const Module* module = const_find_module(name);
    if (module == NULL)
	return false;
    return (module->status() != MODULE_NOT_STARTED);
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
				     int old_status, int new_status)
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
    map<string, Module*>::iterator i;
    for (i = _modules.begin(); i != _modules.end(); i++) {
	i->second->terminate(callback(this,
				      &ModuleManager::module_shutdown_done));
    }
}

void ModuleManager::module_shutdown_done()
{
    // XXX
}

bool
ModuleManager::shutdown_complete()
{
    bool complete = true;
    map<string, Module*>::iterator i, i2;
    for (i = _modules.begin(); i != _modules.end();) {
	if (i->second->status() == MODULE_NOT_STARTED) {
	    delete i->second;
	    i2 = i;
	    i++;
	    _modules.erase(i2);
	} else {
	    complete = false;
	    i++;
	}
    }
    return complete;
}
