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

#ident "$XORP: xorp/cli/cli_node.cc,v 1.6 2003/03/25 01:21:44 pavlin Exp $"


//
// CLI (Command-Line Interface) implementation for XORP.
//


#include "cli_module.h"
#include "cli_private.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipvxnet.hh"
#include "libxorp/utils.hh"
#include "libcomm/comm_api.h"
#include "cli_client.hh"
#include "cli_node.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


/**
 * CliNode::CliNode:
 * @init_family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @init_module_id: The module ID (must be %XORP_MODULE_CLI).
 * @init_event_loop: The event loop.
 * 
 * CLI node constructor.
 **/
CliNode::CliNode(int init_family, xorp_module_id module_id,
		 EventLoop& init_event_loop)
    : ProtoNode<Vif>(init_family, module_id, init_event_loop),
      _cli_command_root(NULL, "", "")
{
    assert(module_id == XORP_MODULE_CLI);
    if (module_id != XORP_MODULE_CLI) {
	XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_CLI' = %d)",
		   module_id, XORP_MODULE_CLI);
    }
    
    _cli_socket = -1;
    _cli_port = 0;		// XXX: not defined yet.
    _next_session_id = 0;
    
    cli_command_root()->set_allow_cd(true, XORP_CLI_PROMPT);
    cli_command_root()->create_default_cli_commands();
    cli_command_root()->add_pipes();
}

/**
 * CliNode::~CliNode:
 * @void: 
 * 
 * CLI node destructor.
 **/
CliNode::~CliNode(void)
{
    stop();
    
    // TODO: perform CLI-specific operations.
}

int
CliNode::start(void)
{
    if (ProtoNode<Vif>::start() < 0)
	return (XORP_ERROR);
    
    // Perform misc. CLI-specific start operations
    
    //
    // Start accepting connections
    //
    if (sock_serv_open() >= 0) {
	event_loop().add_selector(_cli_socket, SEL_RD,
				  callback(this, &CliNode::accept_connection));
    }
    
    add_internal_cli_commands();
    
    return (XORP_OK);
}

/**
 * CliNode::stop:
 * @void: 
 * 
 * Stop the CLI operation.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::stop(void)
{
    if (ProtoNode<Vif>::stop() < 0)
	return (XORP_ERROR);
    
    // Perform misc. CLI-specific stop operations
    // TODO: add as necessary
    
    delete_pointers_list(_client_list);
    event_loop().remove_selector(_cli_socket, SEL_RD);
    
    return (XORP_OK);
}

/**
 * CliNode::add_enable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to add.
 * 
 * Add a subnet address to the list of subnet addresses enabled
 * for CLI access.
 * This method can be called more than once to add a number of
 * subnet addresses.
 **/
void
CliNode::add_enable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
    list<IPvXNet>::iterator iter;
    
    // Test if we have this subnet already
    for (iter = _enable_cli_access_subnet_list.begin();
	 iter != _enable_cli_access_subnet_list.end();
	 ++iter) {
	const IPvXNet& tmp_ipvxnet = *iter;
	if (tmp_ipvxnet == subnet_addr)
	    return;		// Subnet address already added
    }
    
    _enable_cli_access_subnet_list.push_back(subnet_addr);
}

/**
 * CliNode::delete_enable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to delete.
 * 
 * Delete a subnet address from the list of subnet addresses enabled
 * for CLI access.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::delete_enable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
    list<IPvXNet>::iterator iter;
    
    // Test if we have this subnet already
    for (iter = _enable_cli_access_subnet_list.begin();
	 iter != _enable_cli_access_subnet_list.end();
	 ++iter) {
	const IPvXNet& tmp_ipvxnet = *iter;
	if (tmp_ipvxnet == subnet_addr) {
	    _enable_cli_access_subnet_list.erase(iter);
	    return (XORP_OK);	// Entry erased
	}
    }
    
    return (XORP_ERROR);		// No entry found
}

/**
 * CliNode::add_disable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to add.
 * 
 * Add a subnet address to the list of subnet addresses disabled
 * for CLI access.
 * This method can be called more than once to add a number of
 * subnet addresses.
 **/
void
CliNode::add_disable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
    list<IPvXNet>::iterator iter;
    
    // Test if we have this subnet already
    for (iter = _disable_cli_access_subnet_list.begin();
	 iter != _disable_cli_access_subnet_list.end();
	 ++iter) {
	const IPvXNet& tmp_ipvxnet = *iter;
	if (tmp_ipvxnet == subnet_addr)
	    return;		// Subnet address already added
    }
    
    _disable_cli_access_subnet_list.push_back(subnet_addr);
}

