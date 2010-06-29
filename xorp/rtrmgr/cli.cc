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



#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"



#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "cli.hh"
#include "command_tree.hh"
#include "op_commands.hh"
#include "slave_conf_tree.hh"
#include "slave_conf_tree_node.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"

static string
get_user_name(uid_t uid)
{
    string result;

    struct passwd* pw = getpwuid(uid);
    if (pw != NULL)
	result = pw->pw_name;
    endpwent();
    return (result);
}

static string
get_my_user_name()
{
    return (get_user_name(getuid()));
}

//
// Append the list elements one-by-one to the vector.
// XXX: we need this helper function instead of
// "v.append(v.end(), l.begin(), l.end())"
// because the above statement generates a compilation error on
// FreeBSD-4.10 with gcc-2.95.4 (probably because of a bug in the STL
// implementation on that system).
//
static void
append_list_to_vector(vector<string>& result_vector,
		      const list<string>& append_list)
{
    list<string>::const_iterator iter;

    for (iter = append_list.begin(); iter != append_list.end(); ++iter)
	result_vector.push_back(*iter);
}

const string RouterCLI::DEFAULT_XORP_PROMPT_OPERATIONAL = "Xorp> ";
const string RouterCLI::DEFAULT_XORP_PROMPT_CONFIGURATION = "XORP# ";

RouterCLI::RouterCLI(XorpShellBase& xorpsh, CliNode& cli_node,
		     XorpFd cli_client_input_fd, XorpFd cli_client_output_fd,
		     bool verbose) throw (InitError)
    : _xorpsh(xorpsh),
      _cli_node(cli_node),
      _cli_client_ptr(NULL),
      _removed_cli_client_ptr(NULL),
      _verbose(verbose),
      _operational_mode_prompt(DEFAULT_XORP_PROMPT_OPERATIONAL),
      _configuration_mode_prompt(DEFAULT_XORP_PROMPT_CONFIGURATION),
      _mode(CLI_MODE_NONE),
      _changes_made(false),
      _op_mode_cmd(NULL)
{
    string error_msg;

    //
    // Set the operational and configuration mode prompts to be
    // "user@hostname> " and "user@hostname# " respectively.
    //
    string user_name = get_my_user_name();
    string host_name;
    do {
	char buf[MAXHOSTNAMELEN];
	memset(buf, 0, sizeof(buf));
	if (gethostname(buf, sizeof(buf)) < 0) {
            XLOG_FATAL("gethostname() failed: %s", strerror(errno));
	}
	buf[sizeof(buf) - 1] = '\0';
	host_name = buf;
    } while (false);
    _operational_mode_prompt = c_format("%s@%s> ",
					user_name.c_str(),
					host_name.c_str());
    _configuration_mode_prompt = c_format("%s@%s# ",
					  user_name.c_str(),
					  host_name.c_str());
#if 0
    //
    // XXX: if we want to change the promps to be
    // "user@hostname> " and "USER@HOSTNAME# "
    // then uncomment this code.
    //
    string::size_type iter;
    for (iter = 0; iter < _operational_mode_prompt.size(); ++iter) {
	char& c = _operational_mode_prompt[iter];
	if (xorp_isalpha(c))
	    c = xorp_tolower(c);
    }
    for (iter = 0; iter < _configuration_mode_prompt.size(); ++iter) {
	char& c = _configuration_mode_prompt[iter];
	if (xorp_isalpha(c))
	    c = xorp_toupper(c);
    }
#endif

    // Check for environmental variables that may overwrite the prompts
    char* value = NULL;
    value = getenv("XORP_PROMPT_OPERATIONAL");
    if (value != NULL)
	_operational_mode_prompt = value;
    value = getenv("XORP_PROMPT_CONFIGURATION");
    if (value != NULL)
	_configuration_mode_prompt = value;

    _cli_client_ptr = _cli_node.add_client(cli_client_input_fd,
					   cli_client_output_fd,
					   false,
					   _operational_mode_prompt,
					   error_msg);
    if (_cli_client_ptr == NULL) {
	error_msg = c_format("Cannot add CliClient: %s", error_msg.c_str());
	xorp_throw(InitError, error_msg);
    }

    //
    // We do all this here to allow for later extensions for
    // internationalization.
    //
    _help_o["configure"] = "Switch to configuration mode";
    _help_o["configure exclusive"] 
	= "Switch to configuration mode, locking out other users";
    _help_o["exit"] = "Exit this command session";
    _help_o["help"] = "Provide help with commands";
    _help_o["quit"] = "Quit this command session";

    _help_c["commit"] = "Commit the current set of changes";
    _help_c["create"] = "Alias for the \"set\" command (obsoleted)";
    _help_c["delete"] = "Delete a configuration element";
    _help_c["edit"] = "Edit a sub-element";
    _help_c["exit"] = "Exit from this configuration level";
    _help_c["exit configuration-mode"] =  "Exit from configuration mode";
    _help_c["exit discard"] 
	= "Exit from configuration mode, discarding changes";
    _help_c["help"] = "Provide help with commands";
    _help_c["load"] = "Load configuration from a file";
    _help_c["quit"] = "Quit from this level";
    _help_c["run"] = "Run an operational-mode command";
    _help_c["save"] = "Save configuration to a file";
    _help_c["set"] = "Set the value of a parameter or create a new element";
    _help_c["show"] = "Show the configuration (default values may be suppressed)";
    _help_c["show -all"] = "Show the configuration (all values displayed)";
    _help_c["top"] = "Exit to top level of configuration";
    _help_c["up"] = "Exit one level of configuration";

    _help_long_o["NULL"] = "\
A XORP router has two modes: operational mode and configuration mode.\n\
Operational mode is used to monitor the state of the router, whereas \n\
configuration mode is used to change the configuration of the router.\n\
\n\
You are currently in operational mode.\n\
To switch to configuration mode, use the \"configure\" command.\n\
You have to be in the xorp group to be able to use configuration mode.\n\
\n\
At most places in the user interface, typing \"?\" will give a list of\n\
possible commands or completions for the current command. Hitting <tab>\n\
will cause auto-completion to occur if this can be done unambiguously.\n\
Command cannot be abbreviated - always complete then, either by typing or \n\
by using <tab>. \n\
\n\
Type \"help help\" for more details on how to use help.\n\
Type \"help <command>\" for details of how to use a particular command.\n\
Type ? to list currently available commands.";

    _help_long_o["configure"] = "\
A XORP router has two modes: operational mode and configuration mode.\n\
Operational mode is used to monitor the state of the router, whereas \n\
configuration mode is used to change the configuration of the router.\n\
\n\
You are currently in operational mode.\n\
To switch to configuration mode, use the \"configure\" command.\n\
You have to be in the xorp group to be able to use configuration mode.\n\
\n\
Optional parameters to configure: \n\
    exclusive\n\
Use \"help configure exclusive\" for more details.";

    _help_long_o["configure exclusive"] = "\
To enter exclusive configuration mode you use the command:\n\
    configure exclusive\n\
This will allow you to make changes to the router configuration and lock\n\
all other users out of configuration mode until you exit.\n\
Use this command sparingly.";

    _help_long_o["exit"] = "\
The \"exit\" command will log you out of this command session.\n\
Typing ^D (control-D) also has the same effect.";

    _help_long_o["quit"] = "\
The \"quit\" command will log you out of this command session.\n\
Typing ^D (control-D) also has the same effect.";

    _help_long_c["NULL"] = "\
A XORP router has two modes: operational mode and configuration mode.\n\
Operational mode is used to monitor the state of the router, whereas \n\
configuration mode is used to change the configuration of the router.\n\
\n\
You are currently in configuration mode.\n\
To return to operational mode, use the \"exit\" command.\n\
\n\
At most places in the user interface, typing \"?\" will give a list of\n\
possible commands or completions for the current command. Hitting <tab>\n\
will cause auto-completion to occur if this can be done unambiguously.\n\
Command cannot be abbreviated - always complete then, either by typing or \n\
by using <tab>. \n\
\n\
Type \"help help\" for more details on how to use help.\n\
Type \"help <command>\" for details of how to use a particular command.\n\
Type ? to list currently available commands.";

    _help_long_c["commit"] = "\
When you change the configuration of a XORP router, the changes do not take\n\
place immediately.  This allows you to make additional related changes and\n\
then apply all the changes together.  To apply changes to the running\n\
configuration, the \"commit\" command is used.  Should there be an error,\n\
the configuration should be rolled back to the previous state.";

    _help_long_c["create"] = "\
The \"create\" command is an alias for the \"set\" command.\n\
It is obsoleted and will be removed in the future.\n\
\n\
See the \"set\" command.";

    _help_long_c["delete"] ="\
The \"delete\" command is used to delete a part of the configuration tree.\n\
All configuration nodes under the node that is named in the delete command\n\
will also be deleted.  For example, if the configuration contained:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
then typing \"delete protocols bgp\", followed by \"commit\" would cause bgp\n\
to be removed from the configuration, together with the peer 10.0.0.1, etc.\n\
In this case this would also cause the BGP routing process to be shut down.\n\
\n\
See also the \"edit\" and \"set\" commands.";

    _help_long_c["edit"] = "\
The \"edit\" command allows you to navigate around the configuration tree, so\n\
that changes can be made with minimal typing, or just a part of the router\n\
configuration examined. For example, if the configuration contained:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
Then typing \"edit protocols bgp peer 10.0.0.1\" would move the current \n\
position to the bgp peer 10.0.0.1.\n\
Then if you wanted to set the as number, you could use the command\n\
  set as 65002\n\
instead of the much longer\n\
  set protocols bgp peer 10.0.0.1 as 65002\n\
which would have been necessary without changing the current position in \n\
the configuration tree.\n\
\n\
See also the \"exit\", \"quit\", \"top\" and \"up\" commands.";

    _help_long_c["exit"] = "\
The \"exit\" command will cause the current position in the configuration \n\
to exit one level.  For example, if the configuration was:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
and the current position is \"protocols bgp peer 10.0.0.1\", then typing \n\
\"exit\" would cause the current position to become \"protocols bgp\".  In\n\
this case \"peer 10.0.0.1\" is a single level of configuration\n\
corresponding to a BGP peering session.\n\
\n\
If the current position is at the top level of the configuration, then \n\
\"exit\" will return the login session to operational mode.\n\
\n\
See also \"exit discard\" and \"exit configuration-mode\" for other\n\
uses of the exit command.";

     _help_long_c["exit configuration-mode"] = "\
The \"exit configuration-mode\" command will cause the login session to \n\
return to operational mode.  If there are uncommitted changes, the command\n\
will fail.\n\
\n\
See also the \"exit\" and \"exit discard\" commands.";

     _help_long_c["exit discard"] = "\
The \"exit discard\" command will cause the login session to return\n\
to operational mode, discarding any changes to the current configuration\n\
that have not been committed.\n\
\n\
See also the \"commit\" and \"exit configuration-mode\" commands.";

    _help_long_c["help"] = "\
The \"help\" command is used to provide help on many aspects of the command\n\
line interface.  If a command has options, usually separate help will be \n\
available for those options.  For example, \"help exit\" and \"help exit discard\"\n\
give different and complementory information.";

    _help_long_c["load"] = "\
The \"load\" command loads a complete router configuration from a named file\n\
and causes the new configuration to become the running configuration.\n\
By default, files are loaded from your own home directory unless an absolute\n\
pathname is given.  Directories in the path should be separated by \n\
slashes (\"/\") and not backslashes (\"\\\\\").\n\
\n\
For example:\n\
   load xorp-config\n\
   load /tmp/test-config\n\
\n\
See also: \"save\".";

    _help_long_c["quit"] = "\
The \"quit\" command will cause the current position in the configuration \n\
to quit one level.  For example, if the configuration was:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
and the current position is \"protocols bgp peer 10.0.0.1\", then typing \n\
\"quit\" would cause the current position to become \"protocols bgp\".  In\n\
this case \"peer 10.0.0.1\" is a single level of configuration\n\
corresponding to a BGP peering session.\n\
\n\
If the current position is at the top level of the configuration, then \n\
\"quit\" has no effect.\n\
\n\
See also the \"exit\", \"up\" and \"top\" commands.";


    _help_long_c["run"] = "\
The \"run\" command allows operational-mode commands to be executed without\n\
leaving configuration-mode.  This is particularly important if there are\n\
uncommitted changes to the configuration.\n\
\n\
For example, the operational-mode command \"show bgp peers\" can be run from\n\
configuration-mode as \"run show bgp peers\".\n\
\n\
Navigation commands such as the operational-mode \"configure\" command are not\n\
available using the run command.";


    _help_long_c["save"] = "\
The \"save\" command causes the current running configuration to be saved to\n\
a named file.  By default, files are loaded from your own home directory \n\
unless an absolute pathname is given.  Directories in the path should be\n\
separated by slashes (\"/\") and not backslashes (\"\\\").\n\
\n\
For example:\n\
   save xorp-config\n\
   save /tmp/test-config\n\
\n\
See also: \"load\".";

    _help_long_c["set"] = "\
The \"set\" command allows you to set or change configuration parameters. \n\
For example, if the configuration contained:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
Then typing \"set protocols bgp peer 10.0.0.1 as 65002\" would change \n\
the AS number of the BGP peer from 65001 to 65002.  Only parameters \n\
(leaves in the configuration tree) can be set using the \"set\" \n\
command.  New parts of the configuration tree such as a new BGP peer \n\
are created by directly typing them in configuration mode.\n\
\n\
The \"set\" command also allows you to create a new branch of the \n\
configuration tree.  For example, if the configuration contained:\n\
\n\
    protocols {\n\
        bgp {\n\
            peer 10.0.0.1 {\n\
                as: 65001 \n\
            }\n\
        }\n\
    }\n\
\n\
Then typing \"set protocols static\" would create a configuration\n\
branch for the static routes, and will fill it in with the template \n\
default values:\n\
\n\
    protocols {\n\
        bgp {\n\
            peer 10.0.0.1 {\n\
                as: 65001 \n\
            }\n\
        }\n\
>       static {\n\
>           targetname: \"static_routes\"\n\
>           enabled: true\n\
>       }\n\
    }\n\
\n\
Then you can use the command \"edit\" to navigate around the new branch\n\
of the configuration tree.  Use the command \"set\" to change some of the\n\
values of the new configuration branch:\n\
    set protocols static enabled false\n\
would disable the new static routes module.\n\
\n\
See also the \"commit\", \"delete\" and \"edit\" commands.";

    _help_long_c["show"] = "\
The \"show\" command will display all or part of the router configuration.\n\
Without any parameters, the \"show\" command will display all of the router\n\
configuration below the current position in the command tree (See the \n\
\"edit\" command for how to move the current position).  The show command \n\
can also take a part of the configuration as parameters; it will then show \n\
only the selected part of the configuration.\n\
Note that all configuration parameters that have default values are not\n\
displayed.\n\
\n\
If the configuration has been modified, any changes not yet committed will \n\
be highlighted.  For example, if \"show\" displays:\n\
\n\
  protocols {\n\
      bgp {\n\
>        peer 10.0.0.1 {\n\
>           as: 65001 \n\
>        }\n\
  }\n\
\n\
then this indicates that the peer 10.0.0.1 has been created or changed, \n\
and the change has not yet been applied to the running router configuration.\n\
\n\
See also the \"show -all\" command.";

    _help_long_c["show -all"] = "\
The \"show -all\" command will display all or part of the router configuration.\n\
It is same as the \"show\" command except that it displays all configuration\n\
parameters including those that have default values.\n\
\n\
See also the \"show\" command.";

    _help_long_c["top"] = "\
The \"top\" command will cause the current position in the configuration \n\
to return to the top level of the configuration.  For example, if the \n\
configuration was:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
and the current position is \"protocols bgp peer 10.0.0.1\", then \n\
typing \"top\" would cause the current position to become the top \n\
of the configuration, outside of the \"protocols\" grouping.  The same \n\
result could have been obtained by using the \"exit\" command three \n\
times.\n\
\n\
See also the \"exit\", \"quit\" and \"up\" commands.";

    _help_long_c["up"] = "\
The \"up\" command will cause the current position in the configuration \n\
to move up one level.  For example, if the configuration was:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as: 65001 \n\
          }\n\
   }\n\
\n\
and the current position is \"protocols bgp peer 10.0.0.1\", then typing \n\
\"up\" would cause the current position to become \"protocols bgp\".  In\n\
this case \"peer 10.0.0.1\" is a single level of configuration\n\
corresponding to a BGP peering session.\n\
\n\
If the current position is at the top level of the configuration, then \n\
\"up\" has no effect.\n\
\n\
See also the \"exit\", \"quit\" and \"top\" commands.";


    //    _current_config_node = &(config_tree()->root_node());
    operational_mode();
}

