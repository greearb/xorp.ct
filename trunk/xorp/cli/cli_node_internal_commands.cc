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

#ident "$XORP: xorp/cli/cli_node_internal_commands.cc,v 1.3 2003/03/10 23:20:12 hodson Exp $"


//
// CLI internal commands.
//


#include "cli_module.h"
#include "cli_private.hh"
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
 * @void: 
 * 
 * Add the internal default CLI commands from the top.
 * XXX: used by the CLI itself for internal processing of a command.
 * TODO: for consistency, even the internal commands should use XRLs instead.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::add_internal_cli_commands(void)
{
    CliCommand *c0, *c1;
    
    c0 = cli_command_root();
    
    if (c0 == NULL)
	return (XORP_ERROR);
    
    // TODO: check that each command succeeded
    c1 = c0->add_command("show", "Display information");
    c1 = c0->add_command("show log",
			 "Display information about log files and users",
			 callback(this, &CliNode::cli_show_log));
    c1 = c0->add_command("show log user",
			 "Display information about users",
			 callback(this, &CliNode::cli_show_log_user));
    c1 = c0->add_command("set", "Set variable");
    c1 = c0->add_command("set log", "Set log-related state");
    c1 = c0->add_command("set log output",
			 "Set output destination for log messages");
    c1 = c0->add_command("set log output cli",
			 "Set output CLI terminal for log messages",
			 callback(this, &CliNode::cli_set_log_output_cli));
    c1 = c0->add_command("set log output file",
			 "Set output file for log messages",
			 callback(this, &CliNode::cli_set_log_output_file));
    c1 = c0->add_command("set log output remove",
			 "Remove output destination for log messages");
    c1 = c0->add_command("set log output remove cli",
			 "Remove output CLI terminal for log messages",
			 callback(this, &CliNode::cli_set_log_output_remove_cli));
    c1 = c0->add_command("set log output remove file",
			 "Remove output file for log messages",
			 callback(this, &CliNode::cli_set_log_output_remove_file));
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show log [filename]"
//
// Display information about log files
// TODO: probably should change the command name to "show log file" ??
int
CliNode::cli_show_log(const char *		, // server_name
		      const char *cli_term_name,
		      uint32_t			, // cli_session_id
		      const char *		, // command_global_name
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
CliNode::cli_show_log_user(const char *		, // server_name
			   const char *cli_term_name,
			   uint32_t		, // cli_session_id
			   const char *		, // command_global_name
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
    for (iter = client_list().begin();
	 iter != client_list().end();
	 ++iter) {
	CliClient *tmp_cli_client = *iter;
	
	if (user_name.size()
	    && (strncmp(user_name.c_str(),
			tmp_cli_client->cli_session_user_name(),
			user_name.size())
		!= 0)) {
	    continue;
	}
	user_name_found = true;
	
	// Get the start time
	TimeVal start_time_timeval = tmp_cli_client->cli_session_start_time();
	string start_time;
	{
	    int maxlen = sizeof("999999999/99/99 99/99/99.999999999 ");
	    char buf[maxlen];
	    time_t time_clock = start_time_timeval.sec();
	    struct tm *local_time = localtime(&time_clock);
	    if (strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", local_time)
		== 0) {
		snprintf(buf, maxlen / sizeof(buf[0]), "strftime ERROR");
	    }
	    start_time = buf;
	}
	
	cli_client->cli_print(
	    c_format("%-16s%-16s%-16s%-16s\n",
		     tmp_cli_client->cli_session_user_name(),
		     tmp_cli_client->cli_session_term_name(),
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
		     tmp_cli_client->cli_session_user_name(),
		     tmp_cli_client->cli_session_term_name(),
		     cstring(tmp_cli_client->cli_session_from_address()),
		     tmp_cli_client->cli_session_start_time(),
		     tmp_cli_client->cli_session_stop_time(),
		     tmp_cli_client->cli_session_duration_time())
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
CliNode::cli_set_log_output_cli(const char *	, // server_name
				const char *cli_term_name,
				uint32_t	, // cli_session_id
				const char *	, // command_global_name
				const vector<string>&argv)
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
				 tmp_cli_client->cli_session_term_name()));
		}
	    }
	}
    } else {
	tmp_cli_client = find_cli_by_term_name(term_name.c_str());
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
			     tmp_cli_client->cli_session_term_name())
		    );
		return (XORP_ERROR);
	    }
	}
    }
    
    cli_client->cli_print(c_format("Added %u CLI terminal(s)\n", cli_n));
    
    return (XORP_OK);
}

//
// CLI COMMAND: "set log output cli remove <cli_term_name> | 'all' "
//
// Remove CLI terminal from the set of output destinations of log messages
// TODO: this is a home-brew own command
// TODO: merge this function with "set log output cli"
int
CliNode::cli_set_log_output_remove_cli(const char *	, // server_name
				       const char *cli_term_name,
				       uint32_t		, // cli_session_id
				       const char *	, // command_global_name
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
				 tmp_cli_client->cli_session_term_name()));
		}
	    }
	}
    } else {
	tmp_cli_client = find_cli_by_term_name(term_name.c_str());
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
			     tmp_cli_client->cli_session_term_name()));
		return (XORP_ERROR);
	    }
	}
    }
    
    cli_client->cli_print(c_format("Removed %u CLI terminal(s)\n", cli_n));
    
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
CliNode::cli_set_log_output_file(const char *		, // server_name
				 const char *cli_term_name,
				 uint32_t		, // cli_session_id
				 const char *		, // command_global_name
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
CliNode::cli_set_log_output_remove_file(const char * ,	// server_name
					const char *cli_term_name,
					uint32_t ,	// cli_session_id
					const char * ,	// command_global_name
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
