// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/bgp/profile_vars.hh,v 1.15 2008/10/02 21:56:18 bms Exp $

#ifndef __BGP_PROFILE_VARS_HH__
#define __BGP_PROFILE_VARS_HH__

#ifndef XORP_DISABLE_PROFILE

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

#endif // profile

#endif // __BGP_PROFILE_VARS_HH__
