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



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/profile.hh"

#include "profile_vars.hh"

struct profile_vars {
    string var;
    string comment;
} profile_vars[] = {
    {profile_route_ribin, 	"Routes entering RIB"},
    {profile_route_rpc_in, 	"Routes being queued for the FEA"},
    {profile_route_rpc_out, 	"Routes being sent to the FEA"},
};

void
initialize_profiling_variables(Profile& p)
{
    for (size_t i = 0; i < sizeof(profile_vars) / sizeof(struct profile_vars);
	 i++)
	p.create(profile_vars[i].var, profile_vars[i].comment);
}
