// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/fea/profile_vars.hh,v 1.1 2004/10/02 04:09:16 atanu Exp $

#ifndef __FEA_PROFILE_VARS_HH__
#define __FEA_PROFILE_VARS_HH__

/**
 * Profile variables
 * See: profile_vars.cc for definitions.
 */
const string profile_route_in = "route_in";
const string profile_route_out = "route_out";

void initialize_profiling_variables(Profile& p);

#endif // __FEA_PROFILE_VARS_HH__
