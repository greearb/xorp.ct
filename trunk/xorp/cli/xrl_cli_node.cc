// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/cli/xrl_cli_node.cc,v 1.2 2003/01/23 11:14:23 pavlin Exp $"

#include "libxipc/xrl_router.hh"

#include "cli_module.h"
#include "cli_private.hh"
#include "xrl_cli_node.hh"
#include "cli_node.hh"


XrlCliNode::XrlCliNode(XrlRouter* r, CliNode& cli_node)
	: XrlCliTargetBase(r),
	  XrlCliProcessorV0p1Client(r),
	  _cli_node(cli_node)
{
    _cli_node.set_send_process_command_callback(callback(this,
							 &XrlCliNode::send_process_command));
}


//
// XrlCliNode front-end interface
//
int
XrlCliNode::enable_cli()
{
    int ret_code = XORP_OK;
    
    cli_node().enable();
    
    return (ret_code);
}

int
XrlCliNode::disable_cli()
{
    int ret_code = XORP_OK;
    
    cli_node().disable();
    
    return (ret_code);
}

int
XrlCliNode::start_cli()
{
    int ret_code = XORP_OK;
    
    if (cli_node().start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlCliNode::stop_cli()
{
    int ret_code = XORP_OK;
    
    if (cli_node().stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

XrlCmdError
XrlCliNode::common_0_1_get_target_name(
    // Output values, 
    string&	name)
{
    name = XrlCliTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::common_0_1_get_version(
    // Output values, 
    string&	version)
{
    version = XORP_MODULE_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_enable_cli(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (enable_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_disable_cli(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (disable_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_start_cli(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (start_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_stop_cli(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (stop_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_enable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    cli_node().add_enable_cli_access_from_subnet(IPvXNet(subnet_addr));
    fail = false;
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_enable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    cli_node().add_enable_cli_access_from_subnet(IPvXNet(subnet_addr));
    fail = false;
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_enable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (cli_node().delete_enable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_enable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (cli_node().delete_enable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_disable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    cli_node().add_disable_cli_access_from_subnet(IPvXNet(subnet_addr));
    fail = false;
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_disable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    cli_node().add_disable_cli_access_from_subnet(IPvXNet(subnet_addr));
    fail = false;
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_disable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (cli_node().delete_disable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_disable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (cli_node().delete_disable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlCliNode::cli_manager_0_1_add_cli_command(
    // Input values, 
    const string&	processor_name, 
    const string&	command_name, 
    const string&	command_help, 
    const bool&		is_command_cd, 
    const string&	command_cd_prompt, 
    const bool&		is_command_processor, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    cli_node().add_cli_command(processor_name,
			       command_name,
			       command_help,
			       is_command_cd,
			       command_cd_prompt,
			       is_command_processor,
			       fail,
			       reason);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_cli_command(
    // Input values, 
    const string& , //	processor_name, 
    const string& , //	command_name, 
    // Output values, 
    bool& , //		fail, 
    string& // 		reason
    )
{
    // TODO: implement it
    return XrlCmdError::OKAY();
}

//
// The CLI client-side (i.e., the CLI sending XRLs)
//

//
// Send a request to a CLI processor
//
void
XrlCliNode::send_process_command(const char *target,
				 const string& processor_name,
				 const string& cli_term_name,
				 uint32_t cli_session_id,
				 const string& command_name,
				 const string& command_args)
{
    //
    // Send the request
    //
    XrlCliProcessorV0p1Client::send_process_command(
	target,
	processor_name,
	cli_term_name,
	cli_session_id,
	command_name,
	command_args,
	callback(this, &XrlCliNode::recv_process_command_output));
    
    return;
}

//
// Process the response of a command processed by a remote CLI processor
//
void
XrlCliNode::recv_process_command_output(const XrlError& xrl_error,
					const string *processor_name,
					const string *cli_term_name,
					const uint32_t *cli_session_id,
					const string *command_output)
{
    if (xrl_error != XrlError::OKAY())
	return;
    
    cli_node().recv_process_command_output(processor_name,
					   cli_term_name,
					   cli_session_id,
					   command_output);
    
    return;
}
