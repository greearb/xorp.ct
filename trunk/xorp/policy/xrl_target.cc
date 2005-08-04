// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/policy/xrl_target.cc,v 1.6 2005/07/15 02:27:07 abittau Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "policy_module.h"
#include "xrl_target.hh"
#include "common/policy_utils.hh"
#include "libxorp/status_codes.h"

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
    try {
	uint64_t o = strtoll(order.c_str(), NULL, 10);
	_policy_target.create_term(policy, o, term);
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
    try {
	uint64_t o = strtoll(order.c_str(), NULL, 10);
	_policy_target.update_term_block(policy, term, block, o, statement);
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
				   const string&   policies) 
{
    try {
	_policy_target.update_import(protocol,policies);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Import of " + protocol + 
					   " failed: " + e.str());
    }

    return XrlCmdError::OKAY();
}	
 
XrlCmdError 
XrlPolicyTarget::policy_0_1_export(const string&   protocol,
				   const string&   policies)
{
    try {
	_policy_target.update_export(protocol,policies);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Export of " + protocol + 
					   " failed: " + e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPolicyTarget::policy_0_1_add_varmap(const string& protocol,
				       const string& variable,
				       const string& type,
				       const string& access)
{
    try {
	_policy_target.add_varmap(protocol, variable, type, access);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Adding varmap failed for protocol: " 
					   + protocol + " var: " + variable
					   + " :" + e.str());
    }

    return XrlCmdError::OKAY();

}

XrlCmdError 
XrlPolicyTarget::policy_0_1_dump_state(const uint32_t& id,
				       string& state)
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
