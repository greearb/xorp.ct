// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/process_watch.cc,v 1.14 2008/10/10 01:54:58 paulz Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "libxorp/debug.h"

#include "process_watch.hh"

ProcessWatch::ProcessWatch(XrlStdRouter& rtr, ProtocolMap& pmap) :
		_pmap(pmap),
		_finder(&rtr), 
		_instance_name(rtr.instance_name()),
		_notifier(NULL),
		_finder_name("finder") // FIXME: hardcoded value
{
}

void 
ProcessWatch::register_cb(const XrlError& err)
{
    string error_msg;

    if (err != XrlError::OKAY()) {
	error_msg = c_format("XRL register_cb() error: %s", err.str().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
// 	xorp_throw(PWException, error_msg);
    }
}

void 
ProcessWatch::add_interest(const string& proc) 
{
    // check if we already added interested, if so do nothing
    if (_watching.find(proc) != _watching.end())
	return;

    _watching.insert(proc);

    debug_msg("[POLICY] ProcessWatch Add interest in process: %s\n",
	      proc.c_str());

    // add interested in process
    _finder.send_register_class_event_interest(_finder_name.c_str(),
		_instance_name, _pmap.xrl_target(proc),
		callback(this,&ProcessWatch::register_cb));
}

void 
ProcessWatch::birth(const string& proto) 
{
    const string& p = _pmap.protocol(proto);
    _alive.insert(p);

    // inform any hooked notifier
    if (_notifier)
	_notifier->birth(p);

}

void 
ProcessWatch::death(const string& proto) 
{
    const string& p = _pmap.protocol(proto);
    _alive.erase(p);

    if (_notifier)
	_notifier->death(p);
}

bool 
ProcessWatch::alive(const string& proto) 
{
    if (_watching.find(proto) == _watching.end())
	xorp_throw(PWException, "Not watching protocol: " + proto);

    return _alive.find(proto) != _alive.end();
}

void
ProcessWatch::set_notifier(PWNotifier& notifier) 
{
    // old notifier is lost... it is a feature, not a bug.
    _notifier = &notifier;
}
