// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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




#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "module_command.hh"
#include "rtrmgr_error.hh"
#include "task.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "master_conf_tree_node.hh"
#include "util.hh"

#ifdef DEBUG_XRLDB
#include "xrldb.hh"
#endif

static string
strip_quotes(const string& command, const string& value) throw (ParseError)
{
    string error_msg;
    size_t old_size = value.size();
    string tmp_value = unquote(value);

    if (tmp_value.size() != old_size && tmp_value.size() != old_size - 2) {
	error_msg = c_format("subcommand %s has invalid argument: %s",
			     command.c_str(), value.c_str());
	xorp_throw(ParseError, error_msg);
    }

    if (unquote(tmp_value).empty()) {
	error_msg = c_format("subcommand %s has empty argument",
			     command.c_str());
	xorp_throw(ParseError, error_msg);
    }

    return tmp_value;
}

ModuleCommand::ModuleCommand(TemplateTree& template_tree,
			     TemplateTreeNode& template_tree_node,
			     const string& cmd_name)
    : Command(template_tree_node, cmd_name),
      _tt(template_tree),
      _start_commit(NULL),
      _end_commit(NULL),
      _status_method(NULL),
      _startup_method(NULL),
      _shutdown_method(NULL),
      _execute_done(false),
      _verbose(template_tree.verbose())
{
    XLOG_ASSERT(cmd_name == "%modinfo");
}

ModuleCommand::~ModuleCommand()
{
    if (_start_commit != NULL)
	delete _start_commit;
    if (_end_commit != NULL)
	delete _end_commit;
    if (_status_method != NULL)
	delete _status_method;
    if (_startup_method != NULL)
	delete _startup_method;
    if (_shutdown_method != NULL)
	delete _shutdown_method;
}

