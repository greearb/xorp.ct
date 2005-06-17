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

#ident "$XORP: xorp/rtrmgr/module_command.cc,v 1.31 2005/03/25 02:54:36 pavlin Exp $"


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
#include "xrldb.hh"


static string
strip_quotes(const string& command, const string& value) throw (ParseError)
{
    size_t old_size = value.size();
    string tmp_value = unquote(value);

    if (tmp_value.size() != old_size && tmp_value.size() != old_size - 2) {
	string errmsg = c_format("subcommand %s has invalid argument: %s",
				 command.c_str(), value.c_str());
	xorp_throw(ParseError, errmsg);
    }

    if (unquote(tmp_value).empty()) {
	string errmsg = c_format("subcommand %s has empty argument",
				 command.c_str());
	xorp_throw(ParseError, errmsg);
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
ModuleCommand::add_action(const list<string>& action, const XRLdb& xrldb)
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

    if (err != XrlError::OKAY()) {
	//
	// There was an error.  There's nothing we can so here - errors
	// are handled in the TaskManager.
	//
	return;
    }

    if (xrl_args->empty())
	return;

    //
    // Handle the XRL arguments
    //
    debug_msg("ARGS: %s\n", xrl_args->str().c_str());

    // Create a list with the return arguments
    list<string> spec_args;
    XrlAction* xa = dynamic_cast<XrlAction*>(action);
    XLOG_ASSERT(xa != NULL);
    string s = xa->xrl_return_spec();
    while (true) {
	string::size_type start = s.find("&");
	if (start == string::npos) {
	    spec_args.push_back(s);
	    break;
	}
	spec_args.push_back(s.substr(0, start));
	debug_msg("spec_args: %s\n", s.substr(0, start).c_str());
	s = s.substr(start + 1, s.size() - (start + 1));
    }

    list<string>::const_iterator iter;
    for (iter = spec_args.begin(); iter != spec_args.end(); ++iter) {
	string::size_type eq = iter->find("=");
	if (eq == string::npos)
	    continue;

	XrlAtom atom(iter->substr(0, eq).c_str());
	debug_msg("atom name=%s\n", atom.name().c_str());
	string varname = iter->substr(eq + 1, iter->size() - (eq + 1));
	debug_msg("varname=%s\n", varname.c_str());
	XrlAtom returned_atom;
	try {
	    returned_atom = xrl_args->item(atom.name());
	} catch (const XrlArgs::XrlAtomNotFound& x) {
	    // TODO: XXX: IMPLEMENT IT!!
	    XLOG_UNFINISHED();
	}
	string value = returned_atom.value();
	debug_msg("found atom = %s\n", returned_atom.str().c_str());
	debug_msg("found value = %s\n", value.c_str());
	ctn->set_variable(varname, value);
    }

    UNUSED(cmd);
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
