// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/xrl_fea_io.cc,v 1.5 2008/01/04 03:15:51 pavlin Exp $"


//
// FEA (Forwarding Engine Abstraction) XRL-based I/O implementation.
//


#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "xrl_fea_io.hh"


XrlFeaIo::XrlFeaIo(EventLoop& eventloop, XrlRouter& xrl_router,
		   const string& xrl_finder_targetname)
    : FeaIo(eventloop),
      _xrl_router(xrl_router),
      _xrl_finder_targetname(xrl_finder_targetname)
{
}

XrlFeaIo::~XrlFeaIo()
{
    shutdown();
}

int
XrlFeaIo::startup()
{
    return (FeaIo::startup());
}

int
XrlFeaIo::shutdown()
{
    return (FeaIo::shutdown());
}

bool
XrlFeaIo::is_running() const
{
    return (FeaIo::is_running());
}

int
XrlFeaIo::register_instance_event_interest(const string& instance_name,
					   string& error_msg)
{
    XrlFinderEventNotifierV0p1Client client(&_xrl_router);
    bool success;

    success = client.send_register_instance_event_interest(
	_xrl_finder_targetname.c_str(), _xrl_router.instance_name(),
	instance_name,
	callback(this, &XrlFeaIo::register_instance_event_interest_cb,
		 instance_name));
    if (success != true) {
	error_msg = c_format("Failed to register event interest in "
			     "instance %s: could not transmit the request",
			     instance_name.c_str());
	// XXX: If an error, then assume the target is dead
	instance_death(instance_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
XrlFeaIo::deregister_instance_event_interest(const string& instance_name,
					     string& error_msg)
{
    XrlFinderEventNotifierV0p1Client client(&_xrl_router);
    bool success;

    success = client.send_deregister_instance_event_interest(
	_xrl_finder_targetname.c_str(), _xrl_router.instance_name(),
	instance_name,
	callback(this, &XrlFeaIo::deregister_instance_event_interest_cb,
		 instance_name));
    if (success != true) {
	error_msg = c_format("Failed to deregister event interest in "
			     "instance %s: could not transmit the request",
			     instance_name.c_str());
	//
	// XXX: If we are deregistering, then we don't care whether the
	// target is dead.
	//
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlFeaIo::register_instance_event_interest_cb(const XrlError& xrl_error,
					      string instance_name)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to register event interest in instance %s: %s",
		   instance_name.c_str(), xrl_error.str().c_str());
	// XXX: If an error, then assume the target is dead
	instance_death(instance_name);
    }
}

void
XrlFeaIo::deregister_instance_event_interest_cb(const XrlError& xrl_error,
						string instance_name)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to deregister event interest in instance %s: %s",
		   instance_name.c_str(), xrl_error.str().c_str());
	//
	// XXX: If we are deregistering, then we don't care whether the
	// target is dead.
	//
    }
}