void
ModuleCommand::add_action(const list<string>& action, const XRLdb* xrldb)
    throw (ParseError)
{
    size_t expected_action_size = 2;

    //
    // Check the subcommand size
    //
    if (action.size() < expected_action_size) {
	xorp_throw(ParseError, "too few parameters to %modinfo");
    }
    string subcommand = action.front();
    if ((subcommand == "start_commit")
	|| (subcommand == "end_commit")
	|| (subcommand == "status_method")
	|| (subcommand == "startup_method")
	|| (subcommand == "shutdown_method")) {
	expected_action_size = 3;
    }
    if (action.size() > expected_action_size) {
	xorp_throw(ParseError, "too many parameters to %modinfo");
    }
    if (action.size() < expected_action_size) {
	xorp_throw(ParseError, "too few parameters to %modinfo");
    }

    typedef list<string>::const_iterator CI;
    CI ptr = action.begin();
    string cmd = *ptr;
    ++ptr;
    string value = *ptr;
    if (cmd == "provides") {
	_module_name = strip_quotes(cmd, value);
	_tt.register_module(_module_name, this);
	template_tree_node().set_subtree_module_name(_module_name);
    } else if (cmd == "depends") {
	if (_module_name.empty()) {
	    xorp_throw(ParseError,
		       "\"depends\" must be preceded by \"provides\"");
	}
	_depends.push_back(strip_quotes(cmd, value));
    } else if (cmd == "path") {
	if (_module_name == "") {
	    xorp_throw(ParseError,
		       "\"path\" must be preceded by \"provides\"");
	}
	if (_module_exec_path != "") {
	    xorp_throw(ParseError, "duplicate \"path\" subcommand");
	}
	_module_exec_path = strip_quotes(cmd, value);
    } else if (cmd == "default_targetname") {
	if (_module_name == "") {
	    xorp_throw(ParseError,
		       "\"default_targetname\" must be preceded by \"provides\"");
	}
	_default_target_name = strip_quotes(cmd, value);
	template_tree_node().set_subtree_default_target_name(_default_target_name);
    } else if (cmd == "start_commit") {
	debug_msg("start_commit:\n");
	list<string>::const_iterator iter;
	for (iter = action.begin(); iter != action.end(); ++iter)
	    debug_msg(">%s< ", (*iter).c_str());
	debug_msg("\n");

	list<string> newaction = action;
	newaction.pop_front();
	do {
	    if (newaction.front() == "xrl") {
		_start_commit = new XrlAction(template_tree_node(), newaction,
					      xrldb);
		break;
	    }
	    if (newaction.front() == "program") {
		_start_commit = new ProgramAction(template_tree_node(),
						  newaction);
		break;
	    }
	    _start_commit = new Action(template_tree_node(), newaction);
	    break;
	} while (false);
    } else if (cmd == "end_commit") {
	list<string> newaction = action;
	newaction.pop_front();
	do {
	    if (newaction.front() == "xrl") {
		_end_commit = new XrlAction(template_tree_node(), newaction,
					    xrldb);
		break;
	    }
	    if (newaction.front() == "program") {
		_end_commit = new ProgramAction(template_tree_node(),
						newaction);
		break;
	    }
	    _end_commit = new Action(template_tree_node(), newaction);
	    break;
	} while (false);
    } else if (cmd == "status_method") {
	list<string> newaction = action;
	newaction.pop_front();
	do {
	    if (newaction.front() == "xrl") {
		_status_method = new XrlAction(template_tree_node(), newaction,
					       xrldb);
		break;
	    }
	    if (newaction.front() == "program") {
		_status_method = new ProgramAction(template_tree_node(),
						   newaction);
		break;
	    }
	    _status_method = new Action(template_tree_node(), newaction);
	    break;
	} while (false);
    } else if (cmd == "startup_method") {
	list<string> newaction = action;
	newaction.pop_front();
	do {
	    if (newaction.front() == "xrl") {
		_startup_method = new XrlAction(template_tree_node(),
						newaction,
						xrldb);
		break;
	    }
	    if (newaction.front() == "program") {
		_startup_method = new ProgramAction(template_tree_node(),
						    newaction);
		break;
	    }
	    _startup_method = new Action(template_tree_node(), newaction);
	    break;
	} while (false);
    } else if (cmd == "shutdown_method") {
	list<string> newaction = action;
	newaction.pop_front();
	do {
	    if (newaction.front() == "xrl") {
		_shutdown_method = new XrlAction(template_tree_node(),
						 newaction,
						 xrldb);
		break;
	    }
	    if (newaction.front() == "program") {
		_shutdown_method = new ProgramAction(template_tree_node(),
						     newaction);
		break;
	    }
	    _shutdown_method = new Action(template_tree_node(), newaction);
	    break;
	} while (false);
    } else {
	string err = "invalid subcommand \"" + cmd + "\" to %modinfo";
	xorp_throw(ParseError, err);
    }
}

bool
ModuleCommand::expand_actions(string& error_msg)
{
    //
    // Expand all module-specific methods
    //
    if (_start_commit != NULL) {
	if (_start_commit->expand_action(error_msg) != true)
	    return (false);
    }
    if (_end_commit != NULL) {
	if (_end_commit->expand_action(error_msg) != true)
	    return (false);
    }
    if (_status_method != NULL) {
	if (_status_method->expand_action(error_msg) != true)
	    return (false);
    }
    if (_startup_method != NULL) {
	if (_startup_method->expand_action(error_msg) != true)
	    return (false);
    }
    if (_shutdown_method != NULL) {
	if (_shutdown_method->expand_action(error_msg) != true)
	    return (false);
    }

    return (true);
}