/**
 * CliNode::delete_disable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to delete.
 * 
 * Delete a subnet address from the list of subnet addresses disabled
 * for CLI access.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::delete_disable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
    list<IPvXNet>::iterator iter;
    
    // Test if we have this subnet already
    for (iter = _disable_cli_access_subnet_list.begin();
	 iter != _disable_cli_access_subnet_list.end();
	 ++iter) {
	const IPvXNet& tmp_ipvxnet = *iter;
	if (tmp_ipvxnet == subnet_addr) {
	    _disable_cli_access_subnet_list.erase(iter);
	    return (XORP_OK);	// Entry erased
	}
    }
    
    return (XORP_ERROR);		// No entry found
}

bool
CliNode::is_allow_cli_access(const IPvX& ipvx) const
{
    list<IPvXNet>::const_iterator iter;
    IPvXNet best_enable = IPvXNet(IPvX::ZERO(ipvx.af()), 0);
    IPvXNet best_disable = IPvXNet(IPvX::ZERO(ipvx.af()), 0);
    bool best_enable_found = false;
    bool best_disable_found = false;
    
    // Find the longest-match subnet address that may enable access
    for (iter = _enable_cli_access_subnet_list.begin();
	 iter != _enable_cli_access_subnet_list.end();
	 ++iter) {
	const IPvXNet& ipvxnet = *iter;
	if (ipvx.af() != ipvxnet.masked_addr().af())
	    continue;
	if (! ipvxnet.contains(ipvx))
	    continue;
	if (best_enable.contains(ipvxnet))
	    best_enable = ipvxnet;
	best_enable_found = true;
    }

    // Find the longest-match subnet address that may disable access
    for (iter = _disable_cli_access_subnet_list.begin();
	 iter != _disable_cli_access_subnet_list.end();
	 ++iter) {
	const IPvXNet& ipvxnet = *iter;
	if (ipvx.af() != ipvxnet.masked_addr().af())
	    continue;
	if (! ipvxnet.contains(ipvx))
	    continue;
	if (best_disable.contains(ipvxnet))
	    best_disable = ipvxnet;
	best_disable_found = true;
    }

    if (! best_disable_found) {
	// XXX: no disable match, so enable access by default
	return (true);
    }
    
    if (! best_enable_found) {
	// XXX: no enable match, so definitely disable
	return (false);
    }
    
    // Both enable and disable match
    if (best_enable.prefix_len() > best_disable.prefix_len())
	return (true);
    
    return (false);		// XXX: disable even if conflicting config
}

/**
 * CliNode::find_cli_by_term_name:
 * @term_name: The CLI terminal name to search for.
 * 
 * Find a CLI client #CliClient for a given terminal name.
 * 
 * Return value: The CLI client with name of @term_name on success,
 * otherwise NULL.
 **/
CliClient *
CliNode::find_cli_by_term_name(const char *term_name) const
{
    list<CliClient *>::const_iterator iter;
    
    for (iter = _client_list.begin();
	 iter != _client_list.end();
	 ++ iter) {
	CliClient *cli_client = *iter;
	if (strncmp(term_name, cli_client->cli_session_term_name(),
		    strlen(term_name))
	    == 0) {
	    return (cli_client);
	}
    }
    
    return (NULL);
}

/**
 * CliNode::find_cli_by_session_id:
 * @session_id: The CLI session ID to search for.
 * 
 * Find a CLI client #CliClient for a given session ID.
 * 
 * Return value: The CLI client with session ID of @session_id on success,
 * otherwise NULL.
 **/
CliClient *
CliNode::find_cli_by_session_id(uint32_t session_id) const
{
    list<CliClient *>::const_iterator iter;
    
    for (iter = _client_list.begin();
	 iter != _client_list.end();
	 ++ iter) {
	CliClient *cli_client = *iter;
	if (cli_client->cli_session_session_id() == session_id)
	    return (cli_client);
    }
    
    return (NULL);
}

/**
 * CliNode::xlog_output:
 * @obj: The #CliClient object to apply this function to.
 * @msg: A C-style string with the message to output.
 * 
 * Output a log message to a #CliClient object.
 * 
 * Return value: On success, the number of characters printed,
 * otherwise %XORP_ERROR.
 **/
