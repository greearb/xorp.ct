// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/run_command.hh"
#include "libxorp/utils.hh"

#ifdef HAVE_GLOB_H
#include <glob.h>
#elif defined(HOST_OS_WINDOWS)
#include "glob_win32.h"
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif

#include "cli.hh"
#include "op_commands.hh"
#include "slave_conf_tree.hh"
#include "template_tree.hh"
#include "slave_module_manager.hh"
#if defined(NEED_LEX_H_HACK)
#include "y.opcmd_tab.cc.h"
#else
#include "y.opcmd_tab.hh"
#endif

#ifdef HOST_OS_WINDOWS
#ifdef _NO_OLDNAMES
#define	stat	_stat
#define	S_IFDIR	_S_IFDIR
#define	S_ISREG	_S_ISREG
#endif
#endif

extern int init_opcmd_parser(const char *filename, OpCommandList *o);
extern void parse_opcmd() throw (ParseError);
extern int opcmderror(const char *s);

OpInstance::OpInstance(EventLoop&			eventloop,
		       OpCommand&			op_command,
		       const string&			executable_filename,
		       const list<string>&		command_argument_list,
		       RouterCLI::OpModePrintCallback	print_cb,
		       RouterCLI::OpModeDoneCallback	done_cb)
    : _eventloop(eventloop),
      _op_command(op_command),
      _executable_filename(executable_filename),
      _command_argument_list(command_argument_list),
      _run_command(NULL),
      _print_cb(print_cb),
      _done_cb(done_cb)
{
    string errmsg;
    bool success = true;

    XLOG_ASSERT(_op_command.is_executable());

    _op_command.add_instance(this);

    do {
	if (_executable_filename.empty()) {
	    errmsg = c_format("Empty program filename");
	    success = false;
	    break;
	}

	// Run the program
	XLOG_ASSERT(_run_command == NULL);
	_run_command = new RunCommand(
	    _eventloop,
	    _executable_filename,
	    _command_argument_list,
	    callback(this, &OpInstance::stdout_cb),
	    callback(this, &OpInstance::stderr_cb),
	    callback(this, &OpInstance::done_cb),
	    true /* redirect_stderr_to_stdout */,
	    XorpTask::PRIORITY_LOWEST);	// Give the user input highest priority
	if (_run_command->execute() != XORP_OK) {
	    // Create the string with the command name and its arguments
	    string program_request = _executable_filename;
	    list<string>::iterator iter;
	    for (iter = _command_argument_list.begin();
		 iter != _command_argument_list.end();
		 ++iter) {
		program_request += " ";
		program_request += iter->c_str();
	    }

	    delete _run_command;
	    _run_command = NULL;
	    errmsg = c_format("Could not execute program %s",
			      program_request.c_str());
	    success = false;
	    break;
	}
	break;
    } while (false);

    if (! success)
	_done_cb->dispatch(false, errmsg);
}

OpInstance::~OpInstance()
{
    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
    _op_command.remove_instance(this);
}

void
OpInstance::terminate()
{
    if (_run_command != NULL)
	_run_command->terminate();
}

void
OpInstance::terminate_with_prejudice()
{
    if (_run_command != NULL)
	_run_command->terminate_with_prejudice();
}

void
OpInstance::stdout_cb(RunCommand* run_command, const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    // XXX: don't accumulate the output, but print it immediately
    _print_cb->dispatch(output);
}

void
OpInstance::stderr_cb(RunCommand* run_command, const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    // XXX: print immediately the stderr along with the stdout
    _print_cb->dispatch(output);
}

void
OpInstance::done_cb(RunCommand* run_command, bool success,
		    const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);

    if (! success)
	_error_msg += error_msg;

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }

    execute_done(success);
}

void
OpInstance::execute_done(bool success)
{
    _done_cb->dispatch(success, _error_msg);
    // The callback will delete us. Don't do anything more in this method.
    //    delete this;
}

OpCommand::OpCommand(OpCommandList& ocl, const list<string>& command_parts)
    : _ocl(&ocl),
      _command_parts(command_parts),
      _is_invalid(false),
      _default_nomore_mode(false)
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

