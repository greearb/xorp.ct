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

// $XORP: xorp/rtrmgr/op_commands.hh,v 1.16 2004/06/10 22:41:52 hodson Exp $

#ifndef __RTRMGR_OP_COMMAND_HH__
#define __RTRMGR_OP_COMMAND_HH__


#include <list>
#include <set>

#include "libxorp/asyncio.hh"

#include "cli.hh"
#include "rtrmgr_error.hh"


class ConfigTree;
class OpCommand;
class TemplateTree;

class OpInstance {
public:
    OpInstance(EventLoop* eventloop, const string& executable_filename,
	       const string& command_arguments,
	       RouterCLI::OpModePrintCallback print_cb,
	       RouterCLI::OpModeDoneCallback done_cb,
	       OpCommand* op_command);
    OpInstance(const OpInstance& orig);
    ~OpInstance();

    void append_data(AsyncFileOperator::Event e, const uint8_t* buffer,
		     size_t buffer_bytes, size_t offset);
    void done(bool success);
    bool operator<(const OpInstance& them) const;

private:
    static const size_t OP_BUF_SIZE = 8192;

    string		_executable_filename;
    string		_command_arguments;
    OpCommand*		_op_command;
    AsyncFileReader*	_stdout_file_reader;
    AsyncFileReader*	_stderr_file_reader;
    char		_outbuffer[OP_BUF_SIZE];
    char		_errbuffer[OP_BUF_SIZE];
    bool		_error;
    size_t		_last_offset;
    string		_response;
    RouterCLI::OpModePrintCallback _print_callback;
    RouterCLI::OpModeDoneCallback _done_callback;
};

class OpCommand {
public:
    OpCommand(const list<string>& command_parts);

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
     * @param eventloop
     * @param command_line command to execute and arguments
     * @param print_cb callback to be invoked with output from command.
     * @param done_cb callback to invoke when the command terminates.
     */
    void execute(EventLoop* eventloop,
		 const list<string>& command_line,
		 RouterCLI::OpModePrintCallback print_cb,
		 RouterCLI::OpModeDoneCallback done_cb);

    bool command_match(const list<string>& path_parts,
		       SlaveConfigTree* sct, bool exact_match) const;
    void get_matches(size_t wordnum, SlaveConfigTree* sct,
		     map<string, string>& return_matches,
		     bool& is_executable,
		     bool& can_pipe) const;
    void remove_instance(OpInstance* instance);

private:
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
    OpCommandList(const string& config_template_dir, const TemplateTree* tt)
	throw (InitError);
    ~OpCommandList();

    void set_config_tree(SlaveConfigTree* sct) { _conf_tree = sct; }
    bool check_variable_name(const string& variable_name) const;
    OpCommand* find_op_command(const list<string>& command_parts);
    OpCommand* add_op_command(const OpCommand& op_command);
    bool command_match(const list<string>& command_parts,
		       bool exact_match) const;
    void execute(EventLoop* eventloop, const list<string>& command_parts,
		 RouterCLI::OpModePrintCallback print_cb,
		 RouterCLI::OpModeDoneCallback done_cb) const;
    map<string, string> top_level_commands() const;
    map<string, string> childlist(const string& path,
				  bool& is_executable,
				  bool& can_pipe) const;
    bool find_executable_filename(const string& command_filename,
				  string& executable_filename) const;

private:
    list<OpCommand*>	_op_commands;

    // Below here is temporary storage for use in parsing
    list<string>	_path_segments;
    OpCommand*		_current_command;
    const TemplateTree*	_template_tree;
    SlaveConfigTree*	_conf_tree;
};

#endif // __RTRMGR_OP_COMMAND_HH__
