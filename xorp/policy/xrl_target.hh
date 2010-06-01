// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/policy/xrl_target.hh,v 1.16 2008/10/02 21:58:02 bms Exp $

#ifndef __POLICY_XRL_TARGET_HH__
#define __POLICY_XRL_TARGET_HH__

#include "libxipc/xrl_std_router.hh"
#include "xrl/targets/policy_base.hh"
#include "policy_target.hh"

/**
 * @short The XORP Xrl target.
 *
 * This class simply forwards calls to the PolicyTarget.
 */
class XrlPolicyTarget : public XrlPolicyTargetBase {
public:
    /**
     * @param r XrlRouter to use.
     * @param ptarget the main PolicyTarget.
     */
    XrlPolicyTarget(XrlStdRouter* r, PolicyTarget& ptarget);
    
    XrlCmdError common_0_1_get_target_name(
        // Output values,
        string& name);

    XrlCmdError common_0_1_get_version(
        // Output values,
        string& version);

    XrlCmdError common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason);

    XrlCmdError common_0_1_shutdown();

    virtual XrlCmdError common_0_1_startup() {
	return XrlCmdError::OKAY();
    }

    XrlCmdError policy_0_1_create_term(
        // Input values,
        const string&   policy,
	const string&   order,
        const string&   term);

    XrlCmdError policy_0_1_delete_term(
        // Input values,
        const string&   policy,
        const string&   term);
   
    XrlCmdError policy_0_1_update_term_block(
        // Input values,
        const string&   policy,
        const string&   term,
	const uint32_t& block,
        const string&   order,
        const string&   statement);
    
    XrlCmdError policy_0_1_create_policy(
        // Input values,
        const string&   policy);

    XrlCmdError policy_0_1_delete_policy(
        // Input values,
        const string&   policy);

    XrlCmdError policy_0_1_create_set(
        // Input values,
        const string&   set); 

    XrlCmdError policy_0_1_update_set(
        // Input values,
	const string&   type,
        const string&   set,
        const string&   elements);
    
    XrlCmdError policy_0_1_delete_set(
        // Input values,
        const string&   set);

    XrlCmdError policy_0_1_add_to_set(
	// Input values,
	const string&	type,
	const string&	set,
	const string&	element);

    XrlCmdError policy_0_1_delete_from_set(
	// Input values,
	const string&	type,
	const string&	set,
	const string&	element);
    
    XrlCmdError policy_0_1_done_global_policy_conf();

    XrlCmdError policy_0_1_import(
        // Input values,
        const string&   protocol,
        const string&   policies,
	const string&   modifier);
	
    XrlCmdError policy_0_1_export(
        // Input values,
        const string&   protocol,
        const string&   policies,
	const string&   modifier);
   
    XrlCmdError policy_0_1_add_varmap(
        // Input values,
        const string&   protocol,
        const string&   variable,
        const string&   type,
        const string&   access,
	const uint32_t& id);
   
    XrlCmdError policy_0_1_dump_state(
        // Input values,
        const uint32_t& id,
        // Output values,
        string& state);
   
    XrlCmdError policy_0_1_set_proto_target(
        // Input values,
        const string&   protocol,
        const string&   target);
   
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
        // Input values,
        const string&   target_class,
        const string&   target_instance);

    XrlCmdError finder_event_observer_0_1_xrl_target_death(
        // Input values,
        const string&   target_class,
        const string&   target_instance);

    XrlCmdError cli_processor_0_1_process_command(
        // Input values,
        const string&   processor_name,
        const string&   cli_term_name,
        const uint32_t& cli_session_id,
        const string&   command_name,
        const string&   command_args,
        // Output values,
        string&		ret_processor_name,
        string&		ret_cli_term_name,
        uint32_t&       ret_cli_session_id,
        string& ret_command_output);

private:
    PolicyTarget&   _policy_target;
};

#endif // __POLICY_XRL_TARGET_HH__
