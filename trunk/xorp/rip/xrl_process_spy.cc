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

#ident "$XORP: xorp/rip/xrl_process_spy.cc,v 1.3 2004/04/22 01:11:52 pavlin Exp $"

#define DEBUG_LOGGING

#include "rip_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/xorp.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "xrl_config.hh"
#include "xrl_process_spy.hh"

XrlProcessSpy::XrlProcessSpy(XrlRouter& rtr)
    : ServiceBase("FEA/RIB Process Watcher"), _rtr(rtr)
{
    _cname[FEA_IDX] = xrl_fea_name();
    _cname[RIB_IDX] = xrl_rib_name();
}

XrlProcessSpy::~XrlProcessSpy()
{
}

bool
XrlProcessSpy::fea_present() const
{
    if (status() == RUNNING)
	return _iname[FEA_IDX].empty() == false;
    debug_msg("XrlProcessSpy::fea_present() called when not RUNNING.\n");
    return false;
}

bool
XrlProcessSpy::rib_present() const
{
    if (status() == RUNNING)
	return _iname[RIB_IDX].empty() == false;
    debug_msg("XrlProcessSpy::rib_present() called when not RUNNING.\n");
    return false;
}


void
XrlProcessSpy::birth_event(const string& class_name,
			   const string& instance_name)
{
    debug_msg("Birth event: class %s instance %s\n",
	      class_name.c_str(), instance_name.c_str());

    for (uint32_t i = 0; i < END_IDX; i++) {
	if (class_name != _cname[i]) {
	    continue;
	}
	if (_iname[i].empty() == false) {
	    XLOG_WARNING("Got ");
	}
	_iname[i] = instance_name;
    }
}

void
XrlProcessSpy::death_event(const string& class_name,
			   const string& instance_name)
{
    debug_msg("Death event: class %s instance %s\n",
	      class_name.c_str(), instance_name.c_str());

    for (uint32_t i = 0; i < END_IDX; i++) {
	if (class_name != _cname[i]) {
	    continue;
	}
	if (_iname[i] == instance_name) {
	    _iname[i].erase();
	    return;
	}
    }
}


bool
XrlProcessSpy::startup()
{
    if (status() == READY || status() == SHUTDOWN) {
	send_register(0);
	set_status(STARTING);
    }

    return true;
}

void
XrlProcessSpy::schedule_register_retry(uint32_t idx)
{
    EventLoop& e = _rtr.eventloop();
    _retry = e.new_oneoff_after_ms(100,
				   callback(this,
					    &XrlProcessSpy::send_register,
					    idx));
}

void
XrlProcessSpy::send_register(uint32_t idx)
{
    XrlFinderEventNotifierV0p1Client x(&_rtr);
    if (x.send_register_class_event_interest("finder", _rtr.instance_name(),
		_cname[idx], callback(this, &XrlProcessSpy::register_cb, idx))
	== false) {
	XLOG_ERROR("Failed to send interest registration for \"%s\"\n",
		   _cname[idx].c_str());
	schedule_register_retry(idx);
    }
}

void
XrlProcessSpy::register_cb(const XrlError& xe, uint32_t idx)
{
    if (XrlError::OKAY() != xe) {
	XLOG_ERROR("Failed to register interest in \"%s\": %s\n",
		   _cname[idx].c_str(), xe.str().c_str());
	schedule_register_retry(idx);
	return;
    }
    debug_msg("Registered interest in %s\n", _cname[idx].c_str());
    idx++;
    if (idx < END_IDX) {
	send_register(idx);
    } else {
	set_status(RUNNING);
    }
}


bool
XrlProcessSpy::shutdown()
{
    if (status() == RUNNING) {
	send_deregister(0);
	set_status(SHUTTING_DOWN);
    }

    return true;
}

void
XrlProcessSpy::schedule_deregister_retry(uint32_t idx)
{
    EventLoop& e = _rtr.eventloop();
    _retry = e.new_oneoff_after_ms(100,
				   callback(this,
					    &XrlProcessSpy::send_deregister,
					    idx));
}

void
XrlProcessSpy::send_deregister(uint32_t idx)
{
    XrlFinderEventNotifierV0p1Client x(&_rtr);
    if (x.send_deregister_class_event_interest(
		"finder", _rtr.instance_name(), _cname[idx],
		callback(this, &XrlProcessSpy::deregister_cb, idx))
	== false) {
	XLOG_ERROR("Failed to send interest registration for \"%s\"\n",
		   _cname[idx].c_str());
	schedule_deregister_retry(idx);
    }
}

void
XrlProcessSpy::deregister_cb(const XrlError& xe, uint32_t idx)
{
    if (XrlError::OKAY() != xe) {
	XLOG_ERROR("Failed to deregister interest in \"%s\": %s\n",
		   _cname[idx].c_str(), xe.str().c_str());
	schedule_deregister_retry(idx);
	return;
    }
    debug_msg("Deregistered interest in %s\n", _cname[idx].c_str());
    idx++;
    if (idx < END_IDX) {
	send_deregister(idx);
    } else {
	set_status(SHUTDOWN);
    }
}
