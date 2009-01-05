// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/mibs/xorp_if_mib_xrl_target.hh,v 1.10 2008/10/02 21:57:42 bms Exp $

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