list<string>
OpCommand::select_positional_argument(const list<string>& argument_list,
				      const string& position,
				      string& error_msg)
{
    list<string> resolved_list;

    //
    // Check the positional argument
    //
    if (position.empty()) {
	error_msg = c_format("Empty positional argument");
	return resolved_list;
    }
    if (position[0] != '$') {
	error_msg = c_format("Invalid positional argument \"%s\": "
			     "first symbol is not '$'", position.c_str());
	return resolved_list;
    }
    if (position.size() <= 1) {
	error_msg = c_format("Invalid positional argument \"%s\": "
			     "missing position value", position.c_str());
	return resolved_list;
    }

    //
    // Get the positional argument value
    //
    const string pos_str = position.substr(1, string::npos);
    int pos = atoi(pos_str.c_str());
    if ((pos < 0) || (pos > static_cast<int>(argument_list.size()))) {
	error_msg = c_format("Invalid positional argument \"%s\": "
			     "expected values must be in interval "
			     "[0, %u]",
			     position.c_str(),
			     XORP_UINT_CAST(argument_list.size()));
	return resolved_list;
    }

    list<string>::const_iterator iter;
    if (pos == 0) {
	// Add all arguments
	resolved_list = argument_list;
    } else {
	// Select a single argument
	int tmp_pos = 0;
	for (iter = argument_list.begin();
	     iter != argument_list.end();
	     ++iter) {
	    tmp_pos++;
	    if (tmp_pos == pos) {
		resolved_list.push_back(*iter);
		break;
	    }
	}
    }
    XLOG_ASSERT(! resolved_list.empty());

    return resolved_list;
}

OpInstance *
OpCommand::execute(EventLoop& eventloop, const list<string>& command_line,
		   RouterCLI::OpModePrintCallback print_cb,
		   RouterCLI::OpModeDoneCallback done_cb)
{
    list<string> resolved_command_argument_list;
    list<string>::const_iterator iter;

    if (! is_executable()) {
	done_cb->dispatch(false, "Command is not executable");
	return 0;
    }

    //
    // Add all arguments. If an argument is positional (e.g., $0, $1, etc),
    // then resolve it by using the strings from the command line.
    //
    for (iter = _command_action_argument_list.begin();
	 iter != _command_action_argument_list.end();
	 ++iter) {
	const string& arg = *iter;
	XLOG_ASSERT(! arg.empty());

	// If the argument is not positional, then just add it
	if (arg[0] != '$') {
	    resolved_command_argument_list.push_back(arg);
	    continue;
	}

	//
	// The argument is positional, hence resolve it using
	// the command-line strings.
	//
	string error_msg;
	list<string> resolved_list = select_positional_argument(command_line,
								arg,
								error_msg);
	if (resolved_list.empty()) {
	    XLOG_FATAL("Internal programming error: %s", error_msg.c_str());
	}
	resolved_command_argument_list.insert(
	    resolved_command_argument_list.end(),
	    resolved_list.begin(),
	    resolved_list.end());
    }

    OpInstance *opinst = new OpInstance(eventloop, *this,
					_command_executable_filename,
					resolved_command_argument_list,
					print_cb, done_cb);

    return opinst;
}

