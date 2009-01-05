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

// $XORP: xorp/mibs/bgp4_mib_xrl_target.hh,v 1.12 2008/10/02 21:57:41 bms Exp $

#ifndef __MIBS_BGP4_MIB_XRL_TARGET_HH__
#define __MIBS_BGP4_MIB_XRL_TARGET_HH__

#include "xrl/targets/bgp4_mib_base.hh"

class BgpMib;

/**
 * This class implements the Xrl client for the BGP MIB.  It is contained by
 * @ref BgpMib.
 *
 * @short Xrl client for BGP MIB
 */
class XrlBgpMibTarget :  XrlBgp4MibTargetBase {
public:
    XrlBgpMibTarget (XrlRouter *r, BgpMib& bgp_mib);

    /**
     * Get target name
     */
    XrlCmdError common_0_1_get_target_name(string& name) {
	name = "bgp4_mib";
	return XrlCmdError::OKAY();
    }

    /**
     * Get version
     */
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
    /**
     * Not to be used.  Loading and unloading of mib modules is done via the
     * xorp_if_mib module:
     * finder://xorp_if_mib/xorp_if_mib/0.1/unload_mib?mib_index:u32
     *
     * @short not to be used
     */
    XrlCmdError common_0_1_shutdown()
    {
	return XrlCmdError::COMMAND_FAILED("Use finder://xorp_if_mib/"
	    "xorp_if_mib/0.1/unload_mib?mib_index:u32 instead");
    }

    /**
     *  Send bgpEstablished trap
     *  
     *  @param bgp_last_error the last error code and subcode seen by this peer
     *  on this connection.  If no error has occurred, this field is zero.
     *  Otherwise, the first byte of this two byte OCTET STRING contains the
     *  error code, and the second byte contains the subcode.
     *
     *  @param the BGP peer connection state: idel, connect, active, opensent,
     *  openconfirm or established.
     */
    XrlCmdError bgp_mib_traps_0_1_send_bgp_established_trap(
	// Input values, 
	const string&	bgp_last_error, 
	const uint32_t&	bgp_state);

    /**
     *  Send bgpBackwardTransition trap
     *
     *  @param bgp_last_error the last error code and subcode seen by this peer
     *  on this connection.  If no error has occurred, this field is zero.
     *  Otherwise, the first byte of this two byte OCTET STRING contains the
     *  error code, and the second byte contains the subcode.
     *
     *  @param the BGP peer connection state: idel, connect, active, opensent,
     *  openconfirm or established.
     */
    XrlCmdError bgp_mib_traps_0_1_send_bgp_backward_transition_trap(
	// Input values, 
	const string&	bgp_last_error, 
	const uint32_t&	bgp_state);
    
private:
    /**
    * The main object that all requests go to.
    */
    BgpMib& _bgp_mib;

};

#endif // __MIBS_BGP4_MIB_XRL_TARGET_HH__
