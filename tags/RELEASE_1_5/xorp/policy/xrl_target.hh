// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/xrl_target.hh,v 1.12 2008/01/04 03:17:14 pavlin Exp $

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
        const string&   policies);
	
    XrlCmdError policy_0_1_export(
        // Input values,
        const string&   protocol,
        const string&   policies);
   
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

private:
    PolicyTarget&   _policy_target;
};

#endif // __POLICY_XRL_TARGET_HH__
