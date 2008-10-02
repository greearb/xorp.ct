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

#ident "$XORP: xorp/fea/profile_vars.cc,v 1.8 2008/07/23 05:10:12 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/profile.hh"

#include "profile_vars.hh"

struct profile_vars {
    string var;
    string comment;
} profile_vars[] = {
    { profile_route_in, 	"Routes entering FEA" },
    { profile_route_out,	"Routes being sent to the KERNEL" }
};

void
initialize_profiling_variables(Profile& p)
{
    for (size_t i = 0; i < sizeof(profile_vars) / sizeof(struct profile_vars);
	 i++)
	p.create(profile_vars[i].var, profile_vars[i].comment);
}
