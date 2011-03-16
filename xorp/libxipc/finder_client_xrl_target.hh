// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/finder_client_xrl_target.hh,v 1.15 2008/10/02 21:57:20 bms Exp $

#ifndef __LIBXIPC_FINDER_CLIENT_XRL_TARGET_HH__
#define __LIBXIPC_FINDER_CLIENT_XRL_TARGET_HH__

#include "xrl/targets/finder_client_base.hh"

class FinderClientXrlCommandInterface; 

class FinderClientXrlTarget : public XrlFinderclientTargetBase {
public:
    FinderClientXrlTarget(FinderClientXrlCommandInterface* client,
			  XrlCmdMap* cmds);

    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();
    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    XrlCmdError finder_client_0_2_hello();

    XrlCmdError finder_client_0_2_remove_xrl_from_cache(const string& xrl);

    XrlCmdError finder_client_0_2_remove_xrls_for_target_from_cache(
							const string& target);

    XrlCmdError finder_client_0_2_dispatch_tunneled_xrl(const string& xrl,
							uint32_t& xrl_errno,
							string&   xrl_errtxt);
    
protected:
    FinderClientXrlCommandInterface* _client;
};

#endif // __LIBXIPC_FINDER_CLIENT_XRL_TARGET_HH__
