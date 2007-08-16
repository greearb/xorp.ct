// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/cli/xrl_cli_node.cc,v 1.30 2007/02/16 22:45:29 pavlin Exp $"

#include "cli_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"
#include "libxorp/token.hh"

#include "xrl_cli_node.hh"
#include "cli_node.hh"


XrlCliNode::XrlCliNode(EventLoop&	eventloop,
		       const string&	class_name,
		       const string&	finder_hostname,
		       uint16_t		finder_port,
		       const string&	finder_target,
		       CliNode&		cli_node)
    : XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
    XrlCliTargetBase(&xrl_router()),
      _eventloop(eventloop),
      _cli_node(cli_node),
      _xrl_cli_processor_client(&xrl_router()),
      _is_finder_alive(false)
{
    _cli_node.set_send_process_command_callback(
	callback(this, &XrlCliNode::send_process_command));

    UNUSED(finder_target);
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

//
// Finder-related events
//
/**
 * Called when Finder connection is established.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlCliNode::finder_connect_event()
{
    _is_finder_alive = true;
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlCliNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    _is_finder_alive = false;

    stop_cli();
}

XrlCmdError
XrlCliNode::common_0_1_get_target_name(
    // Output values, 
    string&	name)
{
    name = xrl_router().class_name();
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
XrlCliNode::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&	reason)
{
    // XXX: Default to PROC_READY status because this probably won't be used.
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::common_0_1_shutdown()
{
    // TODO: XXX: PAVPAVPAV: implement it!!
    return XrlCmdError::COMMAND_FAILED("Not implemented yet");
}

XrlCmdError
XrlCliNode::cli_manager_0_1_enable_cli(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_cli();
    else
	ret_value = disable_cli();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = "Failed to enable CLI";
	else
	    error_msg = "Failed to disable CLI";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_start_cli()
{
    if (start_cli() != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED("Failed to start CLI");
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_stop_cli()
{
    string error_msg;

    if (stop_cli() != XORP_OK) {
	error_msg = c_format("Failed to stop CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_enable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr)
{
    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    cli_node().add_enable_cli_access_from_subnet(IPvXNet(subnet_addr));
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_enable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr)
{
    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    cli_node().add_enable_cli_access_from_subnet(IPvXNet(subnet_addr));
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_enable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr)
{
    string error_msg;

    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    if (cli_node().delete_enable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	error_msg = c_format("Failed to delete enabled CLI access from subnet %s",
			     cstring(subnet_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_enable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr)
{
    string error_msg;

    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    if (cli_node().delete_enable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	error_msg = c_format("Failed to delete enabled CLI access from subnet %s",
			     cstring(subnet_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_disable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr)
{
    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    cli_node().add_disable_cli_access_from_subnet(IPvXNet(subnet_addr));
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_add_disable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr)
{
    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    cli_node().add_disable_cli_access_from_subnet(IPvXNet(subnet_addr));
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_disable_cli_access_from_subnet4(
    // Input values, 
    const IPv4Net&	subnet_addr)
{
    string error_msg;

    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    if (cli_node().delete_disable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	error_msg = c_format("Failed to delete disabled CLI access from subnet %s",
			     cstring(subnet_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_disable_cli_access_from_subnet6(
    // Input values, 
    const IPv6Net&	subnet_addr)
{
    string error_msg;

    //
    // XXX: we don't need to verify the address family, because we are
    // handling both IPv4 and IPv6.
    //

    if (cli_node().delete_disable_cli_access_from_subnet(IPvXNet(subnet_addr))
	!= XORP_OK) {
	error_msg = c_format("Failed to delete disabled CLI access from subnet %s",
			     cstring(subnet_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
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
    const bool&		is_command_processor)
{
    string error_msg;
    
    if (cli_node().add_cli_command(processor_name,
				   command_name,
				   command_help,
				   is_command_cd,
				   command_cd_prompt,
				   is_command_processor,
				   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCliNode::cli_manager_0_1_delete_cli_command(
    // Input values, 
    const string&	processor_name, 
    const string&	command_name
    )
{
    string error_msg;

    if (cli_node().delete_cli_command(processor_name,
				      command_name,
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

//
// The CLI client-side (i.e., the CLI sending XRLs)
//

//
// Send a request to a CLI processor
//
void
XrlCliNode::send_process_command(const string& target,
				 const string& processor_name,
				 const string& cli_term_name,
				 uint32_t cli_session_id,
				 const vector<string>& command_global_name,
				 const vector<string>& command_argv)
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    string command_line = token_vector2line(command_global_name);
    string args_line = token_vector2line(command_argv);

    //
    // Send the request
    //
    _xrl_cli_processor_client.send_process_command(
	target.c_str(),
	processor_name,
	cli_term_name,
	cli_session_id,
	command_line,
	args_line,
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
    if (xrl_error == XrlError::OKAY()) {
	cli_node().recv_process_command_output(processor_name,
					       cli_term_name,
					       cli_session_id,
					       command_output);
	return;
    }

    //
    // TODO: if the command failed because of transport error,
    // then we should retransmit it.
    //
    XLOG_ERROR("Failed to process a command: %s", xrl_error.str().c_str());
}
