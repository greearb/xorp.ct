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

// $XORP: xorp/cli/cli_command.hh,v 1.30 2008/10/02 21:56:29 bms Exp $


#ifndef __CLI_CLI_COMMAND_HH__
#define __CLI_CLI_COMMAND_HH__


//
// CLI command definition.
//







#include "libxorp/callback.hh"
#include "cli/libtecla/libtecla.h"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//
class CliCommand;
class CliClient;
class CliCommandMatch;

//
// The callback to print-out the return result from a processing function
//
typedef XorpCallback5<int,	/* return_value */
    const string&,		/* server_name */
    const string&,		/* cli_term_name */
    uint32_t,			/* cli_session_id */
    const vector<string>&,	/* command_global_name */
    const vector<string>&	/* command_args */
>::RefPtr CLI_PROCESS_CALLBACK;

typedef XorpCallback5<void,	/* return_value */
    const string&,		/* server_name */
    const string&,		/* cli_term_name */
    uint32_t,			/* cli_session_id */
    const vector<string>&,	/* command_global_name */
    const vector<string>&	/* command_args */
>::RefPtr CLI_INTERRUPT_CALLBACK;

typedef XorpCallback1<map<string, CliCommandMatch>, /* return value */
    const vector<string>&	/* global_name */
>::RefPtr DYNAMIC_CHILDREN_CALLBACK;

//
// The type of a processing function that handles CLI commands
//
typedef int (* CLI_PROCESS_FUNC)(const string& server_name,
				 const string& cli_term_name,
				 uint32_t cli_session_id,
				 const vector<string>& command_global_name,
				 const vector<string>& command_args);
typedef void (* CLI_INTERRUPT_FUNC)(const string& server_name,
				    const string& cli_term_name,
				    uint32_t cli_session_id,
				    const vector<string>& command_global_name,
				    const vector<string>& command_args);

//
// The type of a function that handles CLI command completions
//
#define CLI_COMPLETION_FUNC_(func) bool (func)(void *obj,		\
					      WordCompletion *cpl,	\
					      void *data,		\
					      const char *line,		\
					      int word_end,		\
					      list<CliCommand *>& cli_command_match_list)
typedef CLI_COMPLETION_FUNC_(CLI_COMPLETION_FUNC);

/**
 * @short The class for the CLI command.
 */
class CliCommand {
public:
    typedef XorpCallback2<bool, const string&, string&>::RefPtr TypeMatchCb;

    /**
     * Constructor for a given parent command, command name, and command help.
     * 
     * @param init_parent_command the parent @ref CliCommand command.
     * @param init_command_name the command name (this name should not
     * include the command name of the parent command and its ancestors).
     * @param init_command_help the command help.
     */
    CliCommand(CliCommand *init_parent_command,
	       const string& init_command_name,
	       const string& init_command_help);

    /**
     * Destructor
     */
    virtual ~CliCommand();
    
    /**
     * Enable/disable whether this command allows "change directory" to it.
     * 
     * @param v if true, enable "change directory", otherwise disable it.
     * @param init_cd_prompt if @ref v is true, the CLI prompt to display
     * when "cd" to this command. If an empty string, the CLI prompt will
     * not be changed.
     */
    void set_allow_cd(bool v, const string& init_cd_prompt);
    
