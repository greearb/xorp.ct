// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/cli/cli_command.hh,v 1.9 2004/06/10 22:40:42 hodson Exp $


#ifndef __CLI_CLI_COMMAND_HH__
#define __CLI_CLI_COMMAND_HH__


//
// CLI command definition.
//


#include <string>
#include <list>
#include <map>

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

//
// The callback to print-out the return result from a processing function
//
typedef XorpCallback5<int,	/* return_value */
    const string&,		/* server_name */
    const string&,		/* cli_term_name */
    uint32_t,			/* cli_session_id */
    const string&,		/* command_global_name */
    const vector<string>&	/* command_args */
>::RefPtr CLI_PROCESS_CALLBACK;

typedef XorpCallback5<void,	/* return_value */
    const string&,		/* server_name */
    const string&,		/* cli_term_name */
    uint32_t,			/* cli_session_id */
    const string&,		/* command_global_name */
    const vector<string>&	/* command_args */
>::RefPtr CLI_INTERRUPT_CALLBACK;

typedef XorpCallback3<map<string, string>, /* return value */
    const string& ,		/* global_name */
    bool& ,			/* can_be_run */
    bool&			/* can_pipe */
>::RefPtr DYNAMIC_CHILDREN_CALLBACK;

//
// The type of a processing function that handles CLI commands
//
typedef int (* CLI_PROCESS_FUNC)(const string& server_name,
				 const string& cli_term_name,
				 uint32_t cli_session_id,
				 const string& command_global_name,
				 const vector<string>& command_args);
typedef void (* CLI_INTERRUPT_FUNC)(const string& server_name,
				    const string& cli_term_name,
				    uint32_t cli_session_id,
				    const string& command_global_name,
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_pipes();
    
    /**
     * Add a child CLI command.
     * 
     * By default, we cannot "cd" to this command.
     * 
     * @param init_command_name the command name to add. It can include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help);
    
    /**
     * Add a child CLI command we can "cd" to it.
     * 
     * By default, we can "cd" to this command.
     * 
     * @param init_command_name the command name to add. It can include
     * more than one command levels in the middle. E.g., "set pim bsr".
     * However, commands "set" and "set pim" must have been installed first.
     * @param init_command_help the command help.
     * @param init_cd_prompt if not an empty string, the CLI prompt
     * when "cd" to this command.
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    const string& init_cd_prompt);
    
    /**
     * Add a child command with a processing callback.
     * 
     * @param init_command_name the command name to add. It can include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param init_cli_process_callback the callback to call when the
     * command is entered for execution from the command-line.
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    const CLI_PROCESS_CALLBACK& init_cli_process_callback);

    /**
     * Add a child command with a processing and an interrupt callbacks.
     * 
     * @param init_command_name the command name to add. It can include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param init_cli_process_callback the callback to call when the
     * command is entered for execution from the command-line.
     * @param init_cli_interrupt_callback the callback to call when the
     * user has interrupted the command (e.g., by typing Ctrl-C).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    const CLI_PROCESS_CALLBACK& init_cli_process_callback,
			    const CLI_INTERRUPT_CALLBACK& init_cli_interrupt_callback);
    
    /**
     * Add a child command with a processing function.
     * 
     * @param init_command_name the command name to add. It can include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param init_cli_process_func the processing function to call when the
     * command is entered for execution from the command-line.
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    CLI_PROCESS_FUNC init_cli_process_func) {
	CLI_PROCESS_CALLBACK cb = callback(init_cli_process_func);
	return (add_command(init_command_name, init_command_help, cb));
    }

    /**
     * Add a child command with a processing function and an interrupt
     * handler.
     * 
     * @param init_command_name the command name to add. It can include
     * more than one command levels in the middle. E.g., "show version pim".
     * However, commands "show" and "show version" must have been installed
     * first.
     * @param init_command_help the command help.
     * @param init_cli_process_func the processing function to call when the
     * command is entered for execution from the command-line.
     * @param init_cli_interrupt_func the function to call when the
     * user has interrupted the command (e.g., by typing Ctrl-C).
     * @return the new child command on success, otherwise NULL.
     */
    CliCommand *add_command(const string& init_command_name,
			    const string& init_command_help,
			    CLI_PROCESS_FUNC init_cli_process_func,
			    CLI_INTERRUPT_FUNC init_cli_interrupt_func) {
	CLI_PROCESS_CALLBACK cb1 = callback(init_cli_process_func);
	CLI_INTERRUPT_CALLBACK cb2 = callback(init_cli_interrupt_func);
	return (add_command(init_command_name, init_command_help, cb1, cb2));
    }
    
    /**
     * Add a child command.
     * 
     * @param child_command the child command to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_command(CliCommand *child_command);
    
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
     * Get the global name of this command (i.e., the full name starting from
     * the root).
     * 
     * @return the global (full) name of this command.
     */
    const string& global_name() const { return (_global_name); }
    
    /**
     * Set the global name for this command.
     * 
     * @param v the global name value to set.
     */
    void set_global_name(const string& v) { _global_name = v; }
    
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

protected:
    void set_cli_process_callback(const CLI_PROCESS_CALLBACK& v) {
	_cli_process_callback = v;
    }

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
    CliCommand *multi_command_find(const string& command_line);
    
    bool find_command_help(const char *line, int word_end, string& ret_string);
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
    bool can_pipe() { return (_can_pipe); }
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
    string		_global_name;		// The command global name
    string		_server_name;		// The server to process this command
    string		_help_completion;	// The command help completion
    bool		_allow_cd;		// True if we can "cd" to this
    string		_cd_prompt;		// The prompt if we can "cd"
    bool		_can_pipe;		// True if accepts "|" after it
    CliCommand		*_cli_command_pipe;	// The "|" pipe command
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __CLI_CLI_COMMAND_HH__
