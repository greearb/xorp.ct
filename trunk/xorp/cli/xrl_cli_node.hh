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

// $XORP: xorp/cli/xrl_cli_node.hh,v 1.10 2003/12/20 01:43:34 pavlin Exp $

#ifndef __CLI_XRL_CLI_NODE_HH__
#define __CLI_XRL_CLI_NODE_HH__

#include <string>

#include "libxorp/xlog.h"
#include "libxipc/xrl_router.hh"
#include "xrl/targets/cli_base.hh"
#include "xrl/interfaces/cli_processor_xif.hh"
#include "cli_node.hh"

//
// TODO: XrlCliProcessorV1p0Client should NOT be a base class. Temp. solution..
//
class XrlCliNode : public XrlCliTargetBase {
public:
    XrlCliNode(XrlRouter* xrl_router, CliNode& cli_node);
    virtual ~XrlCliNode() {}

    //
    // XrlCliNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();
    
protected:
    //
    // Methods to be implemented by derived classes supporting this interface.
    //

    /**
     *  Get name of Xrl Target
     */
    virtual XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    virtual XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    virtual XrlCmdError common_0_1_get_status(
	// Output values,
        uint32_t& status,
	string&	reason);

    /**
     *  shutdown cleanly
     */
    virtual XrlCmdError common_0_1_shutdown();

    /**
     *  Enable/disable/start/stop the CLI.
     *
     *  @param enable if true, then enable the CLI, otherwise disable it.
     */
    virtual XrlCmdError cli_manager_0_1_enable_cli(
	// Input values,
	const bool&	enable);

    virtual XrlCmdError cli_manager_0_1_start_cli();

    virtual XrlCmdError cli_manager_0_1_stop_cli();

    /**
     *  Add a subnet address to the list of subnet addresses enabled for CLI
     *  access. This method can be called more than once to add a number of
     *  subnet addresses.
     *  
     *  @param subnet_addr the subnet address to add.
     */
    virtual XrlCmdError cli_manager_0_1_add_enable_cli_access_from_subnet4(
	// Input values, 
	const IPv4Net&	subnet_addr);

    virtual XrlCmdError cli_manager_0_1_add_enable_cli_access_from_subnet6(
	// Input values, 
	const IPv6Net&	subnet_addr);

    /**
     *  Delete a subnet address from the list of subnet addresses enabled for
     *  CLI access.
     *  
     *  @param subnet_addr the subnet address to delete.
     */
    virtual XrlCmdError cli_manager_0_1_delete_enable_cli_access_from_subnet4(
	// Input values, 
	const IPv4Net&	subnet_addr);

    virtual XrlCmdError cli_manager_0_1_delete_enable_cli_access_from_subnet6(
	// Input values, 
	const IPv6Net&	subnet_addr);

    /**
     *  Add a subnet address to the list of subnet addresses disabled for CLI
     *  access. This method can be called more than once to add a number of
     *  subnet addresses.
     *  
     *  @param subnet_addr the subnet address to add.
     */
    virtual XrlCmdError cli_manager_0_1_add_disable_cli_access_from_subnet4(
	// Input values, 
	const IPv4Net&	subnet_addr);

    virtual XrlCmdError cli_manager_0_1_add_disable_cli_access_from_subnet6(
	// Input values, 
	const IPv6Net&	subnet_addr);

    /**
     *  Delete a subnet address from the list of subnet addresses disabled for
     *  CLI access.
     *  
     *  @param subnet_addr the subnet address to delete.
     */
    virtual XrlCmdError cli_manager_0_1_delete_disable_cli_access_from_subnet4(
	// Input values, 
	const IPv4Net&	subnet_addr);

    virtual XrlCmdError cli_manager_0_1_delete_disable_cli_access_from_subnet6(
	// Input values, 
	const IPv6Net&	subnet_addr);

    /**
     *  Add a CLI command to the CLI manager
     *  
     *  @param processor_name the name of the module that will process that
     *  command.
     *  
     *  @param command_name the name of the command to add.
     *  
     *  @param command_help the help for the command to add.
     *  
     *  @param is_command_cd is true, the string that will replace the CLI
     *  prompt after we "cd" to that level of the CLI command-tree.
     *  
     *  @param command_cd_prompt if
     *  
     *  @param is_command_processor if true, this is a processing command that
     *  would be performed by processor_name.
     */
    virtual XrlCmdError cli_manager_0_1_add_cli_command(
	// Input values, 
	const string&	processor_name, 
	const string&	command_name, 
	const string&	command_help, 
	const bool&	is_command_cd, 
	const string&	command_cd_prompt, 
	const bool&	is_command_processor);

    /**
     *  Delete a CLI command from the CLI manager
     *  
     *  @param processor_name the name of the module that sends the request.
     *  
     *  @param command_name the name of the command to delete.
     */
    virtual XrlCmdError cli_manager_0_1_delete_cli_command(
	// Input values, 
	const string&	processor_name, 
	const string&	command_name);
    
    //
    // The CLI client-side (i.e., the CLI sending XRLs)
    //
    void send_process_command(const string& target,
			      const string& processor_name,
			      const string& cli_term_name,
			      uint32_t cli_session_id,
			      const string& command_name,
			      const string& command_args);
    void recv_process_command_output(const XrlError& xrl_error,
				     const string *processor_name,
				     const string *cli_term_name,
				     const uint32_t *cli_session_id,
				     const string *command_output);
private:
    const string& my_xrl_target_name() {
	return XrlCliTargetBase::name();
    }
    CliNode&	cli_node() const { return (_cli_node); }
    
    CliNode&	_cli_node;

    XrlCliProcessorV0p1Client _xrl_cli_processor_client;
};

#endif // __CLI_XRL_CLI_NODE_HH__
