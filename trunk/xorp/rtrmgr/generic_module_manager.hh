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

// $XORP: xorp/rtrmgr/generic_module_manager.hh,v 1.3 2004/12/08 22:47:27 mjh Exp $

#ifndef __RTRMGR_GENERIC_MODULE_MANAGER_HH__
#define __RTRMGR_GENERIC_MODULE_MANAGER_HH__

#include <vector>
#include <map>

#include "libxorp/timer.hh"
#include "libxorp/callback.hh"

class EventLoop;

class GenericModule {
public:
    GenericModule(const string& name);
    virtual ~GenericModule();

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
	MODULE_NOT_STARTED	= 6,

	// We have no record of such a module
	NO_SUCH_MODULE          = 7
    };
 
    virtual string str() const;
    inline ModuleStatus status() const { return _status; }
    inline const string& name() const { return _name; }

protected:
    string	_name;
    ModuleStatus _status;
    virtual void new_status(ModuleStatus new_status);

private:
    
};


class GenericModuleManager {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    GenericModuleManager(EventLoop& eventloop);
    virtual ~GenericModuleManager() {};

    virtual bool new_module(const string& module_name, const string& path);
    virtual int start_module(const string& module_name, bool do_exec, 
		   XorpCallback1<void, bool>::RefPtr cb);
    virtual int kill_module(const string& module_name, XorpCallback0<void>::RefPtr cb);
    bool module_exists(const string& module_name) const;
    GenericModule::ModuleStatus module_status(const string& module_name) const;
    virtual bool module_has_started(const string& module_name) const;
    virtual int shell_execute(uid_t userid, const vector<string>& argv, 
			      GenericModuleManager::CallBack cb, bool do_exec);
    EventLoop& eventloop() { return _eventloop; }

protected:
    EventLoop&	_eventloop;
    GenericModule* find_module(const string& module_name);
    const GenericModule* const_find_module(const string& module_name) const;

    map<string, GenericModule *> _modules;
private:
};

#endif // __RTRMGR_GENERIC_MODULE_MANAGER_HH__
