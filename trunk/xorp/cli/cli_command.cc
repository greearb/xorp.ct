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

#ident "$XORP: xorp/cli/cli_command.cc,v 1.1.1.1 2002/12/11 23:55:51 hodson Exp $"


//
// CLI (Command-Line Interface) commands structuring implementation
//


#include "cli_module.h"
#include "cli_private.hh"
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

//
// Local functions prototypes
//


CliCommand::CliCommand(CliCommand *init_parent_command,
		       const char *init_command_name,
		       const char *init_command_help)
    : _parent_command(init_parent_command),
      _name(init_command_name),
      _help(init_command_help)
{
    if (_parent_command != NULL)
	_root_command = _parent_command->root_command();
    else
	_root_command = this;
    
    set_allow_cd(false, NULL);
    set_can_pipe(false);	// XXX: default
    set_cli_command_pipe(NULL);
    
    // Set the command-completion help string
    // TODO: parameterize the hard-coded number
    _help_completion = c_format(" %*s%s\r\n", (int)(15 - _name.size()), " ",
				_help.c_str());
    
    set_cli_completion_func(cli_attempt_command_completion_byname);	// XXX: default
    
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
CliCommand::set_allow_cd(bool v, const char *init_cd_prompt)
{
    _allow_cd = v;
    if (init_cd_prompt != NULL)
	_cd_prompt = init_cd_prompt;
}

//
// Return %XORP_OK, on success, otherwise %XORP_ERROR
//
int
CliCommand::add_command(CliCommand *child_command)
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
	    XLOG_ERROR("Command already installed");
	    return (XORP_ERROR);
	}
	if (string(cli_command->name()) < string(child_command->name())) {
	    insert_pos = iter;
	    ++insert_pos;
	}
    }
    
    if (insert_pos == child_command_list().end())
	_child_command_list.push_back(child_command);
    else
	_child_command_list.insert(insert_pos, child_command);
    child_command->set_root_command(this->root_command());
    
    return(XORP_OK);
}

//
// Create a new command.
// Return the new child command on success, otherwise NULL.
// XXX: By default, we CANNOT "cd" to this command.
// XXX: @init_command_name can include more than one command levels
// in the middle.
// E.g. "show version pim". However, commands "show" and "show version" must
// have been installed first.
//
CliCommand *
CliCommand::add_command(const char *init_command_name,
			const char *init_command_help)
{
    CliCommand *parent_cli_command = NULL;
    CliCommand *cli_command = NULL;
    vector<string> command_tokens;
    string token;
    string token_line = init_command_name;
    string command_name_string;
    
    // Create a vector of all takens in the command
    for (token = pop_token(token_line);
	 ! token.empty();
	 token = pop_token(token_line)) {
	command_tokens.push_back(token);
    }
    if (command_tokens.empty())
	return (NULL);
    command_name_string = command_tokens[command_tokens.size() - 1];
    
    // Traverse all tokens and find the parent command where to install
    // the new command
    parent_cli_command = this;
    for (size_t i = 0; i < command_tokens.size() - 1; i++) {
	parent_cli_command = parent_cli_command->command_find(command_tokens[i]);
	if (parent_cli_command == NULL)
	    break;
    }
    if (parent_cli_command == NULL)
	goto error_label_missing;
    
    cli_command = new CliCommand(parent_cli_command,
				 command_name_string.c_str(),
				 init_command_help);
    
    if (parent_cli_command->add_command(cli_command) < 0) {
	delete cli_command;
	goto error_label_failed;
    }
    
    cli_command->set_allow_cd(false, NULL);
    
    return (cli_command);
    
 error_label_missing:
    XLOG_ERROR("Error installing '%s' on non-existent node '%s'", 
	       init_command_name,
	       ((this->name() != NULL) && strlen(this->name())) ?
	       this->name() : "<ROOT>");
    return (NULL);		// Invalid path to the command
 error_label_failed:
    XLOG_ERROR("Error installing '%s' on '%s'", init_command_name,
	       ((this->name() != NULL) && strlen(this->name())) ?
	       this->name() : "<ROOT>");
    return (NULL);		// Invalid path to the command
}

//
// Create a command and assign a processing callback to it.
// Return the new child command on success, otherwise NULL.
//
CliCommand *
CliCommand::add_command(const char *init_command_name,
			const char *init_command_help,
			const CLI_PROCESS_CALLBACK& init_cli_process_callback)
{
    CliCommand *cli_command = add_command(init_command_name,
					  init_command_help);
    
    if (cli_command == NULL)
	return (NULL);
    cli_command->set_cli_process_callback(init_cli_process_callback);
    cli_command->set_allow_cd(false, NULL);
    if (! init_cli_process_callback.is_empty()) {
	// XXX: by default, enable pipe processing if there is a callback func
	cli_command->set_can_pipe(true);
    }
    
    return (cli_command);
}

