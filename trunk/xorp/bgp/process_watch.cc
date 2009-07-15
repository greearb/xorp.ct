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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/exceptions.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "exceptions.hh"
#include "process_watch.hh"

ProcessWatch::ProcessWatch(XrlStdRouter *xrl_router, EventLoop& eventloop,
			   const char *bgp_mib_name, TerminateCallback cb) :
    _eventloop(eventloop), _shutdown(cb), _fea(false), _rib(false)
{
	
    /*
    ** Register interest in the fea and rib and snmp trap handler.
    */
    XrlFinderEventNotifierV0p1Client finder(xrl_router);
    finder.send_register_class_event_interest("finder",
	xrl_router->instance_name(), "fea",
	    callback(this, &ProcessWatch::interest_callback));
    finder.send_register_class_event_interest("finder",
	xrl_router->instance_name(), "rib",
	    callback(this, &ProcessWatch::interest_callback));
    finder.send_register_class_event_interest("finder",
	xrl_router->instance_name(), bgp_mib_name,
	    callback(this, &ProcessWatch::interest_callback));
}

void
ProcessWatch::interest_callback(const XrlError& error)
{
    debug_msg("callback %s\n", error.str().c_str());

    if(XrlError::OKAY() != error.error_code()) {
	XLOG_FATAL("callback: %s",  error.str().c_str());
    }
}

void
ProcessWatch::birth(const string& target_class, const string& target_instance)
{
    debug_msg("birth: %s %s\n", target_class.c_str(), target_instance.c_str());
	
    if(target_class == "fea" && false == _fea) {
	_fea = true;
	_fea_instance = target_instance;
    } else if(target_class == "rib" && false == _rib) {
	_rib = true;
	_rib_instance = target_instance;
    } else
	add_target(target_class, target_instance);
}

void 
ProcessWatch::death(const string& target_class, const string& target_instance)
{
    debug_msg("death: %s %s\n", target_class.c_str(), target_instance.c_str());

    if (_fea_instance == target_instance) {
	XLOG_ERROR("The fea died");
	::exit(-1);
    } else if (_rib_instance == target_instance) {
	XLOG_ERROR("The rib died");
	start_kill_timer();
	_shutdown->dispatch();
    } else
	remove_target(target_class, target_instance);
}

void
ProcessWatch::finder_death(const char *file, const int lineno)
{
    XLOG_ERROR("The finder has died BGP process exiting called from %s:%d",
	       file, lineno);

    start_kill_timer();
    xorp_throw(NoFinder, "");
}

void
ProcessWatch::start_kill_timer()
{
    _shutdown_timer = _eventloop.
	new_oneoff_after_ms(1000 * 10, ::callback(::exit, -1));
}

bool
ProcessWatch::ready() const
{
    return _fea && _rib;
}

bool
ProcessWatch::target_exists(const string& target) const
{
    debug_msg("target_exists: %s\n", target.c_str());

    list<Process>::const_iterator i;
    for(i = _processes.begin(); i != _processes.end(); i++)
	if(target == i->_target_class) {
	    debug_msg("target: %s found\n", target.c_str());
	    return true;
	}

    debug_msg("target %s not found\n", target.c_str());

    return false;
}

void 
ProcessWatch::add_target(const string& target_class,
			  const string& target_instance)
{
    debug_msg("add_target: %s %s\n", target_class.c_str(),
	      target_instance.c_str());

    _processes.push_back(Process(target_class, target_instance));
}

void 
ProcessWatch::remove_target(const string& target_class,
			     const string& target_instance)
{
    debug_msg("remove_target: %s %s\n", target_class.c_str(),
	      target_instance.c_str());

    list<Process>::iterator i;
    for(i = _processes.begin(); i != _processes.end(); i++)
	if(target_class == i->_target_class &&
	   target_instance == i->_target_instance) {
	    _processes.erase(i);
	    return;
	}

    XLOG_FATAL("unknown target %s %s", target_class.c_str(),
	       target_instance.c_str());
}
