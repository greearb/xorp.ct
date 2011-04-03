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



#include "libxorp/status_codes.h"
#include "finder_client_xrl_target.hh"
#include "finder_client.hh"

FinderClientXrlTarget::FinderClientXrlTarget(
			 FinderClientXrlCommandInterface* client,
			 XrlCmdMap* cmds)
    : XrlFinderclientTargetBase(cmds), _client(client)
{}

XrlCmdError
FinderClientXrlTarget::common_0_1_get_target_name(string& n)
{
    n = get_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderClientXrlTarget::common_0_1_get_version(string& v)
{
    v = version();
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderClientXrlTarget::common_0_1_get_status(uint32_t& status, string& r)
{
    //Finder client is always ready if it can receive requests.
    status = PROC_READY;
    r = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderClientXrlTarget::common_0_1_shutdown()
{
    //This isn't the right way to shutdown a process. The common
    //interface specific to the target process should be used instead,
    //as it can do the necessary cleanup.
    return XrlCmdError::COMMAND_FAILED();
}

XrlCmdError
FinderClientXrlTarget::finder_client_0_2_hello()
{
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderClientXrlTarget::finder_client_0_2_remove_xrl_from_cache(const string& x)
{
    _client->uncache_xrl(x);
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderClientXrlTarget::finder_client_0_2_remove_xrls_for_target_from_cache(
							const string& target
							)
{
    _client->uncache_xrls_from_target(target);
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderClientXrlTarget::finder_client_0_2_dispatch_tunneled_xrl(
						const string& xrl,
						uint32_t&     xrl_errno,
						string&	      xrl_errtxt
						)
{
    XrlCmdError e = _client->dispatch_tunneled_xrl(xrl);
    xrl_errno  = e.error_code();
    xrl_errtxt = e.note();
    return XrlCmdError::OKAY();
}
