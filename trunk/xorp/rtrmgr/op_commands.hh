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

// $XORP: xorp/rtrmgr/op_commands.hh,v 1.5 2003/12/02 09:38:55 pavlin Exp $

#ifndef __RTRMGR_OP_COMMAND_HH__
#define __RTRMGR_OP_COMMAND_HH__

#include <list>
#include <set>
#include "libxorp/xorp.h"
#include "libxorp/asyncio.hh"
#include "rtrmgr_error.hh"
#include "cli.hh"


class ConfigTree;
class OpCommand;
class TemplateTree;

class OpInstance {
public:
    OpInstance(EventLoop* eventloop, const string& executable,
	       const string& cmd_line, RouterCLI::OpModeCallback cb,
	       const OpCommand* op_cmd);
    OpInstance(const OpInstance& orig);
    ~OpInstance();

    static const size_t OP_BUF_SIZE = 8192;

    void append_data(AsyncFileOperator::Event e, const uint8_t* buffer,
		     size_t buffer_bytes, size_t offset);
    void done(bool success);
    bool operator<(const OpInstance& them) const;

private:
    string		_executable;
    string		_cmd_line;
    const OpCommand*	_op_cmd;
    AsyncFileReader*	_stdout_file_reader;
    AsyncFileReader*	_stderr_file_reader;
    char		_outbuffer[OP_BUF_SIZE];
    char		_errbuffer[OP_BUF_SIZE];
    bool		_error;
    size_t		_last_offset;
    string		_response;
    RouterCLI::OpModeCallback _done_callback;
};

class OpCommand {
public:
    OpCommand(const list<string>& parts);

    int add_command_file(const string& cmd_file);
    int add_module(const string& module);
    int add_opt_param(const string& op_param);
    string cmd_name() const;
    string str() const;
    void execute(EventLoop* eventloop, const list<string>& cmd_line,
		 RouterCLI::OpModeCallback cb) const;

    bool operator==(const OpCommand& them) const;
    bool prefix_matches(const list<string>& pathparts, SlaveConfigTree* sct);
    set<string> get_matches(size_t wordnum, SlaveConfigTree* sct);
    void remove_instance(OpInstance* instance) const;

private:
    list<string>	_cmd_parts;
    string		_cmd_file;
    string		_module;
    list<string>	_opt_params;
    mutable set<OpInstance*> _instances;
};

class OpCommandList {
public:
    OpCommandList(const string& config_template_dir, const TemplateTree* tt)
	throw (InitError);
    ~OpCommandList();

    void set_config_tree(SlaveConfigTree* sct) { _conf_tree = sct; }
    OpCommand* new_op_command(const list<string>& parts);
    int check_variable_name(const string& name) const;
    OpCommand* find(const list<string>& parts);
    bool prefix_matches(const list<string>& parts) const;
    void execute(EventLoop* eventloop, const list<string>& parts,
		 RouterCLI::OpModeCallback cb) const;
    int append_path(const string& path);
    int push_path();
    int pop_path();
    int add_cmd(const string& cmd);
    int add_cmd_action(const string& cmd, const list<string>& parts);
    void display_list() const;
    set<string> top_level_commands() const;
    set<string> childlist(const string& path, bool& make_executable) const;

private:
    bool find_executable(const string& filename, string& executable) const;

    list<OpCommand*>	_op_cmds;

    // Below here is temporary storage for use in parsing
    list<string>	_path_segments;
    OpCommand*		_current_cmd;
    const TemplateTree*	_template_tree;
    SlaveConfigTree*	_conf_tree;
};

#endif // __RTRMGR_OP_COMMAND_HH__
