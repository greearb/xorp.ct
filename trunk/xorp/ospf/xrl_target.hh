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

// $XORP$

#ifndef __OSPF_XRL_TARGET_HH__
#define __OSPF_XRL_TARGET_HH__

#include "xrl/targets/ospfv2_base.hh"
#include "xrl/targets/ospfv3_base.hh"

#include "ospf.hh"

class XrlOspfV2Target : XrlOspfv2TargetBase {
 public:
    XrlOspfV2Target(XrlRouter *r, Ospf<IPv4>& ospf, XrlIO& io);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Set router id
     */
    XrlCmdError ospfv2_0_1_set_router_id(
	// Input values,
	const uint32_t&	id);

 private:
    Ospf<IPv4>& _ospf;
    XrlIO& _xrl_io;
};

class XrlOspfV3Target : XrlOspfv3TargetBase {
 public:
    XrlOspfV3Target(XrlRouter *r,
		    Ospf<IPv4>& ospf_ipv4,
		    Ospf<IPv6>& ospf_ipv6,
		    XrlIO& io);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Set router id
     */
    XrlCmdError ospfv3_0_1_set_router_id(
	// Input values,
	const uint32_t&	id);

 private:
    Ospf<IPv4>& _ospf_ipv4;
    Ospf<IPv6>& _ospf_ipv6;
    XrlIO& _xrl_io;
};
#endif // __OSPF_XRL_TARGET_HH__
