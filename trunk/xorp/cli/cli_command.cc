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




//
// CLI (Command-Line Interface) commands structuring implementation
//


#include "cli_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"

#include "cli_command_pipe.hh"


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

static string EXECUTE_THIS_COMMAND_STRING =
    "<[Enter]>            Execute this command\r\n";

//
// Local functions prototypes
//


CliCommand::CliCommand(CliCommand *init_parent_command,
		       const string& init_command_name,
		       const string& init_command_help)
    : _parent_command(init_parent_command),
      _name(init_command_name),
      _help(init_command_help),
      _default_nomore_mode(false),
      _is_command_argument(false),
      _is_argument_expected(false)
{
    if (_parent_command != NULL)
	_root_command = _parent_command->root_command();
    else
	_root_command = this;
    
    set_allow_cd(false, "");
    set_can_pipe(false);	// XXX: default
    set_cli_command_pipe(NULL);
    
    // Set the command-completion help string
    // TODO: parameterize the hard-coded number
    _help_completion = c_format(" %*s%s\r\n", (int)(20 - _name.size()), " ",
				_help.c_str());

    // XXX: set the CLI completion function to its default value
    set_cli_completion_func(cli_attempt_command_completion_byname);
    
    _has_dynamic_children = false;
}

CliCommand::~CliCommand()
{
    // Delete recursively all child commands
    delete_pointers_list(_child_command_list);
}

//
// Enable/disable "cd" to this command, and set the "cd prompt"
//
void
CliCommand::set_allow_cd(bool v, const string& init_cd_prompt)
{
    _allow_cd = v;
    if (init_cd_prompt.size())
	_cd_prompt = init_cd_prompt;
}

