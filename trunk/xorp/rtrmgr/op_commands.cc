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

#ident "$XORP: xorp/rtrmgr/op_commands.cc,v 1.22 2004/06/01 19:49:57 hodson Exp $"


#include <glob.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "cli.hh"
#include "op_commands.hh"
#include "popen.hh"
#include "slave_conf_tree.hh"
#include "template_tree.hh"
#include "util.hh"
#include "y.opcmd_tab.h"


extern int init_opcmd_parser(const char *filename, OpCommandList *o);
extern void parse_opcmd() throw (ParseError);
extern int opcmderror(const char *s);

OpInstance::OpInstance(EventLoop* eventloop, const string& executable_filename,
		       const string& command_arguments,
		       RouterCLI::OpModeCallback cb,
		       OpCommand* op_command)
    : _executable_filename(executable_filename),
      _command_arguments(command_arguments),
      _op_command(op_command),
      _stdout_file_reader(NULL),
      _stderr_file_reader(NULL),
      _error(false),
      _last_offset(0),
      _done_callback(cb)
{
    XLOG_ASSERT(op_command->is_executable());

    FILE *out_stream, *err_stream;

    memset(_outbuffer, 0, OP_BUF_SIZE);
    memset(_errbuffer, 0, OP_BUF_SIZE);

    string execute = executable_filename + " " + command_arguments;

    popen2(execute, out_stream, err_stream);
    if (out_stream == NULL) {
	cb->dispatch(false, "Failed to execute command", false);
    }

    _stdout_file_reader = new AsyncFileReader(*eventloop, fileno(out_stream));
    _stdout_file_reader->add_buffer((uint8_t*)_outbuffer, OP_BUF_SIZE,
				    callback(this, &OpInstance::append_data));
    if (! _stdout_file_reader->start()) {
	cb->dispatch(false, "Failed to execute command", false);
    }

    _stderr_file_reader = new AsyncFileReader(*eventloop, fileno(err_stream));
    _stderr_file_reader->add_buffer((uint8_t*)_errbuffer, OP_BUF_SIZE,
				    callback(this, &OpInstance::append_data));
    if (! _stderr_file_reader->start()) {
	cb->dispatch(false, "Failed to execute command", false);
    }
}

OpInstance::~OpInstance()
{
    if (_stdout_file_reader != NULL)
	delete _stdout_file_reader;
    if (_stderr_file_reader != NULL)
	delete _stderr_file_reader;
}

void
OpInstance::append_data(AsyncFileOperator::Event e,
			const uint8_t* buffer,
			size_t /* buffer_bytes */,
			size_t offset)
{
    if ((const char *)buffer == _errbuffer) {
	if (_error == false) {
	    // We hadn't previously seen any error output
	    if (e == AsyncFileOperator::END_OF_FILE) {
		// We just got EOF on stderr - ignore this and wait for
		// EOF on stdout
		return;
	    }
	    _error = true;
	    // Reset for reading error response
	    _response = "";
	    _last_offset = 0;
	}
    } else {
	if (_error == true) {
	    // Discard further output
	    return;
	}
    }

    if ((e == AsyncFileOperator::DATA)
	|| (e == AsyncFileOperator::END_OF_FILE)) {
	XLOG_ASSERT(offset >= _last_offset);
	if (offset == _last_offset) {
	    XLOG_ASSERT(e == AsyncFileOperator::END_OF_FILE);
	    done(!_error);
	    return;
	}
	size_t len = offset - _last_offset;
	const char *newdata = (const char *)buffer + _last_offset;
	char tmpbuf[len + 1];
	memcpy(tmpbuf, newdata, len);
	tmpbuf[len] = '\0';
	_response += tmpbuf;
	if (e == AsyncFileOperator::END_OF_FILE) {
	    done(!_error);
	    return;
	}
	_last_offset = offset;
	if (offset == OP_BUF_SIZE) {
	    // The buffer is exhausted
	    _last_offset = 0;
	    if (_error) {
		memset(_errbuffer, 0, OP_BUF_SIZE);
		_stderr_file_reader->add_buffer((uint8_t*)_errbuffer,
						OP_BUF_SIZE,
				callback(this, &OpInstance::append_data));
		_stderr_file_reader->start();
	    } else {
		memset(_outbuffer, 0, OP_BUF_SIZE);
		_stdout_file_reader->add_buffer((uint8_t*)_outbuffer,
						OP_BUF_SIZE,
				callback(this, &OpInstance::append_data));
		_stdout_file_reader->start();
	    }
	}
    } else {
	// Something bad happened
	// XXX ideally we'd figure out what.
	_response += "\nsomething bad happened\n";
	done(false);
    }
}

