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

#ident "$XORP: xorp/mibs/xorp_if_mib_xrl_target.cc,v 1.13 2008/10/02 21:57:42 bms Exp $"


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "fixconfigs.h"

// XXX: this file is only in net-snmp source tree.  Must find a solution for
// this
extern "C" {
#include "dlmod.h"
}

#include "xorp_if_module.h"
#include "libxorp/xorp.h"
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
    mibmod->name[MAXNAME] = '\0';
    strncpy(mibmod->path, abs_path.c_str(), MAXPATH);
    mibmod->path[MAXPATH] = '\0';
 
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


static void slow_death() 
{
    exit(0);
}

XrlCmdError XrlXorpIfMibTarget::common_0_1_shutdown()
{
    static XorpTimer * xt;
    xt = new XorpTimer;
    *xt = SnmpEventLoop::the_instance().new_oneoff_after_ms(1,
	callback(slow_death));
    return XrlCmdError::OKAY();
}