//
// Return the new child command on success, otherwise NULL.
// Setup a command we can "cd" to. If @cd_prompt is not-NULL,
// then the CLI prompt will be set to @cd_prompt;
// otherwise, it will remain unchanged.
//
CliCommand *
CliCommand::add_command(const char *init_command_name,
			const char *init_command_help,
			const char *init_cd_prompt)
{
    CliCommand *cli_command = add_command(init_command_name,
					  init_command_help);
    
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
CliCommand::delete_command(const char *delete_command_name)
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
    
    if (parent_cli_command->delete_command(delete_cli_command) < 0)
	goto error_label;
    return (XORP_OK);
    
 error_label:
    XLOG_ERROR("Error deleting %s on %s", delete_command_name, this->name());
    return (XORP_ERROR);
}

// Recursively delete all the children of this command.
void
CliCommand::delete_all_commands() 
{
    list <CliCommand*>::iterator i;
    for (i=_child_command_list.begin(); i!= _child_command_list.end(); i++) {
	(*i)->delete_all_commands();
	delete *i;
    }
    while (! _child_command_list.empty())
	_child_command_list.pop_front();
}

/**
 * CliCommand::create_default_cli_commands:
 * @void: 
 * 
 * Create the default CLI commands at each level of the command tree.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliCommand::create_default_cli_commands(void)
{
    // TODO: add commands like "help", "list" (all available subcommands, etc
    
    return (XORP_OK);
}

/**
 * CliCommand::add_pipes:
 * @void: 
 * 
 * Create and add the default CLI pipe commands.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliCommand::add_pipes(void)
{
    CliPipe *cli_pipe;
    CliCommand *com0;
    
    com0 = new CliCommand(this, "|", "Pipe through a command");
    // com0 = add_command("|", "Pipe through a command");
    if (com0 == NULL) {
	return (XORP_ERROR);
    }
    set_cli_command_pipe(com0);
    
    cli_pipe = new CliPipe("count");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("except");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("find");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("hold");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("match");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("no-more");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("resolve");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("save");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    cli_pipe = new CliPipe("trim");
    if (com0->add_command(cli_pipe) != XORP_OK) {
	delete_pipes();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * CliCommand::delete_pipes:
 * @void: 
 * 
 * Delete the default CLI pipe commands.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliCommand::delete_pipes(void)
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
						  void *data, const char *line,
						  int word_end,
						  list<CliCommand *>& cli_command_match_list)
{
    CliCommand *cli_command = reinterpret_cast<CliCommand*>(obj);
    int word_start = 0;		// XXX: complete from the beginning of 'line'
    const char *type_suffix = NULL;
    const char *cont_suffix = " "; // XXX: a space after a command is completed
    string token, token_line;
    const string name_string = cli_command->name();
    bool completed_command_bool;	// 'true' if complete command typed
    
    if ((cpl == NULL) || (line == NULL) || (word_end < 0)) {
	return (false);
    }
    
    token_line = string(line, word_end);
    token = pop_token(token_line);
    if (! cli_command->is_same_prefix(token))
	return (false);
    
    if (token_line.length()
	&& (is_token_separator(token_line[0]) || (token == "|")))
	completed_command_bool = true;
    else
	completed_command_bool = false;
    
    // Check if a potential sub-prefix
    if (! completed_command_bool) {
	int name_end = token.length();
	string name_complete = name_string.substr(name_end);
	
	type_suffix = cli_command->help_completion();
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
    if (! cli_command->is_same_command(token))
	return (false);
    
    bool child_completion_bool = false;
    
    if (cli_command->can_complete()
	&& ! has_more_tokens(token_line)) {
	// Add the appropriate completion info if the command can be run
	string line_string1 = "  <[Enter]>       Execute this command";
	cpl_add_completion(cpl, line_string1.c_str(), word_start,
			   line_string1.size(),
			   "",
			   type_suffix, cont_suffix);
	child_completion_bool = true;
    }
    
    if (cli_command->can_pipe() && (cli_command->cli_command_pipe() != NULL)) {
	// Add the pipe completions
	if (cli_command->_cli_completion_func(cli_command->cli_command_pipe(),
					      cpl, data,
					      token_line.c_str(),
					      token_line.length(),
					      cli_command_match_list)) {
	    child_completion_bool = true;
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
						    cpl, data,
						    token_line.c_str(),
						    token_line.length(),
						    cli_command_match_list)) {
	    child_completion_bool = true;
	}
    }
    
    return (child_completion_bool);
}

CliCommand *
CliCommand::command_find(const string& token)
{
    list<CliCommand *>::iterator iter;
    
    for (iter = child_command_list().begin();
	 iter != child_command_list().end();
	 ++iter) {
	CliCommand *cli_command = *iter;
	if (cli_command->is_same_command(token)) {
	    // XXX: assume that no two subcommands have the same name
	    return (cli_command);
	}
    }
    
    return (NULL);
}

// Find a multi-level command
// TODO: consider/ignore the command arguments as appropriate
CliCommand *
CliCommand::multi_command_find(const string& command_line)
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
	
	if (parent_cli_command->has_cli_process_callback()) {
	    // The parent command has processing function, so the rest
	    // of the tokens could be arguments for that function
	    child_cli_command = parent_cli_command;
	}
	break;
    }
    
    return (child_cli_command);
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
			      string& ret_string)
{
    string token, token_line;
    bool ret_bool = false;
    bool no_space_at_end_bool;
    
    if ((line == NULL) || (word_end < 0)) {
	return (false);
    }
    
    token_line = string(line, word_end);
    token = pop_token(token_line);
    if (! is_same_prefix(token))
	return (false);
    
    if (token_line.length()
	&& is_token_separator(token_line[0])
	&& (! is_same_command(token))) {
	// Not a match
	return (false);
    }
    
    no_space_at_end_bool = (token_line.empty()) ? (true) : (false);
    
    // Get the token for the child's command (if any)
    token = pop_token(token_line);
    
    if ((token.length() == 0) && no_space_at_end_bool) {
	// The last token, and there is no space, so print my help.
	ret_string += c_format("  %-15s %s\r\n",
			       name(), help());
	return (true);
    }
    
    if ((token.length() == 0) && can_complete()) {
	// The last token, and there is space at the end,
	// so print the "default" help.
	ret_string += c_format("  %-15s %s\r\n",
			       "<[Enter]>", "Execute this command");
	ret_bool = true;
    }
    
    // Not the last token, so search down for help
    list<CliCommand *>::iterator iter;    
    for (iter = child_command_list().begin();
	 iter != child_command_list().end();
	 ++iter) {
	CliCommand *cli_command = *iter;
	string tmp_token_line = copy_token(token) + token_line;
	ret_bool |= cli_command->find_command_help(tmp_token_line.c_str(),
						   tmp_token_line.length(),
						   ret_string);
    }
    
    if (can_pipe() && (cli_command_pipe() != NULL)) {
	// Add the pipe completions
	string tmp_token_line = copy_token(token) + token_line;
	ret_bool |= cli_command_pipe()->find_command_help(
	    tmp_token_line.c_str(),
	    tmp_token_line.length(),
	    ret_string);
    }
    
    return (ret_bool);
}

bool
CliCommand::can_complete()
{
    if (has_cli_process_callback() || allow_cd())
	return true;
    return false;
}

bool
CliCommand::has_dynamic_process_callback()
{
    return (!_dynamic_process_callback.is_empty());
}

bool
CliCommand::has_cli_process_callback()
{
    if (_has_dynamic_children) 
	//force the children to be evaluated, which forces the
	//cli_process_callback to be set
	child_command_list();
    return (!_cli_process_callback.is_empty());
}


list<CliCommand *>&	
CliCommand::child_command_list()
{ 
    if (_has_dynamic_children)
	assert(_child_command_list.empty());

    // Handle dynamic children generation
    if (_child_command_list.empty() && _has_dynamic_children) {
	//now we've run this, we won't need to run it again.
	_has_dynamic_children = false;

	//add dynamic children
	assert(global_name() != NULL);
	bool can_be_run;
	set <string> dynamic_children = 
	    _dynamic_children_callback->dispatch(global_name(), can_be_run);
	if (can_be_run) {
	    if (_cli_process_callback.is_empty())
		_cli_process_callback = _dynamic_process_callback;
	}
	set <string>::iterator dci;
	CliCommand *new_cmd;
	for(dci = dynamic_children.begin(); 
	    dci != dynamic_children.end();
	    dci++) {
	    new_cmd = add_command(dci->c_str(), "help");
	    string child_name = global_name() + (" " + *dci);
	    new_cmd->set_global_name(child_name.c_str());
	    new_cmd->set_dynamic_children(_dynamic_children_callback);
	    new_cmd->set_dynamic_process_callback(dynamic_process_callback());
	}
    }

    return (_child_command_list); 
}
