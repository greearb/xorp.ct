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

#ident "$XORP: xorp/rtrmgr/cli.cc,v 1.47 2004/06/02 04:15:35 hodson Exp $"


#include <pwd.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "cli.hh"
#include "command_tree.hh"
#include "op_commands.hh"
#include "slave_conf_tree.hh"
#include "slave_conf_tree_node.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"


RouterCLI::RouterCLI(XorpShell& xorpsh, CliNode& cli_node, bool verbose)
    : _xorpsh(xorpsh),
      _cli_node(cli_node),
      _cli_client(*(_cli_node.add_stdio_client())),
      _verbose(verbose),
      _mode(CLI_MODE_NONE),
      _changes_made(false)
{
    //
    // We do all this here to allow for later extensions for
    // internationalization.
    //
    _help_o["configure"] = "Switch to configuration mode";
    _help_o["configure exclusive"] 
	= "Switch to configuration mode, locking out other users";
    _help_o["help"] = "Provide help with commands";
    _help_o["quit"] = "Quit this command session";


    _help_c["commit"] = "Commit the current set of changes";
    _help_c["create"] = "Create a sub-element";
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
    _help_c["set"] = "Set the value of a parameter";
    _help_c["show"] = "Show the value of a parameter";
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
configuration, the \"commit\" command is used.  Should the be an error, the\n\
configuration should be rolled back to the previous state.";

    _help_long_c["create"] = "\
The \"create\" command allows you to create a new branch of the \n\
configuration tree.  For example, if the configuration contained:\n\
\n\
    protocols {\n\
        bgp {\n\
            peer 10.0.0.1 {\n\
                as 65001 \n\
            }\n\
        }\n\
    }\n\
\n\
Then typing \"create protocols static\" would create a configuration\n\
branch for the static routes, and will fill it in with the template \n\
default values:\n\
\n\
    protocols {\n\
        bgp {\n\
            peer 10.0.0.1 {\n\
                as 65001 \n\
            }\n\
        }\n\
>       static {\n\
>           targetname: \"static_routes\"\n\
>           enabled: true\n\
>       }\n\
    }\n\
\n\
Then you can use the command \"edit\" to navigate around the new branch\n\
of the configuration tree.  use the command \"set\" to change some of the\n\
values of the new configuration branch:\n\
    set protocols static enabled false\n\
would disable the new static routes module.\n\
\n\
See also the \"commit\", \"delete\", \"edit\" and \"set\" commands.";

    _help_long_c["delete"] ="\
The \"delete\" command is used to delete a part of the configuration tree.\n\
All configuration nodes under the node that is named in the delete command\n\
will also be deleted.  For example, if the configuration contained:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as 65001 \n\
          }\n\
   }\n\
\n\
then typing \"delete protocols bgp\", followed by \"commit\" would cause bgp\n\
to be removed from the configuration, together with the peer 10.0.0.1, etc.\n\
In this case this would also cause the BGP routing process to be shut down.";

    _help_long_c["edit"] = "\
The \"edit\" command allows you to navigate around the configuration tree, so\n\
that changes can be made with minimal typing, or just a part of the router\n\
configuration examined. For example, if the configuration contained:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as 65001 \n\
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
             as 65001 \n\
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
See also \"exit\", \"exit discard\".";

     _help_long_c["exit discard"] = "\
The \"exit discard\" command will cause the login session to return\n\
to operational mode, discarding any changes to the current configuration\n\
that have not been committed.\n\
\n\
See also \"commit\", \"exit configuration-mode\".";

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
             as 65001 \n\
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
See also \"exit\", \"up\", \"top\".";


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
             as 65001 \n\
          }\n\
   }\n\
\n\
Then typing \"set protocols bgp peer 10.0.0.1 as 65002\" would change \n\
the AS number of the BGP peer from 65001 to 65002.  Only parameters \n\
(leaves in the configuration tree) can be set using the \"set\" \n\
command.  New parts of the configuration tree such as a new BGP peer \n\
are created by directly typing them in configuration mode.\n\
\n\
See also \"create\" and \"edit\".";

    _help_long_c["show"] = "\
The \"show\" command will display all or part of the router configuration.\n\
Without any parameters, the \"show\" command will display all of the router\n\
configuration below the current position in the command tree (See the \n\
\"edit\" command for how to move the current position).  The show command \n\
can also take a part of the configuration as parameters; it will then show \n\
only the selected part of the configuration.\n\
\n\
If the configuration has been modified, any changes not yet committed will \n\
be highlighted.  For example, if \"show\" displays:\n\
\n\
  protocols {\n\
      bgp {\n\
>        peer 10.0.0.1 {\n\
>           as 65001 \n\
>        }\n\
  }\n\
\n\
then this indicates that the peer 10.0.0.1 has been created or changed, \n\
and the change has not yet been applied to the running router configuration.";

    _help_long_c["top"] = "\
The \"top\" command will cause the current position in the configuration \n\
to return to the top level of the configuration.  For example, if the \n\
configuration was:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as 65001 \n\
          }\n\
   }\n\
\n\
and the current position is \"protocols bgp peer 10.0.0.1\", then \n\
typing \"top\" would cause the current position to become the top \n\
of the configuration, outside of the \"protocols\" grouping.  The same \n\
result could have been obtained by using the \"exit\" command three \n\
times.\n\
\n\
See also \"exit\", \"quit\", \"up\".";

    _help_long_c["up"] = "\
The \"up\" command will cause the current position in the configuration \n\
to move up one level.  For example, if the configuration was:\n\
\n\
   protocols {\n\
       bgp {\n\
          peer 10.0.0.1 {\n\
             as 65001 \n\
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
See also \"exit\", \"quit\", \"top\".";


    //    _current_config_node = &(config_tree()->root_node());
    operational_mode();

    
}

