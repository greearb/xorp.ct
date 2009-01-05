// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rtrmgr/generic_module_manager.hh,v 1.15 2008/10/02 21:58:22 bms Exp $

#ifndef __RTRMGR_GENERIC_MODULE_MANAGER_HH__
#define __RTRMGR_GENERIC_MODULE_MANAGER_HH__

#include <map>


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
    ModuleStatus status() const { return _status; }
    const string& name() const { return _name; }
    virtual void new_status(ModuleStatus new_status);

protected:
    string		_name;
    ModuleStatus	_status;

private:
};

class GenericModuleManager {
public:
    GenericModuleManager(EventLoop& eventloop, bool verbose);
    virtual ~GenericModuleManager();

    bool module_exists(const string& module_name) const;
    GenericModule::ModuleStatus module_status(const string& module_name) const;
    EventLoop& eventloop() { return _eventloop; }
    GenericModule* find_module(const string& module_name);
    const GenericModule* const_find_module(const string& module_name) const;

protected:
    bool store_new_module(GenericModule *module, string& error_msg);

    EventLoop&	_eventloop;
    map<string, GenericModule *> _modules;	// Map module name to module
    bool	_verbose;	// Set to true if output is verbose
private:
};

#endif // __RTRMGR_GENERIC_MODULE_MANAGER_HH__
