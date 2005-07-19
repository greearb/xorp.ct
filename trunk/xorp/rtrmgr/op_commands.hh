// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/op_commands.hh,v 1.28 2005/07/18 22:29:17 pavlin Exp $

#ifndef __RTRMGR_OP_COMMAND_HH__
#define __RTRMGR_OP_COMMAND_HH__


#include <list>
#include <set>

#include "libxorp/asyncio.hh"

#include "cli.hh"
#include "rtrmgr_error.hh"


class ConfigTree;
class OpCommand;
class RunCommand;
class TemplateTree;
class SlaveModuleManager;

class OpInstance {
public:
    OpInstance(EventLoop&			eventloop,
	       OpCommand&			op_command,
	       const string&			executable_filename,
	       const string&			command_arguments,
	       RouterCLI::OpModePrintCallback	print_cb,
	       RouterCLI::OpModeDoneCallback	done_cb);
    ~OpInstance();

private:
    OpInstance(const OpInstance&);		// Not implemented
    OpInstance& operator=(const OpInstance&);	// Not implemented

    void stdout_cb(RunCommand* run_command, const string& output);
    void stderr_cb(RunCommand* run_command, const string& output);
    void done_cb(RunCommand* run_command, bool success,
		 const string& error_msg);
    void execute_done(bool success);

    EventLoop&		_eventloop;
    OpCommand&		_op_command;
    string		_executable_filename;
    string		_command_arguments;

    RunCommand*		_run_command;
    string		_error_msg;

    RouterCLI::OpModePrintCallback	_print_cb;
    RouterCLI::OpModeDoneCallback	_done_cb;
};

class OpCommand {
public:
    OpCommand(OpCommandList& ocl, const list<string>& command_parts);

    const list<string>& command_parts() const { return _command_parts; }
    const string& command_name() const { return _command_name; }
    const string& help_string() const { return _help_string; }
    const string& module() const { return _module; }
    const string& command_action() const { return _command_action; }
    void set_help_string(const string& v) { _help_string = v; }
    void set_module(const string& v) { _module = v; }
    void set_command_action(const string& v) { _command_action = v; }
    void set_command_action_filename(const string& v) { _command_action_filename = v; }
    void set_command_action_arguments(const list<string>& v) { _command_action_arguments = v; }
    void set_command_executable_filename(const string& v) { _command_executable_filename = v; }
    bool is_executable() const { return (! _command_action.empty()); }

    void add_opt_param(const string& opt_param, const string& opt_param_help);
    bool has_opt_param(const string& opt_param) const;
    string str() const;
    static string command_parts2command_name(const list<string>& command_parts);
    /**
     * Select a positional argument.
     *
     * @param arguments the list with the arguments.
     * @param position the positional argument (e.g., "$0" specifies all
     * arguments, "$1" is the first argument, "$2" the second argument, etc.)
     * @param error_msg the error message (if error).
     * @return if @ref position is valid, then the string with the selected
     * argument, or an empty string if an error.
     */
    static string select_positional_argument(const list<string>& arguments,
					     const string& position,
					     string& error_msg);
    /**
     * Execute an operational mode command.
     *
     * @param eventloop the event loop.
     * @param command_line command to execute and arguments
     * @param print_cb callback to be invoked with output from command.
     * @param done_cb callback to invoke when the command terminates.
     *
     * @return a pointer to the command instance on success.
     */
    OpInstance* execute(EventLoop& eventloop,
			const list<string>& command_line,
			RouterCLI::OpModePrintCallback print_cb,
			RouterCLI::OpModeDoneCallback done_cb);

    bool command_match(const list<string>& path_parts,
		       SlaveConfigTree* sct, bool exact_match) const;
    void get_matches(size_t wordnum, SlaveConfigTree* sct,
		     map<string, CliCommandMatch>& return_matches) const;
    void add_instance(OpInstance* instance);
    void remove_instance(OpInstance* instance);

private:
    OpCommandList&	_ocl;
    list<string>	_command_parts;
    string		_command_name;
    string		_help_string;
    string		_module;
    string		_command_action;
    string		_command_action_filename;
    list<string>	_command_action_arguments;
    string		_command_executable_filename;
    map<string, string>	_opt_params;	// Optional parameters and the CLI help
    set<OpInstance*>	_instances;
};

class OpCommandList {
public:
    OpCommandList(const string& config_template_dir, const TemplateTree* tt,
		  SlaveModuleManager& mmgr)
	throw (InitError);
    ~OpCommandList();

    bool done() const;
    void incr_running_op_instances_n();
    void decr_running_op_instances_n();

    void set_slave_config_tree(SlaveConfigTree* sct) { _slave_config_tree = sct; }
    bool check_variable_name(const string& variable_name) const;
    OpCommand* find_op_command(const list<string>& command_parts);
    OpCommand* add_op_command(const OpCommand& op_command);
    bool command_match(const list<string>& command_parts,
		       bool exact_match) const;
    OpInstance *execute(EventLoop& eventloop,
			const list<string>& command_parts,
			RouterCLI::OpModePrintCallback print_cb,
			RouterCLI::OpModeDoneCallback done_cb) const;
    map<string, string> top_level_commands() const;
    map<string, CliCommandMatch> childlist(const string& path) const;

private:
    list<OpCommand*>	_op_commands;
    size_t		_running_op_instances_n;

    // Below here is temporary storage for use in parsing
    list<string>	_path_segments;
    OpCommand*		_current_command;
    const TemplateTree*	_template_tree;
    SlaveConfigTree*	_slave_config_tree;
    SlaveModuleManager& _mmgr;
};

#endif // __RTRMGR_OP_COMMAND_HH__