RouterCLI::~RouterCLI()
{
    if (_cli_client_ptr != NULL) {
	delete _cli_client_ptr;
	_cli_client_ptr = NULL;
    }
    if (_removed_cli_client_ptr != NULL) {
	delete _removed_cli_client_ptr;
	_removed_cli_client_ptr = NULL;
    }
}

bool
RouterCLI::done() const
{
    if (_cli_client_ptr == NULL)
	return (true);

    if (! _cli_client_ptr->done())
	return (false);

    return (true);
}

string 
RouterCLI::get_help_o(const string& s) const
{
    map<string,string>::const_iterator i = _help_o.find(s);

    if (i == _help_o.end())
	return string("");
    else
	return i->second;
}

string 
RouterCLI::get_help_c(const string& s) const
{
    map<string,string>::const_iterator i = _help_c.find(s);

    if (i == _help_c.end())
	return string("");
    else
	return i->second;
}

void
RouterCLI::operational_mode()
{
    string error_msg;

    if (_mode == CLI_MODE_OPERATIONAL)
	return;

    _mode = CLI_MODE_OPERATIONAL;
    clear_command_set();
    reset_path();
    set_prompt("", _operational_mode_prompt);
    if (add_op_mode_commands(NULL, error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot add operational mode commands: %s",
		   error_msg.c_str());
    }
}

int
RouterCLI::add_op_mode_commands(CliCommand* com0, string& error_msg)
{
    CliCommand *com1, *com2, *help_com, *exit_com, *quit_com;

#if 0
    com0->add_command("clear", "Clear information in the system *",
		      false, error_msg);
#endif

    // If no root node is specified, default to the true root
    if (com0 == NULL)
	com0 = _cli_node.cli_command_root();

    if (com0 == _cli_node.cli_command_root()) {
	com1 = com0->add_command("configure",
				 get_help_o("configure"),
				 false,
				 callback(this, &RouterCLI::configure_func),
				 error_msg);
	if (com1 == NULL)
	    return (XORP_ERROR);
	com1->set_global_name(token_line2vector("configure"));
	com1->set_can_pipe(false);
	com2 = com1->add_command("exclusive",
				 get_help_o("configure exclusive"),
				 false,
				 callback(this, &RouterCLI::configure_func),
				 error_msg);
	if (com2 == NULL)
	    return (XORP_ERROR);
	com2->set_global_name(token_line2vector("configure exclusive"));
	com2->set_can_pipe(false);

	// Help Command
	help_com = com0->add_command("help", get_help_o("help"), false,
				     error_msg);
	if (help_com == NULL)
	    return (XORP_ERROR);
	help_com->set_global_name(token_line2vector("help"));
	help_com->set_dynamic_children_callback(
	    callback(this, &RouterCLI::op_mode_help));
	help_com->set_dynamic_process_callback(
	    callback(this, &RouterCLI::op_help_func));
	help_com->set_can_pipe(true);

	// Exit Command
	exit_com = com0->add_command("exit",
				     get_help_o("exit"),
				     false,
				     callback(this, &RouterCLI::logout_func),
				     error_msg);
	if (exit_com == NULL)
	    return (XORP_ERROR);
	exit_com->set_global_name(token_line2vector("exit"));
	exit_com->set_can_pipe(false);

	// Quit Command
	quit_com = com0->add_command("quit",
				     get_help_o("quit"),
				     false,
				     callback(this, &RouterCLI::logout_func),
				     error_msg);
	if (quit_com == NULL)
	    return (XORP_ERROR);
	quit_com->set_global_name(token_line2vector("quit"));
	quit_com->set_can_pipe(false);
    }

    map<string, CliCommandMatch> cmds = op_cmd_list()->top_level_commands();
    map<string, CliCommandMatch>::const_iterator iter;
    for (iter = cmds.begin(); iter != cmds.end(); ++iter) {
	const CliCommandMatch& ccm = iter->second;
	com1 = com0->add_command(ccm.command_name(), ccm.help_string(), false,
				 error_msg);
	if (com1 == NULL)
	    return (XORP_ERROR);
	vector<string> command_vector_name;
	command_vector_name.push_back(ccm.command_name());
	com1->set_global_name(command_vector_name);
	com1->set_can_pipe(ccm.can_pipe());
	com1->set_default_nomore_mode(ccm.default_nomore_mode());
	com1->set_type_match_cb(ccm.type_match_cb());
	// Set the callback to generate the node's children
	com1->set_dynamic_children_callback(
	    callback(op_cmd_list(), &OpCommandList::childlist));
	//
	// Set the callback to pass to the node's children when they
	// are executed.
	//
	com1->set_dynamic_process_callback(
	    callback(this, &RouterCLI::op_mode_func));
	com1->set_dynamic_interrupt_callback(
	    callback(this, &RouterCLI::op_mode_cmd_interrupt));
	if (ccm.is_executable()) {
	    // XXX: set the processing and interrupt callbacks
	    com1->set_cli_process_callback(
		callback(this, &RouterCLI::op_mode_func));
	    com1->set_cli_interrupt_callback(
		callback(this, &RouterCLI::op_mode_cmd_interrupt));
	}
    }

    return (XORP_OK);
}

map<string, CliCommandMatch> 
RouterCLI::op_mode_help(const vector<string>& command_global_name) const
{
    string command_name;
    string help_string;
    bool is_executable = false;
    bool can_pipe = false;
    map<string, CliCommandMatch> children;
    string path, trimmed_path;

    path = token_vector2line(command_global_name);
    XLOG_ASSERT(path.substr(0, 4) == "help");
    if (path.size() == 4) {
	trimmed_path == "";
    } else {
	XLOG_ASSERT(path.substr(0, 5) == "help ");
	trimmed_path = path.substr(5, path.size() - 5);
    }

    if (trimmed_path == "") {
	// Add the static commands
	string commands[] = { "configure", "exit", "quit", "help" };
	is_executable = true;
	can_pipe = false;
	size_t i;
	for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
	    command_name = commands[i];
	    help_string = get_help_o(command_name);
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    children.insert(make_pair(command_name, ccm));
	}
	map<string, CliCommandMatch> cmds;
	cmds = op_cmd_list()->top_level_commands();
	map<string, CliCommandMatch>::const_iterator iter;
	for (iter = cmds.begin(); iter != cmds.end(); ++iter) {
	    const CliCommandMatch& ccm_top = iter->second;
	    const string& command_name = ccm_top.command_name();
	    help_string = c_format("Give help on the \"%s\" command",
				   command_name.c_str());
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    children.insert(make_pair(command_name, ccm));
	}
    } else if (trimmed_path == "configure") {
	is_executable = true;
	can_pipe = false;
	command_name = "exclusive";
	help_string = get_help_o(trimmed_path + " " + command_name);
	CliCommandMatch ccm(command_name, help_string, is_executable,
			    can_pipe);
	children.insert(make_pair(command_name, ccm));
    } else if (trimmed_path == "configure exclusive") {
	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "exit") {
	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "quit") {
	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "help") {
	is_executable = true;
	can_pipe = true;
    } else {
	children = op_cmd_list()->childlist(token_line2vector(trimmed_path));
    }

    return children;
}

void
RouterCLI::configure_mode()
{
    //    _nesting_depth = 0;
    if (_mode == CLI_MODE_CONFIGURE)
	return;

    if (_mode != CLI_MODE_TEXTENTRY) {
	display_config_mode_users();
    }
    display_alerts();
    _mode = CLI_MODE_CONFIGURE;
    clear_command_set();

    set_prompt("", _configuration_mode_prompt);

    // Add all the menus
    apply_path_change();
}

void
RouterCLI::display_config_mode_users() const
{
    cli_client()->cli_print("Entering configuration mode.\n");

    if (_config_mode_users.empty()) {
	cli_client()->cli_print("There are no other users in configuration mode.\n");
	return;
    }

    //
    // There are other users
    //
    if (_config_mode_users.size() == 1)
	cli_client()->cli_print("User ");
    else
	cli_client()->cli_print("Users ");
    list<uint32_t>::const_iterator iter, iter2;
    for (iter = _config_mode_users.begin();
	 iter != _config_mode_users.end();
	 ++iter) {
	if (iter != _config_mode_users.begin()) {
	    iter2 = iter;
	    ++iter2;
	    if (iter2 == _config_mode_users.end())
		cli_client()->cli_print(" and ");
	    else
		cli_client()->cli_print(", ");
	}
	string user_name = get_user_name(*iter);
	if (user_name.empty())
	    user_name = c_format("UID:%u", XORP_UINT_CAST(*iter));
	cli_client()->cli_print(c_format("%s", user_name.c_str()));
    }
    if (_config_mode_users.size() == 1)
	cli_client()->cli_print(" is also in configuration mode.\n");
    else
	cli_client()->cli_print(" are also in configuration mode.\n");
}

void
RouterCLI::display_alerts()
{
    // Display any alert messages that accumulated while we were busy
    while (!_alerts.empty()) {
	cli_client()->cli_print(_alerts.front());
	_alerts.pop_front();
    }
}

