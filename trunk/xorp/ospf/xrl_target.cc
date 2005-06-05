// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#include "config.h"
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "ospf.hh"
#include "xrl_io.hh"
#include "xrl_target.hh"

XrlOspfV2Target::XrlOspfV2Target(XrlRouter *r, Ospf<IPv4>& ospf,
				 XrlIO<IPv4>& io)
	: XrlOspfv2TargetBase(r), _ospf(ospf), _xrl_io(io)
{
}

XrlOspfV3Target::XrlOspfV3Target(XrlRouter *r,
				 Ospf<IPv4>& ospf_ipv4, 
				 Ospf<IPv6>& ospf_ipv6, 
				 XrlIO<IPv4>& io_ipv4,
				 XrlIO<IPv6>& io_ipv6)
    : XrlOspfv3TargetBase(r),
      _ospf_ipv4(ospf_ipv4), _ospf_ipv6(ospf_ipv6),
      _xrl_io_ipv4(io_ipv4), _xrl_io_ipv6(io_ipv6)
{
}

XrlCmdError
XrlOspfV2Target::common_0_1_get_target_name(string&  name)
{
    name = "ospfv2";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_target_name(string&  name)
{
    name = "ospfv3";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::common_0_1_get_version(string&	version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_version(string&	version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::common_0_1_get_status(uint32_t& /*status*/,
				       string& /*reason*/)
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_status(uint32_t& /*status*/,
				       string& /*reason*/)
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::common_0_1_shutdown()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_shutdown()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_router_id(const uint32_t& /*id*/)
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_router_id(const uint32_t& /*id*/)
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}
