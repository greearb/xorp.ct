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

#ident "$XORP: xorp/bgp/profile_vars.cc,v 1.3 2004/09/21 23:13:06 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include "bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/profile.hh"
#include "profile_vars.hh"

struct profile_vars {
    string var;
    string comment;
} profile_vars[] = {
    {profile_message_in, 	"Messages entering BGP"},
    {profile_route_ribin, 	"Routes entering BGP"},
    {profile_route_rpc_in, 	"Routes being queued for the RIB"},
    {profile_route_rpc_out, 	"Routes being sent to the RIB"},
};

void
initialize_profiling_variables(Profile& p)
{
    for (size_t i = 0; i < sizeof(profile_vars) / sizeof(struct profile_vars);
	 i++)
	p.create(profile_vars[i].var, profile_vars[i].comment);
}

/******* Profile Server Support */

void
profile_transmit_log(const string& pname, XrlStdRouter *xrl_router,
		     const string& instance_name,
		     Profile *profile)
{
    ProfileLogEntry ple;
    if (profile->read_log(pname, ple)) {
	TimeVal t = ple.time();
	XrlProfileClientV0p1Client pc(xrl_router);
	pc.send_log(instance_name.c_str(), pname, t.sec(), t.usec(), ple.loginfo(),
		    callback(profile_transmit_callback, 
			     pname, xrl_router, instance_name, profile));
    } else {
	// Unlock the log entry.
	profile->release_log(pname);
	profile_transmit_finished(pname, xrl_router, instance_name);
    }
}

void
profile_transmit_callback(const XrlError& error, const string pname,
			  XrlStdRouter *xrl_router,
			  const string instance_name,
			  Profile *profile)
{
    if (XrlError::OKAY() != error) {
	XLOG_WARNING(error.error_msg());
	// Unlock the log entry.
	profile->release_log(pname);
	return;
    }
    profile_transmit_log(pname, xrl_router, instance_name, profile);
}

void
profile_transmit_finished(const string& pname, XrlStdRouter *xrl_router,
			  const string& instance_name)
{
    debug_msg("pname = %s instance_name = %s\n", pname.c_str(),
	      instance_name.c_str());

    XrlProfileClientV0p1Client pc(xrl_router);
    pc.send_finished(instance_name.c_str(), pname,
		     callback(profile_transmit_finished_callback, pname));
}

void
profile_transmit_finished_callback(const XrlError& error,
				   const string /*pname*/)
{
    if (XrlError::OKAY() != error)
	XLOG_WARNING(error.error_msg());
}
