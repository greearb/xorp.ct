// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rtrmgr/xrl_xorpsh_interface.cc,v 1.25 2008/07/23 05:11:46 pavlin Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "xorpsh_main.hh"
#include "xrl_xorpsh_interface.hh"


XrlXorpshInterface::XrlXorpshInterface(XrlRouter* r, XorpShell& xorpsh)
    : XrlXorpshTargetBase(r),
      _xorpsh(xorpsh),
      _verbose(xorpsh.verbose())
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
					   string& v)
{
    v = string(version());
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
    // TODO: XXX: implement it!!
    exit(0);
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_2_new_config_user(// Input values, 
						      const uint32_t& user_id)
{
    _xorpsh.new_config_user((uid_t)user_id);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_2_config_saved_done(// Input values, 
							 const bool& success, 
							 const string& errmsg)
{
    if (success) {
	XLOG_TRACE(_verbose, "Configuration saved: success");
    } else {
	XLOG_TRACE(_verbose, "Failure saving the configuration: %s",
		   errmsg.c_str());
    }
    _xorpsh.config_saved_done(success, errmsg);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_2_config_change_done(// Input values, 
							 const bool& success, 
							 const string& errmsg)
{
    if (success) {
	XLOG_TRACE(_verbose, "Configuration changed: success");
    } else {
	XLOG_TRACE(_verbose, "Failure changing the configuration: %s",
		   errmsg.c_str());
    }
    _xorpsh.commit_done(success, errmsg);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_2_config_changed(// Input values, 
						     const uint32_t& user_id, 
						     const string& deltas, 
						     const string& deletions)
{
    XLOG_TRACE(_verbose,
	       "config changed: user_id: %u\nDELTAS:\n%sDELETIONS:\n%s\n",
	       XORP_UINT_CAST(user_id), deltas.c_str(), deletions.c_str());
    _xorpsh.config_changed(user_id, deltas, deletions);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlXorpshInterface::rtrmgr_client_0_2_module_status(// Input values,
						    const string&   modname,
						    const uint32_t& status)
{
    XLOG_TRACE(_verbose,
	       "module status: %s changed to status %u\n",
	       modname.c_str(), XORP_UINT_CAST(status));
    _xorpsh.module_status_change(modname, (GenericModule::ModuleStatus)status);
    return XrlCmdError::OKAY();
}
