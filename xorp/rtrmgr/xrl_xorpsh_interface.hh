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

// $XORP: xorp/rtrmgr/xrl_xorpsh_interface.hh,v 1.17 2008/10/02 21:58:27 bms Exp $

#ifndef __RTRMGR_XRL_XORPSH_INTERFACE_HH__
#define __RTRMGR_XRL_XORPSH_INTERFACE_HH__


#include "xrl/targets/xorpsh_base.hh"


class XorpShell;

class XrlXorpshInterface : public XrlXorpshTargetBase {
public:
    XrlXorpshInterface(XrlRouter* r, XorpShell& xorpsh);

    XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(// Output values,
				      uint32_t& status,
				      string&	reason);
    
    /**
     * Shutdown cleanly
     */
    XrlCmdError common_0_1_shutdown();
    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    XrlCmdError rtrmgr_client_0_2_new_config_user(
	// Input values, 
	const uint32_t&	user_id);

    XrlCmdError rtrmgr_client_0_2_config_saved_done(
	// Input values,
	const bool&	success,
	const string&	errmsg);

    XrlCmdError rtrmgr_client_0_2_config_change_done(
	// Input values, 
	const bool&	success, 
	const string&	errmsg);

    XrlCmdError rtrmgr_client_0_2_config_changed(
	// Input values, 
	const uint32_t&	user_id, 
	const string&	deltas, 
	const string&	deletions);

    XrlCmdError rtrmgr_client_0_2_module_status(
	// Input values,
        const string&   modname,
        const uint32_t& status);

private:
    XorpShell&	_xorpsh;

    bool	_verbose;		// Set to true if output is verbose
};

#endif // __RTRMGR_XRL_XORPSH_INTERFACE_HH__