bool
RouterCLI::is_config_mode() const
{
    // This is a switch statement to get compile time checking
    switch (_mode) {
    case CLI_MODE_NONE:
    case CLI_MODE_OPERATIONAL:
	return false;
    case CLI_MODE_CONFIGURE:
    case CLI_MODE_TEXTENTRY:
	return true;
    }
    XLOG_UNREACHABLE();
}

void
RouterCLI::text_entry_mode()
{
    string prompt;

    _mode = CLI_MODE_TEXTENTRY;
    clear_command_set();

    prompt = "    > ";
    for (size_t i = 0; i < _braces.size(); i++)
	prompt += "  ";
    set_prompt("", prompt);
}

int
RouterCLI::add_static_configure_mode_commands(string& error_msg)
{
    CliCommand *com0, *com1, *com2, *help_com;

    com0 = _cli_node.cli_command_root();
    // Commit command
    com1 = com0->add_command("commit", get_help_c("commit"), false,
			     callback(this, &RouterCLI::commit_func),
			     error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("commit"));
    com1->set_can_pipe(false);

    // Create command
    _create_node = com0->add_command("create", get_help_c("create"), false,
				     error_msg);
    if (_create_node == NULL)
	return (XORP_ERROR);
    _create_node->set_global_name(token_line2vector("create"));
    _create_node->set_can_pipe(false);

    // Delete command
    _delete_node = com0->add_command("delete", get_help_c("delete"), false,
				     error_msg);
    if (_delete_node == NULL)
	return (XORP_ERROR);
    _delete_node->set_global_name(token_line2vector("delete"));
    _delete_node->set_can_pipe(false);

    // Edit command
    _edit_node = com0->add_command("edit", get_help_c("edit"), false,
				   callback(this, &RouterCLI::edit_func),
				   error_msg);
    if (_edit_node == NULL)
	return (XORP_ERROR);
    _edit_node->set_global_name(token_line2vector("edit"));
    _edit_node->set_can_pipe(false);

    // Exit command
    com1 = com0->add_command("exit", get_help_c("exit"), false,
			     callback(this, &RouterCLI::exit_func),
			     error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("exit"));
    com1->set_can_pipe(false);
    com2 = com1->add_command("configuration-mode",
			     get_help_c("exit configuration_mode"),
			     false,
			     callback(this, &RouterCLI::exit_func),
			     error_msg);
    if (com2 == NULL)
	return (XORP_ERROR);
    com2->set_global_name(token_line2vector("exit configuration-mode"));
    com2->set_can_pipe(false);
    com2 = com1->add_command("discard",
			     "Exit from configuration mode, discarding changes",
			     false,
			     callback(this, &RouterCLI::exit_func),
			     error_msg);
    if (com2 == NULL)
	return (XORP_ERROR);
    com2->set_global_name(token_line2vector("exit discard"));
    com2->set_can_pipe(false);

    // Help Command
    help_com = com0->add_command("help", get_help_c("help"), false, error_msg);
    if (help_com == NULL)
	return (XORP_ERROR);
    help_com->set_global_name(token_line2vector("help"));
    help_com->set_dynamic_children_callback(
	callback(this, &RouterCLI::configure_mode_help));
    help_com->set_dynamic_process_callback(
	callback(this, &RouterCLI::conf_help_func));
    help_com->set_can_pipe(true);

    // Load Command
    com1 = com0->add_command("load", get_help_c("load"), false,
			     callback(this, &RouterCLI::load_func), error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("load"));
    com1->set_can_pipe(false);

    // Quit Command
    com1 = com0->add_command("quit", get_help_c("quit"), false,
			     callback(this, &RouterCLI::exit_func), error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("quit"));
    com1->set_can_pipe(false);

    _run_node = com0->add_command("run", get_help_c("run"), false, error_msg);
    if (_run_node == NULL)
	return (XORP_ERROR);
    _run_node->set_global_name(token_line2vector("run"));
    _run_node->set_can_pipe(false);
    if (add_op_mode_commands(_run_node, error_msg) != XORP_OK)
	return (XORP_ERROR);

    com1 = com0->add_command("save", get_help_c("save"), false,
			     callback(this, &RouterCLI::save_func), error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("save"));
    com1->set_can_pipe(false);

    // Set Command
    _set_node = com0->add_command("set", get_help_c("set"), false, error_msg);
    if (_set_node == NULL)
	return (XORP_ERROR);
    _set_node->set_global_name(token_line2vector("set"));
    _set_node->set_can_pipe(false);

    // Show Command
    _show_node = com0->add_command("show", get_help_c("show"), false,
				   callback(this, &RouterCLI::show_func),
				   error_msg);
    if (_show_node == NULL)
	return (XORP_ERROR);
    _show_node->set_global_name(token_line2vector("show"));
    _show_node->set_can_pipe(true);
    com1 = _show_node->add_command("-all", get_help_c("show -all"), false,
				   callback(this, &RouterCLI::show_func),
				   error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("show -all"));
    com1->set_can_pipe(true);

    // Top Command
    com1 = com0->add_command("top", get_help_c("top"), false,
			     callback(this, &RouterCLI::exit_func), error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("top"));
    com1->set_can_pipe(false);

    // Up Command
    com1 = com0->add_command("up", get_help_c("up"), false,
			     callback(this, &RouterCLI::exit_func), error_msg);
    if (com1 == NULL)
	return (XORP_ERROR);
    com1->set_global_name(token_line2vector("up"));
    com1->set_can_pipe(false);

    return (XORP_OK);
}

map<string, CliCommandMatch>
RouterCLI::configure_mode_help(const vector<string>& command_global_name) const
{
    string command_name;
    string help_string;
    bool is_executable = false;
    bool can_pipe = false;
    map<string, CliCommandMatch> children;
    string path, trimmed_path;

    path = token_vector2line(command_global_name);
    XLOG_ASSERT(path.substr(0, 4) == "help");
    if (path.size() == 4) {
	trimmed_path == "";
    } else {
	XLOG_ASSERT(path.substr(0, 5) == "help ");
	trimmed_path = path.substr(5, path.size() - 5);
    }

    if (trimmed_path == "") {
	// Add the static commands:
	string commands[] = { "commit", "create", "delete", "edit", "exit",
			      "help", "load", "quit", "run", "save", "set",
			      "show", "top", "up" };
	is_executable = true;
	can_pipe = false;
	size_t i;
	for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
	    command_name = commands[i];
	    help_string = get_help_c(command_name);
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    children.insert(make_pair(command_name, ccm));
	}

	//
	// TODO: need to insert the commands that come from the template
	// tree here.
	//

    } else if (trimmed_path == "exit") {
	string commands[] = { "configuration-mode", "discard" };
	is_executable = true;
	can_pipe = false;
	size_t i;
	for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
	    command_name = commands[i];
	    help_string = get_help_c(trimmed_path + " " + command_name);
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    children.insert(make_pair(command_name, ccm));
	}
    } else if (trimmed_path == "show") {
	string commands[] = { "-all" };
	is_executable = true;
	can_pipe = false;
	size_t i;
	for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
	    command_name = commands[i];
	    help_string = get_help_c(trimmed_path + " " + command_name);
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    children.insert(make_pair(command_name, ccm));
	}
    } else {
	// Make the help for static commands executable
	map<string,string>::const_iterator i;
	is_executable = false;
	for (i = _help_c.begin(); i != _help_c.end(); ++i) {
	    if (trimmed_path == i->first) {
		is_executable = true;
		can_pipe = true;
		break;
	    }
	}
    }

    return children;
}

void
RouterCLI::reset_path()
{
    _path.clear();
}

void
RouterCLI::set_path(string path)
{
    reset_path();
    debug_msg("set path: >%s<\n", path.c_str());
    string::size_type ix;

    while (! path.empty()) {
	ix = path.find(' ');
	if (ix == string::npos) {
	    _path.push_back(path);
	    break;
	}
	_path.push_back(path.substr(0, ix));
	path = path.substr(ix + 1, path.size() - ix + 1);
    }
}

void
RouterCLI::config_mode_prompt()
{
    string prompt;

    if (pathstr() == "") {
	prompt = "[edit]";
    } else {
	prompt = "[edit " + pathstr() + "]";
    }
    set_prompt(prompt, _configuration_mode_prompt);
}

void
RouterCLI::config_changed_by_other_user()
{
    if (! is_config_mode()) {
	// XXX: we care about the changes only if we are in config mode
	return;	
    }

    //
    // Someone just did a commit, so we may need to reset the session's
    // editing path and mode.
    // Also, regenerate the command tree to include the configuration changes.
    //
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    if (current_config_node == NULL) {
	// XXX: the current config node has disappeared hence reset the path
	reset_path();
	if (_mode != CLI_MODE_CONFIGURE) {
	    // If we are in, say text-entry mode, reset back to configure mode
	    configure_mode();
	    return;
	}
    }

    // Regenerate the command tree
    apply_path_change();
}

void
RouterCLI::apply_path_change()
{
    string error_msg;

    _cli_node.cli_command_root()->delete_all_commands();

    if (add_static_configure_mode_commands(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot add static configure mode commands: %s",
		   error_msg.c_str());
    }

    // Rebuild the command subtree for the "edit" command
    _edit_node->delete_all_commands();
    //
    // Note: the edit subtree must be done before the other subtrees,
    // because it can force a path change in the root node if the "edit"
    // subtree is not a node that can be edited.
    //
    add_edit_subtree();

    // Rebuild the command subtree for the "create" command
    _create_node->delete_all_commands();
    add_create_subtree();

    // Rebuild the command subtree for the "delete" command
    _delete_node->delete_all_commands();
    add_delete_subtree();

    // Rebuild the command subtree for the "set" command
    _set_node->delete_all_commands();
    add_set_subtree();

    // Rebuild the command subtree for the "show" command
    _show_node->delete_all_commands();
    add_show_subtree();
    vector<string> vector_path;
    vector_path.push_back("show");
    append_list_to_vector(vector_path, _path);
    _show_node->set_global_name(vector_path);

    // Set the prompt appropriately
    config_mode_prompt();
}

void
RouterCLI::set_prompt(const string& line1, const string& line2)
{
    if (line1 != "") {
	cli_client()->cli_print(line1 + "\n");
    }
    _cli_node.cli_command_root()->set_allow_cd(true, line2);
    cli_client()->set_current_cli_prompt(line2);
}

void
RouterCLI::add_command_subtree(CliCommand& current_cli_node,
			       const CommandTreeNode& current_ctn,
			       const CLI_PROCESS_CALLBACK& cb,
			       vector<string> vector_path,
			       size_t depth,
			       bool can_pipe,
			       bool include_allowed_values)
{
    string error_msg;

    const list<CommandTreeNode*>& children = current_ctn.children();
    if (depth > 0) {
	vector_path.push_back(current_ctn.name());
    }

    list<CommandTreeNode*>::const_iterator cmd_iter;
    for (cmd_iter = children.begin(); cmd_iter != children.end(); ++cmd_iter) {
	const CommandTreeNode& ctn = *(*cmd_iter);
	const string& cmd_name = ctn.name();
	vector<string> vector_subpath = vector_path;
	vector_subpath.push_back(cmd_name);
	string help = ctn.help();
	if (help == "") {
	    help = "-- No help available --";
	}

	CliCommand* com = NULL;
	if (ctn.has_command()) {
	    com = current_cli_node.add_command(cmd_name, help, false, cb,
					       error_msg);
	} else {
	    com = current_cli_node.add_command(cmd_name, help, false,
					       error_msg);
	}
	if (com == NULL) {
	    XLOG_FATAL("add_command %s failed: %s",
		       cmd_name.c_str(), error_msg.c_str());
	}

	com->set_can_pipe(can_pipe);
	com->set_global_name(vector_subpath);
	add_command_subtree(*com, ctn, cb, vector_path, depth + 1, can_pipe,
			    include_allowed_values);
    }

    const TemplateTreeNode* ttn = current_ctn.template_tree_node();
    if (include_allowed_values && (ttn != NULL)) {
	//
	// Add the command-line completion for the allowed values
	//
	map<string, string>::const_iterator values_iter;
	for (values_iter = ttn->allowed_values().begin();
	     values_iter != ttn->allowed_values().end();
	     ++values_iter) {
	    const string& cmd_name = values_iter->first;
	    vector<string> vector_subpath = vector_path;
	    vector_subpath.push_back(cmd_name);
	    string help = values_iter->second;
	    if (help == "") {
		help = "-- No help available --";
	    }

	    CliCommand* com = NULL;
	    if (current_ctn.has_command()) {
		com = current_cli_node.add_command(cmd_name, help, false, cb,
						   error_msg);
	    } else {
		com = current_cli_node.add_command(cmd_name, help, false,
						   error_msg);
	    }
	    if (com == NULL) {
		XLOG_FATAL("add_command %s failed: %s",
			   cmd_name.c_str(), error_msg.c_str());
	    }
	    com->set_can_pipe(can_pipe);
	    com->set_global_name(vector_subpath);
	    com->set_is_command_argument(true);
	}

	//
	// Add the command-line completion for the allowed ranges
	//
	map<pair<int64_t, int64_t>, string>::const_iterator ranges_iter;
	for (ranges_iter = ttn->allowed_ranges().begin();
	     ranges_iter != ttn->allowed_ranges().end();
	     ++ranges_iter) {
	    const pair<int64_t, int64_t>& range = ranges_iter->first;
	    ostringstream ost;
	    ost << "[" << range.first << ".." << range.second << "]";
	    string cmd_name = ost.str();
	    vector<string> vector_subpath = vector_path;
	    vector_subpath.push_back(cmd_name);
	    string help = ranges_iter->second;
	    if (help == "") {
		help = "-- No help available --";
	    }

	    CliCommand* com = NULL;
	    if (current_ctn.has_command()) {
		com = current_cli_node.add_command(cmd_name, help, false, cb,
						   error_msg);
	    } else {
		com = current_cli_node.add_command(cmd_name, help, false,
						   error_msg);
	    }
	    if (com == NULL) {
		XLOG_FATAL("add_command %s failed: %s",
			   cmd_name.c_str(), error_msg.c_str());
	    }
	    com->set_can_pipe(can_pipe);
	    com->set_global_name(vector_subpath);
	    CliCommand::TypeMatchCb cb;
	    cb = callback(ttn, &TemplateTreeNode::type_match);
	    com->set_type_match_cb(cb);
	    com->set_is_command_argument(true);
	}
    }
}