int
CliNode::xlog_output(void *obj, const char *msg)
{
    CliClient *cli_client = static_cast<CliClient *>(obj);

    int ret_value = cli_client->cli_print(msg);
    if (ret_value >= 0 && 
	cli_client->cli_print("") >= 0 && cli_client->cli_flush() == 0) { 
	    return ret_value;
	}
    return  XORP_ERROR;
}

//
// CLI add_cli_command
//
int
CliNode::add_cli_command(
    // Input values,
    const string&	processor_name,
    const string&	command_name,
    const string&	command_help,
    const bool&		is_command_cd,
    const string&	command_cd_prompt,
    const bool&		is_command_processor,
    // Output values,
    string&		reason)
{
    // Reset the return value
    reason = "";
    
    //
    // Check the request
    //
    if (command_name.empty()) {
	reason = "ERROR: command name is empty";
	return (XORP_ERROR);
    }
    
    CliCommand *c0 = cli_command_root();
    CliCommand *c1 = NULL;
    
    if (! is_command_processor) {
	if (is_command_cd) {
	    c1 = c0->add_command(command_name.c_str(), command_help.c_str(),
				 command_cd_prompt.c_str());
	} else {
	    c1 = c0->add_command(command_name.c_str(), command_help.c_str());
	}
    } else {
	// Command processor
	c1 = c0->add_command(command_name.c_str(), command_help.c_str(),
			     callback(this,
				      &CliNode::send_process_command));
	if (c1 != NULL)
	    c1->set_can_pipe(true);
    }
    
    //
    // TODO: return the result of the command installation
    //
    
    if (c1 == NULL) {
	reason = c_format("Cannot install command '%s'", command_name.c_str());
	return (XORP_ERROR);
    }
    
    c1->set_global_name(command_name.c_str());
    c1->set_server_name(processor_name.c_str());
    
    return (XORP_OK);
}

//
// Send a command request to a remote node
//
int
CliNode::send_process_command(const char *server_name,
			      const char *cli_term_name,
			      uint32_t cli_session_id,
			      const char *command_global_name,
			      const vector<string>& argv)
{
    if (server_name == NULL)
	return (XORP_ERROR);
    if (cli_term_name == NULL)
	return (XORP_ERROR);
    if (command_global_name == NULL)
	return (XORP_ERROR);
    
    CliClient *cli_client = find_cli_by_session_id(cli_session_id);
    if (cli_client == NULL)
	return (XORP_ERROR);
    if (cli_client != find_cli_by_term_name(cli_term_name))
	return (XORP_ERROR);
    
    // Create a single string of all arguments
    string command_args;
    for (size_t i = 0; i < argv.size(); i++) {
	if (command_args.size())
	    command_args += " ";
	command_args += argv[i];
    }
    
    //
    // Send the request
    //
    if (! _send_process_command_callback.is_empty()) {
	(_send_process_command_callback)->dispatch(server_name,
						   string(server_name),
						   string(cli_term_name),
						   cli_session_id,
						   string(command_global_name),
						   string(command_args));
    }
    
    cli_client->set_is_waiting_for_data(true);
    
    return (XORP_OK);
}

//
// Process the response of a command processed by a remote node
//
void
CliNode::recv_process_command_output(const string * , // processor_name,
				     const string *cli_term_name,
				     const uint32_t *cli_session_id,
				     const string *command_output)
{
    //
    // Find if a client is waiting for that command
    //
    if ((cli_term_name == NULL) || (cli_session_id == NULL))
	return;
    CliClient *cli_client = find_cli_by_session_id(*cli_session_id);
    if (cli_client == NULL)
	return;
    if (cli_client != find_cli_by_term_name(cli_term_name->c_str()))
	return;
    
    if (! cli_client->is_waiting_for_data()) {
	// ERROR: client is not waiting for that data (e.g., probably too late)
	return;
    }
    
    //
    // Print the result and reset client status
    //
    if (command_output != NULL) {
	cli_client->cli_print(c_format("%s", command_output->c_str()));
    }
    cli_client->cli_flush();
    cli_client->set_is_waiting_for_data(false);
    cli_client->waiting_for_result_timer().unschedule();
    cli_client->post_process_command(false);
}

CliClient *
CliNode::enable_stdio_access()
{
    return (add_connection(fileno(stdin)));
}
