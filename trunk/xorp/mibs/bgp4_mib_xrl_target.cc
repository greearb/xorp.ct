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

#ident "$Header$"

#include "config.h"

#include "bgp4_mib_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "xorpevents.hh"
#include "libxipc/xrl_std_router.hh"
#include "bgp4_mib_xrl_target.hh"

XrlBgpMibTarget::XrlBgpMibTarget(XrlRouter *r, BgpMib& bgp_mib)
	: XrlBgp4MibTargetBase(r), _bgp_mib(bgp_mib)
{
}

XrlCmdError
XrlBgpMibTarget::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&	/* reason */)
{
    status = PROC_READY;
    return XrlCmdError::OKAY();
}

    
