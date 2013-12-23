// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/cli/xrl_cli_node.hh,v 1.25 2008/10/02 21:56:30 bms Exp $

#ifndef __CLI_XRL_CLI_NODE_HH__
#define __CLI_XRL_CLI_NODE_HH__



#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "xrl/targets/cli_base.hh"
#include "xrl/interfaces/cli_processor_xif.hh"
#include "cli_node.hh"

//
// TODO: XrlCliProcessorV1p0Client should NOT be a base class. Temp. solution..
//
class XrlCliNode : public XrlStdRouter,
		   public XrlCliTargetBase {
public:
    XrlCliNode(EventLoop&	eventloop,
	       const string&	class_name,
	       const string&	finder_hostname,
	       uint16_t		finder_port,
	       const string&	finder_target,
	       CliNode&		cli_node);
    virtual ~XrlCliNode() {}

    //
    // XrlCliNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();

    /**
     * Get a reference to the XrlRouter instance.
     *
     * @return a reference to the XrlRouter (@ref XrlRouter) instance.
     */
    XrlRouter&	xrl_router() { return *this; }

    /**
     * Get a const reference to the XrlRouter instance.
     *
     * @return a const reference to the XrlRouter (@ref XrlRouter) instance.
     */
    const XrlRouter& xrl_router() const { return *this; }

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
     *  Shutdown cleanly
     */
    virtual XrlCmdError common_0_1_shutdown();

    virtual XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

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
			      const vector<string>& command_global_name,
			      const vector<string>& command_argv);
    void recv_process_command_output(const XrlError& xrl_error,
				     const string *processor_name,
				     const string *cli_term_name,
				     const uint32_t *cli_session_id,
				     const string *command_output);
private:
    /**
     * Called when Finder connection is established.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_connect_event();

    /**
     * Called when Finder disconnect occurs.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_disconnect_event();

    CliNode&	cli_node() const { return (_cli_node); }

    CliNode&		_cli_node;

    XrlCliProcessorV0p1Client _xrl_cli_processor_client;

    bool		_is_finder_alive;
};

#endif // __CLI_XRL_CLI_NODE_HH__
