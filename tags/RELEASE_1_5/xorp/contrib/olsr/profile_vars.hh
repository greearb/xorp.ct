// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/profile_vars.hh,v 1.1 2008/04/24 15:19:54 bms Exp $

#ifndef __OLSR_PROFILE_VARS_HH__
#define __OLSR_PROFILE_VARS_HH__

/**
 * Profile variables
 * See: profile_vars.cc for definitions.
 */
const string profile_message_in = "message_in";
const string profile_route_ribin = "route_ribin";
const string profile_route_rpc_in = "route_rpc_in";
const string profile_route_rpc_out = "route_rpc_out";

const string trace_message_in = "trace_message_in";
const string trace_message_out = "trace_message_out";
const string trace_nexthop_resolution = "trace_nexthop_resolution";
const string trace_policy_configure = "trace_policy_configure";
const string trace_state_change = "trace_state_change";

void initialize_profiling_variables(Profile& p);

#endif // __OLSR_PROFILE_VARS_HH__
