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

#ident "$XORP: xorp/rtrmgr/op_commands.cc,v 1.8 2003/11/20 06:05:05 pavlin Exp $"

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "template_tree.hh"
#include "slave_conf_tree.hh"
#include "op_commands.hh"
#include "y.opcmd_tab.h"
#include "cli.hh"
#include "util.hh"
#include "popen.hh"

extern int init_opcmd_parser(const char *filename, OpCommandList *o);
extern int parse_opcmd();
extern int opcmderror(const char *s);

OpInstance::OpInstance(EventLoop* eventloop, const string& executable,
		       const string& cmd_line, RouterCLI::OpModeCallback cb,
		       const OpCommand* op_cmd)
    : _executable(executable),
      _cmd_line(cmd_line),
      _op_cmd(op_cmd),
      _error(false),
      _last_offset(0),
      _done_callback(cb)
{
    memset(_outbuffer, 0, OP_BUF_SIZE);
    memset(_errbuffer, 0, OP_BUF_SIZE);

    string execute = executable + " " + cmd_line;
    FILE *out_stream, *err_stream;
    popen2(execute, out_stream, err_stream);
    if (out_stream == NULL) {
	cb->dispatch(false, "Failed to execute command\n");
    }

    _stdout_file_reader = new AsyncFileReader(*eventloop, fileno(out_stream));
    _stdout_file_reader->add_buffer((uint8_t*)_outbuffer, OP_BUF_SIZE,
				    callback(this, &OpInstance::append_data));
    if (! _stdout_file_reader->start()) {
	cb->dispatch(false, "Failed to execute command\n");
    }

    _stderr_file_reader = new AsyncFileReader(*eventloop, fileno(err_stream));
    _stderr_file_reader->add_buffer((uint8_t*)_errbuffer, OP_BUF_SIZE,
				    callback(this, &OpInstance::append_data));
    if (! _stderr_file_reader->start()) {
	cb->dispatch(false, "Failed to execute command\n");
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
#if 0
    if (success)
	printf("command succeeded\n");
    else
	printf("command failed\n");
#endif // 0

    _done_callback->dispatch(success, _response);
    _op_cmd->remove_instance(this);
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

OpCommand::OpCommand(const list<string>& parts)
{
    _cmd_parts = parts;
}

int
OpCommand::add_command_file(const string& cmd_file)
{
    if (!_cmd_file.empty()) {
	string err = "Only one %command allowed per CLI command.";
	opcmderror(err.c_str());
	return XORP_ERROR;
    }
    _cmd_file = cmd_file;
    return XORP_OK;
}

int
OpCommand::add_module(const string& module)
{
    if (!_module.empty()) {
	string err = "Only one %module allowed per CLI command.";
	opcmderror(err.c_str());
	return XORP_ERROR;
    }
    _module = module;
    return XORP_OK;
}

int
OpCommand::add_opt_param(const string& op_param)
{
    _opt_params.push_back(op_param);
    return XORP_OK;
}

string
OpCommand::cmd_name() const
{
    string res;

    list<string>::const_iterator iter;
    for (iter = _cmd_parts.begin(); iter != _cmd_parts.end(); ++iter) {
	if (iter != _cmd_parts.begin())
	    res += " ";
	res += *iter;
    }
    return res;
}

string
OpCommand::str() const
{
    string res = cmd_name() + "\n";

    if (_cmd_file.empty()) {
	res += "  No command file specified\n";
    } else {
	res += "  Command: " + _cmd_file + "\n";
    }

    list<string>::const_iterator iter;
    for (iter = _opt_params.begin(); iter != _opt_params.end(); ++iter) {
	res += "  Optional Parameter: " + *iter + "\n";
    }
    return res;
}

void
OpCommand::execute(EventLoop *eventloop, const list<string>& cmd_line,
		   RouterCLI::OpModeCallback cb) const
{
#if 0
    printf("OP_COMMAND EXECUTE: OpCommandNode: %s, CmdLine: ",
	   cmd_name().c_str());
#endif // 0

    string cmd_line_str;

    list<string>::const_iterator iter;
    for (iter = cmd_line.begin(); iter != cmd_line.end(); ++iter) {
	if (iter != cmd_line.begin())
	    cmd_line_str += " ";
	cmd_line_str += *iter;
    }
    _instances.insert(new OpInstance(eventloop, _cmd_file, cmd_line_str,
				     cb, this));
}

bool
OpCommand::operator==(const OpCommand& them) const
{
    return (cmd_name() == them.cmd_name());
}

bool
OpCommand::prefix_matches(const list<string>& pathparts,
			  SlaveConfigTree* conf_tree)
{
    list<string>::const_iterator them, us;

    them = pathparts.begin();
    us = _cmd_parts.begin();
    // First go through the fixed parts of the command
    while (true) {
	if (them == pathparts.end())
	    return true;
	if (us == _cmd_parts.end())
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
    // no more fixed parts, try optional parameters
    while (them != pathparts.end()) {
	list<string>::const_iterator opi;
	bool ok = false;
	for (opi = _opt_params.begin(); opi != _opt_params.end(); ++opi) {
	    if (*opi == *them) {
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

set<string>
OpCommand::get_matches(size_t wordnum, SlaveConfigTree* conf_tree)
{
    set<string> matches;

    list<string>::const_iterator ci = _cmd_parts.begin();
    for (size_t i = 1; i <= wordnum; i++) {
	++ci;
	if (ci == _cmd_parts.end())
	    break;
    }
    if (ci == _cmd_parts.end()) {
	// Add all the optional parameters
	list<string>::const_iterator opi;
	for (opi = _opt_params.begin(); opi != _opt_params.end(); ++opi) {
	    matches.insert(*opi);
	}
	// Add empty string to imply hitting enter is also OK
	matches.insert("");
    } else {
	string match = *ci;
	if (match[0] == '$') {
	    XLOG_ASSERT(match[1] == '(');
	    XLOG_ASSERT(match[match.size() - 1] == ')');
	    list<string> varmatches;
	    conf_tree->expand_varname_to_matchlist(match, varmatches);
	    list<string>::const_iterator vi;
	    for (vi = varmatches.begin(); vi != varmatches.end(); ++vi) {
		matches.insert(*vi);
	    }
	} else {
	    matches.insert(match);
	}
    }
    return matches;
}

void
OpCommand::remove_instance(OpInstance* instance) const
{
    _instances.erase(_instances.find(instance));
}

OpCommandList::OpCommandList(const string &config_template_dir,
			     const TemplateTree *tt)
{
    _template_tree = tt;
    list<string> files;

    struct stat dirdata;
    if (stat(config_template_dir.c_str(), &dirdata) < 0) {
	string errstr = "rtrmgr: error reading config directory "
	    + config_template_dir + "\n";
	errstr += strerror(errno);
	errstr += "\n";
	fprintf(stderr, "%s", errstr.c_str());
	exit(1);
    }

    if ((dirdata.st_mode & S_IFDIR) == 0) {
	string errstr = "rtrmgr: error reading config directory "
	    + config_template_dir + "\n" + config_template_dir
	    + " is not a directory\n";
	fprintf(stderr, "%s", errstr.c_str());
	exit(1);
    }

    // TODO: file suffix is hardcoded here!
    string globname = config_template_dir + "/*.cmds";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	fprintf(stderr, "rtrmgr failed to find config files in %s\n",
		config_template_dir.c_str());
	globfree(&pglob);
	exit(1);
    }

    if (pglob.gl_pathc == 0) {
	fprintf(stderr, "rtrmgr failed to find any template files in %s\n",
		config_template_dir.c_str());
	globfree(&pglob);
	exit(1);
    }

    for (size_t i = 0; i < (size_t)pglob.gl_pathc; i++) {
	if (init_opcmd_parser(pglob.gl_pathv[i], this) < 0) {
	    fprintf(stderr, "Failed to open template file: %s\n",
		    config_template_dir.c_str());
	    globfree(&pglob);
	    exit(-1);
	}
	try {
	    parse_opcmd();
	} catch (ParseError &pe) {
	    opcmderror(pe.why().c_str());
	    globfree(&pglob);
	    exit(1);
	}
	if (_path_segments.size() != 0) {
	    fprintf(stderr, "Error: file %s is not terminated properly\n",
		    pglob.gl_pathv[i]);
	    globfree(&pglob);
	    exit(1);
	}
    }

    globfree(&pglob);
}

OpCommandList::~OpCommandList()
{
    while (!_op_cmds.empty()) {
	delete _op_cmds.front();
	_op_cmds.pop_front();
    }
}

int
OpCommandList::check_variable_name(const string& name) const
{
    if (_template_tree->check_variable_name(name) == false) {
	string err = "Bad template variable name: \"" + name + "\"\n";
	opcmderror(err.c_str());
	exit(1);
    }
    return XORP_OK;
}

OpCommand*
OpCommandList::find(const list<string>& parts)
{
    OpCommand dummy(parts);

    list<OpCommand*>::const_iterator iter;
    for (iter = _op_cmds.begin(); iter != _op_cmds.end(); ++iter) {
	if (**iter == dummy)
	    return *iter;
    }
    return NULL;
}

bool
OpCommandList::prefix_matches(const list<string>& parts) const
{
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_cmds.begin(); iter != _op_cmds.end(); ++iter) {
	if ((*iter)->prefix_matches(parts, _conf_tree))
	    return true;
    }
    return false;
}

void
OpCommandList::execute(EventLoop *eventloop, const list<string>& parts,
		       RouterCLI::OpModeCallback cb) const
{
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_cmds.begin(); iter != _op_cmds.end(); ++iter) {
	// Find the right command
	if ((*iter)->prefix_matches(parts, _conf_tree)) {
	    // Execute it
	    (*iter)->execute(eventloop, parts, cb);
	    // XXX: don't worry about errors - the callback reports them.
	    return;
	}
    }
    cb->dispatch(false, string("No matching command"));
}

OpCommand*
OpCommandList::new_op_command(const list<string>& parts)
{
    OpCommand *new_cmd, *existing_cmd;

    existing_cmd = find(parts);
    if (existing_cmd != NULL) {
	string err = "Duplicate command: \"";
	err += existing_cmd->cmd_name() + "\"";
	opcmderror(err.c_str());
	exit(1);
    }

    new_cmd = new OpCommand(parts);
    _op_cmds.push_back(new_cmd);
    return new_cmd;
}

void
OpCommandList::display_list() const
{
    printf("\nOperational Command List:\n");

    list<OpCommand*>::const_iterator iter;
    for (iter = _op_cmds.begin(); iter != _op_cmds.end(); ++iter) {
	printf("  %s\n", (*iter)->str().c_str());
    }
    printf("\n");
}

int
OpCommandList::append_path(const string& path)
{
    if (path[0] == '$')
	check_variable_name(path);
    _path_segments.push_back(path);
    return XORP_OK;
}

int
OpCommandList::push_path()
{
    _current_cmd = new_op_command(_path_segments);
    return XORP_OK;
}

int
OpCommandList::pop_path()
{
    // In the OpCommand file, we don't currently allow nesting, so just
    // clear the path.
    while (!_path_segments.empty()) {
	_path_segments.pop_front();
    }
    return XORP_OK;
}

int
OpCommandList::add_cmd(const string& cmd)
{
    if (cmd != "%command" && cmd != "%opt_parameter" && cmd != "%module") {
	string err = "Unknown command \"" + cmd + "\"";
	opcmderror(err.c_str());
	exit(1);
    }
    return XORP_OK;
}

int
OpCommandList::add_cmd_action(const string& cmd, const list<string>& parts)
{
    // May change this restriction later
    XLOG_ASSERT(parts.size() == 1);

    if (cmd == "%command") {
	string executable;
	if (find_executable(parts.front(), executable)) {
	    return _current_cmd->add_command_file(executable);
	} else {
	    string err = "File not found: " + parts.front();
	    opcmderror(err.c_str());
	    exit(1);
	}
    }
    if (cmd == "%module") {
	return _current_cmd->add_module(parts.front());
    }
    if (cmd == "%opt_parameter") {
	return _current_cmd->add_opt_param(parts.front());
    }
    string err = "Unknown command \"" + cmd + "\"";
    opcmderror(err.c_str());
    exit(1);
}

bool
OpCommandList::find_executable(const string& filename, string& executable)
    const
{
    struct stat statbuf;

    printf("find_executable: %s\n", filename.c_str());

    if (filename[0] == '/') {
	if ((stat(filename.c_str(), &statbuf) == 0)
	    && (statbuf.st_mode & S_IFREG > 0)
	    && (statbuf.st_mode & S_IXUSR > 0)) {
	    executable = filename;
	    printf("found executable: %s\n", executable.c_str());
	    return true;
	}
	return false;
    } else {
#if 0	// TODO: XXX: old code
	char cwd[MAXPATHLEN + 1];
	getcwd(cwd, MAXPATHLEN);
#endif
	string xorp_root_dir = _template_tree->xorp_root_dir();

	list<string> path;
#if 0	// TODO: XXX: old code
	path.push_back(string(cwd) + "/");
	path.push_back(string(cwd) + "/../");
#endif
	path.push_back(xorp_root_dir + "/");
	while (!path.empty()) {
	    if ((stat((path.front() + filename).c_str(), &statbuf) == 0)
		&& (statbuf.st_mode & S_IFREG > 0)
		&& (statbuf.st_mode & S_IXUSR > 0)) {
		executable = path.front() + filename;
		printf("found executable: %s\n", executable.c_str());
		return true;
	    }
	    path.pop_front();
	}
	return false;
    }
}

set<string>
OpCommandList::top_level_commands() const
{
    set<string> cmds;

    list<OpCommand*>::const_iterator iter;
    for (iter = _op_cmds.begin(); iter != _op_cmds.end(); ++iter) {
	// Add the first word of every command
	cmds.insert(split((*iter)->cmd_name(), ' ').front());
    }
    return cmds;
}

set<string>
OpCommandList::childlist(const string& path, bool &make_executable) const
{
    set<string> children;
    list<string> pathparts = split(path, ' ');

    make_executable = false;
    list<OpCommand*>::const_iterator iter;
    for (iter = _op_cmds.begin(); iter != _op_cmds.end(); ++iter) {
	if ((*iter)->prefix_matches(pathparts, _conf_tree)) {
	    set<string> matches = (*iter)->get_matches(pathparts.size(),
						       _conf_tree);
	    set<string>::iterator mi;
	    for (mi = matches.begin(); mi != matches.end(); ++mi) {
		if (*mi == "") {
		    make_executable = true;
		} else {
		    children.insert(*mi);
		}
	    }
	}
    }
    return children;
}