RouterCLI::~RouterCLI()
{
    // TODO: for now we leave the deletion to be implicit when xorpsh exits
    // _cli_node.delete_stdio_client(&_cli_client);
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
RouterCLI::commit_done_by_user(int uid)
{
    if (_mode != CLI_MODE_OPERATIONAL)
	return;

    //
    // Someone just did a commit, so we may need to regenerate options
    //
    string message = c_format("\nUser %d changed the configuration.\n", uid);
    _cli_client.cli_print(message);
    clear_command_set();
    add_op_mode_commands(NULL);
}

void
RouterCLI::operational_mode()
{
    if (_mode == CLI_MODE_OPERATIONAL)
	return;

    _mode = CLI_MODE_OPERATIONAL;
    clear_command_set();
    reset_path();
    set_prompt("", "Xorp> ");
    add_op_mode_commands(NULL);
}

void
RouterCLI::add_op_mode_commands(CliCommand* com0)
{
    CliCommand *com1, *com2, *help_com, *quit_com;

    // com0->add_command("clear", "Clear information in the system *");

    // If no root node is specified, default to the true root
    if (com0 == NULL)
	com0 = _cli_node.cli_command_root();

    if (com0 == _cli_node.cli_command_root()) {
	com1 = com0->add_command("configure",
				 get_help_o("configure"),
				 callback(this, &RouterCLI::configure_func));
	com1->set_global_name("configure");
	com1->set_can_pipe(false);
	com2 = com1->add_command("exclusive",
				 get_help_o("configure exclusive"),
				 callback(this, &RouterCLI::configure_func));
	com2->set_global_name("configure exclusive");
	com2->set_can_pipe(false);

	// Help Command
	help_com = com0->add_command("help", get_help_o("help"));
	help_com->set_global_name("help");
	help_com->set_dynamic_children_callback(
	    callback(this, &RouterCLI::op_mode_help));
	help_com->set_dynamic_process_callback(
	    callback(this, &RouterCLI::op_help_func));
	help_com->set_can_pipe(true);

	// Quit Command
	quit_com = com0->add_command("quit",
				     get_help_o("quit"),
				     callback(this, &RouterCLI::logout_func));
	quit_com->set_can_pipe(false);
    }

    map<string, string> cmds = op_cmd_list()->top_level_commands();
    map<string, string>::const_iterator iter;
    for (iter = cmds.begin(); iter != cmds.end(); ++iter) {
	com1 = com0->add_command(iter->first, iter->second);
	com1->set_global_name(iter->first);
	// Set the callback to generate the node's children
	com1->set_dynamic_children_callback(callback(op_cmd_list(),
						     &OpCommandList::childlist));
	//
	// Set the callback to pass to the node's children when they
	// are executed.
	//
	com1->set_dynamic_process_callback(callback(this,
						    &RouterCLI::op_mode_func));
	com1->set_can_pipe(false);
    }

    // com1 = com0->add_command("file", "Perform file operations");
    // com1->set_can_pipe(false);
    // com1 = com0->add_command("ping", "Ping a network host");
    // com1->set_can_pipe(true);
    // _set_node = com0->add_command("set", "Set CLI properties");
    // _set_node->set_can_pipe(false);
    // _show_node = com0->add_command("show", "Show router operational imformation");
    // _show_node->set_can_pipe(true);
    // com1 = com0->add_command("traceroute", "Trace the unicast path to a network host");
    // com1->set_can_pipe(true);
}

map<string, string> 
RouterCLI::op_mode_help(const string& path, bool& is_executable,
			bool& can_pipe) const
{
    map<string, string> children;
    string trimmed_path;

    XLOG_ASSERT(path.substr(0, 4) == "help");
    if (path.size() == 4) {
	trimmed_path == "";
    } else {
	XLOG_ASSERT(path.substr(0, 5) == "help ");
	trimmed_path = path.substr(5, path.size() - 5);
    }

    if (trimmed_path == "") {
	// Add the static commands:
	children["configure"] = get_help_o("configure");
	children["quit"] = get_help_o("quit");
	children["help"] = get_help_o("help");
	map<string, string> cmds = op_cmd_list()->top_level_commands();
	map<string, string>::const_iterator iter;
	for (iter = cmds.begin(); iter != cmds.end(); ++iter) {
	    children[iter->first] = c_format("Give help on the \"%s\" command",
					     iter->first.c_str());
	}
	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "configure") {
	is_executable = true;
	can_pipe = false;
	children["exclusive"] = get_help_o("configure exclusive");
    } else if (trimmed_path == "configure exclusive") {
	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "quit") {
	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "help") {
	is_executable = true;
	can_pipe = true;
    } else {
	children = op_cmd_list()->childlist(trimmed_path, is_executable,
					    can_pipe);
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

    set_prompt("", "XORP> ");

    // Add all the menus
    apply_path_change();
}

void
RouterCLI::display_config_mode_users() const
{
    _cli_client.cli_print("Entering configuration mode.\n");

    if (_config_mode_users.empty()) {
	_cli_client.cli_print("There are no other users in configuration mode.\n");
	return;
    }

    //
    // There are other users
    //
    if (_config_mode_users.size() == 1)
	_cli_client.cli_print("User ");
    else
	_cli_client.cli_print("Users ");
    list<uint32_t>::const_iterator iter, iter2;
    struct passwd* pwent;
    for (iter = _config_mode_users.begin();
	 iter != _config_mode_users.end();
	 ++iter) {
	if (iter != _config_mode_users.begin()) {
	    iter2 = iter;
	    ++iter2;
	    if (iter2 == _config_mode_users.end())
		_cli_client.cli_print(" and ");
	    else
		_cli_client.cli_print(", ");
	}
	pwent = getpwuid(*iter);
	if (pwent == NULL)
	    _cli_client.cli_print(c_format("UID:%d", *iter));
	else
	    _cli_client.cli_print(pwent->pw_name);
    }
    endpwent();
    if (_config_mode_users.size() == 1)
	_cli_client.cli_print(" is also in configuration mode.\n");
    else
	_cli_client.cli_print(" are also in configuration mode.\n");
}

void
RouterCLI::display_alerts()
{
    // Display any alert messages that accumulated while we were busy
    while (!_alerts.empty()) {
	_cli_client.cli_print(_alerts.front());
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

void
RouterCLI::add_static_configure_mode_commands()
{
    CliCommand *com0, *com1, *com2, *help_com;

    com0 = _cli_node.cli_command_root();
    if (_changes_made) {
	// Commit command
	com1 = com0->add_command("commit", get_help_c("commit"),
				 callback(this, &RouterCLI::commit_func));
	com1->set_can_pipe(false);
    }

    // Create command
    _create_node = com0->add_command("create", get_help_c("create"));
    _create_node->set_can_pipe(false);

    // Delete command
    _delete_node = com0->add_command("delete", get_help_c("delete"));
    _delete_node->set_can_pipe(false);

    // Edit command
    _edit_node = com0->add_command("edit", get_help_c("edit"),
				   callback(this, &RouterCLI::edit_func));
    _edit_node->set_global_name("edit");
    _edit_node->set_can_pipe(false);

    // Exit command
    com1 = com0->add_command("exit", get_help_c("exit"),
			     callback(this, &RouterCLI::exit_func));
    com1->set_global_name("exit");
    com1->set_can_pipe(false);
    com2 = com1->add_command("configuration-mode",
			     get_help_c("exit configuration_mode"),
			     callback(this, &RouterCLI::exit_func));
    com2->set_global_name("exit configuration-mode");
    com2->set_can_pipe(false);
    com2 = com1->add_command("discard",
			     "Exit from configuration mode, discarding changes",
			     callback(this, &RouterCLI::exit_func));
    com2->set_global_name("exit discard");
    com2->set_can_pipe(false);

    // Help Command
    help_com = com0->add_command("help", get_help_c("help"));
    help_com->set_global_name("help");
    help_com->set_dynamic_children_callback(
	callback(this, &RouterCLI::configure_mode_help));
    help_com->set_dynamic_process_callback(
	callback(this, &RouterCLI::conf_help_func));
    help_com->set_can_pipe(true);

    // Load Command
    com1 = com0->add_command("load", get_help_c("load"),
			     callback(this, &RouterCLI::load_func));
    com1->set_global_name("load");
    com1->set_can_pipe(false);

    // Quit Command
    com1 = com0->add_command("quit", get_help_c("quit"),
			     callback(this, &RouterCLI::exit_func));
    com1->set_global_name("quit");
    com1->set_can_pipe(false);

    _run_node = com0->add_command("run", get_help_c("run"));
    _run_node->set_can_pipe(false);
    add_op_mode_commands(_run_node);

    com1 = com0->add_command("save", get_help_c("save"),
			     callback(this, &RouterCLI::save_func));
    com1->set_global_name("save");
    com1->set_can_pipe(false);

    // Set Command
    _set_node = com0->add_command("set", get_help_c("set"));
    _set_node->set_can_pipe(false);

    // Show Command
    _show_node = com0->add_command("show", get_help_c("show"),
				   callback(this, &RouterCLI::show_func));
    _show_node->set_can_pipe(true);

    // Top Command
    com1 = com0->add_command("top", get_help_c("top"),
			     callback(this, &RouterCLI::exit_func));
    com1->set_global_name("top");
    com1->set_can_pipe(false);

    // Up Command
    com1 = com0->add_command("up", get_help_c("up"),
			     callback(this, &RouterCLI::exit_func));
    com1->set_global_name("up");
    com1->set_can_pipe(false);
}

map<string, string>
RouterCLI::configure_mode_help(const string& path,
			       bool& is_executable,
			       bool& can_pipe) const
{
    map<string, string> children;
    string trimmed_path;

    XLOG_ASSERT(path.substr(0, 4) == "help");
    if (path.size() == 4) {
	trimmed_path == "";
    } else {
	XLOG_ASSERT(path.substr(0, 5) == "help ");
	trimmed_path = path.substr(5, path.size() - 5);
    }

    if (trimmed_path == "") {
	// Add the static commands:
	children["commit"] = get_help_c("commit");
	children["create"] = get_help_c("create");
	children["delete"] = get_help_c("delete");
	children["edit"] = get_help_c("edit");
	children["exit"] = get_help_c("exit");
	children["help"] = get_help_c("help");
	children["load"] = get_help_c("load");
	children["quit"] = get_help_c("quit");
	children["run"] = get_help_c("run");
	children["save"] = get_help_c("save");
	children["set"] = get_help_c("set");
	children["show"] = get_help_c("show");
	children["top"] = get_help_c("top");
	children["up"] = get_help_c("up");

	//
	// TODO: need to insert the commands that come from the template
	// tree here.
	//

	is_executable = true;
	can_pipe = false;
    } else if (trimmed_path == "exit") {
	children["configuration-mode"] = get_help_c("exit configuration-mode");
	children["discard"] = get_help_c("exit discard");
	is_executable = true;
	can_pipe = false;
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
    set_prompt(prompt, "XORP> ");
}

void
RouterCLI::apply_path_change()
{
    _cli_node.cli_command_root()->delete_all_commands();
    add_static_configure_mode_commands();

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
    string cmdpath;
    if (_path.empty())
	cmdpath = "show";
    else
	cmdpath = "show " + pathstr();
    _show_node->set_global_name(cmdpath);

    // Set the prompt appropriately
    config_mode_prompt();
}

void
RouterCLI::set_prompt(const string& line1, const string& line2)
{
    if (line1 != "") {
	_cli_client.cli_print(line1 + "\n");
    }
    _cli_node.cli_command_root()->set_allow_cd(true, line2);
    _cli_client.set_current_cli_prompt(line2);
}

void
RouterCLI::add_command_subtree(CliCommand& current_cli_node,
			       const CommandTreeNode& current_ctn,
			       const CLI_PROCESS_CALLBACK& cb,
			       string path,
			       size_t depth,
			       bool can_pipe)
{
    const list<CommandTreeNode*>& children = current_ctn.children();
    if (depth > 0) {
	if (path.empty())
	    path = current_ctn.name();
	else
	    path += " " + current_ctn.name();
    }

    list<CommandTreeNode*>::const_iterator cmd_iter;
    for (cmd_iter = children.begin(); cmd_iter != children.end(); ++cmd_iter) {
	string cmd_name = (*cmd_iter)->name();
	CliCommand* com;
	string subpath = path + " " + cmd_name;
	string help = (*cmd_iter)->help();
	if (help == "") {
	    help = "-- no help available --";
	}
	if ((*cmd_iter)->has_command()) {
	    com = current_cli_node.add_command(cmd_name, help, cb);
	} else {
	    com = current_cli_node.add_command(cmd_name, help);
	}
	if (com == NULL) {
	    XLOG_FATAL("add_command %s failed", cmd_name.c_str());
	}

	com->set_can_pipe(can_pipe);
	com->set_global_name(subpath);
	add_command_subtree(*com, *(*cmd_iter), cb, path, depth + 1, can_pipe);
    }
}

#if 0
void
RouterCLI::add_immediate_commands(CliCommand& current_cli_node,
				  const CommandTree& command_tree,
				  const list<string>& cmd_names,
				  bool include_intermediates,
				  const CLI_PROCESS_CALLBACK& cb,
				  const string& path,
				  bool can_pipe)
{
    string subpath;
    set<string> existing_children;
    const list<CommandTreeNode*>& children = command_tree.root_node().children();
    list<CommandTreeNode*>::const_iterator cmd_iter;

    for (cmd_iter = children.begin(); cmd_iter != children.end(); ++cmd_iter) {
	if (include_intermediates || (*cmd_iter)->has_command()) {
	    if (path.empty())
		subpath = (*cmd_iter)->name();
	    else
		subpath = path + " " + (*cmd_iter)->name();

	    CliCommand* com;
	    string help = (*cmd_iter)->help();
	    if (help == "") {
		help = "-- no help available --";
	    }
	    com = current_cli_node.add_command((*cmd_iter)->name(), help, cb);
	    if (com == NULL) {
		XLOG_FATAL("AI: add_command %s failed",
			   (*cmd_iter)->name().c_str());
	    } else {
		com->set_global_name(subpath);
	    }
	    com->set_can_pipe(can_pipe);
	    existing_children.insert((*cmd_iter)->name());
	}
    }

    const TemplateTreeNode* ttn;
    if (_current_config_node->is_root_node()) {
	ttn = template_tree()->root_node();
    } else {
	ttn = _current_config_node->template_tree_node();
    }
    XLOG_ASSERT(ttn != NULL);

    list<TemplateTreeNode*>::const_iterator tti;
    for (tti = ttn->children().begin(); tti != ttn->children().end(); ++tti) {
	// We don't need to consider this child if it's already added
	if (existing_children.find((*tti)->segname())
	    != existing_children.end()) {
	    continue;
	}
	if ((*tti)->check_command_tree(cmd_names, include_intermediates,
				       0 /* depth */)) {
	    if (path.empty())
		subpath = (*tti)->segname();
	    else
		subpath = path + " " + (*tti)->segname();

	    string help = (*tti)->help();
	    if (help == "") {
		help = "-- no help available --";
	    }
	    CliCommand* com;
	    com = current_cli_node.add_command((*tti)->segname(), help, cb);
	    if (com == NULL) {
		XLOG_FATAL("AI: add_command %s for template failed",
			   (*tti)->segname().c_str());
	    } else {
		com->set_global_name(subpath);
	    }
	    com->set_can_pipe(can_pipe);
	}
    }
}
#endif // 0

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
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    XLOG_ASSERT(current_config_node != NULL);

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

    current_config_node->create_command_tree(cmd_tree,
					     cmds,
					     true, /* include_intermediates */
					     false /* include_templates */);
    debug_msg("==========================================================\n");
    debug_msg("edit subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    if (_braces.empty()) {
	string cmdpath;
	if (_path.empty())
	    cmdpath = "edit";
	else
	    cmdpath = "edit " + pathstr();

	add_command_subtree(*_edit_node, cmd_tree.root_node(),
			    callback(this, &RouterCLI::edit_func),
			    cmdpath, 0,
			    false /* can_pipe */);
    }
#if 0
    add_immediate_commands(*(_cli_node.cli_command_root()),
			   cmd_tree,
			   cmds,
			   true, /* include_intermediates */
			   callback(this, &RouterCLI::text_entry_func),
			   pathstr(),
			   false /* can_pipe */);
#endif
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
    cmds.push_back("%set");
    cmds.push_back("%delete");
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    current_config_node->create_command_tree(cmd_tree,
					     cmds,
					     true, /* include_intermediates */
					     false /* include_templates */);

    debug_msg("==========================================================\n");
    debug_msg("delete subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    string cmdpath;
    if (_path.empty())
	cmdpath = "delete";
    else
	cmdpath = "delete " + pathstr();

    add_command_subtree(*_delete_node, cmd_tree.root_node(),
			callback(this, &RouterCLI::delete_func),
			cmdpath, 0,
			false /* can_pipe */);
}

void
RouterCLI::add_set_subtree()
{
    CommandTree cmd_tree;
    list<string> cmds;

    cmds.push_back("%set");
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    current_config_node->create_command_tree(cmd_tree,
					     cmds,
					     false, /* include_intermediates */
					     true /* include_templates */);

    debug_msg("==========================================================\n");
    debug_msg("set subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    if (_braces.empty()) {
	string cmdpath;
	if (_path.empty())
	    cmdpath = "set";
	else
	    cmdpath = "set " + pathstr();

	add_command_subtree(*_set_node, cmd_tree.root_node(),
			    callback(this, &RouterCLI::set_func),
			    cmdpath, 0,
			    false /* can_pipe */);
    }
#if 0
    add_immediate_commands(*(_cli_node.cli_command_root()),
			   cmd_tree,
			   cmds,
			   false, /* include_intermediates */
			   callback(this, &RouterCLI::immediate_set_func),
			   pathstr(),
			   false /* can_pipe */);
#endif // 0
}

void
RouterCLI::add_show_subtree()
{
    CommandTree cmd_tree;
    list<string> cmds;

    cmds.push_back("%get");
    SlaveConfigTreeNode *current_config_node = config_tree()->find_node(_path);
    current_config_node->create_command_tree(cmd_tree,
					     cmds,
					     true, /* include_intermediates */
					     false /* include_templates */);

    debug_msg("==========================================================\n");
    debug_msg("show subtree is:\n\n");
    debug_msg("%s", cmd_tree.tree_str().c_str());
    debug_msg("==========================================================\n");

    string cmdpath;
    if (_path.empty())
	cmdpath = "show";
    else
	cmdpath = "show " + pathstr();

    add_command_subtree(*_show_node, cmd_tree.root_node(),
			callback(this, &RouterCLI::show_func),
			cmdpath, 0,
			true /* can_pipe */);
}

void
RouterCLI::add_text_entry_commands(CliCommand* com0)
{
    const SlaveConfigTreeNode* current_config_node;
    const TemplateTreeNode* ttn;

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
	 CliCommand* com;
	 string subpath;

	 if (pathstr().empty())
	     subpath = (*tti)->segname();
	 else
	     subpath = pathstr() + " " + (*tti)->segname();

	 string help = (*tti)->help();
	 if (help == "") {
	     help = "-- no help available --";
	 }

	 com = com0->add_command((*tti)->segname(),
				 help, callback(this,
						&RouterCLI::text_entry_func));
	 if (com == NULL) {
	     XLOG_FATAL("AI: add_command %s for template failed",
			(*tti)->segname().c_str());
	 } else {
	     com->set_global_name(subpath);
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
			  const string& command_global_name,
			  const vector<string>& /* argv */)
{
    bool exclusive = false;

    if (command_global_name == "configure exclusive")
	exclusive = true;

    idle_ui();
    _xorpsh.enter_config_mode(exclusive,
			       callback(this, &RouterCLI::enter_config_done));

     return (XORP_OK);
}

void
RouterCLI::enter_config_done(const XrlError& e)
{
    if (e == XrlError::OKAY()) {
	_xorpsh.get_config_users(callback(this, &RouterCLI::got_config_users));
	return;
    }

    if ((e == XrlError::COMMAND_FAILED()) && (e.note() == "AUTH_FAIL")) {
	check_for_rtrmgr_restart();
	return;
    }

    //
    // Either something really bad happened, or a user that didn't
    // have permission attempted to enter config mode.
    //
    string errmsg = c_format("ERROR: %s.\n", e.note().c_str());
    _cli_client.cli_print(errmsg);
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
	_cli_client.cli_print("ERROR: failed to get config users from rtrmgr.\n");
    } else {
	_config_mode_users.clear();

	size_t nusers = users->size();
	bool doneme = false;
	for (size_t i = 0; i < nusers; i++) {
	    XrlAtom a;
	    a = users->get(i);
	    try {
		uint32_t uid = a.uint32();
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
    struct passwd* pwent = getpwuid(user_id);
    string username;
    if (pwent == NULL)
	username = c_format("UID:%d", user_id);
    else
	username = pwent->pw_name;
    string alert = c_format("User %s entered configuration mode\n",
			    username.c_str());
    notify_user(alert, false /* not urgent */);
}

void
RouterCLI::leave_config_done(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	_cli_client.cli_print("ERROR: failed to inform rtrmgr of entry into config mode.\n");
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
	_cli_client.cli_print(alert);
    }
}

int
RouterCLI::op_help_func(const string& ,
			const string& ,
			uint32_t ,		// cli_session_id
			const string& command_global_name,
			const vector<string>& argv)
{
    if (! argv.empty())
	return (XORP_ERROR);

    string cmd_name = command_global_name;
    string path;
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
	_cli_client.cli_print("\n" + i->second + "\n\n");
	return (XORP_OK);
    } 

    //
    // There was no long help description available.  If there's a
    // short description, that will have to do.
    //
    i = _help_o.find(path);
    if (i != _help_o.end()) {
	_cli_client.cli_print(i->second + "\n");
	return (XORP_OK);
    }

    _cli_client.cli_print("Sorry, no help available for " + path + "\n");

    return (XORP_OK);
}

int
RouterCLI::conf_help_func(const string& ,
			  const string& ,
			  uint32_t ,		// cli_session_id
			  const string& command_global_name,
			  const vector<string>& argv)
{
    if (! argv.empty())
	return (XORP_ERROR);

    string cmd_name = command_global_name;
    string path;
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
	_cli_client.cli_print("\n" + i->second + "\n\n");
	return (XORP_OK);
    } 

    //
    // There was no long help description available.  If there's a
    // short description, that will have to do.
    //
    i = _help_c.find(path);
    if (i != _help_c.end()) {
	_cli_client.cli_print(i->second + "\n");
	return (XORP_OK);
    }

    _cli_client.cli_print("Sorry, no help available for " + path + "\n");

    return (XORP_OK);
}

int
RouterCLI::logout_func(const string& ,
		       const string& ,
		       uint32_t ,		// cli_session_id
		       const string& command_global_name,
		       const vector<string>& argv)
{
    if (! argv.empty()) {
	string errmsg = c_format("ERROR: \"%s\" does not take "
				 "any additional parameters.\n",
				 command_global_name.c_str());
	_cli_client.cli_print(errmsg);
	return (XORP_ERROR);
    }

    idle_ui();
    _cli_node.delete_stdio_client(&_cli_client);

    return (XORP_OK);
}

int
RouterCLI::exit_func(const string& ,
		     const string& ,
		     uint32_t ,
		     const string& command_global_name,
		     const vector<string>& argv)
{
    if (command_global_name == "exit configuration-mode") {
	if (! argv.empty()) {
	    string errmsg = c_format("ERROR: \"%s\" does not take "
				     "any additional parameters.\n",
				     command_global_name.c_str());
	    _cli_client.cli_print(errmsg);
	    return (XORP_ERROR);
	}
	if (_changes_made) {
	    _cli_client.cli_print("ERROR: There are uncommitted changes.\n");
	    _cli_client.cli_print("Use \"commit\" to commit the changes, "
				  "or \"exit discard\" to discard them.\n");
	    return (XORP_ERROR);
	}
	idle_ui();
	_xorpsh.leave_config_mode(callback(this,
					   &RouterCLI::leave_config_done));
	return (XORP_OK);
    }
    if (command_global_name == "exit discard") {
	if (! argv.empty()) {
	    string errmsg = c_format("ERROR: \"%s\" does not take "
				     "any additional parameters.\n",
				     command_global_name.c_str());
	    _cli_client.cli_print(errmsg);
	    return (XORP_ERROR);
	}
	config_tree()->discard_changes();
	idle_ui();
	_xorpsh.leave_config_mode(callback(this,
					   &RouterCLI::leave_config_done));
	return (XORP_OK);
    }
    if (command_global_name == "top") {
	if (! argv.empty()) {
	    string errmsg = c_format("ERROR: \"%s\" does not take "
				     "any additional parameters.\n",
				     command_global_name.c_str());
	    _cli_client.cli_print(errmsg);
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
    if (command_global_name == "up") {
	if (! argv.empty()) {
	    string errmsg = c_format("ERROR: \"%s\" does not take "
				     "any additional parameters.\n",
				     command_global_name.c_str());
	    _cli_client.cli_print(errmsg);
	    return (XORP_ERROR);
	}
	if (! _path.empty())
	    _path.pop_back();
    }
    if ((command_global_name == "exit") || (command_global_name == "quit")) {
	if (! _path.empty()) {
	    _path.pop_back();
	} else {
	    if (_changes_made) {
		_cli_client.cli_print("ERROR: There are uncommitted changes.\n");
		_cli_client.cli_print("Use \"commit\" to commit the changes, "
				      "or \"exit discard\" to discard them.\n");
		return (XORP_ERROR);
	    }
	    idle_ui();
	    _xorpsh.leave_config_mode(callback(this,
					       &RouterCLI::leave_config_done));
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
		     const string& command_global_name,
		     const vector<string>& argv)
{
    string cmd_name = command_global_name;
    string path;

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
	_cli_client.cli_print(msg);
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

#if 0
int
RouterCLI::text_entry_func(const string& ,
			   const string& ,
			   uint32_t ,		// cli_session_id
			   const string& command_global_name,
			   const vector<string>& argv)
{
    string path = command_global_name;
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

    SlaveConfigTreeNode* ctn = config_tree()->find_node(path_segments);
    const TemplateTreeNode* tag_ttn  = 0;
    const TemplateTreeNode* data_ttn = 0;
    bool create_needed = false;
    string nodename;

    if ((! argv.empty()) || (path_segments.back() != "}")) {
	if (ctn != NULL) {
	    tag_ttn = ctn->template_tree_node();
	} else {
	    create_needed = true;
	    tag_ttn = config_tree()->find_template(path_segments);
	    XLOG_ASSERT(tag_ttn != NULL);
	    nodename = path_segments.back();
	    path_segments.pop_back();
	    ctn = config_tree()->find_node(path_segments);
	}

	// If there are no arguments, it must be a void structuring node
	if (argv.empty()
	    && (tag_ttn->type() != NODE_VOID || tag_ttn->is_tag())) {
	    _cli_client.cli_print("ERROR: Insufficient arguments.\n");
	    return (XORP_ERROR);
	}

	XLOG_ASSERT(tag_ttn->is_tag() || (tag_ttn->type() == NODE_VOID));

	if (tag_ttn->is_tag()) {
	    // We're dealing with a tag node, plus value.

	    // Check type of argv[0] is OK
	    list<TemplateTreeNode*>::const_iterator iter;
	    data_ttn = NULL;

	    //
	    // The tag can have multiple children of different types -
	    // take the first that has valid syntax.
	    //
	    // TODO: we really ought to check the allow commands here
	    for (iter = tag_ttn->children().begin();
		 iter != tag_ttn->children().end();
		 ++iter) {
		if ((*iter)->type_match(argv[0])) {
		    data_ttn = (*iter);
		    break;
		}
	    }
	    if (data_ttn == NULL) {
		string errmsg = c_format("ERROR: argument \"%s\" "
					 "is not a valid \"%s\".\n",
					 argv[0].c_str(),
					 tag_ttn->typestr().c_str());
		_cli_client.cli_print(errmsg);
		return (XORP_ERROR);
	    }

	    //
	    // Check we're not being asked to create a node that
	    // already exists.
	    //
	    if (create_needed == false) {
		list<ConfigTreeNode*>::const_iterator cti;
		for (cti = ctn->children().begin();
		     cti != ctn->children().end();
		     ++cti) {
		    if ((*cti)->segname() == argv[0]) {
			string errmsg = c_format("ERROR: can't create \"%s\", "
						 "as it already exists.\n",
						 argv[0].c_str());
			_cli_client.cli_print(errmsg);
			return (XORP_ERROR);
		    }
		}
	    }

	    // Check remaining args are OK
	    if (argv.size() == 2 && argv[1] != "{") {
		string errmsg = c_format("ERROR: \"{\" expected instead of "
					 "\"%s\".\n",
					 argv[1].c_str());
		_cli_client.cli_print(errmsg);
		return (XORP_ERROR);
	    } else if (argv.size() > 2) {
		_cli_client.cli_print("ERROR: too many arguments.\n");
		return (XORP_ERROR);
	    }
	} else {
	    //
	    // We're dealing with a NODE_VOID structuring node.
	    //
	    XLOG_ASSERT(tag_ttn->type() == NODE_VOID);
	    if (argv.size() == 1 && argv[0] != "{") {
		string errmsg = c_format("ERROR: \"{\" expected instead of "
					 "\"%s\".\n",
					 argv[0]);
		_cli_client.cli_print(errmsg);
		return (XORP_ERROR);
	    } else if (argv.size() > 1) {
		_cli_client.cli_print("ERROR: too many arguments.\n");
		return (XORP_ERROR);
	    }
	}

	// Create the parent node, if needed
	string newpath = pathstr();
	SlaveConfigTreeNode* newnode;
	if (create_needed) {
	    path_segments.push_back(nodename);
	    list<string>::const_iterator iter;
	    for (iter = path_segments.begin(); iter != path_segments.end();
		 ++iter) {
		if (newpath.empty())
		    newpath = *iter;
		else
		    newpath += " " + *iter;
	    }
	    ctn = new SlaveConfigTreeNode(path_segments.back(), newpath,
					  tag_ttn, _current_config_node,
					  getuid());
	}

	if (tag_ttn->is_tag()) {
	    //
	    // We're dealing with a tag node, plus value.
	    //
	    newpath += " " + argv[0];
	    newnode = new SlaveConfigTreeNode(argv[0], newpath, data_ttn,
					      ctn, getuid());

	    // Check that the value was OK
	    string errmsg;
	    if (newnode->check_allowed_value(errmsg) == false) {
		_cli_client.cli_print(errmsg);
		// If the value was bad, remove the nodes we just added
		ctn->remove_child(newnode);
		delete newnode;
		if (create_needed) {
		    _current_config_node->remove_child(ctn);
		    delete ctn;
		}
		return (XORP_ERROR);
	    }

	    if (argv.size() == 1 && _nesting_depth == 0) {
		// Nothing more to enter
		_cli_client.cli_print("OK\n");
		_changes_made = true;
		// Apply path change to add the commit command
		apply_path_change();
		// Add any default children for the new node
		newnode->add_default_children();
		return (XORP_OK);
	    }

	    if (argv.size() > 1) {
		_current_config_node = newnode;
		_path.push_back(path_segments.back());
		_path.push_back(argv[0]);
		_nesting_lengths.push_back(2);
		_nesting_depth++;
	    }
	} else {
	    //
	    // We're dealing with a NODE_VOID structuring node.
	    //
	    if (argv.empty() && _nesting_depth == 0) {
		// Nothing more to enter
		_cli_client.cli_print("OK\n");
		_changes_made = true;
		// Apply path change to add the commit command
		apply_path_change();
		// Add any default children for the new node
		ctn->add_default_children();
		return (XORP_OK);
	    }

	    XLOG_ASSERT(tag_ttn->type() == NODE_VOID);
	    _current_config_node = ctn;
	    _path.push_back(path_segments.back());
	    _nesting_lengths.push_back(1);
	    _nesting_depth++;
	}
	text_entry_mode();
	add_edit_subtree();
	add_set_subtree();
	CliCommand* com;
	com = _cli_node.cli_command_root()->add_command(
	    "}",
	    "complete this configuration level",
	    callback(this, &RouterCLI::text_entry_func));
	com->set_global_name("}");
	com->set_can_pipe(false);
	_changes_made = true;
	return (XORP_OK);
    } else {
	XLOG_ASSERT(path_segments.back() == "}");
	XLOG_ASSERT(_nesting_depth > 0);
	for (size_t i = 0; i < _nesting_lengths.back(); i++) {
	    _current_config_node = _current_config_node->parent();
	    XLOG_ASSERT(_current_config_node != NULL);
	    _path.pop_back();
	}
	_nesting_lengths.pop_back();
	_nesting_depth--;
	if (_nesting_depth > 0) {
	    XLOG_ASSERT(!_path.empty());
	    text_entry_mode();
	    add_edit_subtree();
	    add_set_subtree();
	    CliCommand* com;
	    com = _cli_node.cli_command_root()->add_command(
		"}",
		"complete this configuration level",
		callback(this, &RouterCLI::text_entry_func));
	    com->set_global_name("}");
	    com->set_can_pipe(false);
	    _changes_made = true;
	    return (XORP_OK);
	} else {
	    _cli_client.cli_print("OK.  Use \"commit\" to apply these changes.\n");
	    _changes_made = true;
	    config_tree()->add_default_children();
	    configure_mode();
	    return (XORP_OK);
	}
    }
}
#endif // 0

int
RouterCLI::text_entry_func(const string& ,
			   const string& ,
			   uint32_t ,		// cli_session_id
			   const string& command_global_name,
			   const vector<string>& argv)
{
    //
    // The path contains the part of the command we managed to command-line
    // complete.  The remainder is in argv.
    //
    string path = command_global_name;
    list<string> path_segments;
    XLOG_TRACE(_verbose, "text_entry_func: %s\n", path.c_str());
    SlaveConfigTreeNode *ctn = NULL, *original_ctn = NULL, *brace_ctn;

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
	    brace_ctn = &(config_tree()->root_node());
	} else {
	    _braces.push_front(brace_ctn->depth());
	}
    }

    path_segments = split(path, ' ');

    //
    // Push argv onto the list of path segments - there's no
    // substantial difference between the path and argv, except that
    // path has already seen some sanity checking.
    //
    for (size_t i = 0; i < argv.size(); i++) {
	path_segments.push_back(argv[i]);
    }

    //
    // The path_segments probably contain part of the path that already
    // exists and part that does not yet exist.  Find which is which.
    //
    list <string> new_path_segments;
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
	ctn = &(config_tree()->root_node());

    //
    // At this point, path_segments contains the path of nodes that
    // already exist, new_path_segments contains the path that does not
    // yet exist, and ctn points to the last existing node.
    //

    const TemplateTreeNode* ttn = NULL;

    //
    // Variable "value_expected" keeps track of whether the next
    // segment should be a value for the node we just created.
    //
    bool value_expected = false;


    if (ctn->is_tag() || ctn->is_leaf())
	value_expected = true;

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
		string errmsg = c_format("ERROR: a value for \"%s\" "
					 "is required.\n",
					 ctn->segname().c_str());
		_cli_client.cli_print(errmsg);
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
		list<TemplateTreeNode*>::const_iterator tti;
		for (tti = ttn->children().begin();
		     tti != ttn->children().end();
		     ++tti) {
		    if ((*tti)->type_match(value)) {
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
		    string errmsg = c_format("ERROR: argument \"%s\" "
					     "is not a valid %s.\n",
					     value.c_str(),
					     cand_types.c_str());
		    _cli_client.cli_print(errmsg);
		    goto cleanup;
		}
		path_segments.push_back(value);
		XLOG_TRACE(_verbose, "creating node %s\n", value.c_str());
		ctn = new SlaveConfigTreeNode(value, 
					      makepath(path_segments),
					      data_ttn, ctn, getuid(),
					      _verbose);
		_changes_made = true;
		value_expected = false;
	    } else if (ctn->is_leaf()) {
		// It must be a leaf, and we're expecting a value
		if (ttn->type_match(value)) {
		    XLOG_TRACE(_verbose, "setting node %s to %s\n", 
			       ctn->segname().c_str(),
			       value.c_str());
		    ctn->set_value(value, getuid());
		    value_expected = false;
		} else {
		    string errmsg = c_format("ERROR: argument \"%s\" "
					     "is not a valid \"%s\".\n",
					     value.c_str(),
					     ttn->typestr().c_str());
		    _cli_client.cli_print(errmsg);
		    goto cleanup;
		}
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
		    _cli_client.cli_print("ERROR: mismatched \"}\".\n");
		    goto cleanup;
		}
		// The last brace we entered should match the current depth
		XLOG_TRACE(_verbose, "braces: %u ctn depth: %u ctn: %s\n", 
			   _braces.back(),
			   ctn->depth(), ctn->segname().c_str());
		XLOG_ASSERT(_braces.back() == ctn->depth());
		_braces.pop_back();
		// The next brace should be where we're jumping to.
		uint32_t new_depth = 0;
		XLOG_ASSERT(!_braces.empty());
		new_depth = _braces.back();
		XLOG_TRACE(_verbose, "new_depth: %u\n", new_depth);
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
		string errmsg = c_format("ERROR: path \"%s\" "
					 "is not valid.\n",
					 makepath(path_segments).c_str());
		_cli_client.cli_print(errmsg);
		goto cleanup;
	    }

	    XLOG_TRACE(_verbose, "creating node %s\n", ttn->segname().c_str());
	    ctn = new SlaveConfigTreeNode(ttn->segname(), 
					  makepath(path_segments),
					  ttn, ctn, getuid(),
					  _verbose);
	    _changes_made = true;
	    if (ttn->is_tag() || ctn->is_leaf()) {
		XLOG_TRACE(_verbose, "value expected\n");
		value_expected = true;
	    } else {
		XLOG_TRACE(_verbose, "value not expected\n");
		value_expected = false;
	    }

	}
	//
	// Keep track of where we started because if there's an error we'll
	// need to clear out this node and all its children.
	//
	if (original_ctn == NULL)
	    original_ctn = ctn;
	    
	new_path_segments.pop_front();
    }

    //
    // If there's no more input and we still expected a value, the
    // input was erroneous.
    //
    if (value_expected) {
	string errmsg = c_format("ERROR: node \"%s\" requires a value.\n",
				 ctn->segname().c_str());
	_cli_client.cli_print(errmsg);
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
	com = _cli_node.cli_command_root()->add_command(
	    "}",
	    "complete this configuration level",
	    callback(this, &RouterCLI::text_entry_func));
	com->set_global_name("}");
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

    if (original_ctn != NULL) {
	XLOG_TRACE(_verbose, "deleting node %s\n",
		   original_ctn->segname().c_str());
	original_ctn->delete_subtree_silently();
    }

    return (XORP_ERROR);
}

map<string, string> 
RouterCLI::text_entry_children_func(const string& path,
				    bool& is_executable,
				    bool& can_pipe) const
{
    map <string, string> children;
    list<string> path_segments;

    XLOG_TRACE(_verbose, "text_entry_children_func: %s\n", path.c_str());

    string newpath = path;
    while (! newpath.empty()) {
	string::size_type ix = newpath.find(' ');
	if (ix == string::npos){
	    path_segments.push_back(newpath);
	    break;
	}
	path_segments.push_back(newpath.substr(0, ix));
	newpath = newpath.substr(ix + 1, newpath.size() - ix + 1);
    }

    const TemplateTreeNode *ttn = template_tree()->find_node(path_segments);
    is_executable = true;
    can_pipe = false;
    if (ttn != NULL) {
	list<TemplateTreeNode*>::const_iterator tti;
	for (tti = ttn->children().begin(); tti != ttn->children().end(); 
	     ++tti) {
	    string help;
	    help = (*tti)->help();
	    string subpath;
	    if (help == "") {
		help = "-- no help available --";
	    }
	    if ((*tti)->segname() == "@") {
#if 0
		string typestr = "<" + (*tti)->typestr() + ">";
		children[typestr] = help;
#endif // 0
	    } else {
		children[(*tti)->segname()] = help;
	    }
	}
#if 0
	if (ttn->is_tag()) {
	    is_executable = false;
	    can_pipe = false;
	}
#endif // 0
	if (!ttn->is_tag() && !ttn->children().empty()) {
	    children["{"] = "enter text on multiple lines";
	}
    }

    return children;
}

int
RouterCLI::delete_func(const string& ,
		       const string& ,
		       uint32_t ,
		       const string& command_global_name,
		       const vector<string>& argv)
{
    if (! argv.empty())
	return (XORP_ERROR);

    string cmd_name = command_global_name;
    XLOG_ASSERT(cmd_name.substr(0, 7) == "delete ");
    string path = cmd_name.substr(7, cmd_name.size() - 7);
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

    string result = config_tree()->show_subtree(path_segments);
    _cli_client.cli_print("Deleting: \n");
    _cli_client.cli_print(result + "\n");

    result = config_tree()->mark_subtree_for_deletion(path_segments, getuid());
    _cli_client.cli_print(result + "\n");

    // Regenerate the available command tree without the deleted stuff
    _changes_made = true;
    apply_path_change();

    return (XORP_OK);
}

int
RouterCLI::set_func(const string& ,
		    const string& ,
		    uint32_t ,
		    const string& command_global_name,
		    const vector<string>& argv)
{
    string cmd_name = command_global_name;
    XLOG_ASSERT(cmd_name.substr(0, 4) == "set ");
    string path = cmd_name.substr(4, cmd_name.size() - 4);
    string response = run_set_command(path, argv);

    _cli_client.cli_print(response + "\n");
    apply_path_change();

    return (XORP_OK);
}

int
RouterCLI::immediate_set_func(const string& ,
			      const string& ,
			      uint32_t ,
			      const string& command_global_name,
			      const vector<string>& argv)
{
    string path = command_global_name;
    string response = run_set_command(path, argv);

    if (_braces.empty()) {
	_cli_client.cli_print(response + "\n");
	config_mode_prompt();
    } else {
	if (response != "OK")
	    _cli_client.cli_print(response + "\n");
	text_entry_mode();
	add_edit_subtree();
	add_set_subtree();
	CliCommand* com;
	com = _cli_node.cli_command_root()->add_command(
	    "}",
	    "complete this configuration level",
	    callback(this, &RouterCLI::text_entry_func));
	com->set_global_name("}");
	com->set_can_pipe(false);
    }

    return (XORP_OK);
}

string
RouterCLI::run_set_command(const string& path, const vector<string>& argv)
{
    list<string> path_parts;
    path_parts = split(path, ' ');

    ConfigTreeNode* ctn = config_tree()->find_node(path_parts);
    const TemplateTreeNode* ttn;
    bool create_needed = false;
    string nodename;

    if (ctn == NULL) {
	create_needed = true;
	ttn = config_tree()->find_template(path_parts);
	nodename = path_parts.back();
	path_parts.pop_back();
	ctn = config_tree()->find_node(path_parts);
    } else {
	XLOG_ASSERT(ctn->is_leaf());
	ttn = ctn->template_tree_node();
    }

    XLOG_ASSERT(ctn != NULL && ttn != NULL);

    if (argv.size() != 1) {
	string result = c_format("ERROR: \"set %s\" should take "
				 "one argument of type \"%s\".",
				 path.c_str(), ttn->typestr().c_str());
	return result;
    }

    if (ttn->type_match(argv[0]) == false) {
	string result = c_format("ERROR: argument \"%s\" "
				 "is not a valid \"%s\".",
				 argv[0].c_str(),
				 ttn->typestr().c_str());
	return result;
    }

    if (create_needed) {
	string newpath;
	path_parts.push_back(nodename);
	list<string>::const_iterator iter;

	for (iter = path_parts.begin(); iter != path_parts.end(); ++iter) {
	    if (newpath.empty())
		newpath = *iter;
	    else
		newpath += " " + *iter;
	}
	ConfigTreeNode* newnode = new ConfigTreeNode(path_parts.back(),
						     newpath, ttn,
						     ctn,
						     getuid(),
						     _verbose);
	ctn = newnode;
	ctn->set_value(argv[0], getuid());
    } else {
	ctn->set_value(argv[0], getuid());
    }

    //
    // We don't execute commands until a commit is received
    //
    // string result = config_tree()->execute_node_command(ctn, "%set");
    _changes_made = true;

    string result = "OK";
    return result;
}

int
RouterCLI::commit_func(const string& ,
		       const string& ,
		       uint32_t ,
		       const string& command_global_name,
		       const vector<string>& argv)
{
    if (! argv.empty()) {
	string errmsg = c_format("ERROR: \"%s\" does not take "
				 "any additional parameters.\n",
				 command_global_name.c_str());
	_cli_client.cli_print(errmsg);
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
	_cli_client.cli_print(response);
	_cli_client.cli_print("The configuration has not been changed.\n");
	_cli_client.cli_print("Fix this error, and run \"commit\" again.\n");
	//	set_prompt("", "XORP> ");
	//	apply_path_change();
	silent_reenable_ui();

	return (XORP_ERROR);
    }

    // _cli_client.cli_print(response + "\n");
    // apply_path_change();

    return (XORP_OK);
}

void
RouterCLI::idle_ui()
{
    _cli_client.set_is_waiting_for_data(true);
}

void
RouterCLI::silent_reenable_ui()
{
    _cli_client.set_is_waiting_for_data(false);
}

void
RouterCLI::reenable_ui()
{
    _cli_client.set_is_waiting_for_data(false);
    _cli_client.post_process_command();
}

void
RouterCLI::commit_done(bool success, string errmsg)
{
    //
    // If we get a failure back here, report it.  Otherwise expect an
    // XRL from the rtrmgr to inform us what really happened as the
    // change was applied.
    //
    if (! success) {
	_cli_client.cli_print("Commit Failed\n");
	_cli_client.cli_print(errmsg);
    } else {
	_changes_made = false;
	_cli_client.cli_print("OK\n");
	_cli_client.cli_print(errmsg);
    }
    _xorpsh.set_mode(XorpShell::MODE_IDLE);
    apply_path_change();
    reenable_ui();
}

int
RouterCLI::show_func(const string& ,
		     const string& ,
		     uint32_t ,			// cli_session_id
		     const string& command_global_name,
		     const vector<string>& argv)
{
    if (! argv.empty())
	return (XORP_ERROR);

    string cmd_name = command_global_name;
    XLOG_ASSERT(cmd_name.substr(0, 4) == "show");
    string path;

    if (cmd_name == "show") {
	// Command "show" with no parameters at the top level
	path = "";
    } else {
	path = cmd_name.substr(5, cmd_name.size() - 5);
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

    string result = config_tree()->show_subtree(path_segments);
    _cli_client.cli_print(result + "\n");
    config_mode_prompt();

    return (XORP_OK);
}

int
RouterCLI::op_mode_func(const string& ,
			const string& ,		// cli_term_name
			uint32_t ,		// cli_session_id
			const string& command_global_name,
			const vector<string>& argv)
{
    string full_command = command_global_name;
    list<string> path_segments;

    path_segments = split(command_global_name, ' ');
    for (size_t i = 0; i < argv.size(); i++) {
	if (argv[i] == "|")
	    break;	// The pipe command
	path_segments.push_back(argv[i]);
	full_command += " " + argv[i];
    }

    if (op_cmd_list()->command_match(path_segments, true)) {
	// Clear the UI
	idle_ui();

	op_cmd_list()->execute(&(_xorpsh.eventloop()), path_segments,
			       callback(this, &RouterCLI::op_mode_cmd_done));
    } else {
	//
	// Try to figure out where the error is, so we can give useful
	// feedback to the user.
	//
	list<string>::const_iterator iter;
	list<string> test_parts;
	string cmd_error;

	_cli_client.cli_print("ERROR: no matching command:\n");
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
	_cli_client.cli_print(
	    c_format("%s\n%s\n", full_command.c_str(), cmd_error.c_str())
	    );
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
RouterCLI::op_mode_cmd_done(bool success, const string& result,
			    bool is_result_delayed)
{
    if (success) {
	// XXX: don't print anything after a command in operational mode
	// _cli_client.cli_print("OK\n");
    } else {
	_cli_client.cli_print("ERROR:\n");
    }
    if (! result.empty())
	_cli_client.cli_print(result + "\n");

    // Re-enable the CLI
    //  clear_command_set();
    //  add_op_mode_commands(NULL);
    if (is_result_delayed)
	reenable_ui();
    else
	silent_reenable_ui();
}

int
RouterCLI::save_func(const string& ,
		     const string& ,
		     uint32_t ,			// cli_session_id
		     const string& command_global_name,
		     const vector<string>& argv)
{
    if (argv.size() != 1) {
	_cli_client.cli_print("Usage: save <filename>\n");
	return (XORP_ERROR);
    }

    XLOG_ASSERT(command_global_name == "save");
    _cli_client.cli_print(c_format("save, filename = %s\n", argv[0].c_str()));
    _xorpsh.save_to_file(argv[0], callback(this, &RouterCLI::save_done));
    idle_ui();

    return (XORP_OK);
}

void
RouterCLI::save_done(const XrlError& e)
{
    debug_msg("in save done\n");

    if (e != XrlError::OKAY()) {
	_cli_client.cli_print("ERROR: Save failed.\n");
	if (e == XrlError::COMMAND_FAILED()) {
	    if (e.note() == "AUTH_FAIL") {
		check_for_rtrmgr_restart();
		return;
	    }  else {
		_cli_client.cli_print(e.note());
	    }
	} else {
	    _cli_client.cli_print("Failed to communicate save command to rtrmgr\n");
	    _cli_client.cli_print(c_format("%s\n", e.error_msg()));
	}
	reenable_ui();
	return;
    }
    _cli_client.cli_print("Save done\n");
    reenable_ui();
};

int
RouterCLI::load_func(const string& ,
		     const string& ,
		     uint32_t ,
		     const string& command_global_name,
		     const vector<string>& argv)
{
    if (argv.size() != 1) {
	_cli_client.cli_print("Usage: load <filename>\n");
	return (XORP_ERROR);
    }

    XLOG_ASSERT(command_global_name == "load");
    XLOG_TRACE(_verbose, "load, filename = %s\n", argv[0].c_str());
    _xorpsh.load_from_file(argv[0],
			   callback(this, &RouterCLI::load_communicated),
			   callback(this, &RouterCLI::load_done));
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
	_cli_client.cli_print("ERROR: Load failed.\n");
	if (e == XrlError::COMMAND_FAILED()) {
	    if (e.note() == "AUTH_FAIL") {
		check_for_rtrmgr_restart();
		return;
	    }  else {
		_cli_client.cli_print(e.note());
	    }
	} else {
	    _cli_client.cli_print("Failed to communicate load command to rtrmgr.\n");
	    _cli_client.cli_print(c_format("%s\n", e.error_msg()));
	}
	_xorpsh.set_mode(XorpShell::MODE_IDLE);
	reenable_ui();
	return;
    }
    _xorpsh.set_mode(XorpShell::MODE_LOADING);
    //
    // Don't enable the UI - we'll get called back when the commit has
    // completed.
    //
};

//
// Method load_done() is called when the load has successfully been applied to
// all the router modules, or when something goes wrong during this process.
//
void
RouterCLI::load_done(bool success, string errmsg)
{
    if (! success) {
	_cli_client.cli_print("ERROR: Load failed.\n");
	_cli_client.cli_print(errmsg);
    } else {
	_cli_client.cli_print("Load done.\n");
    }
    _xorpsh.set_mode(XorpShell::MODE_IDLE);
    reenable_ui();
};

//
// Just to make the code more readable:
//
SlaveConfigTree*
RouterCLI::config_tree()
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

void
RouterCLI::check_for_rtrmgr_restart()
{
    _xorpsh.get_rtrmgr_pid(callback(this, &RouterCLI::verify_rtrmgr_restart));
}

void
RouterCLI::verify_rtrmgr_restart(const XrlError& e, const uint32_t* pid)
{
    if (e == XrlError::OKAY()) {
	if (_xorpsh.rtrmgr_pid() != *pid) {
	    _cli_client.cli_print("FATAL ERROR.\n");
	    _cli_client.cli_print("The router manager process has restarted.\n");
	    _cli_client.cli_print("Advise logout and login again.\n");
	    reenable_ui();
	    return;
	}

	//
	// It's not clear what happened, but attempt to carry on.
	//
	_cli_client.cli_print("ERROR: authentication failure.\n");
	reenable_ui();
	return;
    } else {
	_cli_client.cli_print("FATAL ERROR.\n");
	_cli_client.cli_print("The router manager process is no longer functioning correctly.\n");
	_cli_client.cli_print("Advise logout and login again, but if login fails the router may need rebooting.\n");
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
