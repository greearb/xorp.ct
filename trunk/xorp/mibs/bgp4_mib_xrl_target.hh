// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/mibs/bgp4_mib_xrl_target.hh,v 1.2 2003/05/20 00:58:34 jcardona Exp $

#ifndef __MIBS_BGP4_MIB_XRL_TARGET_HH__
#define __MIBS_BGP4_MIB_XRL_TARGET_HH__

#include "xrl/targets/bgp4_mib_base.hh"

class BgpMib;

class XrlBgpMibTarget :  XrlBgp4MibTargetBase {
public:
    XrlBgpMibTarget (XrlRouter *r, BgpMib& bgp_mib);

    XrlCmdError common_0_1_get_target_name(string& name) {
	name = "bgp4_mib";
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_get_version(string& version) {
	version = "0.1";
	return XrlCmdError::OKAY();
    }

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(
				      // Output values,
				      uint32_t& status,
				      string&	reason);

    XrlCmdError common_0_1_shutdown()
    {
	return XrlCmdError::COMMAND_FAILED("Not Implemented");
    }
    
private:
    /**
    * The main object that all requests go to.
    */
    BgpMib& _bgp_mib;

};

#endif // __MIBS_BGP4_MIB_XRL_TARGET_HH__