void
OpInstance::done(bool success)
{
    _done_callback->dispatch(success, _response, true);
    _op_command->remove_instance(this);
    delete this;
}

bool
OpInstance::operator<(const OpInstance& them) const
{
    // Completely arbitrary but unique sorting order
    if (_stdout_file_reader < them._stdout_file_reader)
	return true;
    else
	return false;
}

OpCommand::OpCommand(const list<string>& command_parts)
    : _command_parts(command_parts)
{
    _command_name = command_parts2command_name(command_parts);
}

void
OpCommand::add_opt_param(const string& opt_param, const string& opt_param_help)
{
    XLOG_ASSERT(_opt_params.find(opt_param) == _opt_params.end());
    _opt_params.insert(make_pair(opt_param, opt_param_help));
}

bool
OpCommand::has_opt_param(const string& opt_param) const
{
    return (_opt_params.find(opt_param) != _opt_params.end());
}

string
OpCommand::str() const
{
    string res = command_name() + "\n";

    if (_command_action.empty()) {
	res += "  No command action specified\n";
    } else {
	res += "  Command: " + _command_action + "\n";
    }

    map<string, string>::const_iterator iter;
    for (iter = _opt_params.begin(); iter != _opt_params.end(); ++iter) {
	res += "  Optional Parameter: " + iter->first;
	res += "  (Parameter Help: )" + iter->second;
	res += "\n";
    }
    return res;
}

string
OpCommand::command_parts2command_name(const list<string>& command_parts)
{
    string res;

    list<string>::const_iterator iter;
    for (iter = command_parts.begin(); iter != command_parts.end(); ++iter) {
	if (iter != command_parts.begin())
	    res += " ";
	res += *iter;
    }
    return res;
}

string
OpCommand::select_positional_argument(const list<string>& arguments,
				      const string& position,
				      string& error_msg)
{
    //
    // Check the positional argument
    //
    if (position.empty()) {
	error_msg = c_format("Empty positional argument");
	return string("");
    }
    if (position[0] != '$') {
	error_msg = c_format("Invalid positional argument \"%s\": "
			     "first symbol is not '$'", position.c_str());
	return string("");
    }
    if (position.size() <= 1) {
	error_msg = c_format("Invalid positional argument \"%s\": "
			     "missing position value", position.c_str());
	return string("");
    }

    //
    // Get the positional argument value
    //
    const string pos_str = position.substr(1, string::npos);
    int pos = atoi(pos_str.c_str());
    if ((pos < 0) || (pos > static_cast<int>(arguments.size()))) {
	error_msg = c_format("Invalid positional argument \"%s\": "
			     "expected values must be in interval "
			     "[0, %u]",
			     position.c_str(),
			     static_cast<uint32_t>(arguments.size()));
	return string("");
    }

    string resolved_str;
    list<string>::const_iterator iter;
    if (pos == 0) {
	// Add all arguments
	for (iter = arguments.begin(); iter != arguments.end(); ++iter) {
	    if (! resolved_str.empty())
		resolved_str += " ";
	    resolved_str += *iter;
	}
    } else {
	// Select a single argument
	int tmp_pos = 0;
	for (iter = arguments.begin(); iter != arguments.end(); ++iter) {
	    tmp_pos++;
	    if (tmp_pos == pos) {
		resolved_str += *iter;
		break;
	    }
	}
    }
    XLOG_ASSERT(! resolved_str.empty());

    return resolved_str;
}

void
OpCommand::execute(EventLoop* eventloop, const list<string>& command_line,
		   RouterCLI::OpModeCallback cb)
{
    string command_arguments_str;
    list<string>::const_iterator iter;

    if (! is_executable()) {
	cb->dispatch(false, "Command is not executable", false);
	return;
    }

    //
    // Add all arguments. If an argument is positional (e.g., $0, $1, etc),
    // then resolve it by using the strings from the command line.
    //
    for (iter = _command_action_arguments.begin();
	 iter != _command_action_arguments.end();
	 ++iter) {
	const string& arg = *iter;
	XLOG_ASSERT(! arg.empty());

	// Add an extra space between arguments
	if (! command_arguments_str.empty())
	    command_arguments_str += " ";

	// If the argument is not positional, then just add it
	if (arg[0] != '$') {
	    command_arguments_str += arg;
	    continue;
	}

	//
	// The argument is positional, hence resolve it using
	// the command-line strings.
	//
	string error_msg;
	string resolved_str = select_positional_argument(command_line, arg,
							 error_msg);
	if (resolved_str.empty()) {
	    XLOG_FATAL("Internal programming error: %s", error_msg.c_str());
	}
	command_arguments_str += resolved_str;
    }

    _instances.insert(new OpInstance(eventloop, _command_executable_filename,
				     command_arguments_str, cb, this));
}

