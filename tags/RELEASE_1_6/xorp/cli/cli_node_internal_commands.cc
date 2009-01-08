// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/cli/cli_node_internal_commands.cc,v 1.20 2008/10/02 21:56:29 bms Exp $"


//
// CLI internal commands.
//


#include "cli_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "cli_node.hh"
#include "cli_client.hh"
#include "cli_command.hh"


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
 * CliNode::add_internal_cli_commands:
 * @error_msg: The error message (if error).
 * 
 * Add the internal default CLI commands from the top.
 * XXX: used by the CLI itself for internal processing of a command.
 * TODO: for consistency, even the internal commands should use XRLs instead.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::add_internal_cli_commands(string& error_msg)
{
    CliCommand *c0;
    
    c0 = cli_command_root();
    
    if (c0 == NULL) {
	error_msg = c_format("Cannot find root CLI command");
	return (XORP_ERROR);
    }
    
    if (c0->add_command("show", "Display information", true, error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("show log",
			"Display information about log files and users",
			true,
			callback(this, &CliNode::cli_show_log),
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("show log user",
			"Display information about users",
			true,
			callback(this, &CliNode::cli_show_log_user),
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set", "Set variable", true, error_msg) == NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log", "Set log-related state", true, error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log output",
			"Set output destination for log messages",
			true,
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log output cli",
			"Set output CLI terminal for log messages",
			true,
			callback(this, &CliNode::cli_set_log_output_cli),
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log output file",
			"Set output file for log messages",
			true,
			callback(this, &CliNode::cli_set_log_output_file),
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log output remove",
			"Remove output destination for log messages",
			true,
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log output remove cli",
			"Remove output CLI terminal for log messages",
			true,
			callback(this, &CliNode::cli_set_log_output_remove_cli),
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    if (c0->add_command("set log output remove file",
			"Remove output file for log messages",
			true,
			callback(this, &CliNode::cli_set_log_output_remove_file),
			error_msg)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show log [filename]"
//
// Display information about log files
// TODO: probably should change the command name to "show log file" ??
int
CliNode::cli_show_log(const string&		, // server_name
		      const string& cli_term_name,
		      uint32_t			, // cli_session_id
		      const vector<string>&	, // command_global_name
		      const vector<string>& argv)
{
    CliClient *cli_client = find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);
    
    for (size_t i = 0; i < argv.size(); i++) {
	// Show log information about optional files
	cli_client->cli_print(c_format("Showing information about file '%s'\n",
				       argv[i].c_str()));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show log user [username]"
//
// Display information about users
// TODO: add the missing info at the end to make it 'who'-like
int
CliNode::cli_show_log_user(const string& 	, // server_name
			   const string& cli_term_name,
			   uint32_t		, // cli_session_id
			   const vector<string>& , // command_global_name
			   const vector<string>&argv)
{
    CliClient *cli_client = find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);
    
    string user_name;		// The optional user name to return info for
    bool user_name_found = false;
    
    if (argv.size()) {
	user_name = argv[0];
	cli_client->cli_print(c_format("Showing information about user '%s'\n",
				       user_name.c_str()));
    } else {
	user_name_found = true;
    }
    
    list<CliClient *>::iterator iter;
    for (iter = client_list().begin(); iter != client_list().end(); ++iter) {
	CliClient *tmp_cli_client = *iter;
	
	if (user_name.size()
	    && (user_name != tmp_cli_client->cli_session_user_name())) {
	    continue;
	}
	user_name_found = true;
	
	// Get the start time
	TimeVal start_time_tv = tmp_cli_client->cli_session_start_time();
	string start_time;
	{
	    char buf[sizeof("999999999/99/99 99/99/99.999999999 ")];
	    size_t maxlen = sizeof(buf);
	    time_t time_clock = start_time_tv.sec();
	    struct tm *local_time = localtime(&time_clock);
	    if (strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", local_time)
		== 0) {
		snprintf(buf, maxlen / sizeof(buf[0]), "strftime ERROR");
	    }
	    start_time = buf;
	}
	
	cli_client->cli_print(
	    c_format("%-16s%-16s%-16s%-16s\n",
		     tmp_cli_client->cli_session_user_name().c_str(),
		     tmp_cli_client->cli_session_term_name().c_str(),
		     cstring(tmp_cli_client->cli_session_from_address()),
		     start_time.c_str())
	    );
    }
    
    if (user_name.size() && (! user_name_found)) {
	cli_client->cli_print(
	    c_format("No such user '%s'\n", user_name.c_str())
	    );
    }
    
#if 0
    list<CliClient *>::iterator iter;
    for (iter = client_list().begin();
	 iter != client_list().end(); ++iter) {
	CliClient *tmp_cli_client = *iter;
	
	cli_client->cli_print(
	    c_format("%-16s%-16s%-16s%-16s - %-16s (%s)\n",
		     tmp_cli_client->cli_session_user_name().c_str(),
		     tmp_cli_client->cli_session_term_name().c_str(),
		     cstring(tmp_cli_client->cli_session_from_address()),
		     tmp_cli_client->cli_session_start_time().c_str(),
		     tmp_cli_client->cli_session_stop_time().c_str(),
		     tmp_cli_client->cli_session_duration_time().c_str())
	    );
    }
#endif
    
    return (XORP_OK);
}

//
// CLI COMMAND: "set log output cli <cli_term_name> | 'all' "
//
// Add CLI terminal to the set of output destinations of log messages
// TODO: this is a home-brew own command
// TODO: this command should set state in the appropriate protocols and
// start getting the logs from each of them (through XRLs) instead
// of applying it only to the local process logs.
int
CliNode::cli_set_log_output_cli(const string&	, // server_name
				const string& cli_term_name,
				uint32_t	, // cli_session_id
				const vector<string>& , // command_global_name
				const vector<string>& argv)
{
    CliClient *cli_client = find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);
    
    CliClient *tmp_cli_client;
    string term_name;
    uint32_t cli_n = 0;
    
    if (argv.empty()) {
	cli_client->cli_print("ERROR: missing CLI terminal name\n");
	return (XORP_ERROR);
    }
    
    term_name = argv[0];
    if (term_name == "all") {
	list<CliClient *>::iterator iter;
	for (iter = client_list().begin();
	     iter != client_list().end();
	     ++iter) {
	    tmp_cli_client = *iter;
	    if (! tmp_cli_client->is_log_output()) {
		if (tmp_cli_client->set_log_output(true) == XORP_OK) {
		    cli_n++;
		} else {
		    cli_client->cli_print(
			c_format("ERROR: cannot add CLI terminal "
				 "'%s' as log output\n",
				 tmp_cli_client->cli_session_term_name().c_str()));
		}
	    }
	}
    } else {
	tmp_cli_client = find_cli_by_term_name(term_name);
	if (tmp_cli_client == NULL) {
	    cli_client->cli_print(
		c_format("ERROR: cannot find CLI terminal '%s'\n",
			 term_name.c_str()));
	    return (XORP_ERROR);
	}
	
	if (! tmp_cli_client->is_log_output()) {
	    if (tmp_cli_client->set_log_output(true) == XORP_OK) {
		cli_n++;
	    } else {
		cli_client->cli_print(
		    c_format("ERROR: cannot add CLI terminal "
			     "'%s' as log output\n",
			     tmp_cli_client->cli_session_term_name().c_str())
		    );
		return (XORP_ERROR);
	    }
	}
    }
    
    cli_client->cli_print(c_format("Added %u CLI terminal(s)\n",
				   XORP_UINT_CAST(cli_n)));
    
    return (XORP_OK);
}

//
// CLI COMMAND: "set log output cli remove <cli_term_name> | 'all' "
//
// Remove CLI terminal from the set of output destinations of log messages
// TODO: this is a home-brew own command
// TODO: merge this function with "set log output cli"
int
CliNode::cli_set_log_output_remove_cli(const string&	, // server_name
				       const string& cli_term_name,
				       uint32_t		, // cli_session_id
				       const vector<string>& , // command_global_name
				       const vector<string>& argv)
{
    CliClient *cli_client = find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);
    
    CliClient *tmp_cli_client;
    string term_name;
    uint32_t cli_n = 0;
    
    if (argv.empty()) {
	cli_client->cli_print("ERROR: missing CLI terminal name\n");
	return (XORP_ERROR);
    }
    
    term_name = argv[0];
    if (term_name == "all") {
	list<CliClient *>::iterator iter;
	for (iter = client_list().begin();
	     iter != client_list().end();
	     ++iter) {
	    tmp_cli_client = *iter;
	    if (tmp_cli_client->is_log_output()) {
		if (tmp_cli_client->set_log_output(false) == XORP_OK) {
		    cli_n++;
		} else {
		    cli_client->cli_print(
			c_format("ERROR: cannot remove CLI terminal "
				 "'%s' as log output\n",
				 tmp_cli_client->cli_session_term_name().c_str()));
		}
	    }
	}
    } else {
	tmp_cli_client = find_cli_by_term_name(term_name);
	if (tmp_cli_client == NULL) {
	    cli_client->cli_print(
		c_format("ERROR: cannot find CLI terminal '%s'\n",
			 term_name.c_str()));
	    return (XORP_ERROR);
	}
	
	if (tmp_cli_client->is_log_output()) {
	    if (tmp_cli_client->set_log_output(false) == XORP_OK) {
		cli_n++;
	    } else {
		cli_client->cli_print(
		    c_format("ERROR: cannot remove CLI terminal "
			     "'%s' from log output\n",
			     tmp_cli_client->cli_session_term_name().c_str()));
		return (XORP_ERROR);
	    }
	}
    }
    
    cli_client->cli_print(c_format("Removed %u CLI terminal(s)\n",
				   XORP_UINT_CAST(cli_n)));
    
    return (XORP_OK);
}

//
// CLI COMMAND: "set log output file <filename>"
//
// Add a file to the set of output destinations of log messages
// TODO: this is a home-brew own command
// TODO: this command should set state in the appropriate protocols and
// start getting the logs from each of them (through XRLs) instead
// of applying it only to the local process logs.
int
CliNode::cli_set_log_output_file(const string&		, // server_name
				 const string& cli_term_name,
				 uint32_t		, // cli_session_id
				 const vector<string>&	, // command_global_name
				 const vector<string>& argv)
{
    CliClient *cli_client = find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);
    
    string file_name;
    
    if (argv.empty()) {
	cli_client->cli_print("ERROR: missing file name\n");
	return (XORP_ERROR);
    }
    
    file_name = argv[0];
    cli_client->cli_print("TODO: function not implemented yet\n");
    
    return (XORP_OK);
}

//
// CLI COMMAND: "set log output remove file <filename>"
//
// Remove a file from the set of output destinations of log messages
// TODO: this is a home-brew own command
int
CliNode::cli_set_log_output_remove_file(const string& ,	// server_name
					const string& cli_term_name,
					uint32_t ,	// cli_session_id
					const vector<string>& ,	// command_global_name
					const vector<string>& argv)
{
    CliClient *cli_client = find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);
    
    string file_name;
    
    if (argv.empty()) {
	cli_client->cli_print("ERROR: missing file name\n");
	return (XORP_ERROR);
    }
    
    file_name = argv[0];
    cli_client->cli_print("TODO: function not implemented yet\n");
    
    return (XORP_OK);
}