void
RouterCLI::clear_command_set()
{
    CliCommand* com0;

    com0 = _cli_node.cli_command_root();
    com0->delete_all_commands();
}

string
RouterCLI::pathstr() const
{
    string s;
    list<string>::const_iterator iter;

    for (iter = _path.begin(); iter != _path.end(); ++iter) {
	if (s.empty())
	    s = *iter;
	else
	    s += " " + *iter;
    }

    return s;
}

string
RouterCLI::pathstr2() const
{
    string s;
    list<string>::const_iterator iter;

    for (iter = _path.begin(); iter != _path.end(); ++iter) {
	if (s.empty())
	    s = ">" + *iter + "<";
	else
	    s += " >" + *iter + "<";
    }

    return s;
}

void
RouterCLI::add_edit_subtree()
{
    CommandTree cmd_tree;
    list<string> cmds;

    cmds.push_back("%create");
    cmds.push_back("%activate");
    cmds.push_back("%update");
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);

    //
    // If we ended up at a node that can't be the root for an edit
    // tree, go back up one level now.
    //
    const TemplateTreeNode* ttn = current_config_node->template_tree_node();
    if (ttn != NULL) {
	if (ttn->is_tag() && !_path.empty()) {
	    _path.pop_back();
	    current_config_node = current_config_node->parent();
	}
    }
    XLOG_ASSERT(current_config_node != NULL);
    current_config_node->create_command_tree(
	cmd_tree,
	cmds,
	true,	/* include_intermediate_nodes */
	false,	/* include_children_templates */
	false,	/* include_leaf_value_nodes */
	true,	/* include_read_only_nodes */
	true,	/* include_permanent_nodes */
	false	/* include_user_hidden_nodes */);

    debug_msg("==========================================================\n");
    debug_msg("edit subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    if (_braces.empty()) {
	vector<string> vector_path;
	vector_path.push_back("edit");
	append_list_to_vector(vector_path, _path);

	add_command_subtree(*_edit_node, cmd_tree.root_node(),
			    callback(this, &RouterCLI::edit_func),
			    vector_path, 0,
			    false, /* can_pipe */
			    false /* include_allowed_values */);
    }
}

void
RouterCLI::add_create_subtree()
{
    add_text_entry_commands(_create_node);
}