bool
OpCommand::prefix_matches(const list<string>& path_parts,
			  SlaveConfigTree* conf_tree) const
{
    list<string>::const_iterator them, us;

    them = path_parts.begin();
    us = _command_parts.begin();
    // First go through the fixed parts of the command
    while (true) {
	if (them == path_parts.end())
	    return true;
	if (us == _command_parts.end())
	    break;
	if ((*us)[0] == '$') {
	    // Matching against a varname
	    list<string> varmatches;
	    conf_tree->expand_varname_to_matchlist(*us, varmatches);
	    list<string>::const_iterator vi;
	    bool ok = false;
	    for (vi = varmatches.begin(); vi != varmatches.end(); ++vi) {
		if (*vi == *them) {
		    ok = true;
		    break;
		}
	    }
	    if (ok == false)
		return false;
	} else if (*them != *us) {
	    return false;
	}
	++them;
	++us;
    }
    // No more fixed parts, try optional parameters
    while (them != path_parts.end()) {
	map<string, string>::const_iterator opi;
	bool ok = false;
	for (opi = _opt_params.begin(); opi != _opt_params.end(); ++opi) {
	    if (opi->first == *them) {
		ok = true;
		break;
	    }
	}
	if (ok == false)
	    return false;
	++them;
    }
    return true;
}

void
OpCommand::get_matches(size_t wordnum, SlaveConfigTree* conf_tree,
		       map<string, string>& return_matches,
		       bool& is_executable,
		       bool& can_pipe) const
{
    is_executable = false;
    can_pipe = false;

    list<string>::const_iterator ci = _command_parts.begin();
    for (size_t i = 0; i < wordnum; i++) {
	++ci;
	if (ci == _command_parts.end())
	    break;
    }
    if (ci == _command_parts.end()) {
	// Add all the optional parameters
	map<string, string>::const_iterator opi;
	for (opi = _opt_params.begin(); opi != _opt_params.end(); ++opi) {
	    return_matches.insert(*opi);
	}
	is_executable = true;
	can_pipe = true;
    } else {
	string match = *ci;
	if (match[0] == '$') {
	    XLOG_ASSERT(match[1] == '(');
	    XLOG_ASSERT(match[match.size() - 1] == ')');
	    list<string> var_matches;
	    conf_tree->expand_varname_to_matchlist(match, var_matches);
	    list<string>::const_iterator vi;
	    for (vi = var_matches.begin(); vi != var_matches.end(); ++vi) {
		return_matches.insert(make_pair(*vi, _help_string));
	    }
	} else {
	    return_matches.insert(make_pair(match, _help_string));
	}
    }

    if (! _command_action.empty()) {
	is_executable = true;
	can_pipe = true;
    }
}

void
OpCommand::remove_instance(OpInstance* instance)
{
    set<OpInstance*>::iterator iter;

    iter = _instances.find(instance);
    XLOG_ASSERT(iter != _instances.end());
    _instances.erase(iter);
}

OpCommandList::OpCommandList(const string& config_template_dir,
			     const TemplateTree* tt) throw (InitError)
{
    list<string> files;
    string errmsg;

    _template_tree = tt;

    struct stat dir_data;
    if (stat(config_template_dir.c_str(), &dir_data) < 0) {
	errmsg = c_format("Error reading config directory %s: %s",
			  config_template_dir.c_str(), strerror(errno));
	xorp_throw(InitError, errmsg);
    }

    if ((dir_data.st_mode & S_IFDIR) == 0) {
	errmsg = c_format("Error reading config directory %s: not a directory",
			  config_template_dir.c_str());
	xorp_throw(InitError, errmsg);
    }

    // TODO: file suffix is hardcoded here!
    string globname = config_template_dir + "/*.cmds";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find config files in %s",
			  config_template_dir.c_str());
	xorp_throw(InitError, errmsg);
    }

    if (pglob.gl_pathc == 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find any template files in %s",
			  config_template_dir.c_str());
	xorp_throw(InitError, errmsg);
    }

    for (size_t i = 0; i < (size_t)pglob.gl_pathc; i++) {
	if (init_opcmd_parser(pglob.gl_pathv[i], this) < 0) {
	    globfree(&pglob);
	    errmsg = c_format("Failed to open template file: %s",
			      config_template_dir.c_str());
	    xorp_throw(InitError, errmsg);
	}
	try {
	    parse_opcmd();
	} catch (const ParseError& pe) {
	    globfree(&pglob);
	    xorp_throw(InitError, pe.why());
	}
	if (_path_segments.size() != 0) {
	    globfree(&pglob);
	    errmsg = c_format("File %s is not terminated properly",
			      pglob.gl_pathv[i]);
	    xorp_throw(InitError, errmsg);
	}
    }

    globfree(&pglob);
}

