// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_forwarding_dummy.cc,v 1.5 2008/07/23 05:10:20 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/fibconfig.hh"

#include "fibconfig_forwarding_dummy.hh"

//
// Configure unicast forwarding.
//
// The mechanism to get/set the information is Dummy (for testing purpose).
//


FibConfigForwardingDummy::FibConfigForwardingDummy(
    FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigForwarding(fea_data_plane_manager),
      _unicast_forwarding_enabled4(false),
      _unicast_forwarding_enabled6(false),
      _accept_rtadv_enabled6(false)
{
}

FibConfigForwardingDummy::~FibConfigForwardingDummy()
{
}

int
FibConfigForwardingDummy::unicast_forwarding_enabled4(bool& ret_value,
						      string& error_msg) const
{
    UNUSED(error_msg);
    ret_value = _unicast_forwarding_enabled4;

    return (XORP_OK);
}

int
FibConfigForwardingDummy::unicast_forwarding_enabled6(bool& ret_value,
						      string& error_msg) const
{
    UNUSED(error_msg);
    ret_value = _unicast_forwarding_enabled6;

    return (XORP_OK);
}

int
FibConfigForwardingDummy::accept_rtadv_enabled6(bool& ret_value,
						string& error_msg) const
{
    UNUSED(error_msg);
    ret_value = _accept_rtadv_enabled6;

    return (XORP_OK);
}

int
FibConfigForwardingDummy::set_unicast_forwarding_enabled4(bool v,
							  string& error_msg)
{
    UNUSED(error_msg);
    _unicast_forwarding_enabled4 = v;

    return (XORP_OK);
}

int
FibConfigForwardingDummy::set_unicast_forwarding_enabled6(bool v,
							  string& error_msg)
{
    UNUSED(error_msg);
    _unicast_forwarding_enabled6 = v;

    return (XORP_OK);
}

int
FibConfigForwardingDummy::set_accept_rtadv_enabled6(bool v, string& error_msg)
{
    UNUSED(error_msg);
    _accept_rtadv_enabled6 = v;

    return (XORP_OK);
}
