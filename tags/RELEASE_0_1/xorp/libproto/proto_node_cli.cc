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

#ident "$XORP: xorp/libproto/proto_node_cli.cc,v 1.16 2002/12/09 18:29:02 hodson Exp $"


//
// Protocol generic CLI access implementation
//


#include "libproto_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/c_format.hh"
#include "libxorp/token.hh"
#include "proto_node_cli.hh"
#include "proto_unit.hh"


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
 * ProtoNodeCli::ProtoNodeCli:
 * @init_family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @init_module_id: The module ID (X_MODULE_*).
 * 
 * ProtoNodeCli node constructor.
 **/
ProtoNodeCli::ProtoNodeCli(int init_family, x_module_id init_module_id)
    : ProtoUnit(init_family, init_module_id)
{
    
}

ProtoNodeCli::~ProtoNodeCli()
{
    // TODO: implement it
    delete_all_cli_commands();
}

int
ProtoNodeCli::add_cli_dir_command(const char *dir_command_name,
				  const char *dir_command_help)
{
    return (add_cli_command_entry(dir_command_name, dir_command_help,
				  false,
				  "",
				  false,
				  callback(this,
					   &ProtoNodeCli::cli_process_dummy)));
}

int
ProtoNodeCli::add_cli_dir_command(const char *dir_command_name,
				  const char *dir_command_help,
				  bool is_allow_cd,
				  const char *dir_cd_prompt)
{
    return (add_cli_command_entry(dir_command_name, dir_command_help,
				  is_allow_cd,
				  dir_cd_prompt,
				  false,
				  callback(this,
					   &ProtoNodeCli::cli_process_dummy)));
}

int
ProtoNodeCli::add_cli_command(const char *command_name,
			      const char *command_help,
			      const CLIProcessCallback& cli_process_callback) 
{
    return (add_cli_command_entry(command_name, command_help,
				  false,
				  "",
				  true,
				  cli_process_callback));
}

int
ProtoNodeCli::add_cli_command_entry(const char *command_name,
				    const char *command_help,
				    bool is_command_cd,
				    const char *command_cd_prompt,
				    bool is_command_processor,
				    const CLIProcessCallback& cli_process_callback)
{
    //
    // XXX: the command name and the command help are mandatory
    //
    if (command_name == NULL) {
	XLOG_ERROR("Cannot add CLI command: invalid command name: NULL");
	return (XORP_ERROR);
    }
    if (command_help == NULL) {
	XLOG_ERROR("Cannot add CLI command '%s': invalid command help: NULL",
		   command_name);
	return (XORP_ERROR);
    }
    
    //
    // Insert the command into the local map with the handler for it.
    //
    _cli_callback_map.insert(pair<string, CLIProcessCallback>(string(command_name), cli_process_callback));
    _cli_callback_vector.push_back(string(command_name));
    
    //
    // Call the virtual function to add the command to the CLI manager.
    //
    if (add_cli_command_to_cli_manager(command_name,
				       command_help,
				       is_command_cd,
				       command_cd_prompt,
				       is_command_processor)
	< 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
ProtoNodeCli::delete_cli_command(const char *command_name)
{
    //
    // XXX: the command name is mandatory
    //
    if (command_name == NULL) {
	XLOG_ERROR("Cannot delete CLI command: invalid command name: NULL");
	return (XORP_ERROR);
    }
    
    //
    // Delete the command from the local vector of commands,
    // and the callback map
    //
    vector<string>::iterator iter;
    for (iter = _cli_callback_vector.begin();
	 iter != _cli_callback_vector.end();
	 ++iter) {
	if (*iter == string(command_name)) {
	    _cli_callback_vector.erase(iter);
	    break;
	}
    }
    map<string, CLIProcessCallback>::iterator pos;
    pos = _cli_callback_map.find(command_name);
    if (pos == _cli_callback_map.end()) {
	XLOG_ERROR("Cannot delete CLI command '%s': not in the local map",
		   command_name);
	return (XORP_ERROR);
    }
    _cli_callback_map.erase(pos);
    
    //
    // Call the virtual function to delete the command from the CLI manager.
    //
    if (delete_cli_command_from_cli_manager(command_name) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Delete all CLI commands
//
int
ProtoNodeCli::delete_all_cli_commands()
{
    int ret_code = XORP_OK;
    
    //
    // Delete all commands one after another in the reverse order they
    // were added.
    //
    while (_cli_callback_vector.size() > 0) {
	size_t i = _cli_callback_vector.size() - 1;
	if (delete_cli_command(_cli_callback_vector[i].c_str()) < 0)
	    ret_code = XORP_ERROR;
    }
    
    return (ret_code);
}

int
ProtoNodeCli::cli_process_command(const string& processor_name,
				  const string& cli_term_name,
				  const uint32_t& cli_session_id,
				  const string& command_name,
				  const string& command_args,
				  string& ret_processor_name,
				  string& ret_cli_term_name,
				  uint32_t& ret_cli_session_id,
				  string& ret_command_output)
{
    //
    // Copy some of the return argument
    //
    ret_processor_name = processor_name;
    ret_cli_term_name = cli_term_name;
    ret_cli_session_id = cli_session_id,
    ret_command_output = "";
    
    //
    // Check the request
    //
    if (command_name.empty())
	return (XORP_ERROR);
    
    //
    // Lookup the command
    //
    map<string, CLIProcessCallback>::iterator pos;
    pos = _cli_callback_map.find(command_name);
    if (pos == _cli_callback_map.end())
	return (XORP_ERROR);
    
    CLIProcessCallback& cli_process_callback = pos->second;
    
    //
    // Create a vector of the arguments
    //
    vector<string> argv;
    string token, token_line(command_args);
    do {
	token = pop_token(token_line);
	if (token.empty())
	    break;
	argv.push_back(token);
    } while (true);
    
    _cli_result_string = "";		// XX: reset the result buffer
    cli_process_callback->dispatch(argv);
    ret_command_output = _cli_result_string;
    _cli_result_string = "";		// Clean-up the result buffer
    
    return (XORP_OK);
}

/**
 * ProtoNodeCli::cli_print:
 * @msg: the message string to display.
 * 
 * Print a message to the CLI interface.
 * 
 * Return value: The number of characters printed (not including
 * the trailing `\0').
 **/
int
ProtoNodeCli::cli_print(const string& msg)
{
    int old_size = _cli_result_string.size();
    
    _cli_result_string += msg;
    
    return (_cli_result_string.size() - old_size);
}

