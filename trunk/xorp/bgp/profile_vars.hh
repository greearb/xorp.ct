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

// $XORP$

#ifndef __BGP_PROFILE_VARS_HH__
#define __BGP_PROFILE_VARS_HH__

/**
 * Profile variables
 * See: profile_vars.cc for definitions.
 */
const string profile_route_ribin = "route_ribin";
const string profile_route_rpc_out = "route_rpc_out";

void initialize_profiling_variables(Profile& p);

/******* Profile Server Support */

#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_error.hh"
#include "xrl/interfaces/profile_client_xif.hh"

void
profile_transmit_log(const string& pname, XrlStdRouter *xrl_router,
		     const string& instance_name,
		     Profile *profile);

void 
profile_transmit_callback(const XrlError& error, string pname,
			  XrlStdRouter *xrl_router, const string instance_name,
			  Profile *profile);

void
profile_transmit_finished(const string& pname, XrlStdRouter *xrl_router,
			  const string& instance_name);

void
profile_transmit_finished_callback(const XrlError& error, const string pname);

#endif // __BGP_PROFILE_VARS_HH__