    /**
     * Create the default CLI commands at each level of the command tree.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int create_default_cli_commands();
    
    /**
     * Create and add the default CLI pipe commands.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_pipes(string& error_msg);
    
    /**
     * Add a CLI command.
     * 
     * By default, we cannot "cd" to this command.
     * 
     * @param init_command_name the command name to add.
     * If @ref is_multilevel_command is true, then it may include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param is_multilevel_command if true, then @ref init_command_name
     * may include more than one command levels in the middle.
     * @param error_msg the error message (if error).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    bool is_multilevel_command,
			    string& error_msg);
    
    /**
     * Add a CLI command we can "cd" to it.
     * 
     * By default, we can "cd" to this command.
     * 
     * @param init_command_name the command name to add.
     * If @ref is_multilevel_command is true, then it may include
     * more than one command levels in the middle. E.g., "set pim bsr".
     * However, commands "set" and "set pim" must have been installed first.
     * @param init_command_help the command help.
     * @param init_cd_prompt if not an empty string, the CLI prompt
     * when "cd" to this command.
     * @param is_multilevel_command if true, then @ref init_command_name
     * may include more than one command levels in the middle.
     * @param error_msg the error message (if error).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    const string& init_cd_prompt,
			    bool is_multilevel_command,
			    string& error_msg);
    
    /**
     * Add a CLI command with a processing callback.
     * 
     * @param init_command_name the command name to add.
     * If @ref is_multilevel_command is true, then it may include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param is_multilevel_command if true, then @ref init_command_name
     * may include more than one command levels in the middle.
     * @param init_cli_process_callback the callback to call when the
     * command is entered for execution from the command-line.
     * @param error_msg the error message (if error).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    bool is_multilevel_command,
			    const CLI_PROCESS_CALLBACK& init_cli_process_callback,
			    string& error_msg);

    /**
     * Add a CLI command with a processing and an interrupt callbacks.
     * 
     * @param init_command_name the command name to add.
     * If @ref is_multilevel_command is true, then it may include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param is_multilevel_command if true, then @ref init_command_name
     * may include more than one command levels in the middle.
     * @param init_cli_process_callback the callback to call when the
     * command is entered for execution from the command-line.
     * @param init_cli_interrupt_callback the callback to call when the
     * user has interrupted the command (e.g., by typing Ctrl-C).
     * @param error_msg the error message (if error).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    bool is_multilevel_command,
			    const CLI_PROCESS_CALLBACK& init_cli_process_callback,
			    const CLI_INTERRUPT_CALLBACK& init_cli_interrupt_callback,
			    string& error_msg);
    
    /**
     * Add a CLI command with a processing function.
     * 
     * @param init_command_name the command name to add.
     * If @ref is_multilevel_command is true, then it may include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param is_multilevel_command if true, then @ref init_command_name
     * may include more than one command levels in the middle.
     * @param init_cli_process_func the processing function to call when the
     * command is entered for execution from the command-line.
     * @param error_msg the error message (if error).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    bool is_multilevel_command,
			    CLI_PROCESS_FUNC init_cli_process_func,
			    string& error_msg) {
	CLI_PROCESS_CALLBACK cb = callback(init_cli_process_func);
	return (add_command(init_command_name, init_command_help,
			    is_multilevel_command, cb, error_msg));
    }

    /**
     * Add a CLI command with a processing function and an interrupt
     * handler.
     * 
     * @param init_command_name the command name to add.
     * If @ref is_multilevel_command is true, then it may include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param is_multilevel_command if true, then @ref init_command_name
     * may include more than one command levels in the middle.
     * @param init_cli_process_func the processing function to call when the
     * command is entered for execution from the command-line.
     * @param init_cli_interrupt_func the function to call when the
     * user has interrupted the command (e.g., by typing Ctrl-C).
     * @param error_msg the error message (if error).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    bool is_multilevel_command,
			    CLI_PROCESS_FUNC init_cli_process_func,
			    CLI_INTERRUPT_FUNC init_cli_interrupt_func,
			    string& error_msg) {
	CLI_PROCESS_CALLBACK cb1 = callback(init_cli_process_func);
	CLI_INTERRUPT_CALLBACK cb2 = callback(init_cli_interrupt_func);
	return (add_command(init_command_name, init_command_help,
			    is_multilevel_command, cb1, cb2, error_msg));
    }
    
    /**
     * Add a child CLI command.
     * 
     * @param child_command the child command to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_command(CliCommand *child_command, string& error_msg);
    
    /**
     * Delete a child command and all sub-commands below it.
     * 
     * @param child_command the child command to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_command(CliCommand *child_command);
    
    /**
     * Delete a child command and all sub-commands below it.
     * 
     * @param delete_command_name the name of the child command to delete.
     * The name can be the full path-name for that command.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_command(const string& delete_command_name);
    
    /**
     * Recursively delete all children of this command.
     */
    void delete_all_commands();
    
    /**
     * Set whether the output of this command can be piped.
     * 
     * @param v if true, then the the output of this command can be piped.
     */
    void set_can_pipe(bool v) { _can_pipe = v; }

    /**
     * Test if "no-more" (i.e., unpaged) is the default output mode.
     *
     * @return true if "no-more" (i.e., unpaged) is the default output mode,
     * otherwise false.
     */
    bool default_nomore_mode() const { return (_default_nomore_mode); }

    /**
     * Set the default paging mode.
     *
     * @param v if true, then "no-more" (i.e., unpaged) is the default
     * output mode.
     */
    void set_default_nomore_mode(bool v) { _default_nomore_mode = v; }

    /**
     * Test if the command actually represents a command argument.
     * 
     * @return true if the command actually represents a command argument.
     */
    bool is_command_argument() const { return (_is_command_argument); }

    /**
     * Set a flag whether the command actually represents a command argument.
     * 
     * @param v true if the command represents a command argument, otherwise
     * false.
     */
    void set_is_command_argument(bool v) { _is_command_argument = v; }