bool
ModuleCommand::check_referred_variables(string& error_msg) const
{
    //
    // Check all module-specific methods
    //
    if (_start_commit != NULL) {
	if (_start_commit->check_referred_variables(error_msg) != true)
	    return (false);
    }
    if (_end_commit != NULL) {
	if (_end_commit->check_referred_variables(error_msg) != true)
	    return (false);
    }
    if (_status_method != NULL) {
	if (_status_method->check_referred_variables(error_msg) != true)
	    return (false);
    }
    if (_startup_method != NULL) {
	if (_startup_method->check_referred_variables(error_msg) != true)
	    return (false);
    }
    if (_shutdown_method != NULL) {
	if (_shutdown_method->check_referred_variables(error_msg) != true)
	    return (false);
    }

    return (true);
}

Validation*
ModuleCommand::startup_validation(TaskManager& taskmgr) const
{
    if ((_status_method != NULL) && (_startup_method != NULL)) {
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL) {
	    return new XrlStatusStartupValidation(_module_name, *xa, taskmgr);
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(_status_method);
	if (pa != NULL) {
	    return new ProgramStatusStartupValidation(_module_name, *pa,
						      taskmgr);
	}
	return NULL;
    } else {
	XLOG_WARNING("WARNING:  Using DelayValidation, module_name: %s", _module_name.c_str());
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000,
				   _verbose);
    }
}

Validation*
ModuleCommand::config_validation(TaskManager& taskmgr) const
{
    if (_status_method != NULL) {
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL) {
	    return new XrlStatusConfigMeValidation(_module_name, *xa, taskmgr);
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(_status_method);
	if (pa != NULL) {
	    return new ProgramStatusConfigMeValidation(_module_name, *pa,
						       taskmgr);
	}
	return NULL;
    } else {
	XLOG_WARNING("WARNING:  Using DelayValidation, module_name: %s", _module_name.c_str());
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000,
				   _verbose);
    }
}

Validation*
ModuleCommand::ready_validation(TaskManager& taskmgr) const
{
    if (_status_method != NULL) {
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL) {
	    return new XrlStatusReadyValidation(_module_name, *xa, taskmgr);
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(_status_method);
	if (pa != NULL) {
	    return new ProgramStatusReadyValidation(_module_name, *pa,
						    taskmgr);
	}
	return NULL;
    } else {
	XLOG_WARNING("WARNING:  Using DelayValidation, module_name: %s", _module_name.c_str());
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000,
				   _verbose);
    }
}

Validation*
ModuleCommand::shutdown_validation(TaskManager& taskmgr) const
{
    if (_status_method != NULL) {
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL) {
	    return new XrlStatusShutdownValidation(_module_name, *xa, taskmgr);
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(_status_method);
	if (pa != NULL) {
	    return new ProgramStatusShutdownValidation(_module_name, *pa,
						       taskmgr);
	}
	return NULL;
    } else {
	XLOG_WARNING("WARNING:  Using DelayValidation, module_name: %s", _module_name.c_str());
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000,
				   _verbose);
    }
}