void
RouterCLI::add_delete_subtree()
{
    CommandTree cmd_tree;
    list<string> cmds;

    cmds.push_back("%create");
    cmds.push_back("%activate");
    cmds.push_back("%update");
    cmds.push_back("%set");
    cmds.push_back("%delete");
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    XLOG_ASSERT(current_config_node != NULL);
    current_config_node->create_command_tree(
	cmd_tree,
	cmds,
	true,	/* include_intermediate_nodes */
	false,	/* include_children_templates */
	true,	/* include_leaf_value_nodes */
	true,	/* include_read_only_nodes */
	false,	/* include_permanent_nodes */
	false	/* include_user_hidden_nodes */);

    debug_msg("==========================================================\n");
    debug_msg("delete subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    vector<string> vector_path;
    vector_path.push_back("delete");
    append_list_to_vector(vector_path, _path);

    add_command_subtree(*_delete_node, cmd_tree.root_node(),
			callback(this, &RouterCLI::delete_func),
			vector_path, 0,
			false, /* can_pipe */
			false /* include_allowed_values */);
}

void
RouterCLI::add_set_subtree()
{
    add_text_entry_commands(_set_node);
}

void
RouterCLI::add_show_subtree()
{
    CommandTree cmd_tree;
    CliCommand* com1;
    list<string> cmds;
    string error_msg;

    // XXX: There is nothing to add to the "cmds" list

    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    XLOG_ASSERT(current_config_node != NULL);
    current_config_node->create_command_tree(
	cmd_tree,
	cmds,
	true,	/* include_intermediate_nodes */
	false,	/* include_children_templates */
	true,	/* include_leaf_value_nodes */
	true,	/* include_read_only_nodes */
	true,	/* include_permanent_nodes */
	false	/* include_user_hidden_nodes */);

    debug_msg("==========================================================\n");
    debug_msg("show subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    vector<string> vector_path, vector_path_all;
    vector_path.push_back("show");
    vector_path_all.push_back("show");
    vector_path_all.push_back("-all");
    append_list_to_vector(vector_path, _path);
    append_list_to_vector(vector_path_all, _path);

    add_command_subtree(*_show_node, cmd_tree.root_node(),
			callback(this, &RouterCLI::show_func),
			vector_path, 0,
			true, /* can_pipe */
			false /* include_allowed_values */);

    com1 = _show_node->add_command("-all", get_help_c("show -all"), false,
				   callback(this, &RouterCLI::show_func),
				   error_msg);
    if (com1 == NULL) {
	XLOG_FATAL("add_command %s failed: %s",
		   "-all", error_msg.c_str());
    }
    com1->set_global_name(vector_path_all);
    com1->set_can_pipe(true);

    add_command_subtree(*com1, cmd_tree.root_node(),
			callback(this, &RouterCLI::show_func),
			vector_path_all, 0,
			true, /* can_pipe */
			false /* include_allowed_values */);
}

void
RouterCLI::add_text_entry_commands(CliCommand* com0)
{
    const SlaveConfigTreeNode* current_config_node;
    const TemplateTreeNode* ttn;
    string error_msg;

    if (com0 == NULL)
	com0 = _cli_node.cli_command_root();

    debug_msg("add_text_entry_commands\n");

    current_config_node = config_tree()->find_node(_path);
    ttn = current_config_node->template_tree_node();
    if (ttn == NULL) {
	XLOG_TRACE(_verbose, "ttn == NULL >>%s<<\n",
		   current_config_node->segname().c_str());
	ttn = template_tree()->root_node();
    }

    list<TemplateTreeNode*>::const_iterator tti;
    for (tti = ttn->children().begin(); tti != ttn->children().end(); ++tti) {
	TemplateTreeNode* ttn_child = *tti;
	CliCommand* com = NULL;
	vector<string> vector_subpath;

	// XXX: ignore deprecated subtrees
	if (ttn_child->is_deprecated())
	    continue;

	// XXX: ignore user-hidden subtrees
	if (ttn_child->is_user_hidden())
	    continue;

	append_list_to_vector(vector_subpath, _path);
	vector_subpath.push_back(ttn_child->segname());

	string help = ttn_child->help();
	if (help == "") {
	    help = "-- No help available --";
	}

	if (ttn_child->is_tag()) {
	    com = com0->add_command(ttn_child->segname(), help, false,
				    error_msg);
	} else {
	    com = com0->add_command(ttn_child->segname(),
				    help, false,
				    callback(this, &RouterCLI::text_entry_func),
				    error_msg);
	}
	if (com == NULL) {
	    XLOG_FATAL("AI: add_command %s for template failed: %s",
		       ttn_child->segname().c_str(), error_msg.c_str());
	} else {
	    com->set_global_name(vector_subpath);
	}
	com->set_dynamic_children_callback(
	    callback(this, &RouterCLI::text_entry_children_func));
	com->set_dynamic_process_callback(
	    callback(this, &RouterCLI::text_entry_func));
	com->set_can_pipe(false);
    }
}

int
RouterCLI::configure_func(const string& ,
			  const string& ,
			  uint32_t ,
			  const vector<string>& command_global_name,
			  const vector<string>& /* argv */)
{
    string error_msg;
    bool exclusive = false;
    string cmd_name = token_vector2line(command_global_name);

    if (cmd_name == "configure exclusive")
	exclusive = true;

    idle_ui();
    if (_xorpsh.enter_config_mode(
	    exclusive,
	    callback(this, &RouterCLI::enter_config_done))
	!= true) {
	error_msg = c_format("ERROR: cannot enter configure mode. "
			     "No Finder?\n");
	cli_client()->cli_print(error_msg);
	reenable_ui();
	return (XORP_ERROR);
    }

     return (XORP_OK);
}

void
RouterCLI::enter_config_done(const XrlError& e)
{
    string error_msg;

    if (e == XrlError::OKAY()) {
	if (_xorpsh.get_config_users(
		callback(this, &RouterCLI::got_config_users))
	    != true) {
	    error_msg = c_format("ERROR: cannot get list of users in "
				 "configure mode. No Finder?\n");
	    cli_client()->cli_print(error_msg);
	    reenable_ui();
	    return;
	}
	return;
    }

    if ((e == XrlError::COMMAND_FAILED()) && (e.note() == "AUTH_FAIL")) {
	if (check_for_rtrmgr_restart() != true) {
	    error_msg = c_format("ERROR: cannot check for rtrmgr restart. "
				 "No Finder?\n");
	    cli_client()->cli_print(error_msg);
	    reenable_ui();
	    return;
	}
	return;
    }

    //
    // Either something really bad happened, or a user that didn't
    // have permission attempted to enter config mode.
    //
    error_msg = c_format("ERROR: %s.\n", e.note().c_str());
    cli_client()->cli_print(error_msg);
    reenable_ui();
}

void
RouterCLI::got_config_users(const XrlError& e, const XrlAtomList* users)
{
    // Clear the old list
    if (e != XrlError::OKAY()) {
	if (e == XrlError::COMMAND_FAILED() && e.note() == "AUTH_FAIL") {
	    check_for_rtrmgr_restart();
	    return;
	}
	cli_client()->cli_print("ERROR: failed to get config users from rtrmgr.\n");
    } else {
	_config_mode_users.clear();

	size_t nusers = users->size();
	bool doneme = false;
	for (size_t i = 0; i < nusers; i++) {
	    XrlAtom a;
	    a = users->get(i);
	    try {
		uid_t uid = a.uint32();
		// Only include me if I'm in the list more than once.
		if (uid == getuid() && doneme == false)
		    doneme = true;
		else
		    _config_mode_users.push_back(uid);
	    }
	    catch (const XrlAtom::WrongType& wt) {
		// This had better not happen
		XLOG_FATAL("Internal Error");
	    }
	}
    }
    reset_path();
    configure_mode();
    reenable_ui();
}

void
RouterCLI::new_config_user(uid_t user_id)
{
    if ((_mode != CLI_MODE_CONFIGURE) && (_mode != CLI_MODE_TEXTENTRY))
	return;

    _config_mode_users.push_back(user_id);
    string user_name = get_user_name(user_id);
    if (user_name.empty())
	user_name = c_format("UID:%u", XORP_UINT_CAST(user_id));
    
    string alert = c_format("User %s entered configuration mode\n",
			    user_name.c_str());
    notify_user(alert, false /* not urgent */);
}

void
RouterCLI::leave_config_done(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	cli_client()->cli_print("ERROR: failed to inform rtrmgr of leaving configuration mode.\n");
	// Something really bad happened - should we just exit here?
	if (e == XrlError::COMMAND_FAILED() && e.note() == "AUTH_FAIL") {
	    check_for_rtrmgr_restart();
	    operational_mode();
	    return;
	}
    }
    operational_mode();
    reenable_ui();
}

void
RouterCLI::notify_user(const string& alert, bool urgent)
{
    if (_mode == CLI_MODE_TEXTENTRY && !urgent) {
	_alerts.push_back(alert);
    } else {
	if (cli_client()) {
	    cli_client()->cli_print(alert);
	}
	else {
	    XLOG_ERROR("cli_client is NULL in notify_user, alert: %s\n", alert.c_str());
	}
    }
}

int
RouterCLI::op_help_func(const string& ,
			const string& ,
			uint32_t ,		// cli_session_id
			const vector<string>& command_global_name,
			const vector<string>& argv)
{
    string path;
    string cmd_name = token_vector2line(command_global_name);

    if (! argv.empty())
	return (XORP_ERROR);

    if (cmd_name.size() == 4) {
	XLOG_ASSERT(cmd_name.substr(0, 4) == "help");
	path = "";
    } else {
	XLOG_ASSERT(cmd_name.substr(0, 5) == "help ");
	path = cmd_name.substr(5, cmd_name.size() - 5);
    }

    if (path == "")
	path = "NULL";

    // Try and find the detailed help for standard commands
    map<string,string>::const_iterator i;
    i = _help_long_o.find(path);
    if (i != _help_long_o.end()) {
	cli_client()->cli_print("\n" + i->second + "\n\n");
	return (XORP_OK);
    } 

    //
    // There was no long help description available.  If there's a
    // short description, that will have to do.
    //
    i = _help_o.find(path);
    if (i != _help_o.end()) {
	cli_client()->cli_print(i->second + "\n");
	return (XORP_OK);
    }

    cli_client()->cli_print("Sorry, no help available for " + path + "\n");

    return (XORP_OK);
}

int
RouterCLI::conf_help_func(const string& ,
			  const string& ,
			  uint32_t ,		// cli_session_id
			  const vector<string>& command_global_name,
			  const vector<string>& argv)
{
    string path;
    string cmd_name = token_vector2line(command_global_name);

    if (! argv.empty())
	return (XORP_ERROR);

    if (cmd_name.size() == 4) {
	XLOG_ASSERT(cmd_name.substr(0, 4) == "help");
	path = "";
    } else {
	XLOG_ASSERT(cmd_name.substr(0, 5) == "help ");
	path = cmd_name.substr(5, cmd_name.size() - 5);
    }

    if (path == "")
	path = "NULL";

    // Try and find the detailed help for standard commands
    map<string,string>::const_iterator i;
    i = _help_long_c.find(path);
    if (i != _help_long_c.end()) {
	cli_client()->cli_print("\n" + i->second + "\n\n");
	return (XORP_OK);
    } 

    //
    // There was no long help description available.  If there's a
    // short description, that will have to do.
    //
    i = _help_c.find(path);
    if (i != _help_c.end()) {
	cli_client()->cli_print(i->second + "\n");
	return (XORP_OK);
    }

    cli_client()->cli_print("Sorry, no help available for " + path + "\n");

    return (XORP_OK);
}

int
RouterCLI::logout_func(const string& ,
		       const string& ,
		       uint32_t ,		// cli_session_id
		       const vector<string>& command_global_name,
		       const vector<string>& argv)
{
    string error_msg;
    string cmd_name = token_vector2line(command_global_name);

    if (! argv.empty()) {
	error_msg = c_format("ERROR: \"%s\" does not take any additional "
			     "parameters.\n",
			     cmd_name.c_str());
	cli_client()->cli_print(error_msg);
	return (XORP_ERROR);
    }

    idle_ui();
    if (_cli_node.remove_client(_cli_client_ptr, error_msg) != XORP_OK) {
	XLOG_FATAL("internal error deleting CLI client: %s",
		   error_msg.c_str());
	return (XORP_ERROR);
    }

    // Save the pointer to the removed client so it can be deleted later
    XLOG_ASSERT(_removed_cli_client_ptr == NULL);
    _removed_cli_client_ptr = _cli_client_ptr;
    _cli_client_ptr = NULL;

    return (XORP_OK);
}

int
RouterCLI::exit_func(const string& ,
		     const string& ,
		     uint32_t ,
		     const vector<string>& command_global_name,
		     const vector<string>& argv)
{
    string error_msg;
    string cmd_name = token_vector2line(command_global_name);

    if (cmd_name == "exit configuration-mode") {
	if (! argv.empty()) {
	    error_msg = c_format("ERROR: \"%s\" does not take any additional "
				 "parameters.\n",
				 cmd_name.c_str());
	    cli_client()->cli_print(error_msg);
	    return (XORP_ERROR);
	}
	if (_changes_made) {
	    cli_client()->cli_print("ERROR: There are uncommitted changes.\n");
	    cli_client()->cli_print("Use \"commit\" to commit the changes, "
				   "or \"exit discard\" to discard them.\n");
	    return (XORP_ERROR);
	}
	idle_ui();
	if (_xorpsh.leave_config_mode(
		callback(this, &RouterCLI::leave_config_done))
	    != true) {
	    error_msg = c_format("ERROR: failed to inform rtrmgr of leaving "
				 "configuration mode. No Finder?\n");
	    cli_client()->cli_print(error_msg);
	    operational_mode();
	    reenable_ui();
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    if (cmd_name == "exit discard") {
	if (! argv.empty()) {
	    error_msg = c_format("ERROR: \"%s\" does not take any additional "
				 "parameters.\n",
				 cmd_name.c_str());
	    cli_client()->cli_print(error_msg);
	    return (XORP_ERROR);
	}
	config_tree()->discard_changes();
	_changes_made = false;
	idle_ui();
	if (_xorpsh.leave_config_mode(
		callback(this, &RouterCLI::leave_config_done))
	    != true) {
	    error_msg = c_format("ERROR: failed to inform rtrmgr of leaving "
				 "config mode. No Finder?\n");
	    cli_client()->cli_print(error_msg);
	    operational_mode();
	    reenable_ui();
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    if (cmd_name == "top") {
	if (! argv.empty()) {
	    error_msg = c_format("ERROR: \"%s\" does not take any additional "
				 "parameters.\n",
				 cmd_name.c_str());
	    cli_client()->cli_print(error_msg);
	    return (XORP_ERROR);
	}
	reset_path();
	apply_path_change();
	return (XORP_OK);
    }

    //
    // Commands up and exit are similar, except up doesn't exit
    // configuration-mode if it's executed at the top level
    //
    if (cmd_name == "up") {
	if (! argv.empty()) {
	    error_msg = c_format("ERROR: \"%s\" does not take any additional "
				 "parameters.\n",
				 cmd_name.c_str());
	    cli_client()->cli_print(error_msg);
	    return (XORP_ERROR);
	}
	if (! _path.empty())
	    _path.pop_back();
    }
    if ((cmd_name == "exit") || (cmd_name == "quit")) {
	if (! _path.empty()) {
	    _path.pop_back();
	} else {
	    if (_changes_made) {
		cli_client()->cli_print("ERROR: There are uncommitted changes.\n");
		cli_client()->cli_print("Use \"commit\" to commit the changes, "
				       "or \"exit discard\" to discard them.\n");
		return (XORP_ERROR);
	    }
	    idle_ui();
	    if (_xorpsh.leave_config_mode(
		    callback(this, &RouterCLI::leave_config_done))
		!= true) {
		error_msg = c_format("ERROR: failed to inform rtrmgr of leaving "
				     "config mode. No Finder?\n");
		cli_client()->cli_print(error_msg);
		operational_mode();
		reenable_ui();
		return (XORP_ERROR);
	    }
	    return (XORP_OK);
	}
    }

    apply_path_change();

    return (XORP_OK);
}

int
RouterCLI::edit_func(const string& ,
		     const string& ,
		     uint32_t ,			// cli_session_id
		     const vector<string>& command_global_name,
		     const vector<string>& argv)
{
    string path;
    string cmd_name = token_vector2line(command_global_name);

    if (cmd_name != "edit") {
	XLOG_ASSERT(cmd_name.substr(0, 5) == "edit ");
	path = cmd_name.substr(5, cmd_name.size() - 5);
    }

    if (! argv.empty()) {
	string arg_string, msg;

	for (size_t i = 0; i < argv.size(); i++) {
	    if (! arg_string.empty())
		arg_string += " ";
	    arg_string += argv[i];
	}

	if (path.empty()) {
	    msg = c_format("ERROR: sub-element \"%s\" does not exist.\n",
			   arg_string.c_str());
	} else {
	    msg = c_format("ERROR: element \"%s\" does not contain "
			   "sub-element \"%s\".\n",
			   path.c_str(),
			   arg_string.c_str());
	}
	cli_client()->cli_print(msg);
	return (XORP_ERROR);
    }

    if (cmd_name == "edit") {
	// Just print the current edit level
	config_mode_prompt();
	return (XORP_OK);
    }

    set_path(path);
    apply_path_change();

    return (XORP_OK);
}

int
RouterCLI::extract_leaf_node_operator_and_value(const TemplateTreeNode& ttn,
						const vector<string>& argv,
						ConfigOperator& node_operator,
						string& value,
						string& error_msg)
{
    string operator_str;

    //
    // Test if the assign operator is allowed
    //
    bool is_assign_operator_allowed = false;
    list<ConfigOperator> allowed_operators = ttn.allowed_operators();
    list<ConfigOperator>::iterator operator_iter;
    if (allowed_operators.empty()) {
	is_assign_operator_allowed = true;
	allowed_operators.push_back(OP_ASSIGN);
    } else {
	for (operator_iter = allowed_operators.begin();
	     operator_iter != allowed_operators.end();
	     ++operator_iter) {
	    if (*operator_iter == OP_ASSIGN) {
		is_assign_operator_allowed = true;
		break;
	    }
	}
    }

    //
    // The arguments must be a value, or an operator and a value
    //
    value = "";
    node_operator = OP_NONE;
    bool is_error = false;
    switch (argv.size()) {
    case 0:
	// Missing value
	is_error = true;
	break;
    case 1:
	// A single argument, hence this implies the assign operator
	if (! is_assign_operator_allowed)
	    is_error = true;
	node_operator = OP_ASSIGN;
	value = argv[0];
	break;
    case 2:
	// An operator followed by a value
	operator_str = argv[0];
	value = argv[1];
	try {
	    node_operator = lookup_operator(operator_str);
	} catch (const ParseError& e) {
	    is_error = true;
	    break;
	}
	// Test if the operator is allowed
	if (find(allowed_operators.begin(), allowed_operators.end(),
		 node_operator)
	    == allowed_operators.end()) {
	    is_error = true;
	}
	break;
    default:
	// Too many arguments
	is_error = true;
	break;
    }

    if (is_error) {
	string operators_str;
	for (operator_iter = allowed_operators.begin();
	     operator_iter != allowed_operators.end();
	     ++operator_iter) {
	    if (! operators_str.empty())
		operators_str += " ";
	    operators_str += operator_to_str(*operator_iter);
	}
	error_msg = c_format("should take one %soperator [%s] followed by "
			     "one argument of type \"%s\"",
			     (is_assign_operator_allowed)? "optional " : "",
			     operators_str.c_str(),
			     ttn.typestr().c_str());
	return (XORP_ERROR);
    }

    if (ttn.type_match(value, error_msg) == false) {
	error_msg = c_format("argument \"%s\" is not a valid \"%s\": %s",
			     value.c_str(), ttn.typestr().c_str(),
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
RouterCLI::text_entry_func(const string& ,
			   const string& ,
			   uint32_t ,		// cli_session_id
			   const vector<string>& command_global_name,
			   const vector<string>& argv)
{
    string path;
    list<string> path_segments;
    string cmd_name = token_vector2line(command_global_name);

    //
    // The path contains the part of the command we managed to command-line
    // complete.  The remainder is in argv.
    //
    path = cmd_name;
    XLOG_TRACE(_verbose, "text_entry_func: %s\n", path.c_str());
    SlaveConfigTreeNode *ctn = NULL, *first_new_ctn = NULL, *brace_ctn;
    const TemplateTreeNode* ttn = NULL;
    bool value_expected = false;
    string error_msg;

    // Restore the indent position from last time we were called
    size_t original_braces_length = _braces.size();
    brace_ctn = config_tree()->find_node(_path);
    if (original_braces_length == 0) {
	//
	// We're coming here from configuration mode, rather that
	// already being in text_entry_mode.
	//
	if (brace_ctn == NULL) {
	    _braces.push_front(0);
	    brace_ctn = &(config_tree()->slave_root_node());
	} else {
	    _braces.push_front(brace_ctn->depth());
	}
    }

    path_segments.insert(path_segments.end(), command_global_name.begin(),
			 command_global_name.end());

    //
    // Push argv onto the list of path segments - there's no
    // substantial difference between the path and argv, except that
    // path has already seen some sanity checking.
    //
    path_segments.insert(path_segments.end(), argv.begin(), argv.end());

    //
    // The path_segments probably contain part of the path that already
    // exists and part that does not yet exist.  Find which is which.
    //
    list<string> new_path_segments;
    while (!path_segments.empty()) {
	ctn = config_tree()->find_node(path_segments);
	if (ctn != NULL) {
	    XLOG_TRACE(_verbose, "ctn: %s\n", ctn->segname().c_str());
	    break;
	}
	new_path_segments.push_front(path_segments.back());
	path_segments.pop_back();
    }
    if (ctn == NULL)
	ctn = brace_ctn;
    if (ctn == NULL)
	ctn = &(config_tree()->slave_root_node());
    XLOG_ASSERT(ctn != NULL);

    //
    // At this point, path_segments contains the path of nodes that
    // already exist, new_path_segments contains the path that does not
    // yet exist, and ctn points to the last existing node.
    //

    //
    // Test if the node is hidden from the user
    //
    if (ctn->is_user_hidden()) {
	const TemplateTreeNode* ttn = ctn->template_tree_node();
	const TemplateTreeNode* user_hidden_ttn;
	user_hidden_ttn = ttn->find_first_user_hidden_ancestor();
	if (user_hidden_ttn != NULL) {
	    error_msg = c_format("ERROR: user-hidden path \"%s\" "
				 "is not valid: %s.\n",
				 makepath(path_segments).c_str(),
				 user_hidden_ttn->user_hidden_reason().c_str());
	    cli_client()->cli_print(error_msg);
	    goto cleanup;
	}
    }

    //
    // Test if the node was already created.
    //
    if (new_path_segments.empty() && (! ctn->deleted())) {
	error_msg = c_format("ERROR: node \"%s\" already exists.\n",
			     ctn->segname().c_str());
	cli_client()->cli_print(error_msg);
	goto cleanup;
    }

    //
    // If the node was deleted previously, and it is not a tag, then
    // undelete the whole subtree below it.
    //
    if (ctn->deleted() && (! ctn->is_tag()))
	ctn->undelete_subtree();

    //
    // Undelete the node and all its ancestors (just in case it was deleted
    // previously).
    //
    ctn->undelete_node_and_ancestors();

    //
    // Variable "value_expected" keeps track of whether the next
    // segment should be a value for the node we just created.
    //
    if (ctn->is_tag() || ctn->is_leaf_value())
	value_expected = true;
    else
	value_expected = false;

    while (! new_path_segments.empty()) {
	if (! ctn->children().empty()) {
	    //
	    // The previous node already has children.  We need to
	    // check that the current path segment isn't one of those
	    // children.  If it is, then we don't need to create a new node.
	    //
	    bool exists = false;
	    list <ConfigTreeNode*>::iterator cti;
	    for (cti = ctn->children().begin();
		 cti != ctn->children().end();  ++cti) {
		if ((*cti)->segname() == new_path_segments.front()) {
		    XLOG_TRACE(_verbose, "Found pre-existing node: %s\n", 
			       (*cti)->segname().c_str());
		    ConfigTreeNode *existing_ctn = (*cti);
		    ctn = (SlaveConfigTreeNode*)(existing_ctn);
		    path_segments.push_back(new_path_segments.front());
		    new_path_segments.pop_front();
		    exists = true;
		    break;
		}
	    }
	    if (exists)
		continue;
	}
	if (value_expected) {
	    // We're expecting a value here
	    if (new_path_segments.front() == "{" 
		|| new_path_segments.front() == "}" ) {
		error_msg = c_format("ERROR: a value for \"%s\" "
				     "is required.\n",
				     ctn->segname().c_str());
		cli_client()->cli_print(error_msg);
		goto cleanup;
	    }
	    string value = new_path_segments.front();
	    ttn = ctn->template_tree_node();
	    XLOG_ASSERT(ttn != NULL);
	    if (ctn->is_tag()) {
		const TemplateTreeNode *data_ttn = NULL;
		//
		// The tag can have multiple children of different types -
		// take the first that has valid syntax.
		//
		// TODO: we really ought to check the allow commands here
		string cand_types;
		string errhelp;
		string allowed_value_error_msg;
		list<TemplateTreeNode*>::const_iterator tti;
		for (tti = ttn->children().begin();
		     tti != ttn->children().end();
		     ++tti) {
		    // XXX: ignore deprecated subtrees
		    if ((*tti)->is_deprecated())
			continue;
		    // XXX: ignore user-hidden subtrees
		    if ((*tti)->is_user_hidden())
			continue;
		    if (! (*tti)->check_allowed_value(value, errhelp)) {
			if (! allowed_value_error_msg.empty())
			    allowed_value_error_msg += " ";
			allowed_value_error_msg += errhelp;
			errhelp = "";
			continue;
		    }
		    if ((*tti)->type_match(value, errhelp)) {
			data_ttn = (*tti);
			break;
		    }
		    if (! cand_types.empty())
			cand_types += " or ";
		    cand_types += c_format("\"%s\"",
					   (*tti)->typestr().c_str());
		}
		if (data_ttn == NULL) {
		    if (ttn->type() != NODE_VOID) {
			cand_types = c_format("\"%s\"",
					      ttn->typestr().c_str());
		    }
		    error_msg = c_format("ERROR: argument \"%s\" "
					 "is not valid.",
					 value.c_str());
		    if (! allowed_value_error_msg.empty()) {
			error_msg += " " + allowed_value_error_msg;
		    }
		    if (! cand_types.empty()) {
			error_msg += c_format(" Allowed type(s): %s.",
					      cand_types.c_str());
		    }
		    error_msg += "\n"; 
		    cli_client()->cli_print(error_msg);
		    goto cleanup;
		}
		path_segments.push_back(value);
		XLOG_TRACE(_verbose, "creating node %s\n", value.c_str());
		ctn = new SlaveConfigTreeNode(value, 
					      makepath(path_segments),
					      data_ttn, ctn,
					      ConfigNodeId::ZERO(),
					      getuid(),
					      clientid(),
					      _verbose);
		if (first_new_ctn == NULL)
		    first_new_ctn = ctn;
		_changes_made = true;
		value_expected = false;
	    } else if (ctn->is_leaf_value()) {
		//
		// It must be a leaf, and we're expecting a value,
		// or an operator and a value.
		//

		// We cannot modify a read-only node
		if (ctn->is_read_only() && (! ctn->is_default_value(value))) {
		    string reason = ctn->read_only_reason();
		    error_msg = c_format("ERROR: node \"%s\" is read-only",
					 ctn->segname().c_str());
		    if (! reason.empty())
			error_msg += c_format(": %s", reason.c_str());
		    error_msg += ".\n";

		    cli_client()->cli_print(error_msg);
		    goto cleanup;
		}

		vector<string> argv;
		list<string>::iterator list_iter;
		for (list_iter = new_path_segments.begin();
		     list_iter != new_path_segments.end();
		     ++list_iter) {
		    argv.push_back(*list_iter);
		}
		if (new_path_segments.size() == 2) {
		    // XXX: pop-up the operator
		    new_path_segments.pop_front();
		}

		ConfigOperator node_operator = OP_NONE;
		if (extract_leaf_node_operator_and_value(*ttn, argv,
							 node_operator,
							 value, error_msg)
		    != XORP_OK) {
		    error_msg = c_format("ERROR: node \"%s\": %s.\n",
					 ctn->segname().c_str(),
					 error_msg.c_str());
		    cli_client()->cli_print(error_msg);
		    goto cleanup;
		}
		XLOG_ASSERT(ttn->type_match(value, error_msg) == true);
		XLOG_TRACE(_verbose, "Setting node \"%s\": operator %s "
			   "and value %s\n",
			   ctn->segname().c_str(),
			   operator_to_str(node_operator).c_str(),
			   value.c_str());
		// Set the value
		if (ctn->set_value(value, getuid(), error_msg) != true) {
		    error_msg = c_format("ERROR: invalid value \"%s\": %s\n",
					 value.c_str(), error_msg.c_str());
		    cli_client()->cli_print(error_msg);
		    goto cleanup;
		}
		// Set the operator
		if (ctn->set_operator(node_operator, getuid(), error_msg)
		    != true) {
		    error_msg = c_format("ERROR: invalid operator value \"%s\": %s\n",
					 operator_to_str(node_operator).c_str(),
					 error_msg.c_str());
		    cli_client()->cli_print(error_msg);
		    goto cleanup;
		}
		_changes_made = true;
		value_expected = false;
	    } else {
		XLOG_UNREACHABLE();
	    }
	} else {
	    if (new_path_segments.front() == "{") {
		//
		// Just keep track of the nesting depth so we can match
		// braces on the way back out.
		//
		_braces.push_back(ctn->depth());
		brace_ctn = ctn;
		new_path_segments.pop_front();
		continue;
	    }
	    if (new_path_segments.front() == "}") {
		// Jump back out to the appropriate nesting depth
		if (_braces.size() == 1) {
		    // There's always at least one entry in _braces
		    cli_client()->cli_print("ERROR: mismatched \"}\".\n");
		    goto cleanup;
		}
		XLOG_TRACE(_verbose, "braces: %u ctn depth: %u ctn: %s\n", 
			   XORP_UINT_CAST(_braces.back()),
			   ctn->depth(), ctn->segname().c_str());
		while (ctn->depth() > _braces.back()) {
		    //looks like one or more close braces on the same
		    //line as some config - just pop out
		    //appropriately before we can handle this correctly.
		    ctn = ctn->parent();
		    XLOG_TRACE(_verbose, "jumping out one level\n");
		    XLOG_TRACE(_verbose, "braces: %u ctn depth: %u ctn: %s\n", 
			       XORP_UINT_CAST(_braces.back()),
			       ctn->depth(), ctn->segname().c_str());
		}
		// The last brace we entered should match the current depth
		XLOG_ASSERT(_braces.back() == ctn->depth());
		_braces.pop_back();
		// The next brace should be where we're jumping to.
		uint32_t new_depth = 0;
		XLOG_ASSERT(!_braces.empty());
		new_depth = _braces.back();
		XLOG_TRACE(_verbose, "new_depth: %u\n",
			   XORP_UINT_CAST(new_depth));
		XLOG_ASSERT(ctn->depth() > new_depth);
		while (ctn->depth() > new_depth) {
		    ctn = ctn->parent();
		}
		while (path_segments.size() > new_depth) {
		    path_segments.pop_back();
		}
		brace_ctn = ctn;
		XLOG_TRACE(_verbose, "brace_ctn = %s\n",
			   brace_ctn->segname().c_str());
		new_path_segments.pop_front();
		continue;
	    }
	    //
	    // We're expecting a tag, a grouping node, or a leaf node.
	    //
	    path_segments.push_back(new_path_segments.front());
	    ttn = config_tree()->find_template(path_segments);
	    if (ttn == NULL) {
		error_msg = c_format("ERROR: path \"%s\" is not valid.\n",
				     makepath(path_segments).c_str());
		cli_client()->cli_print(error_msg);
		goto cleanup;
	    }
	    //
	    // Test if some part of the configuration path is deprecated
	    //
	    const TemplateTreeNode* deprecated_ttn;
	    deprecated_ttn = ttn->find_first_deprecated_ancestor();
	    if (deprecated_ttn != NULL) {
		error_msg = c_format("ERROR: deprecated path \"%s\" "
				     "is not valid: %s.\n",
				     makepath(path_segments).c_str(),
				     deprecated_ttn->deprecated_reason().c_str());
		cli_client()->cli_print(error_msg);
		goto cleanup;
	    }
	    //
	    // Test if some part of the configuration path is user-hidden
	    //
	    const TemplateTreeNode* user_hidden_ttn;
	    user_hidden_ttn = ttn->find_first_user_hidden_ancestor();
	    if (user_hidden_ttn != NULL) {
		error_msg = c_format("ERROR: user-hidden path \"%s\" "
				     "is not valid: %s.\n",
				     makepath(path_segments).c_str(),
				     user_hidden_ttn->user_hidden_reason().c_str());
		cli_client()->cli_print(error_msg);
		goto cleanup;
	    }

	    XLOG_TRACE(_verbose, "creating node %s\n", ttn->segname().c_str());
	    ctn = new SlaveConfigTreeNode(ttn->segname(), 
					  makepath(path_segments),
					  ttn, ctn,
					  ConfigNodeId::ZERO(),
					  getuid(),
					  clientid(),
					  _verbose);
	    if (first_new_ctn == NULL)
		first_new_ctn = ctn;
	    _changes_made = true;
	    if (ttn->is_tag() || ctn->is_leaf_value()) {
		XLOG_TRACE(_verbose, "value expected\n");
		value_expected = true;
	    } else {
		XLOG_TRACE(_verbose, "value not expected\n");
		value_expected = false;
	    }

	}
	new_path_segments.pop_front();
    }

    //
    // If there's no more input and we still expected a value, the
    // input was erroneous.
    //
    if (value_expected) {
	error_msg = c_format("ERROR: node \"%s\" requires a value.\n",
			     ctn->segname().c_str());
	cli_client()->cli_print(error_msg);
	goto cleanup;
    }

    // Preserve state for next time, and set up the command prompts
    _path = split(brace_ctn->path(), ' ');
    if (_braces.size() == 1) {
	// We're finishing text_entry mode
	_braces.pop_front();
	config_tree()->add_default_children();

	// Force the commands to be regenerated if any changes were made
	if (_mode == CLI_MODE_CONFIGURE && _changes_made)
	    _mode = CLI_MODE_TEXTENTRY;

	configure_mode();
    } else {
	//
	// We need to remain in text_entry mode because we haven't seen
	// the closing braces yet.
	//
	text_entry_mode();
	add_text_entry_commands(NULL);
	CliCommand* com;
	string error_msg;

	com = _cli_node.cli_command_root()->add_command(
	    "}",
	    "Complete this configuration level",
	    false,
	    callback(this, &RouterCLI::text_entry_func),
	    error_msg);
	if (com == NULL) {
	    XLOG_FATAL("add_command %s failed: %s",
		       "}", error_msg.c_str());
	}
	com->set_global_name(token_line2vector("}"));
	com->set_can_pipe(false);
    }

    return (XORP_OK);

 cleanup:
    //
    // Cleanup back to the state we were in before we started parsing this mess
    //
    while (_braces.size() > original_braces_length) {
	_braces.pop_back();
    }

    if (first_new_ctn != NULL) {
	XLOG_TRACE(_verbose, "deleting node %s\n",
		   first_new_ctn->segname().c_str());
	first_new_ctn->delete_subtree_silently();
    }

    return (XORP_ERROR);
}

map<string, CliCommandMatch> 
RouterCLI::text_entry_children_func(const vector<string>& vector_path) const
{
    string command_name;
    string help_string;
    bool is_executable = false;
    bool can_pipe = false;
    map<string, CliCommandMatch> children;
    string path;
    list<string> path_segments;
    const TemplateTreeNode *ttn = NULL;

    path = token_vector2line(vector_path);
    XLOG_TRACE(_verbose, "text_entry_children_func: %s\n", path.c_str());

    string newpath = path;
    while (! newpath.empty()) {
	string::size_type ix = newpath.find(' ');
	if (ix == string::npos) {
	    path_segments.push_back(newpath);
	    break;
	}
	path_segments.push_back(newpath.substr(0, ix));
	newpath = newpath.substr(ix + 1, newpath.size() - ix + 1);
    }

    is_executable = true;
    can_pipe = false;

    const ConfigTreeNode* ctn = config_tree()->find_const_node(path_segments);
    if (ctn != NULL)
	ttn = ctn->template_tree_node();

    if (ttn != NULL && ttn->is_tag()) {
	list<ConfigTreeNode*>::const_iterator iter;
	for (iter = ctn->const_children().begin();
	     iter != ctn->const_children().end();
	     ++iter) {
	    const ConfigTreeNode* ctn_child = *iter;
	    if (ctn_child->template_tree_node() != NULL)
		help_string = ctn_child->template_tree_node()->help();
	    if (help_string == "") {
		help_string = "-- No help available --";
	    }
	    command_name = ctn_child->segname();
	    bool is_executable_tmp = is_executable;
	    bool can_pipe_tmp = can_pipe;
	    CliCommandMatch ccm(command_name, help_string,
				is_executable_tmp, can_pipe_tmp);
	    if (ctn_child->is_leaf_value())
		ccm.set_is_argument_expected(true);
	    children.insert(make_pair(command_name, ccm));
	}
    }

    ttn = template_tree()->find_node(path_segments);
    if (ttn != NULL && (! ttn->is_deprecated()) && (! ttn->is_user_hidden())) {
	list<TemplateTreeNode*>::const_iterator tti;
	for (tti = ttn->children().begin(); tti != ttn->children().end(); 
	     ++tti) {
	    const TemplateTreeNode* ttn_child = *tti;
	    // XXX: ignore deprecated subtrees
	    if (ttn_child->is_deprecated())
		continue;
	    // XXX: ignore user-hidden subtrees
	    if (ttn_child->is_user_hidden())
		continue;
	    help_string = ttn_child->help();
	    if (help_string == "") {
		help_string = "-- No help available --";
	    }
	    if (ttn_child->segname() == "@") {
		if (ttn_child->allowed_values().empty()
		    && ttn_child->allowed_ranges().empty()) {
		    string encoded_typestr = ttn_child->encoded_typestr();
		    command_name = encoded_typestr;
		    CliCommandMatch ccm(command_name, help_string,
					is_executable, can_pipe);
		    CliCommand::TypeMatchCb cb;
		    cb = callback(ttn_child, &TemplateTreeNode::type_match);
		    ccm.set_type_match_cb(cb);
		    if (ttn_child->is_leaf_value())
			ccm.set_is_argument_expected(true);
		    children.insert(make_pair(command_name, ccm));
		} else {
		    add_allowed_values_and_ranges(ttn_child, is_executable,
						  can_pipe, children);
		}
	    } else {
		command_name = ttn_child->segname();
		CliCommandMatch ccm(command_name, help_string,
				    is_executable, can_pipe);
		if (ttn_child->is_leaf_value() || ttn_child->is_tag())
		    ccm.set_is_argument_expected(true);
		children.insert(make_pair(command_name, ccm));
	    }
	}

	if (ttn->is_tag()) {
	    is_executable = false;
	    can_pipe = false;
	}

	if (!ttn->is_tag() && !ttn->children().empty()) {
	    help_string = "Enter text on multiple lines";
	    command_name = "{";
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    if (ttn->is_leaf_value())
		ccm.set_is_argument_expected(true);
	    children.insert(make_pair(command_name, ccm));
	}
	
	if (ttn->is_leaf_value() && (! ttn->is_tag())
	    && ttn->allowed_values().empty()
	    && ttn->allowed_ranges().empty()) {
	    // Leaf node: add a child with expected value type
	    help_string = ttn->help();
	    if (help_string == "") {
		help_string = "-- No help available --";
	    }
	    string encoded_typestr = ttn->encoded_typestr();
	    command_name = encoded_typestr;
	    CliCommandMatch ccm(command_name, help_string, is_executable,
				can_pipe);
	    CliCommand::TypeMatchCb cb;
	    cb = callback(ttn, &TemplateTreeNode::type_match);
	    ccm.set_type_match_cb(cb);
	    children.insert(make_pair(command_name, ccm));
	}

	//
	// Add the command-line completion for the allowed values and ranges
	//
	if (ttn->segname() != "@") {
	    add_allowed_values_and_ranges(ttn, is_executable, can_pipe,
					  children);
	}
    }

    return children;
}

void
RouterCLI::add_allowed_values_and_ranges(
    const TemplateTreeNode* ttn,
    bool is_executable, bool can_pipe,
    map<string, CliCommandMatch>& children) const
{
    //
    // Add the command-line completion for the allowed values
    //
    map<string, string>::const_iterator values_iter;
    for (values_iter = ttn->allowed_values().begin();
	 values_iter != ttn->allowed_values().end();
	 ++values_iter) {
	const string& cmd_name = values_iter->first;
	string help_string = values_iter->second;
	if (help_string == "") {
	    help_string = "-- No help available --";
	}
	CliCommandMatch ccm(cmd_name, help_string,
			    is_executable, can_pipe);
	ccm.set_is_command_argument(true);
	children.insert(make_pair(cmd_name, ccm));
    }

    //
    // Add the command-line completion for the allowed ranges
    //
    map<pair<int64_t, int64_t>, string>::const_iterator ranges_iter;
    for (ranges_iter = ttn->allowed_ranges().begin();
	 ranges_iter != ttn->allowed_ranges().end();
	 ++ranges_iter) {
	const pair<int64_t, int64_t>& range = ranges_iter->first;
	ostringstream ost;
	ost << "[" << range.first << ".." << range.second << "]";
	string cmd_name = ost.str();
	string help_string = ranges_iter->second;
	if (help_string == "") {
	    help_string = "-- No help available --";
	}
	CliCommandMatch ccm(cmd_name, help_string,
			    is_executable, can_pipe);
	CliCommand::TypeMatchCb cb;
	cb = callback(ttn, &TemplateTreeNode::type_match);
	ccm.set_type_match_cb(cb);
	ccm.set_is_command_argument(true);
	children.insert(make_pair(cmd_name, ccm));
    }
}

int
RouterCLI::delete_func(const string& ,
		       const string& ,
		       uint32_t ,
		       const vector<string>& command_global_name,
		       const vector<string>& argv)
{
    string error_msg;
    string path;
    list<string> path_segments;
    string cmd_name = token_vector2line(command_global_name);

    XLOG_ASSERT(cmd_name.substr(0, 7) == "delete ");
    path = cmd_name.substr(7, cmd_name.size() - 7);

    while (! path.empty()) {
	string::size_type ix = path.find(' ');
	if (ix == string::npos) {
	    path_segments.push_back(path);
	    break;
	}
	path_segments.push_back(path.substr(0, ix));
	path = path.substr(ix + 1, path.size() - ix + 1);
    }

    if (! argv.empty()) {
	//
	// XXX: The arguments to the "delete" command should refer
	// to a valid node in the configuration tree, and argument
	// "command_global_name" should include the name of that node.
	// Hence, "argv" should always be empty. If "argv" is not empty,
	// then it is an error.
	//

	//
	// First test whether this is not an existing node that cannot
	// be removed.
	//
	vector<string>::const_iterator iter;
	for (iter = argv.begin(); iter != argv.end(); ++iter) {
	    path_segments.push_back(*iter);
	}
	SlaveConfigTreeNode* ctn = config_tree()->find_node(path_segments);
	if ((ctn != NULL) && ctn->is_permanent()) {
	    error_msg = c_format("ERROR: cannot delete \"%s\" because it is "
			      "a permanent node.\n",
			      makepath(argv).c_str());
	} else {
	    error_msg = c_format("ERROR: cannot delete \"%s\" because it "
				 "doesn't exist.\n",
				 makepath(argv).c_str());
	}
	cli_client()->cli_print(error_msg);
	return (XORP_ERROR);
    }

    //
    // Test if this is a valid node that can be deleted
    //
    SlaveConfigTreeNode* ctn = config_tree()->find_node(path_segments);
    if (ctn == NULL) {
	error_msg = c_format("ERROR: cannot delete \"%s\" because it doesn't "
			     "exist.\n",
			     makepath(path_segments).c_str());
	cli_client()->cli_print(error_msg);
	return (XORP_ERROR);
    }

    string result = config_tree()->show_subtree(true, path_segments, false,
						true);
    cli_client()->cli_print("Deleting: \n");
    cli_client()->cli_print(result + "\n");

    result = config_tree()->mark_subtree_for_deletion(path_segments, getuid());
    cli_client()->cli_print(result + "\n");

    // Regenerate the available command tree without the deleted stuff
    _changes_made = true;
    apply_path_change();

    return (XORP_OK);
}

int
RouterCLI::commit_func(const string& ,
		       const string& ,
		       uint32_t ,
		       const vector<string>& command_global_name,
		       const vector<string>& argv)
{
    string error_msg;
    string cmd_name = token_vector2line(command_global_name);

    if (! argv.empty()) {
	error_msg = c_format("ERROR: \"%s\" does not take any additional "
			     "parameters.\n",
			     cmd_name.c_str());
	cli_client()->cli_print(error_msg);
	return (XORP_ERROR);
    }

    if (! _changes_made) {
	error_msg = c_format("No configuration changes to commit.\n");
	cli_client()->cli_print(error_msg);
	config_mode_prompt();
	return (XORP_ERROR);
    }

    // Clear the UI
    idle_ui();

    string response;
    XLOG_ASSERT(_changes_made);
    bool success = config_tree()->commit_changes(
	response, _xorpsh, callback(this, &RouterCLI::commit_done));
    if (! success) {
	// _changes_made = false;
	cli_client()->cli_print(response);
	cli_client()->cli_print("The configuration has not been changed.\n");
	cli_client()->cli_print("Fix this error, and run \"commit\" again.\n");
	//	set_prompt("", _configuration_mode_prompt);
	//	apply_path_change();
	silent_reenable_ui();

	return (XORP_ERROR);
    }

    // cli_client()->cli_print(response + "\n");
    // apply_path_change();

    return (XORP_OK);
}

void
RouterCLI::idle_ui()
{
    cli_client()->set_is_waiting_for_data(true);
}

void
RouterCLI::silent_reenable_ui()
{
    cli_client()->set_is_waiting_for_data(false);
}

void
RouterCLI::reenable_ui()
{
    cli_client()->set_is_waiting_for_data(false);
    cli_client()->post_process_command();
}

void
RouterCLI::commit_done(bool success, string error_msg)
{
    //
    // If we get a failure back here, report it.  Otherwise expect an
    // XRL from the rtrmgr to inform us what really happened as the
    // change was applied.
    //
    if (! success) {
	cli_client()->cli_print("Commit Failed\n");
	cli_client()->cli_print(error_msg);
    } else {
	_changes_made = false;
	cli_client()->cli_print("OK\n");
	cli_client()->cli_print(error_msg);
    }
    _xorpsh.set_mode(XorpShellBase::MODE_IDLE);
    apply_path_change();
    reenable_ui();
}

int
RouterCLI::show_func(const string& ,
		     const string& ,
		     uint32_t ,			// cli_session_id
		     const vector<string>& command_global_name,
		     const vector<string>& argv)
{
    string error_msg;
    string cmd_name = token_vector2line(command_global_name);
    bool is_show_all = false;
    bool suppress_default_values = true;
    bool show_top = false;
    const string show_command_name = "show";
    const string show_all_command_name = "show -all";

    if (! argv.empty()) {
	//
	// XXX: The arguments to the "show" command should refer
	// to a valid node in the configuration tree, and argument
	// "command_global_name" should include the name of that node.
	// Hence, "argv" should always be empty. If "argv" is not empty,
	// then it is an error.
	//
	error_msg = c_format("ERROR: cannot show \"%s\" because it doesn't "
			     "exist.\n",
			     makepath(argv).c_str());
	cli_client()->cli_print(error_msg);
	return (XORP_ERROR);
    }

    XLOG_ASSERT(cmd_name.substr(0, show_command_name.size())
		== show_command_name);

    //
    // Test if this is the "show -all" command
    //
    if (cmd_name.size() >= show_all_command_name.size()) {
	if (cmd_name.substr(0, show_all_command_name.size())
	    == show_all_command_name)
	    is_show_all = true;
    }
    if (is_show_all)
	suppress_default_values = false;

    string path;

    if ((cmd_name == show_command_name)
	|| (cmd_name == show_all_command_name)) {
	// Command "show" or "show -all" with no parameters at the top level
	path = "";
    } else {
	string::size_type s;
	if (is_show_all)
	    s = show_all_command_name.size() + 1;
	else
	    s = show_command_name.size() + 1;
	path = cmd_name.substr(s, cmd_name.size() - s);
    }

    list<string> path_segments;
    while (! path.empty()) {
	string::size_type ix = path.find(' ');
	if (ix == string::npos) {
	    path_segments.push_back(path);
	    break;
	}
	path_segments.push_back(path.substr(0, ix));
	path = path.substr(ix + 1, path.size() - ix + 1);
    }

    //
    // XXX: if we explicitly specify to show the value of a leaf node,
    // then always show it without suppressing it.
    //
    SlaveConfigTreeNode* ctn = config_tree()->find_node(path_segments);
    if ((ctn != NULL) && ctn->is_leaf_value()) {
	suppress_default_values = false;
	show_top = true;
    }

    string result = config_tree()->show_subtree(show_top, path_segments, false,
						suppress_default_values);
    cli_client()->cli_print(result + "\n");
    config_mode_prompt();

    return (XORP_OK);
}

int
RouterCLI::op_mode_func(const string& ,
			const string& ,		// cli_term_name
			uint32_t ,		// cli_session_id
			const vector<string>& command_global_name,
			const vector<string>& argv)
{
    list<string> path_segments;
    string cmd_name = token_vector2line(command_global_name);
    string full_command;

    full_command = cmd_name;
    path_segments.insert(path_segments.end(), command_global_name.begin(),
			 command_global_name.end());
    for (size_t i = 0; i < argv.size(); i++) {
	if (argv[i] == "|")
	    break;	// The pipe command
	path_segments.push_back(argv[i]);
	full_command += " " + argv[i];
    }

    if (op_cmd_list()->command_match(path_segments, true)) {
	// Clear the UI
	idle_ui();

	// Verify that any previous command has been disposed of.
	XLOG_ASSERT(NULL == _op_mode_cmd);
	_op_mode_cmd = op_cmd_list()->execute(
	    _xorpsh.eventloop(),
	    path_segments,
	    callback(this, &RouterCLI::op_mode_cmd_print),
	    callback(this, &RouterCLI::op_mode_cmd_done));
    } else {
	//
	// Try to figure out where the error is, so we can give useful
	// feedback to the user.
	//
	list<string>::const_iterator iter;
	list<string> test_parts;
	string cmd_error;

	cli_client()->cli_print("ERROR: no matching command:\n");
	for (iter = path_segments.begin();
	     iter != path_segments.end();
	     ++iter) {
	    if (iter != path_segments.begin())
		cmd_error += " ";
	    test_parts.push_back(*iter);
	    if (op_cmd_list()->command_match(test_parts, false)) {
		for (size_t i = 0; i < iter->size(); i++) {
		    cmd_error += " ";
		}
	    } else {
		cmd_error += "^";
		break;
	    }
	}
	cli_client()->cli_print(
	    c_format("%s\n%s\n", full_command.c_str(), cmd_error.c_str())
	    );
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
RouterCLI::op_mode_cmd_print(const string& result)
{
    if (! result.empty()) {
	cli_client()->cli_print(result);
	cli_client()->flush_process_command_output();
    }
}

void
RouterCLI::op_mode_cmd_done(bool success, const string& error_msg)
{
    if (! success) {
	string tmp_error_msg;
	if (! error_msg.empty())
	    tmp_error_msg = error_msg;
	else
	    tmp_error_msg = "<unknown error>";
	cli_client()->cli_print(c_format("ERROR: %s\n", tmp_error_msg.c_str()));
    }
    op_mode_cmd_tidy();
    reenable_ui();
}

void
RouterCLI::op_mode_cmd_interrupt(const string& server_name,
				 const string& cli_term_name,
				 uint32_t cli_session_id,
				 const vector<string>& command_global_name,
				 const vector<string>&  command_args)
{
    //
    // The user has sent an interrupt. If an operational mode command
    // is running terminate it.
    //
    op_mode_cmd_tidy();

    UNUSED(server_name);
    UNUSED(cli_term_name);
    UNUSED(cli_session_id);
    UNUSED(command_global_name);
    UNUSED(command_args);
}

void
RouterCLI::op_mode_cmd_tidy()
{
    if (_op_mode_cmd != NULL) {
	delete _op_mode_cmd;
	_op_mode_cmd = NULL;
    }
}

int
RouterCLI::save_func(const string& ,
		     const string& ,
		     uint32_t ,			// cli_session_id
		     const vector<string>& command_global_name,
		     const vector<string>& argv)
{
    string cmd_name = token_vector2line(command_global_name);

    if (argv.size() != 1) {
	cli_client()->cli_print("Usage: save <filename>\n");
	return (XORP_ERROR);
    }

    XLOG_ASSERT(cmd_name == "save");
    XLOG_TRACE(_verbose, "save, filename = %s\n", argv[0].c_str());
    if (_xorpsh.save_to_file(argv[0],
			     callback(this, &RouterCLI::save_communicated),
			     callback(this, &RouterCLI::save_done))
	!= true) {
	save_done(false, "Cannot send file to rtrmgr. No Finder?\n");
	return (XORP_ERROR);
    }
    idle_ui();

    return (XORP_OK);
}

//
// Method save_communicated() is called when the request for the save has been
// communicated to the rtrmgr, or when this communication has failed.
//
void
RouterCLI::save_communicated(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	cli_client()->cli_print("ERROR: Save failed.\n");
	if (e == XrlError::COMMAND_FAILED()) {
	    if (e.note() == "AUTH_FAIL") {
		check_for_rtrmgr_restart();
		return;
	    }  else {
		cli_client()->cli_print(c_format("%s\n", e.note().c_str()));
	    }
	} else {
	    cli_client()->cli_print("Failed to communicate save command to rtrmgr.\n");
	    cli_client()->cli_print(c_format("%s\n", e.error_msg()));
	}
	_xorpsh.set_mode(XorpShellBase::MODE_IDLE);
	reenable_ui();
	return;
    }
    _xorpsh.set_mode(XorpShellBase::MODE_SAVING);
    //
    // Don't enable the UI - we'll get called back when the saving has
    // completed.
    //
}

//
// Method save_done() is called when the save has completed,
// or when something goes wrong during this process.
//
void
RouterCLI::save_done(bool success, string error_msg)
{
    if (! success) {
	cli_client()->cli_print("ERROR: Save failed.\n");
	cli_client()->cli_print(error_msg);
    } else {
	cli_client()->cli_print("Save done.\n");
    }
    _xorpsh.set_mode(XorpShellBase::MODE_IDLE);
    reenable_ui();
}

int
RouterCLI::load_func(const string& ,
		     const string& ,
		     uint32_t ,
		     const vector<string>& command_global_name,
		     const vector<string>& argv)
{
    string cmd_name = token_vector2line(command_global_name);

    if (argv.size() != 1) {
	cli_client()->cli_print("Usage: load <filename>\n");
	return (XORP_ERROR);
    }

    XLOG_ASSERT(cmd_name == "load");
    XLOG_TRACE(_verbose, "load, filename = %s\n", argv[0].c_str());
    if (_xorpsh.load_from_file(argv[0],
			       callback(this, &RouterCLI::load_communicated),
			       callback(this, &RouterCLI::load_done))
	!= true) {
	load_done(false, "Cannot tell the rtrmgr to load the configuration. "
		  "No Finder?\n");
	return (XORP_ERROR);
    }
    idle_ui();

    return (XORP_OK);
}

//
// Method load_communicated() is called when the request for the load has been
// communicated to the rtrmgr, or when this communication has failed.
//
void
RouterCLI::load_communicated(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	cli_client()->cli_print("ERROR: Load failed.\n");
	if (e == XrlError::COMMAND_FAILED()) {
	    if (e.note() == "AUTH_FAIL") {
		check_for_rtrmgr_restart();
		return;
	    }  else {
		cli_client()->cli_print(c_format("%s\n", e.note().c_str()));
	    }
	} else {
	    cli_client()->cli_print("Failed to communicate load command to rtrmgr.\n");
	    cli_client()->cli_print(c_format("%s\n", e.error_msg()));
	}
	_xorpsh.set_mode(XorpShellBase::MODE_IDLE);
	reenable_ui();
	return;
    }
    _xorpsh.set_mode(XorpShellBase::MODE_LOADING);
    //
    // Don't enable the UI - we'll get called back when the commit has
    // completed.
    //
}

//
// Method load_done() is called when the load has successfully been applied to
// all the router modules, or when something goes wrong during this process.
//
void
RouterCLI::load_done(bool success, string error_msg)
{
    if (! success) {
	cli_client()->cli_print("ERROR: Load failed.\n");
	cli_client()->cli_print(error_msg);
    } else {
	cli_client()->cli_print("Load done.\n");
    }
    _xorpsh.set_mode(XorpShellBase::MODE_IDLE);

    //
    // XXX: Reset the path, because the current node may be gone after
    // we have loaded the new configuration.
    // After that rebuild the whole command tree because the configuration
    // has changed.
    //
    reset_path();
    apply_path_change();
    reenable_ui();
}

//
// Just to make the code more readable:
//
SlaveConfigTree*
RouterCLI::config_tree()
{
    return _xorpsh.config_tree();
}

const SlaveConfigTree*
RouterCLI::config_tree() const
{
    return _xorpsh.config_tree();
}

//
// Just to make the code more readable:
//
const TemplateTree*
RouterCLI::template_tree() const
{
    return _xorpsh.template_tree();
}

//
// Just to make the code more readable:
//
OpCommandList*
RouterCLI::op_cmd_list() const
{
    return _xorpsh.op_cmd_list();
}

//
// Just to make the code more readable:
//
uint32_t
RouterCLI::clientid() const
{
    return _xorpsh.clientid();
}

bool
RouterCLI::check_for_rtrmgr_restart()
{
    return (_xorpsh.get_rtrmgr_pid(
		callback(this, &RouterCLI::verify_rtrmgr_restart))
	    == true);
}

void
RouterCLI::verify_rtrmgr_restart(const XrlError& e, const uint32_t* pid)
{
    if (e == XrlError::OKAY()) {
	if (_xorpsh.rtrmgr_pid() != *pid) {
	    cli_client()->cli_print("FATAL ERROR.\n");
	    cli_client()->cli_print("The router manager process has restarted.\n");
	    cli_client()->cli_print("Advise logout and login again.\n");
	    reenable_ui();
	    return;
	}

	//
	// It's not clear what happened, but attempt to carry on.
	//
	cli_client()->cli_print("ERROR: authentication failure.\n");
	reenable_ui();
	return;
    } else {
	cli_client()->cli_print("FATAL ERROR.\n");
	cli_client()->cli_print("The router manager process is no longer functioning correctly.\n");
	cli_client()->cli_print("Advise logout and login again, but if login fails the router may need rebooting.\n");
	reenable_ui();
	return;
    }
}

string
RouterCLI::makepath(const list<string>& parts) const
{
    string path;
    list<string>::const_iterator iter;

    for (iter = parts.begin(); iter != parts.end(); ++iter) {
	if (path.empty())
	    path = *iter;
	else
	    path += " " + *iter;
    }

    return path;
}

string
RouterCLI::makepath(const vector<string>& parts) const
{
    string path;
    vector<string>::const_iterator iter;

    for (iter = parts.begin(); iter != parts.end(); ++iter) {
	if (path.empty())
	    path = *iter;
	else
	    path += " " + *iter;
    }

    return path;
}
