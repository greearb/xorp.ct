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

// $XORP: xorp/mibs/xorp_if_mib_xrl_target.hh,v 1.2 2003/05/29 23:50:13 hodson Exp $

#ifndef __MIBS_XORP_IF_MIB_XRL_TARGET_HH__
#define __MIBS_XORP_IF_MIB_XRL_TARGET_HH___

#include "xrl/targets/xorp_if_mib_base.hh"

class XorpIfMib;

class XrlXorpIfMibTarget :  XrlXorpIfMibTargetBase {
public:
    XrlXorpIfMibTarget (XrlRouter *r, XorpIfMib& xorp_if_mib);

    XrlCmdError common_0_1_get_target_name(string& name) {
	name = "xorp_if_mib";
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

    XrlCmdError common_0_1_shutdown(); 
    
    /**
     *  Load a MIB module
     *  
     *  @param mod_name the mib module file name (without extension)
     *  
     *  @param abs_path absolute path to the module file
     */
    XrlCmdError xorp_if_mib_0_1_load_mib(
	// Input values, 
	const string&	mod_name, 
	const string&	abs_path, 
	// Output values, 
	uint32_t&	mib_index);

    /**
     *  Unload a MIB module
     *  
     *  @param unloaded true if mib not loaded or successfully unloaded
     */
    XrlCmdError xorp_if_mib_0_1_unload_mib(
	// Input values, 
	const uint32_t&	mib_index, 
	// Output values, 
	bool&	unloaded);


private:
    /**
    * The main object that all requests go to.
    */
    XorpIfMib& _xorp_if_mib;

};

#endif // __MIBS_XORP_IF_MIB_XRL_TARGET_HH__