Startup*
ModuleCommand::startup_method(TaskManager& taskmgr) const
{
    if (_startup_method != NULL) {
	XrlAction* xa = dynamic_cast<XrlAction*>(_startup_method);
	if (xa != NULL) {
	    return new XrlStartup(_module_name, *xa, taskmgr);
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(_startup_method);
	if (pa != NULL) {
	    return new ProgramStartup(_module_name, *pa, taskmgr);
	}
	return NULL;
    } else {
	// The startup method is optional
	return NULL;
    }
}

Shutdown*
ModuleCommand::shutdown_method(TaskManager& taskmgr) const
{
    if (_shutdown_method != NULL) {
	XrlAction* xa = dynamic_cast<XrlAction*>(_shutdown_method);
	if (xa != NULL) {
	    return new XrlShutdown(_module_name, *xa, taskmgr);
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(_shutdown_method);
	if (pa != NULL) {
	    return new ProgramShutdown(_module_name, *pa, taskmgr);
	}
	return NULL;
    } else {
	// We can always kill it from the module manager.
	return NULL;
    }
}

int
ModuleCommand::start_transaction(MasterConfigTreeNode& ctn,
				 TaskManager& task_manager) const
{
    if (_start_commit == NULL)
	return XORP_OK;

    XrlAction *xa = dynamic_cast<XrlAction*>(_start_commit);
    if (xa != NULL) {
	XrlRouter::XrlCallback cb = callback(
	    this,
	    &ModuleCommand::xrl_action_complete,
	    &ctn,
	    _start_commit,
	    string("start transaction"));
	return xa->execute(ctn, task_manager, cb);
    }

    ProgramAction *pa = dynamic_cast<ProgramAction*>(_start_commit);
    if (pa != NULL) {
	TaskProgramItem::ProgramCallback cb = callback(
	    this,
	    &ModuleCommand::program_action_complete,
	    &ctn,
	    _start_commit,
	    string("start transaction"));
	return pa->execute(ctn, task_manager, cb);
    }

    XLOG_UNREACHABLE();
}

int
ModuleCommand::end_transaction(MasterConfigTreeNode& ctn,
			       TaskManager& task_manager) const
{
    if (_end_commit == NULL)
	return XORP_OK;

    XrlAction *xa = dynamic_cast<XrlAction*>(_end_commit);
    if (xa != NULL) {
	XrlRouter::XrlCallback cb = callback(
	    this,
	    &ModuleCommand::xrl_action_complete,
	    &ctn,
	    _end_commit,
	    string("end transaction"));
	return xa->execute(ctn, task_manager, cb);
    }

    ProgramAction *pa = dynamic_cast<ProgramAction*>(_end_commit);
    if (pa != NULL) {
	TaskProgramItem::ProgramCallback cb = callback(
	    this,
	    &ModuleCommand::program_action_complete,
	    &ctn,
	    _end_commit,
	    string("end transaction"));
	return pa->execute(ctn, task_manager, cb);
    }

    XLOG_UNREACHABLE();
}

string
ModuleCommand::str() const
{
    string tmp;

    tmp  = "ModuleCommand: provides: " + _module_name + "\n";
    tmp += "               path: " + _module_exec_path + "\n";
    typedef list<string>::const_iterator CI;
    CI ptr = _depends.begin();
    while (ptr != _depends.end()) {
	tmp += "               depends: " + *ptr + "\n";
	++ptr;
    }
    return tmp;
}

void
ModuleCommand::xrl_action_complete(const XrlError& err,
				   XrlArgs* xrl_args,
				   MasterConfigTreeNode *ctn,
				   Action* action,
				   string cmd) const
{
    debug_msg("ModuleCommand::xrl_action_complete\n");

    UNUSED(cmd);

    if (err != XrlError::OKAY()) {
	//
	// There was an error.  There's nothing we can so here - errors
	// are handled in the TaskManager.
	//
	return;
    }

    process_xrl_action_return_arguments(xrl_args, ctn, action);
}

void
ModuleCommand::program_action_complete(bool success,
				       const string& stdout_output,
				       const string& stderr_output,
				       bool do_exec,
				       MasterConfigTreeNode *ctn,
				       Action *action,
				       string cmd) const
{
    debug_msg("ModuleCommand::program_action_complete\n");

    if (! success) {
	//
	// There was an error.  There's nothing we can so here - errors
	// are handled in the TaskManager.
	//
	return;
    }

    if (do_exec) {
	// Obtain the names of the variables to set
	ProgramAction* pa = dynamic_cast<ProgramAction*>(action);
	XLOG_ASSERT(pa != NULL);
	string stdout_variable_name = pa->stdout_variable_name();
	string stderr_variable_name = pa->stderr_variable_name();

	// Set the values
	if (! stdout_variable_name.empty())
	    ctn->set_variable(stdout_variable_name, stdout_output);
	if (! stderr_variable_name.empty())
	    ctn->set_variable(stderr_variable_name, stderr_output);
    }

    UNUSED(cmd);
}
