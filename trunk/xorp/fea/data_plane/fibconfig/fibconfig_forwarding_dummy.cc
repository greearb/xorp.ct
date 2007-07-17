// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP$"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_forwarding.hh"

#include "fibconfig_forwarding_dummy.hh"

//
// Configure unicast forwarding.
//
// The mechanism to get/set the information is dummy (for testing purpose).
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

bool
FibConfigForwardingDummy::test_have_ipv4() const
{
    return (true);
}

bool
FibConfigForwardingDummy::test_have_ipv6() const
{
    return (true);
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