//
// Return %XORP_OK, on success, otherwise %XORP_ERROR
//
int
CliCommand::add_command(CliCommand *child_command, string& error_msg)
{
    list<CliCommand *>::iterator iter, insert_pos;
    
    insert_pos = child_command_list().begin();
    
    // Check if command already installed, as well as find the
    // position to install the command (based on lexicographical ordering).
    for (iter = child_command_list().begin();
	 iter != child_command_list().end();
	 ++iter) {
	CliCommand *cli_command = *iter;
	if (cli_command->is_same_command(child_command->name())) {
	    // Command already installed
	    error_msg = c_format("Command '%s' already installed",
				 child_command->name().c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (cli_command->name() < child_command->name()) {
	    insert_pos = iter;
	    ++insert_pos;
	}
    }
    
    if (insert_pos == child_command_list().end())
	_child_command_list.push_back(child_command);
    else
	_child_command_list.insert(insert_pos, child_command);
    child_command->set_root_command(this->root_command());
    
    return (XORP_OK);
}

//
// Create a new command.
// Return the new child command on success, otherwise NULL.
// XXX: By default, we CANNOT "cd" to this command.
// XXX: If @is_multilevel_command is true, then @init_command_name can
// include more than one command levels in the middle.
// E.g. "show version pim". However, commands "show" and "show version" must
// have been installed first.
//
CliCommand *
CliCommand::add_command(const string& init_command_name,
			const string& init_command_help,
			bool is_multilevel_command,
			string& error_msg)
{
    CliCommand *parent_cli_command = NULL;
    CliCommand *cli_command = NULL;
    vector<string> command_tokens;
    string token;
    string token_line = init_command_name;
    string command_name_string;
    
    if (is_multilevel_command) {
	// Create a vector of all takens in the command
	for (token = pop_token(token_line);
	     ! token.empty();
	     token = pop_token(token_line)) {
	    command_tokens.push_back(token);
	}
    } else {
	if (token_line.empty()) {
	    error_msg = c_format("Empty token line for command %s",
				 init_command_name.c_str());
	    return (NULL);
	}
	command_tokens.push_back(token_line);
    }

    if (command_tokens.empty()) {
	error_msg = c_format("Empty command tokens for command %s",
			     init_command_name.c_str());
	return (NULL);
    }
    command_name_string = command_tokens[command_tokens.size() - 1];
    
    // Traverse all tokens and find the parent command where to install
    // the new command
    parent_cli_command = this;
    for (size_t i = 0; i < command_tokens.size() - 1; i++) {
	parent_cli_command = parent_cli_command->command_find(command_tokens[i]);
	if (parent_cli_command == NULL)
	    break;
    }
    if (parent_cli_command == NULL) {
	error_msg = c_format("Cannot find parent command");
	goto error_label_missing;
    }
    
    cli_command = new CliCommand(parent_cli_command,
				 command_name_string,
				 init_command_help);
    
    if (parent_cli_command->add_command(cli_command, error_msg) != XORP_OK) {
	delete cli_command;
	goto error_label_failed;
    }
    
    cli_command->set_allow_cd(false, "");
    
    return (cli_command);
    
 error_label_missing:
    error_msg = c_format("Error installing '%s' on non-existent node '%s': %s",
			 init_command_name.c_str(),
			 (this->name().size() > 0) ? this->name().c_str() : "<ROOT>",
			 error_msg.c_str());
    XLOG_ERROR("%s", error_msg.c_str());
    return (NULL);		// Invalid path to the command
 error_label_failed:
    error_msg = c_format("Error installing '%s' on '%s': %s",
			 init_command_name.c_str(),
			 (this->name().size() > 0) ? this->name().c_str() : "<ROOT>",
			 error_msg.c_str());
    XLOG_ERROR("%s", error_msg.c_str());
    return (NULL);		// Invalid path to the command
}

//
// Create a command and assign a processing callback to it.
// Return the new child command on success, otherwise NULL.
//
CliCommand *
CliCommand::add_command(const string& init_command_name,
			const string& init_command_help,
			bool is_multilevel_command,
			const CLI_PROCESS_CALLBACK& init_cli_process_callback,
			string& error_msg)
{
    CliCommand *cli_command = add_command(init_command_name,
					  init_command_help,
					  is_multilevel_command,
					  error_msg);
    
    if (cli_command == NULL)
	return (NULL);
    cli_command->set_cli_process_callback(init_cli_process_callback);
    cli_command->set_allow_cd(false, "");
    if (! init_cli_process_callback.is_empty()) {
	// XXX: by default, enable pipe processing if there is a callback func
	cli_command->set_can_pipe(true);
    }
    
    return (cli_command);
}

//
// Create a command and assign a processing and an interrupt callbacks to it.
// Return the new child command on success, otherwise NULL.
//
CliCommand *
CliCommand::add_command(const string& init_command_name,
			const string& init_command_help,
			bool is_multilevel_command,
			const CLI_PROCESS_CALLBACK& init_cli_process_callback,
			const CLI_INTERRUPT_CALLBACK& init_cli_interrupt_callback,
			string& error_msg)
{
    CliCommand *cli_command = add_command(init_command_name,
					  init_command_help,
					  is_multilevel_command,
					  init_cli_process_callback,
					  error_msg);
    
    if (cli_command == NULL)
	return (NULL);
    cli_command->set_cli_interrupt_callback(init_cli_interrupt_callback);
    
    return (cli_command);
}

//
// Return the new child command on success, otherwise NULL.
// Setup a command we can "cd" to. If @cd_prompt is not-NULL,
// then the CLI prompt will be set to @cd_prompt;
// otherwise, it will remain unchanged.
//
CliCommand *
CliCommand::add_command(const string& init_command_name,
			const string& init_command_help,
			const string& init_cd_prompt,
			bool is_multilevel_command,
			string& error_msg)
{
    CliCommand *cli_command = add_command(init_command_name,
					  init_command_help,
					  is_multilevel_command,
					  error_msg);
    
    if (cli_command == NULL)
	return (NULL);
    cli_command->set_allow_cd(true, init_cd_prompt);
    
    return (cli_command);
}


//
// Delete a command, and all sub-commands below it
// Return: %XORP_OK on success, otherwise %XORP_ERROR
//
int
CliCommand::delete_command(CliCommand *child_command)
{
    list<CliCommand *>::iterator iter;
    
    iter = find(_child_command_list.begin(),
		_child_command_list.end(),
		child_command);
    if (iter == _child_command_list.end())
	return (XORP_ERROR);
    
    _child_command_list.erase(iter);
    delete child_command;
    
    return (XORP_OK);
}

//
// Delete a command, and all sub-commands below it
// Return: %XORP_OK on success, otherwise %XORP_ERROR
// XXX: @delete_command_name can be the full path-name for that command
//
int
CliCommand::delete_command(const string& delete_command_name)
{
    CliCommand *parent_cli_command = NULL;
    CliCommand *delete_cli_command = NULL;
    vector<string> command_tokens;
    string token;
    string token_line = delete_command_name;
    
    // Create a vector of all takens in the command
    for (token = pop_token(token_line);
	 ! token.empty();
	 token = pop_token(token_line)) {
	command_tokens.push_back(token);
    }
    if (command_tokens.empty())
	return (XORP_ERROR);
    
    // Traverse all tokens and find the command to delete
    parent_cli_command = this;
    delete_cli_command = NULL;
    for (size_t i = 0; i < command_tokens.size(); i++) {
	if (delete_cli_command != NULL)
	    parent_cli_command = delete_cli_command;
	delete_cli_command = parent_cli_command->command_find(command_tokens[i]);
	if (delete_cli_command == NULL)
	    break;
    }
    if (delete_cli_command == NULL)
	goto error_label;
    
    if (parent_cli_command->delete_command(delete_cli_command) != XORP_OK)
	goto error_label;
    return (XORP_OK);
    
 error_label:
    XLOG_ERROR("Error deleting %s on %s", delete_command_name.c_str(),
	       this->name().c_str());
    return (XORP_ERROR);
}

// Recursively delete all the children of this command.
void
CliCommand::delete_all_commands() 
{
    list <CliCommand*>::iterator iter;
    for (iter = _child_command_list.begin();
	 iter != _child_command_list.end(); ++iter) {
	(*iter)->delete_all_commands();
	delete *iter;
    }
    while (! _child_command_list.empty())
	_child_command_list.pop_front();
}

/**
 * CliCommand::create_default_cli_commands:
 * @: 
 * 
 * Create the default CLI commands at each level of the command tree.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliCommand::create_default_cli_commands()
{
    // TODO: add commands like "help", "list" (all available subcommands, etc
    
    return (XORP_OK);
}

/**
 * CliCommand::add_pipes:
 * @: 
 * 
 * Create and add the default CLI pipe commands.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliCommand::add_pipes(string& error_msg)
{
    CliPipe *cli_pipe;
    CliCommand *com0;
    
    com0 = new CliCommand(this, "|", "Pipe through a command");
    // com0 = add_command("|", "Pipe through a command", false, error_msg);
    if (com0 == NULL) {
	return (XORP_ERROR);
    }
    set_cli_command_pipe(com0);
    
    cli_pipe = new CliPipe("count");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("except");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("find");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("hold");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("match");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("no-more");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("resolve");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("save");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("trim");
    if (com0->add_command(cli_pipe, error_msg) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * CliCommand::delete_pipes:
 * @: 
 * 
 * Delete the default CLI pipe commands.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliCommand::delete_pipes()
{
    if (_cli_command_pipe != NULL)
	delete _cli_command_pipe;
    
    return (XORP_OK);
    // return (delete_command("|"));
}

//
// Attempts to complete the current command.
// Return: true if the attempt was successful, otherwise false.
//
bool
CliCommand::cli_attempt_command_completion_byname(void *obj,
						  WordCompletion *cpl,
						  void *data,
						  const char *line,
						  int word_end,
						  list<CliCommand *>& cli_command_match_list)
{
    CliCommand *cli_command = reinterpret_cast<CliCommand*>(obj);
    int word_start = 0;		// XXX: complete from the beginning of 'line'
    const char *type_suffix = NULL;
    const char *cont_suffix = " "; // XXX: a space after a command is completed
    string token, token_line;
    const string name_string = cli_command->name();
    bool is_command_completed;	// 'true' if complete command typed
    
    if ((cpl == NULL) || (line == NULL) || (word_end < 0)) {
	return (false);
    }
    
    token_line = string(line, word_end);
    token = pop_token(token_line);
    if ((! cli_command->is_same_prefix(token))
	&& (! cli_command->has_type_match_cb())) {
	    return (false);
    }
    
    if (token_line.length()
	&& (is_token_separator(token_line[0]) || (token == "|")))
	is_command_completed = true;
    else
	is_command_completed = false;
    
    // Check if a potential sub-prefix
    if (! is_command_completed) {
	string name_complete;

	if (cli_command->has_type_match_cb()) {
	    //
	    // XXX: Nothing to complete, we just need to print
	    // the help with the command type.
	    //
	    cli_command_match_list.push_back(cli_command);
	    return (true);
	}

	name_complete = name_string.substr(token.length());
	if (cli_command->help_completion().size() > 0)
	    type_suffix = cli_command->help_completion().c_str();
	
	// Add two empty spaces in front
	string line_string = "  ";
	if (token.empty()) {
	    // XXX: ignore the rest of the empty spaces
	    word_end = 0;
	} else {
	    line_string += line;
	}
	cpl_add_completion(cpl, line_string.c_str(), word_start, word_end + 2,
			   name_complete.c_str(),
			   type_suffix, cont_suffix);
	cli_command_match_list.push_back(cli_command);
	return (true);
    }
    
    // Must be a complete command

    bool is_token_match = false;
    if (cli_command->has_type_match_cb()) {
	string errmsg;
	is_token_match = cli_command->type_match_cb()->dispatch(token, errmsg);
    } else {
	is_token_match = cli_command->is_same_command(token);
    }
    if (! is_token_match) {
	return (false);
    }
    
    bool is_child_completion = false;
    
    if (cli_command->can_complete()
	&& (! has_more_tokens(token_line))
	&& (! cli_command->is_argument_expected())) {
	// Add the appropriate completion info if the command can be run
	string line_string1 = "  ";
	type_suffix = EXECUTE_THIS_COMMAND_STRING.c_str();
	cpl_add_completion(cpl, line_string1.c_str(), word_start,
			   line_string1.size(),
			   "",
			   type_suffix, cont_suffix);
	is_child_completion = true;
    }
    
    if (cli_command->can_pipe() && (cli_command->cli_command_pipe() != NULL)) {
	// Add the pipe completions
	if (cli_command->_cli_completion_func(cli_command->cli_command_pipe(),
					      cpl,
					      data,
					      token_line.c_str(),
					      token_line.length(),
					      cli_command_match_list)) {
	    is_child_completion = true;
	}
    }

    // Add completions for the sub-commands (if any)
    list<CliCommand *>::iterator iter;
    for (iter = cli_command->child_command_list().begin();
	 iter != cli_command->child_command_list().end();
	 ++iter) {
	CliCommand *cli_command_child = *iter;
	if (! cli_command_child->has_cli_completion_func())
	    continue;
	if (cli_command_child->_cli_completion_func(cli_command_child,
						    cpl,
						    data,
						    token_line.c_str(),
						    token_line.length(),
						    cli_command_match_list)) {
	    is_child_completion = true;
	}
    }
    
    return (is_child_completion);
}

CliCommand *
CliCommand::command_find(const string& token)
{
    list<CliCommand *>::iterator iter;
    
    for (iter = child_command_list().begin();
	 iter != child_command_list().end();
	 ++iter) {
	CliCommand *cli_command = *iter;
	if (cli_command->has_type_match_cb()) {
	    string errmsg;
	    if (cli_command->type_match_cb()->dispatch(token, errmsg))
		return (cli_command);
	    continue;
	}
	if (cli_command->is_same_command(token)) {
	    // XXX: assume that no two subcommands have the same name
	    return (cli_command);
	}
    }
    
    return (NULL);
}

//
// Test if the command line is a prefix for a multi-level command.
// Note that if there is an exact match, then the return value is false.
//
bool
CliCommand::is_multi_command_prefix(const string& command_line)
{
    string token;
    string token_line = command_line;
    CliCommand *parent_cli_command = this;
    CliCommand *child_cli_command = NULL;

    for (token = pop_token(token_line);
	 ! token.empty();
	 token = pop_token(token_line)) {
	child_cli_command = parent_cli_command->command_find(token);
	if (child_cli_command != NULL) {
	    parent_cli_command = child_cli_command;
	    continue;
	}

	// Test if the token is a prefix for a child command
	list<CliCommand *>::const_iterator iter;
	for (iter = parent_cli_command->child_command_list().begin();
	     iter != parent_cli_command->child_command_list().end();
	     ++iter) {
	    child_cli_command = *iter;
	    if (child_cli_command->is_same_prefix(token))
		return true;
	}

	break;
    }

    return (false);
}

// Tests if the string in @token can be a prefix for this command
bool
CliCommand::is_same_prefix(const string& token)
{
    size_t s = token.length();
    
    if (s > _name.length())
	return (false);
    
    return (token.substr(0, s) == _name.substr(0, s));
}

// Tests if the string in @token matches this command
bool
CliCommand::is_same_command(const string& token)
{
    return (token == _name);
}

bool
CliCommand::find_command_help(const char *line, int word_end,
			      set<string>& help_strings)
{
    string token, token_line;
    bool ret_value = false;
    bool is_no_space_at_end;
    
    if ((line == NULL) || (word_end < 0)) {
        return (false);
    }
    
    token_line = string(line, word_end);
    token = pop_token(token_line);

    if ((! is_same_prefix(token)) && (! has_type_match_cb()))
	return (false);

    //
    // Test if the token matches the command
    //
    bool is_token_match = false;
    if (has_type_match_cb()) {
	string errmsg;
	is_token_match = type_match_cb()->dispatch(token, errmsg);
    } else {
	is_token_match = is_same_command(token);
    }

    if (token_line.length()
	&& is_token_separator(token_line[0])
	&& (! is_token_match)) {
	// Not a match
	return (false);
    }
    
    is_no_space_at_end = (token_line.empty()) ? (true) : (false);
    
    // Get the token for the child's command (if any)
    token = pop_token(token_line);
    
    if ((token.length() == 0) && is_no_space_at_end) {
	// The last token, and there is no space, so print my help.
	help_strings.insert(c_format("  %-19s  %s\r\n",
				     name().c_str(), help().c_str()));
	return (true);
    }
    
    if ((token.length() == 0)
	&& can_complete()
	&& (! is_argument_expected())) {
	// The last token, and there is space at the end,
	// so print the "default" help.
	help_strings.insert(c_format("  %-19s  %s\r\n",
				     "<[Enter]>", "Execute this command"));
	ret_value = true;
    }
    
    // Not the last token, so search down for help
    list<CliCommand *>::iterator iter;    
    for (iter = child_command_list().begin();
	 iter != child_command_list().end();
	 ++iter) {
	CliCommand *cli_command = *iter;
	string tmp_token_line = copy_token(token) + token_line;
	ret_value |= cli_command->find_command_help(tmp_token_line.c_str(),
						    tmp_token_line.length(),
						    help_strings);
    }
    
    if (can_pipe() && (cli_command_pipe() != NULL)) {
	// Add the pipe completions
	string tmp_token_line = copy_token(token) + token_line;
	ret_value |= cli_command_pipe()->find_command_help(
	    tmp_token_line.c_str(),
	    tmp_token_line.length(),
	    help_strings);
    }
    
    return (ret_value);
}

bool
CliCommand::can_complete()
{
    if (has_cli_process_callback() || allow_cd())
	return true;
    return false;
}

CliCommand *
CliCommand::cli_command_pipe()
{
    if (_root_command != this)
	return (_root_command->cli_command_pipe());
    else
	return (_cli_command_pipe);
}

void
CliCommand::set_dynamic_children_callback(DYNAMIC_CHILDREN_CALLBACK v)
{
    XLOG_ASSERT(!_global_name.empty());
    _dynamic_children_callback = v;
    _has_dynamic_children = true;
}

bool
CliCommand::has_dynamic_process_callback()
{
    return (!_dynamic_process_callback.is_empty());
}

bool
CliCommand::has_dynamic_interrupt_callback()
{
    return (!_dynamic_interrupt_callback.is_empty());
}

bool
CliCommand::has_cli_process_callback()
{
    if (_has_dynamic_children) {
	//
	// Force the children to be evaluated, which forces the
	// cli_process_callback to be set.
	//
	child_command_list();
    }
    return (!_cli_process_callback.is_empty());
}

bool
CliCommand::has_cli_interrupt_callback()
{
    return (!_cli_interrupt_callback.is_empty());
}

bool
CliCommand::has_type_match_cb() const
{
    return (! _type_match_cb.is_empty());
}

list<CliCommand *>&
CliCommand::child_command_list()
{
    string error_msg;

    if (_has_dynamic_children)
	XLOG_ASSERT(_child_command_list.empty());

    // Handle dynamic children generation
    if (_child_command_list.empty() && _has_dynamic_children) {
	// Now we've run this, we won't need to run it again.
	_has_dynamic_children = false;

	// Add dynamic children
	XLOG_ASSERT(global_name().size() > 0);
	map<string, CliCommandMatch> dynamic_children;
	map<string, CliCommandMatch>::iterator iter;
	dynamic_children = _dynamic_children_callback->dispatch(global_name());
	CliCommand *new_cmd;
	for (iter = dynamic_children.begin();
	     iter != dynamic_children.end();
	     ++iter) {
	    const CliCommandMatch& ccm = iter->second;
	    const string& command_name = ccm.command_name();
	    const string& help_string = ccm.help_string();
	    bool is_executable = ccm.is_executable();
	    bool can_pipe = ccm.can_pipe();
	    bool default_nomore_mode = ccm.default_nomore_mode();
	    bool is_command_argument = ccm.is_command_argument();
	    bool is_argument_expected = ccm.is_argument_expected();
	    new_cmd = add_command(command_name, help_string, false, error_msg);
	    if (new_cmd == NULL) {
		XLOG_FATAL("Cannot add command '%s' to parent '%s': %s",
			   command_name.c_str(), name().c_str(),
			   error_msg.c_str());
	    }
	    vector<string> child_global_name = global_name();
	    child_global_name.push_back(command_name);
	    new_cmd->set_global_name(child_global_name);
	    new_cmd->set_can_pipe(can_pipe);
	    new_cmd->set_default_nomore_mode(default_nomore_mode);
	    new_cmd->set_is_command_argument(is_command_argument);
	    new_cmd->set_is_argument_expected(is_argument_expected);
	    new_cmd->set_type_match_cb(ccm.type_match_cb());
	    new_cmd->set_dynamic_children_callback(_dynamic_children_callback);
	    new_cmd->set_dynamic_process_callback(_dynamic_process_callback);
	    new_cmd->set_dynamic_interrupt_callback(_dynamic_interrupt_callback);
	    if (is_executable) {
		new_cmd->set_cli_process_callback(_dynamic_process_callback);
		new_cmd->set_cli_interrupt_callback(_dynamic_interrupt_callback);
	    }
	}
    }

    return (_child_command_list); 
}