OpCommandList::~OpCommandList()
{
    while (!_op_commands.empty()) {
	delete _op_commands.front();
	_op_commands.pop_front();
    }
}

bool
OpCommandList::check_variable_name(const string& variable_name) const
{
    return _template_tree->check_variable_name(variable_name);
}

OpCommand*
OpCommandList::find_op_command(const list<string>& command_parts)
{
    string command_name = OpCommand::command_parts2command_name(command_parts);

    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	if ((*iter)->command_name() == command_name)
	    return *iter;
    }
    return NULL;
}

bool
OpCommandList::prefix_matches(const list<string>& command_parts) const
{
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	if ((*iter)->prefix_matches(command_parts, _conf_tree))
	    return true;
    }
    return false;
}

void
OpCommandList::execute(EventLoop *eventloop, const list<string>& command_parts,
		       RouterCLI::OpModeCallback cb) const
{
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	// Find the right command
	if ((*iter)->prefix_matches(command_parts, _conf_tree)) {
	    // Execute it
	    (*iter)->execute(eventloop, command_parts, cb);
	    // XXX: don't worry about errors - the callback reports them.
	    return;
	}
    }
    cb->dispatch(false, string("No matching command"), false);
}

OpCommand*
OpCommandList::add_op_command(const OpCommand& op_command)
{
    OpCommand *new_command;

    XLOG_ASSERT(find_op_command(op_command.command_parts()) == NULL);

    new_command = new OpCommand(op_command);
    _op_commands.push_back(new_command);
    return new_command;
}

bool
OpCommandList::find_executable_filename(const string& command_filename,
					string& executable_filename)
    const
{
    struct stat statbuf;

    if (command_filename[0] == '/') {
	// Absolute path name
	if ((stat(command_filename.c_str(), &statbuf) == 0)
	    && (statbuf.st_mode & S_IFREG > 0)
	    && (statbuf.st_mode & S_IXUSR > 0)) {
	    executable_filename = command_filename;
	    return true;
	}
	return false;
    }

    // Relative path name
    string xorp_root_dir = _template_tree->xorp_root_dir();

    list<string> path;
    path.push_back(xorp_root_dir);

    // Expand path
    const char* p = getenv("PATH");
    if (p != NULL) {
	list <string> l2 = split(p, ':');
	path.splice(path.end(), l2);
    }

    while (!path.empty()) {
	string full_path_executable = path.front() + "/" + command_filename;
	if ((stat(full_path_executable.c_str(), &statbuf) == 0)
	    && (statbuf.st_mode & S_IFREG > 0)
	    && (statbuf.st_mode & S_IXUSR > 0)) {
	    executable_filename = full_path_executable;
	    return true;
	}
	path.pop_front();
    }
    return false;
}

map<string, string>
OpCommandList::top_level_commands() const
{
    map<string, string> commands;

    //
    // Add the first word of every command, and the help if this
    // indeed is a top-level command
    //
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	const OpCommand* op_command = *iter;
	list<string> path_parts = split(op_command->command_name(), ' ');
	const string& top_command = path_parts.front();
	bool is_top_command = false;

	if (path_parts.size() == 1)
	    is_top_command = true;

	if (is_top_command) {
	    commands.insert(make_pair(top_command, op_command->help_string()));
	    continue;
	}

	// TODO: XXX: Get rid of this hard-coded "help" string!!
	commands.insert(make_pair(top_command, string("help")));
    }
    return commands;
}

map<string, string>
OpCommandList::childlist(const string& path, bool& is_executable,
			 bool& can_pipe) const
{
    map<string, string> children;
    list<string> path_parts = split(path, ' ');

    is_executable = false;
    can_pipe = false;
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	const OpCommand* op_command = *iter;
	if (op_command->prefix_matches(path_parts, _conf_tree)) {
	    map<string, string> matches;
	    bool tmp_is_executable;
	    bool tmp_can_pipe;
	    op_command->get_matches(path_parts.size(), _conf_tree,
				    matches, tmp_is_executable, tmp_can_pipe);
	    if (tmp_is_executable)
		is_executable = true;
	    if (tmp_can_pipe)
		can_pipe = true;
	    map<string, string>::iterator mi;
	    for (mi = matches.begin(); mi != matches.end(); ++mi) {
		string command_string = mi->first;
		string help_string = mi->second;
		XLOG_ASSERT(command_string != "");
		children.insert(make_pair(command_string, help_string));
	    }
	}
    }
    return children;
}
