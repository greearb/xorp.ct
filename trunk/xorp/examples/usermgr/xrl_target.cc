// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

#ident "$XORP: xorp/devnotes/template_gpl.cc,v 1.1 2008/10/02 21:56:43 bms Exp $"
#include "usermgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "xrl_target.hh"
#include "usermgr_exception.hh"
#include "usermgr.hh"
#include <vector>

#define DEBUG_INFO(args...)	{ if (_trace) XLOG_INFO(args); }

bool
UserMgrTarget::running() const
{
    return _running || _xrls_pending > 0;
}

UserMgrTarget::UserMgrTarget(XrlRouter& rtr, UserDB userdb)
	: XrlUsermgrTargetBase(&rtr),
	  _rtr(rtr),
	  _running(true),
	  _name(rtr.class_name()),
	  _xrls_pending(0),
	  _userdb(userdb),
	  _trace(false)
{
}

UserMgrTarget::~UserMgrTarget()
{
}

XrlCmdError
UserMgrTarget::common_0_1_get_target_name (string& name)
{
    name = _name;
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::common_0_1_get_version (string& version)
{
    version = XORP_MODULE_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::common_0_1_get_status(uint32_t& status, string& reason)
{
    if (_running) {
	status = PROC_READY;
        reason = "running";
    } else {
        status = PROC_SHUTDOWN;
        reason = "dying";
    } 
    return XrlCmdError::OKAY();
}

void
UserMgrTarget::shutdown(void)
{
    if (_running) {
        _running = false;
    }
}

XrlCmdError
UserMgrTarget::common_0_1_shutdown(void)
{
    shutdown();
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_create_group(
        // Input values,
        const string&   group,
        const uint32_t& groupid)
{
    DEBUG_INFO("%s called with %s, %d.", __func__, group.c_str(), groupid);
    _userdb.AddGroup(group, groupid);
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_delete_group(
        // Input values,
        const string&   group)
{
    DEBUG_INFO("%s called with %s.", __func__, group.c_str());
    _userdb.DelGroup(group);
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_get_groups(
        // Output values,
        XrlAtomList&    groupnames )
{
    DEBUG_INFO("%s called.", __func__);
    vector<string> names = _userdb.list_groups();    
    vector<string>::iterator pos = names.begin();
    for (; pos != names.end(); pos++) {
	string gname = *pos;
	groupnames.append(XrlAtom(gname));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_create_user(
        // Input values,
        const string&   user,
        const uint32_t& userid)
{
    DEBUG_INFO("%s called with %s, %d.", __func__, user.c_str(), userid);
    _userdb.AddUser(user, userid);
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_delete_user(
        // Input values,
        const string&   user)
{
    DEBUG_INFO("%s called with %s.", __func__, user.c_str());
    _userdb.DelUser(user);
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_get_users(
        // Output values,
        XrlAtomList&    usernames )
{
    DEBUG_INFO("%s called.", __func__);
    vector<string> names = _userdb.list_users();    
    vector<string>::iterator pos = names.begin();
    for (; pos != names.end(); pos++) {
	string uname = *pos;
	usernames.append(XrlAtom(uname));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
UserMgrTarget::usermgr_0_1_trace(
	// Input values,
	const string& tvar,
	const bool&   enable )
{
    if (tvar == "all") {
	_trace = enable;
    } else {
	;
    }
    return XrlCmdError::OKAY();
}