    /**
     * Test if the command expects an argument.
     *
     * @return true if the command expects an argument, otherwise false.
     */
    bool is_argument_expected() const { return (_is_argument_expected); }

    /**
     * Set a flag whether the command expects an argument.
     *
     * @param v true if the command expectes an argument, otherwise false.
     */
    void set_is_argument_expected(bool v) { _is_argument_expected = v; }

    /**
     * Get the callback for type matching.
     * 
     * @return the callback for type matching.
     */
    const TypeMatchCb& type_match_cb() const { return (_type_match_cb); }

    /**
     * Set the callback for type matching.
     * 
     * @param cb the callback for type matching.
     */
    void set_type_match_cb(const TypeMatchCb& cb) { _type_match_cb = cb; }

    /**
     * Test if there is a callback for type matching.
     * 
     * @return true if there is a callback for type matching, otherwise false.
     */
    bool has_type_match_cb() const;

    /**
     * Get the global name of this command (i.e., the full name starting from
     * the root).
     * 
     * @return the global (full) name of this command.
     */
    const vector<string>& global_name() const { return (_global_name); }
    
    /**
     * Set the global name for this command.
     * 
     * @param v the global name value to set.
     */
    void set_global_name(const vector<string>& v) { _global_name = v; }
    
    /**
     * Get the server (i.e., processor) name for this command.
     * 
     * @return the server name for this command.
     */
    const string& server_name() const { return (_server_name); }
    
    /**
     * Set the server (i.e., processor) name for this command.
     * 
     * @param v the server name value to set.
     */
    void set_server_name(const string& v) { _server_name = v; }
    
    /**
     * Set the callback for dynamic generation of children commands.
     * 
     * @param v the callback for dynamic generation of children commands.
     */
    void set_dynamic_children_callback(DYNAMIC_CHILDREN_CALLBACK v);
    
    /**
     * Set the callback for command processing for a dynamically generated
     * child command.
     * 
     * @param v the callback for command processing.
     */
    void set_dynamic_process_callback(const CLI_PROCESS_CALLBACK& v) {
	_dynamic_process_callback = v;
    }

    /**
     * Set the callback for command interrupt for a dynamically generated
     * child command.
     * 
     * @param v the callback for command processing.
     */
    void set_dynamic_interrupt_callback(const CLI_INTERRUPT_CALLBACK& v) {
	_dynamic_interrupt_callback = v;
    }

    /**
     * Set the callback for command processing.
     * 
     * @param v the callback for command processing.
     */
    void set_cli_process_callback(const CLI_PROCESS_CALLBACK& v) {
	_cli_process_callback = v;
    }

    /**
     * Set the callback for command interrupt.
     * 
     * @param v the callback for command processing.
     */
    void set_cli_interrupt_callback(const CLI_INTERRUPT_CALLBACK& v) {
	_cli_interrupt_callback = v;
    }

private:
    friend class CliClient;
    const string& name() const { return (_name); }
    const string& cd_prompt() { return (_cd_prompt); }
    
    list<CliCommand *>&	child_command_list();
    
    static bool cli_attempt_command_completion_byname(void *obj,
						      WordCompletion *cpl,
						      void *data,
						      const char *line,
						      int word_end,
						      list<CliCommand *>& cli_command_match_list);
    
    const string& help() const { return (_help); }
    const string& help_completion() const { return (_help_completion); }
    
    int delete_pipes();
    
    bool is_same_prefix(const string& token);
    bool is_same_command(const string& token);
    CliCommand *command_find(const string& token);
    bool is_multi_command_prefix(const string& command_line);
    
    bool find_command_help(const char *line, int word_end,
			   set<string>& help_strings);
    bool allow_cd() { return (_allow_cd); }
    
    
    bool has_cli_process_callback();
    bool has_cli_interrupt_callback();
    bool has_dynamic_process_callback();
    bool has_dynamic_interrupt_callback();
    bool has_cli_completion_func() { return (_cli_completion_func != NULL); }
    void set_cli_completion_func (CLI_COMPLETION_FUNC *v) {
	_cli_completion_func = v;
    }
    CLI_PROCESS_CALLBACK _cli_process_callback;	// The processing callback
    CLI_INTERRUPT_CALLBACK _cli_interrupt_callback; // The interrupt callback
    CLI_COMPLETION_FUNC *_cli_completion_func;	// The function to call
						// to complete a command

    // Store the callback to generate dynamic children
    DYNAMIC_CHILDREN_CALLBACK _dynamic_children_callback;
    bool _has_dynamic_children;
    // The cli_process_callback to copy to dynamic children
    CLI_PROCESS_CALLBACK _dynamic_process_callback;
    CLI_INTERRUPT_CALLBACK _dynamic_interrupt_callback;
    
