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

#ident "$XORP: xorp/rtrmgr/xrl_xorpsh_interface.cc,v 1.8 2003/11/17 19:34:32 pavlin Exp $"

// #define DEBUG_CONFIG_CHANGE
#include "version.h"
#include "libxorp/status_codes.h"
#include "libxipc/xrl_router.hh"
#include "xrl_xorpsh_interface.hh"
#include "xorpsh_main.hh"

XrlXorpshInterface::XrlXorpshInterface(XrlRouter *r, XorpShell &xorpsh)
    : XrlXorpshTargetBase(r),
      _xorpsh(xorpsh)
{
}

XrlCmdError 
XrlXorpshInterface::common_0_1_get_target_name(// Output values, 
					       string& name)
{
    name = XrlXorpshTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::common_0_1_get_version(// Output values, 
					   string& version)
{
    version = XORPSH_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlXorpshInterface::common_0_1_get_status(// Output values, 
					  uint32_t& status,
					  string& reason)
{
    // XXX placeholder only
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlXorpshInterface::common_0_1_shutdown()
{
    // XXX placeholder
    exit(0);
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_1_new_config_user(// Input values, 
						      const uint32_t& user_id)
{
    _xorpsh.new_config_user((uid_t)user_id);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_1_config_change_done(// Input values, 
							 const bool& success, 
							 const string& errmsg)
{
#ifdef DEBUG_CONFIG_CHANGE
    printf("received cfg change done\n");
    if (success)
	printf("success\n");
    else
	printf("%s\n", errmsg.c_str());
#endif
    _xorpsh.commit_done(success, errmsg);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_1_config_changed(// Input values, 
						     const uint32_t& user_id, 
						     const string& deltas, 
						     const string& deletions)
{
#ifdef DEBUG_CONFIG_CHANGE
    printf("config changed: user_id: %d\nDELTAS:\n%sDELETIONS:\n%s\n",
	   user_id, deltas.c_str(), deletions.c_str());
#endif
    _xorpsh.config_changed(user_id, deltas, deletions);
    return XrlCmdError::OKAY();
}

