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

#ident "$Header$"

#include "config.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_trap.h>

#include "bgp4_mib_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "xorpevents.hh"
#include "libxipc/xrl_std_router.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_xrl_target.hh"

static oid snmptrap_oid[] = { SNMP_OID_SNMPMODULES, 1, 1, 5, 1, 0 };
static oid bgp_established_trap_oid[] = { SNMP_OID_MIB2, 15, 7, 1 };
// static oid bgp_backward_transition_trap_oid[] = { SNMP_OID_MIB2, 15, 7, 2 };

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
    snmp_set_var_value(&snmptrap_var, (u_char *) bgp_established_trap_oid, 
	OID_LENGTH(bgp_established_trap_oid));
    snmptrap_var.type = ASN_OBJECT_ID;
    snmptrap_var.next_variable = &bgp_last_err_var;

    // Now the objects required for this specific trap
    // bgpPeerLastError
    memset(&bgp_last_err_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&bgp_last_err_var, bgp_peer_last_error_oid,
                       OID_LENGTH(bgp_peer_last_error_oid));
    snmp_set_var_value(&bgp_last_err_var, 
		       (const u_char *) bgp_last_error.c_str(), 
		       bgp_last_error.size());
    bgp_last_err_var.type = ASN_OCTET_STR;
    bgp_last_err_var.next_variable = &bgp_state_var;

    // bgpPeerState
    memset(&bgp_state_var, 0, sizeof(netsnmp_variable_list));
    snmp_set_var_objid(&bgp_state_var, bgp_peer_state_oid,
                       OID_LENGTH(bgp_peer_state_oid));
    snmp_set_var_value(&bgp_state_var, (const u_char *) &bgp_state, 
		       sizeof(uint32_t));
    bgp_state_var.type = ASN_INTEGER;
    bgp_state_var.next_variable = NULL;

    send_v2trap(&snmptrap_var);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpMibTarget::bgp_mib_traps_0_1_send_bgp_backward_transition_trap(
    // Input values, 
    const string&	/* bgp_last_error */, 
    const uint32_t&	/* bgp_state */)
{

    send_v2trap(NULL);
    return XrlCmdError::OKAY();
}


    
