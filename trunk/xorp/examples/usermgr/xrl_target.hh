// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2009 XORP, Inc.
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

// $XORP: xorp/examples/usermgr/xrl_target.hh,v 1.2 2008/10/29 22:24:15 paulz Exp $

#ifndef __USERMGR_USERMGR_TARGET_HH__
#define __USERMGR_USERMGR_TARGET_HH__

#include "libxipc/xrl_router.hh"
#include "xrl/targets/usermgr_base.hh"
#include "usermgr.hh"

class UserMgrTarget : public XrlUsermgrTargetBase {
public:
    bool	running() const;

    UserMgrTarget(XrlRouter& rtr, UserDB userdb);
    ~UserMgrTarget();

    EventLoop& eventloop();

    XrlCmdError common_0_1_get_target_name(
        // Output values,
        string& name);

    XrlCmdError common_0_1_get_version(
        // Output values,
        string& version);

    XrlCmdError common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason);

    XrlCmdError common_0_1_shutdown();

    XrlCmdError usermgr_0_1_create_group(
        // Input values,
        const string&   groupname,
        const uint32_t& groupid);

    XrlCmdError usermgr_0_1_delete_group(
        // Input values,
        const string&   groupname);

    XrlCmdError usermgr_0_1_get_groups(
        // Output values,
	XrlAtomList&    users);

    XrlCmdError usermgr_0_1_create_user(
        // Input values,
        const string&   username,
        const uint32_t& userid);

    XrlCmdError usermgr_0_1_delete_user(
        // Input values,
        const string&   username);

    XrlCmdError usermgr_0_1_get_users(
        // Output values,
        XrlAtomList&    groups);

    XrlCmdError usermgr_0_1_trace(
        // Input values,
        const string&   tvar,
        const bool&     enable);

private:

    void		    _shutdown(void);
    XrlRouter&              _rtr;
    bool                    _running;
    string		    _name;
    int			    _xrls_pending;
    UserDB		    _userdb;
    bool		    _trace;
};

#endif // __USERMGR_USERMGR_TARGET_HH__