bool
OpCommand::command_match(const list<string>& path_parts,
			 SlaveConfigTree* slave_config_tree,
			 bool exact_match) const
{
    list<string>::const_iterator them, us;

    them = path_parts.begin();
    us = _command_parts.begin();

    //
    // First go through the fixed parts of the command
    //
    while (true) {
	if ((them == path_parts.end()) && (us == _command_parts.end())) {
	    return true;
	}
	if (them == path_parts.end()) {
	    if (exact_match)
		return false;
	    else
		return true;
	}
	if (us == _command_parts.end())
		break;

	if ((*us)[0] == '$') {
	    // Matching against a varname
	    list<string> varmatches;
	    slave_config_tree->expand_varname_to_matchlist(*us, varmatches);
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
	} else if ((*us)[0] == '<') {
	    /* any single word matches a wildcard */
	} else if (*them != *us) {
	    return false;
	}
	++them;
	++us;
    }

    //
    // No more fixed parts, try optional parameters
    //
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
OpCommand::get_matches(size_t wordnum, SlaveConfigTree* slave_config_tree,
		       map<string, CliCommandMatch>& return_matches) const
{
    list<string>::const_iterator ci = _command_parts.begin();
    bool is_last = true;

    for (size_t i = 0; i < wordnum; i++) {
	++ci;
	if (ci == _command_parts.end())
	    break;
    }

    if (ci == _command_parts.end()) {
	// Add all the optional parameters
	map<string, string>::const_iterator opi;
	for (opi = _opt_params.begin(); opi != _opt_params.end(); ++opi) {
	    const string& command_name = opi->first;
	    const string& help_string = opi->second;
	    CliCommandMatch ccm(command_name, help_string, true, true);
	    ccm.set_default_nomore_mode(default_nomore_mode());
	    return_matches.insert(make_pair(command_name, ccm));
	}
	return;
    }

    //
    // Get the matching part of the name and test whether it is the last one
    //
    string match = *ci;
    ++ci;
    if (ci == _command_parts.end()) {
	is_last = true;
    } else {
	is_last = false;
    }

    do {
	if (match[0] == '$') {
	    XLOG_ASSERT(match[1] == '(');
	    XLOG_ASSERT(match[match.size() - 1] == ')');
	    list<string> var_matches;
	    slave_config_tree->expand_varname_to_matchlist(match, var_matches);
	    list<string>::const_iterator vi;
	    for (vi = var_matches.begin(); vi != var_matches.end(); ++vi) {
		const string& command_name = *vi;
		string help_string = _help_string;
		if (! is_last)
		    help_string = "-- No help available --";
		CliCommandMatch ccm(command_name, help_string,
				    is_last && is_executable(),
				    is_last && can_pipe());
		ccm.set_default_nomore_mode(is_last && default_nomore_mode());
		return_matches.insert(make_pair(command_name, ccm));
	    }
	    break;
	}
	if (match[0] == '<') {
	    // A mandatory argument that is supplied by the user
	    XLOG_ASSERT(match[match.size() - 1] == '>');
	    const string& command_name = match;
	    string help_string = _help_string;
	    if (! is_last)
		help_string = "-- No help available --";
	    CliCommandMatch ccm(command_name, help_string,
				is_last && is_executable(),
				is_last && can_pipe());
	    ccm.set_default_nomore_mode(is_last && default_nomore_mode());
	    CliCommand::TypeMatchCb cb;
	    cb = callback(this, &OpCommand::type_match);
	    ccm.set_type_match_cb(cb);
	    return_matches.insert(make_pair(command_name, ccm));
	    break;
	}
	const string& command_name = match;
	string help_string = _help_string;
	if (! is_last)
	    help_string = "-- No help available --";
	CliCommandMatch ccm(command_name, help_string,
			    is_last && is_executable(),
			    is_last && can_pipe());
	ccm.set_default_nomore_mode(is_last && default_nomore_mode());
	return_matches.insert(make_pair(command_name, ccm));
	break;
    } while (false);
}

bool
OpCommand::type_match(const string& s, string& errmsg) const
{
    if (s.empty())
	return (false);

    UNUSED(errmsg);

    //
    // XXX: currently we don't support type-based matching, so
    // any string can match.
    //
    return (true);
}

void
OpCommand::add_instance(OpInstance* instance)
{
    set<OpInstance*>::iterator iter;

    iter = _instances.find(instance);
    XLOG_ASSERT(iter == _instances.end());

    _instances.insert(instance);
    _ocl->incr_running_op_instances_n();
}

void
OpCommand::remove_instance(OpInstance* instance)
{
    set<OpInstance*>::iterator iter;

    iter = _instances.find(instance);
    XLOG_ASSERT(iter != _instances.end());

    _instances.erase(iter);
    _ocl->decr_running_op_instances_n();
}
OpCommandList::OpCommandList(const TemplateTree* tt, SlaveModuleManager& mmgr)
    : _running_op_instances_n(0),
      _template_tree(tt),
      _mmgr(mmgr)
{
}

OpCommandList::OpCommandList(const string& config_template_dir,
			     const TemplateTree* tt,
			     SlaveModuleManager& mmgr) throw (InitError)
    : _running_op_instances_n(0),
      _template_tree(tt),
      _mmgr(mmgr)
{
    string errmsg;

    if (read_templates(config_template_dir, errmsg) != XORP_OK)
	xorp_throw(InitError, errmsg);
}

OpCommandList::~OpCommandList()
{
    while (!_op_commands.empty()) {
	delete _op_commands.front();
	_op_commands.pop_front();
    }
}

int
OpCommandList::read_templates(const string& config_template_dir,
			      string& errmsg)
{
    list<string> files;

    struct stat dir_data;
    if (stat(config_template_dir.c_str(), &dir_data) < 0) {
	errmsg = c_format("Error reading config directory %s: %s",
			  config_template_dir.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    if ((dir_data.st_mode & S_IFDIR) == 0) {
	errmsg = c_format("Error reading config directory %s: not a directory",
			  config_template_dir.c_str());
	return (XORP_ERROR);
    }

    // TODO: file suffix is hardcoded here!
    string globname = config_template_dir + "/*.cmds";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find config files in %s",
			  config_template_dir.c_str());
	return (XORP_ERROR);
    }

    if (pglob.gl_pathc == 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find any template files in %s",
			  config_template_dir.c_str());
	return (XORP_ERROR);
    }

    for (size_t i = 0; i < (size_t)pglob.gl_pathc; i++) {
	if (init_opcmd_parser(pglob.gl_pathv[i], this) < 0) {
	    globfree(&pglob);
	    errmsg = c_format("Failed to open template file: %s",
			      config_template_dir.c_str());
	    return (XORP_ERROR);
	}
	try {
	    parse_opcmd();
	} catch (const ParseError& pe) {
	    globfree(&pglob);
	    errmsg = pe.why();
	    return (XORP_ERROR);
	}
	if (_path_segments.size() != 0) {
	    globfree(&pglob);
	    errmsg = c_format("File %s is not terminated properly",
			      pglob.gl_pathv[i]);
	    return (XORP_ERROR);
	}
    }

    globfree(&pglob);

    return (XORP_OK);
}

