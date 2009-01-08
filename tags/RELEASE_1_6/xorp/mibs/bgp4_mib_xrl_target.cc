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

#ident "$XORP: xorp/mibs/bgp4_mib_xrl_target.cc,v 1.14 2008/10/02 21:57:41 bms Exp $"


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_trap.h>
#include "fixconfigs.h"

#include "bgp4_mib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "xorpevents.hh"
#include "libxipc/xrl_std_router.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_xrl_target.hh"

static oid snmptrap_oid[] = { SNMP_OID_SNMPMODULES, 1, 1, 4, 1, 0 };
static oid bgp_established_trap_oid[] = { SNMP_OID_MIB2, 15, 7, 1 };
static oid bgp_backward_transition_trap_oid[] = { SNMP_OID_MIB2, 15, 7, 2 };

static oid bgp_peer_last_error_oid[] = {SNMP_OID_MIB2, 15, 3, 1, 14 };
static oid bgp_peer_state_oid[] = {SNMP_OID_MIB2, 15, 3, 1, 2 };



XrlBgpMibTarget::XrlBgpMibTarget(XrlRouter *r, BgpMib& bgp_mib)
	: XrlBgp4MibTargetBase(r), _bgp_mib(bgp_mib)
{
}

XrlCmdError
XrlBgpMibTarget::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&	/* reason */)
{
    status = PROC_READY;
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpMibTarget::bgp_mib_traps_0_1_send_bgp_established_trap(
    // Input values, 
    const string&	 bgp_last_error, 
    const uint32_t&	 bgp_state )
{
    netsnmp_variable_list snmptrap_var, bgp_last_err_var, bgp_state_var;

    BgpMib & bgp_mib = BgpMib::the_instance();
    DEBUGMSGTL((bgp_mib.name(), "send_bgp_established_trap %s %d\n",
	bgp_last_error.c_str(), bgp_state));

    // Initialize SNMPv2 required variables
    // Skipping sysUptime.0 (the agent takes care of that)
    
    // snmpTrapOID.0 = bgpEstablishedTrap
    memset(&snmptrap_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&snmptrap_var, snmptrap_oid,
                       OID_LENGTH(snmptrap_oid));
    snmptrap_var.type = ASN_OBJECT_ID;
    snmp_set_var_value(&snmptrap_var, (u_char *) bgp_established_trap_oid, 
	sizeof(bgp_established_trap_oid));
    snmptrap_var.next_variable = &bgp_last_err_var;

    // Now the objects required for this specific trap
    // bgpPeerLastError
    memset(&bgp_last_err_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&bgp_last_err_var, bgp_peer_last_error_oid,
                       OID_LENGTH(bgp_peer_last_error_oid));
    bgp_last_err_var.type = ASN_OCTET_STR;
    snmp_set_var_value(&bgp_last_err_var, 
		       (const u_char *) bgp_last_error.c_str(), 
		       bgp_last_error.size());
    bgp_last_err_var.next_variable = &bgp_state_var;

    // bgpPeerState
    memset(&bgp_state_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&bgp_state_var, bgp_peer_state_oid,
                       OID_LENGTH(bgp_peer_state_oid));
    bgp_state_var.type = ASN_INTEGER;
    snmp_set_var_value(&bgp_state_var, (const u_char *) &bgp_state, 
		       sizeof(uint32_t));
    bgp_state_var.next_variable = NULL;

    send_v2trap(&snmptrap_var);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpMibTarget::bgp_mib_traps_0_1_send_bgp_backward_transition_trap(
    // Input values, 
    const string&	bgp_last_error, 
    const uint32_t&	bgp_state)
{
    netsnmp_variable_list snmptrap_var, bgp_last_err_var, bgp_state_var;

    BgpMib & bgp_mib = BgpMib::the_instance();
    DEBUGMSGTL((bgp_mib.name(), "send_bgp_backward_transition_trap %s %d\n",
	bgp_last_error.c_str(), bgp_state));

    // Initialize SNMPv2 required variables
    // Skipping sysUptime.0 (the agent takes care of that)
    
    // snmpTrapOID.0 = bgpBackwardTransitionTrap
    memset(&snmptrap_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&snmptrap_var, snmptrap_oid,
                       OID_LENGTH(snmptrap_oid));
    snmptrap_var.type = ASN_OBJECT_ID;
    snmp_set_var_value(&snmptrap_var, 
	(u_char *) bgp_backward_transition_trap_oid, 
	sizeof(bgp_backward_transition_trap_oid));
    snmptrap_var.next_variable = &bgp_last_err_var;

    // Now the objects required for this specific trap
    // bgpPeerLastError
    memset(&bgp_last_err_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&bgp_last_err_var, bgp_peer_last_error_oid,
                       OID_LENGTH(bgp_peer_last_error_oid));
    bgp_last_err_var.type = ASN_OCTET_STR;
    snmp_set_var_value(&bgp_last_err_var, 
		       (const u_char *) bgp_last_error.c_str(), 
		       bgp_last_error.size());
    bgp_last_err_var.next_variable = &bgp_state_var;

    // bgpPeerState
    memset(&bgp_state_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&bgp_state_var, bgp_peer_state_oid,
                       OID_LENGTH(bgp_peer_state_oid));
    bgp_state_var.type = ASN_INTEGER;
    snmp_set_var_value(&bgp_state_var, (const u_char *) &bgp_state, 
		       sizeof(uint32_t));
    bgp_state_var.next_variable = NULL;

    send_v2trap(&snmptrap_var);

    return XrlCmdError::OKAY();
}


    
