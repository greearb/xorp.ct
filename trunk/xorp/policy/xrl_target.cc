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

#ident "$XORP: xorp/policy/xrl_target.cc,v 1.17 2008/08/06 08:23:25 abittau Exp $"

#include "policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/status_codes.h"

#include "policy/common/policy_utils.hh"

#include "xrl_target.hh"


using namespace policy_utils;

XrlPolicyTarget::XrlPolicyTarget(XrlStdRouter* r, PolicyTarget& ptarget) : 
	    XrlPolicyTargetBase(r),
	    _policy_target(ptarget)
{
}


XrlCmdError 
XrlPolicyTarget::common_0_1_get_target_name(
        // Output values,
        string& name) 
{
    name = PolicyTarget::policy_target_name;
    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::common_0_1_get_version(
        // Output values,
        string& version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason) 
{
    if(_policy_target.running()) {
        status = PROC_READY;
	reason = "running";
    }
    else {
	status = PROC_SHUTDOWN;
	reason = "dying";
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::common_0_1_shutdown()
{
    _policy_target.shutdown();
    return XrlCmdError::OKAY();
}


XrlCmdError 
XrlPolicyTarget::policy_0_1_create_term(const string&   policy,
					const string&   order,
				        const string&   term)
{
    ConfigNodeId config_node_id(ConfigNodeId::ZERO());

    try {
	config_node_id.copy_in(order);
    } catch (const InvalidString& e) {
        return XrlCmdError::COMMAND_FAILED("Create of policy " + policy
					   + " term " + term + " failed "
					   + "because of invalid node ID "
					   + "\"" + order + "\" : "
					   + e.str());
    }

    try {
	_policy_target.create_term(policy, config_node_id, term);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("create_term failed: " + e.str());
    }
    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::policy_0_1_delete_term(const string&   policy,
					const string&   term)
{
    try {
	_policy_target.delete_term(policy,term);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("delete_term failed: " + e.str());
    }
    
    return XrlCmdError::OKAY();
}	
    

XrlCmdError 
XrlPolicyTarget::policy_0_1_update_term_block(const string&   policy,
					      const string&   term,
					      const uint32_t& block,
					      const string&   order,
					      const string&   statement)
{
    ConfigNodeId config_node_id(ConfigNodeId::ZERO());

    try {
	config_node_id.copy_in(order);
    } catch (const InvalidString& e) {
        return XrlCmdError::COMMAND_FAILED("Update of policy " + policy
					   + " term " + term + " failed "
					   + "because of invalid node ID "
					   + "\"" + order + "\" : "
					   + e.str());
    }
    try {
	_policy_target.update_term_block(policy, term, block, config_node_id,
					 statement);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Update of policy " + policy
					   + " term " + term + " failed: "
					   + e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::policy_0_1_create_policy(const string&   policy)
{
    
    try {
	_policy_target.create_policy(policy);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("create_policy: " + e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::policy_0_1_delete_policy(const string&   policy)
{

    try {
	_policy_target.delete_policy(policy);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("delete_policy: " + e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::policy_0_1_create_set(const string&   set)
{

    try {
	_policy_target.create_set(set);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("create_set: " + e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::policy_0_1_update_set(const string&   type,
				       const string&   set,
				       const string&   elements)
{
    try {
	_policy_target.update_set(type, set, elements);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("update_set: " + e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::policy_0_1_delete_set(const string&   set)
{

    try {
	_policy_target.delete_set(set);
    } catch(const PolicyException& e ) {
        return XrlCmdError::COMMAND_FAILED("delete_set: " + e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError
XrlPolicyTarget::policy_0_1_add_to_set(
    // Input values,
    const string&	type,
    const string&	set,
    const string&	element)
{
    try {
	_policy_target.add_to_set(type, set, element);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("add_to_set: " + e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPolicyTarget::policy_0_1_delete_from_set(
    // Input values,
    const string&	type,
    const string&	set,
    const string&	element)
{
    try {
	_policy_target.delete_from_set(type, set, element);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("delete_from_set: " + e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlPolicyTarget::policy_0_1_done_global_policy_conf()
{
    try {
	_policy_target.commit(0);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Policy configuration failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlPolicyTarget::policy_0_1_import(const string&   protocol,
				   const string&   policies,
				   const string&   modifier) 
{
    try {
	_policy_target.update_import(protocol, policies, modifier);
    } catch (const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Import of " + protocol + 
					   " failed: " + e.str());
    }

    return XrlCmdError::OKAY();
}	
 
XrlCmdError 
XrlPolicyTarget::policy_0_1_export(const string&   protocol,
				   const string&   policies,
				   const string&   modifier)
{
    try {
	_policy_target.update_export(protocol, policies, modifier);
    } catch (const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Export of " + protocol + 
					   " failed: " + e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPolicyTarget::policy_0_1_add_varmap(const string& protocol,
				       const string& variable,
				       const string& type,
				       const string& access,
				       const uint32_t& id)
{
    try {
	_policy_target.add_varmap(protocol, variable, type, access, id);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Adding varmap failed for protocol: " 
					   + protocol + " var: " + variable
					   + " :" + e.str());
    }

    return XrlCmdError::OKAY();

}

XrlCmdError 
XrlPolicyTarget::policy_0_1_dump_state(const uint32_t& id, string& state)
{
    try {
        state = _policy_target.dump_state(id);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Unable to dump state, id: " 
					   + to_str(id));
    }
    return XrlCmdError::OKAY();
}	

XrlCmdError
XrlPolicyTarget::policy_0_1_set_proto_target(const string& protocol,
					     const string& target)
{
    _policy_target.set_proto_target(protocol, target);
    return XrlCmdError::OKAY();
}


XrlCmdError 
XrlPolicyTarget::finder_event_observer_0_1_xrl_target_birth(
			    const string&   target_class,
			    const string&   target_instance)
{
    try {
	_policy_target.birth(target_class,target_instance);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Birth announce of " + 
					   target_class + " failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError 
XrlPolicyTarget::finder_event_observer_0_1_xrl_target_death(
			    const string&   target_class,
			    const string&   target_instance)
{
    try {
	_policy_target.death(target_class,target_instance);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Death announce of " + 
					   target_class + " failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}	

XrlCmdError
XrlPolicyTarget::cli_processor_0_1_process_command(
				// Input values,
				const string&   processor_name,
				const string&   cli_term_name,
				const uint32_t& cli_session_id,
				const string&   command_name,
				const string&   /* command_args */,
				// Output values,
				string&         ret_processor_name,
				string&         ret_cli_term_name,
				uint32_t&       ret_cli_session_id,
				string&		ret_command_output)
{
    try {
	ret_processor_name = processor_name;
	ret_cli_term_name  = cli_term_name;
	ret_cli_session_id = cli_session_id;

	ret_command_output = _policy_target.test_policy(command_name);

    } catch (const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}
