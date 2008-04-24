// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP$"

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/profile.hh"

#include "profile_vars.hh"

struct profile_vars {
    string var;
    string comment;
} profile_vars[] = {
    { profile_message_in, 	"Messages entering OLSR" },
    { profile_route_ribin, 	"Routes entering OLSR" },
    { profile_route_rpc_in, 	"Routes being queued for the RIB" },
    { profile_route_rpc_out, 	"Routes being sent to the RIB" },

    { trace_message_in, 	"Trace Message entering OLSR" },
    { trace_message_out, 	"Trace Message leaving OLSR" },
    { trace_policy_configure,	"Trace policy introduction" },
};

void
initialize_profiling_variables(Profile& p)
{
    for (size_t i = 0;
	 i < sizeof(profile_vars) / sizeof(struct profile_vars);
	 i++)
	p.create(profile_vars[i].var, profile_vars[i].comment);
}
