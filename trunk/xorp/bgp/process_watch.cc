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

#ident "$XORP: xorp/bgp/process_watch.cc,v 1.1 2003/06/17 06:44:51 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "process_watch.hh"

ProcessWatch::ProcessWatch(XrlStdRouter *xrl_router, EventLoop& eventloop,
		 TerminateCallback cb) :
    _eventloop(eventloop), _shutdown(cb), _fea(false), _rib(false)
{
	
    /*
    ** Register interest in the fea and rib.
    */
    XrlFinderEventNotifierV0p1Client finder(xrl_router);
    finder.send_register_class_event_interest("finder",
			      xrl_router->instance_name(), "fea",
			      ::callback(this, &ProcessWatch::callback));
    finder.send_register_class_event_interest("finder",
			      xrl_router->instance_name(), "rib",
			      ::callback(this, &ProcessWatch::callback));
	
}

void
ProcessWatch::callback(const XrlError& error)
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
    }
}

void 
ProcessWatch::death(const string& target_class, const string& target_instance)
{
    debug_msg("death: %s %s\n", target_class.c_str(), target_instance.c_str());

    if (_fea_instance == target_instance) {
	XLOG_ERROR("The fea died died");
	::exit(-1);
    } else if (_rib_instance == target_instance) {
	XLOG_ERROR("The rib died");
	_shutdown->dispatch();
	_shutdown_timer = _eventloop.
	    new_oneoff_after_ms(1000 * 10, ::callback(::exit, -1));
    }
}

void
ProcessWatch::finder_death() const
{
    XLOG_ERROR("The finder died");
    ::exit(-1);
}

bool
ProcessWatch::ready() const
{
    return _fea && _rib;
}
