// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/module_manager.cc,v 1.17 2002/12/09 18:29:37 hodson Exp $"

#include "rtrmgr_module.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h> 
#include <glob.h>
#include <sys/param.h>
#include <sys/wait.h> 
#include <map>
#include "module_manager.hh"
#include "template_commands.hh"
extern "C" int errno;

static map<pid_t, Module*> module_pids;

static void childhandler(int x) {
    printf("childhandler: %d\n", x);
    pid_t pid;
    int status;
    pid = wait(&status);
    printf("pid=%d, status=%d\n", pid, status);
    if (WIFEXITED(status)) {
	printf("process exited normally with value %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
	printf("process was killed with signal %d\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
	printf("process stopped\n");
    }
    module_pids[pid]->failed();
}

Module::Module(const ModuleCommand* cmd)
{
    _cmd = cmd;
    _name = cmd->name();
}

int Module::set_execution_path(const string &path) {
    _path = path;
    _running = false;
    _pid = 0;
    struct stat sb;
    printf("**********************************************************\n");
    printf("new module: %s path: %s\n", _name.c_str(), _path.c_str());
    printf("**********************************************************\n");

    if (path[0] != '/') {
	//we're going to call glob, but don't want to allow wildcard expansion
	for(u_int i = 0; i<path.length(); i++) {
	    char c = path[i];
	    if (c=='*' || c=='?' || c=='[') {
		string err = path + ": bad filename";
		throw ExecutionError(err);
	    }
	}
	glob_t pglob;
	glob(path.c_str(), GLOB_TILDE, NULL, &pglob);
	if (pglob.gl_pathc != 1) {
	    string err(path + ": File does not exist.");
	    XLOG_ERROR(err.c_str());
	    return XORP_ERROR;
	}
	_expath = pglob.gl_pathv[0];
	globfree(&pglob);
    } else {
	_expath = path;
    }
	
	
    if (stat(_expath.c_str(), &sb)<0) {
	string err = _expath + ": ";
	switch errno {
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

int Module::run(bool no_execute)
{
    printf("**********************************************************\n");
    printf("running module: %s path: %s\n", _name.c_str(), _path.c_str());
    printf("**********************************************************\n");
    _no_execute = no_execute;

    if (_no_execute)
	return XORP_OK;
	
    signal(SIGCHLD, childhandler);
    _pid = fork();
    if (_pid == 0) {
	// Detach from the controlling terminal.
	setsid();
	if (execl(_expath.c_str(), _name.c_str(), NULL) < 0) {
	    fprintf(stderr, "Execution of %s failed\n", _expath.c_str());
	    exit(-1);
	}
    }
    printf("New module has PID %d\n", _pid);
    module_pids[_pid] = this;
    _status = MODULE_STARTUP;
    return XORP_OK;
}

void Module::module_run_done(bool success) {
    //do we think we managed to start it?
    if (success == false) {
	_status = MODULE_FAILED;
	return;
    }

    //is it still running?
    if (_status == MODULE_FAILED) {
	string err = _expath + ": module startup failed";
	XLOG_ERROR(err.c_str());
	return;
    }
    if (_status == MODULE_STARTUP) {
	_status = MODULE_INITIALIZING;
    }
    _running = true;
}

Module::~Module() {
    printf("Shutting down %s\n", _name.c_str());
    if (_no_execute)
	return;
    if (_status != MODULE_FAILED) {
	_status = MODULE_SHUTDOWN;
	kill(_pid, SIGTERM);
    }

    //give the process time to exit
    for(int i=0;i<3;i++) {
	sleep(1);
	if (_status == MODULE_FAILED)
	    return;
    }

    //if it still hasn't exited, kill it properly.
    kill(_pid, SIGKILL);
}

void
Module::failed() {
    printf("Module failed: %s status:%d\n", _name.c_str(), _status);
    if (_status == MODULE_STARTUP || _status == MODULE_SHUTDOWN) {
	_status = MODULE_FAILED;
	return;
    }
    _status = MODULE_FAILED;
    printf("need to restart module %s\n", _name.c_str());
}

int 
Module::start_transaction(XorpClient* /*_xclient*/, uint /*tid*/, 
			  bool /*no_execute*/, bool /*no_commit*/) const {
    //XXX
    abort();
    return XORP_OK;
}

int 
Module::end_transaction(XorpClient* /*_xclient*/, uint /*tid*/, 
			bool /*no_execute*/, bool /*no_commit*/) const {
    //XXX
    abort();
    return XORP_OK;
}

string
Module::str() const {
    string s = "Module " + _name + ", path " + _path + "\n";
    if (_running)
	s += c_format("Module is running, pid=%d\n", _pid);
    else
	s += "Module is not running\n";
    return s;
}

ModuleManager::ModuleManager(EventLoop *event_loop) {
    _event_loop = event_loop;
}

ModuleManager::~ModuleManager() {
    shutdown();
}

Module *
ModuleManager::new_module(const ModuleCommand* cmd) {
    printf("ModuleManager::new_module %s\n", cmd->name().c_str());
    string name = cmd->name();
    map<string, Module *>::iterator found_mod;
    found_mod = _modules.find(name);
    if (found_mod == _modules.end()) {
	Module *newmod;
	newmod = new Module(cmd);
	_modules[name] = newmod;
	if (newmod->set_execution_path(cmd->path()) != XORP_OK)
	    return NULL;
	return newmod;
    } else {
	printf("module %s already exists\n", name.c_str());
	return found_mod->second;
    }
}

int 
ModuleManager::run_module(Module *m, bool no_execute) {
    return m->run(no_execute);
}

bool
ModuleManager::module_running(const string &name) const {
    const Module *module = const_find_module(name);
    if (module == NULL)
	return false;
    return (module->is_running());
}

Module*
ModuleManager::find_module(const string &name) {
    map<string, Module *>::iterator found;
    found = _modules.find(name);
    if (found == _modules.end()) {
	return NULL;
    } else {
	return found->second;
    }
}

const Module*
ModuleManager::const_find_module(const string &name) const {
    map<string, Module *>::const_iterator found;
    found = _modules.find(name);
    if (found == _modules.end()) {
	return NULL;
    } else {
	return found->second;
    }
}

void
ModuleManager::shutdown() {
    printf("ModuleManager::shutdown\n");
    map<string, Module *>::iterator found, prev;
    found = _modules.begin(); 
    while (found != _modules.end()) {
	delete found->second;
	prev = found;
	++found;
	_modules.erase(prev);
    }
}