bool
OpCommandList::done() const
{
    if (_running_op_instances_n == 0)
	return (true);
    else
	return (false);
}

void
OpCommandList::incr_running_op_instances_n()
{
    _running_op_instances_n++;
}

void
OpCommandList::decr_running_op_instances_n()
{
    XLOG_ASSERT(_running_op_instances_n > 0);
    _running_op_instances_n--;
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
OpCommandList::command_match(const list<string>& command_parts,
			     bool exact_match) const
{
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	if ((*iter)->command_match(command_parts, _slave_config_tree,
				   exact_match))
	    return true;
    }
    return false;
}

OpInstance*
OpCommandList::execute(EventLoop& eventloop, const list<string>& command_parts,
		       RouterCLI::OpModePrintCallback print_cb,
		       RouterCLI::OpModeDoneCallback done_cb) const
{
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	// Find the right command
	if ((*iter)->command_match(command_parts, _slave_config_tree, true)) {
	    // Execute it
	    return (*iter)->execute(eventloop,
				    command_parts, print_cb, done_cb);
	}
    }
    done_cb->dispatch(false, string("No matching command"));
    return 0;
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

map<string, CliCommandMatch>
OpCommandList::top_level_commands() const
{
    map<string, CliCommandMatch> commands;

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

	//
	// XXX: If the module has not been started, then don't add its
	// commands. However, add all commands that are not associated
	// with any module.
	//
	if ((_mmgr.module_is_active(op_command->module()) == false)
	    && (! op_command->module().empty())) {
	    continue;
	}

	if (path_parts.size() == 1)
	    is_top_command = true;

	if (is_top_command) {
	    commands.erase(top_command);
	    CliCommandMatch ccm(op_command->command_name(),
				op_command->help_string(),
				op_command->is_executable(),
				op_command->can_pipe());
	    ccm.set_default_nomore_mode(op_command->default_nomore_mode());
	    commands.insert(make_pair(top_command, ccm));
	    continue;
	}

	if (commands.find(top_command) == commands.end()) {
	    CliCommandMatch ccm(top_command,
				string("-- No help available --"),
				false,
				false);
	    ccm.set_default_nomore_mode(false);
	    commands.insert(make_pair(top_command, ccm));
	}
    }
    return commands;
}

map<string, CliCommandMatch>
OpCommandList::childlist(const vector<string>& vector_path) const
{
    map<string, CliCommandMatch> children;
    list<string> path_parts;

    path_parts.insert(path_parts.end(), vector_path.begin(),
		      vector_path.end());
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_commands.begin(); iter != _op_commands.end(); ++iter) {
	const OpCommand* op_command = *iter;
	if (op_command->command_match(path_parts, _slave_config_tree, false)) {
	    //
	    // XXX: If the module has not been started, then don't add its
	    // commands. However, add all commands that are not associated
	    // with any module.
	    //
	    if ((_mmgr.module_is_active(op_command->module()) == false)
		&& (! op_command->module().empty())) {
		continue;
	    }

	    map<string, CliCommandMatch> matches;
	    op_command->get_matches(path_parts.size(), _slave_config_tree,
				    matches);
	    map<string, CliCommandMatch>::iterator mi;
	    for (mi = matches.begin(); mi != matches.end(); ++mi) {
		const string& key = mi->first;
		const CliCommandMatch& ccm = mi->second;
		XLOG_ASSERT(ccm.command_name() != "");
		children.insert(make_pair(key, ccm));
	    }
	}
    }
    return children;
}