    bool can_complete();
    bool can_pipe() const { return (_can_pipe); }
    CliCommand *cli_command_pipe();
    void set_cli_command_pipe(CliCommand *v) { _cli_command_pipe = v; }
    
    CliCommand *root_command() { return (_root_command); }
    void set_root_command(CliCommand *v) { _root_command = v; }
    
    CliCommand		*_root_command;		// The root command
    CliCommand		*_parent_command;	// Parent command or NULL if
						// the root
    list<CliCommand *>	_child_command_list;	// A list with child commands
    const string	_name;			// The command name
    const string	_help;			// The command help
    vector<string>	_global_name;		// The command global name
    string		_server_name;		// The server to process this command
    string		_help_completion;	// The command help completion
    bool		_allow_cd;		// True if we can "cd" to this
    string		_cd_prompt;		// The prompt if we can "cd"
    bool		_can_pipe;		// True if accepts "|" after it
    bool		_default_nomore_mode;	// True if "no-more" (i.e., unpaged) mode is default
    bool		_is_command_argument;	// True if this is a command argument
    bool		_is_argument_expected;	// True if an argument is expected
    CliCommand		*_cli_command_pipe;	// The "|" pipe command
    TypeMatchCb		_type_match_cb;		// The type match callback
};

class CliCommandMatch {
public:
    CliCommandMatch(const string& command_name, const string& help_string,
		    bool is_executable, bool can_pipe)
	: _command_name(command_name), _help_string(help_string),
	  _is_executable(is_executable), _can_pipe(can_pipe),
	  _default_nomore_mode(false), _is_command_argument(false),
	  _is_argument_expected(false)
    {}

    /**
     * Get the command name.
     *
     * @return the command name.
     */
    const string& command_name() const { return (_command_name); }

    /**
     * Get the help string for the command.
     *
     * @return the help string for the command.
     */
    const string& help_string() const { return (_help_string); }

    /**
     * Test if the command is executable.
     *
     * @return true if the command is executable, otherwise false.
     */
    bool is_executable() const { return (_is_executable); }

    /**
     * Test if the command supports pipes.
     *
     * @return true if the command supports pipes, otherwise false.
     */
    bool can_pipe() const { return (_can_pipe); }

    /**
     * Test if "no-more" (i.e., unpaged) is the default output mode.
     *
     * @return true if "no-more" (i.e., unpaged) is the default output mode,
     * otherwise false.
     */
    bool default_nomore_mode() const { return (_default_nomore_mode); }

    /**
     * Set the default paging mode.
     *
     * @param v if true, then "no-more" (i.e., unpaged) is the default
     * output mode.
     */
    void set_default_nomore_mode(bool v) { _default_nomore_mode = v; }

    /**
     * Test if the command actually represents a command argument.
     * 
     * @return true if the command actually represents a command argument.
     */
    bool is_command_argument() const { return (_is_command_argument); }

    /**
     * Set a flag whether the command actually represents a command argument.
     * 
     * @param v true if the command represents a command argument, otherwise
     * false.
     */
    void set_is_command_argument(bool v) { _is_command_argument = v; }

    /**
     * Test if the command expects an argument.
     *
     * @return true if the command expects an argument, otherwise false.
     */
    bool is_argument_expected() const { return (_is_argument_expected); }

    /**
     * Set a flag whether the command expects an argument.
     *
     * @param v true if the command expectes an argument, otherwise false.
     */
    void set_is_argument_expected(bool v) { _is_argument_expected = v; }
    
    /**
     * Get a reference to the type matching callback.
     *
     * @return a reference to the type matching callback.
     */
    const CliCommand::TypeMatchCb& type_match_cb() const {
	return (_type_match_cb);
    }

    /**
     * Set the type matching callback.
     *
     * @param cb the type matching callback.
     */
    void set_type_match_cb(const CliCommand::TypeMatchCb& cb) {
	_type_match_cb = cb;
    }

private:
    string	_command_name;		// The command name
    string	_help_string;		// The help string for the command
    bool	_is_executable;		// True if the command is executable
    bool	_can_pipe;		// True if the command supports pipes
    bool	_default_nomore_mode;	// True if "no-more" (i.e., unpaged) mode is default
    bool	_is_command_argument;	// True if this is a command argument
    bool	_is_argument_expected;	// True if an argument is expected
    CliCommand::TypeMatchCb _type_match_cb;	// The type matching callback
};

//
// Global variables
//


//
// Global functions prototypes
//

#endif // __CLI_CLI_COMMAND_HH__
