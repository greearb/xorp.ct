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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

// XXX: this file is only in net-snmp source tree.  Must find a solution for
// this
extern "C" {
#include "dlmod.h"
}

#include "xorp_if_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "xorpevents.hh"
#include "libxipc/xrl_std_router.hh"
#include "xorp_if_mib_module.hh"

XrlXorpIfMibTarget::XrlXorpIfMibTarget(XrlRouter *r, XorpIfMib& xorp_if_mib)
	: XrlXorpIfMibTargetBase(r), _xorp_if_mib(xorp_if_mib)
{
}

XrlCmdError
XrlXorpIfMibTarget::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&	/* reason */)
{
    status = PROC_READY;
    return XrlCmdError::OKAY();
}

XrlCmdError XrlXorpIfMibTarget::xorp_if_mib_0_1_load_mib(
    // Input values, 
    const string&	mod_name, 
    const string&	abs_path, 
    // Output values, 
    uint32_t&	mib_index)
{
    // these values are hard coded in the net-snmp file dlmod.h
    static const uint32_t MAXNAME = 64;
    static const uint32_t MAXPATH = 255;
    
    mib_index = 0;

    if ((mod_name.length() >= MAXNAME) || (abs_path.length() >= MAXPATH))
	return XrlCmdError::BAD_ARGS();

    struct dlmod * mibmod;
    mibmod = dlmod_create_module();

    strncpy(mibmod->name, mod_name.c_str(), MAXNAME);
    strncpy(mibmod->path, abs_path.c_str(), MAXPATH);
 
    dlmod_load_module(mibmod);

    if (DLMOD_LOADED == mibmod->status) {
	mib_index = mibmod->index;
	return XrlCmdError::OKAY();
    }
    
    return XrlCmdError::COMMAND_FAILED();
}

XrlCmdError XrlXorpIfMibTarget::xorp_if_mib_0_1_unload_mib(
    // Input values, 
    const uint32_t&	mib_index, 
    // Output values, 
    bool&	unloaded)
{
    struct dlmod * mibmod;
    mibmod = dlmod_get_by_index(mib_index);
    unloaded = false;

    if (NULL == mibmod) return XrlCmdError::BAD_ARGS();

    dlmod_unload_module(mibmod);

    unloaded = (DLMOD_UNLOADED == mibmod->status);
    
    dlmod_delete_module(mibmod);

    return (unloaded ? XrlCmdError::OKAY() : XrlCmdError::COMMAND_FAILED());
 
}


