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

#ident "$XORP: xorp/rtrmgr/module_command.cc,v 1.28 2004/06/09 03:14:13 hodson Exp $"


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
	template_tree_node().set_module_name(_module_name);
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
	template_tree_node().set_default_target_name(_default_target_name);
    } else if (cmd == "start_commit") {
	debug_msg("start_commit:\n");
	list<string>::const_iterator iter;
	for (iter = action.begin(); iter != action.end(); ++iter)
	    debug_msg(">%s< ", (*iter).c_str());
	debug_msg("\n");

	list<string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _start_commit = new XrlAction(template_tree_node(), newaction,
					  xrldb);
	} else {
	    _start_commit = new Action(template_tree_node(), newaction);
	}
    } else if (cmd == "end_commit") {
	list<string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _end_commit = new XrlAction(template_tree_node(), newaction,
					xrldb);
	} else {
	    _end_commit = new Action(template_tree_node(), newaction);
	}
    } else if (cmd == "status_method") {
	list<string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _status_method = new XrlAction(template_tree_node(), newaction,
					   xrldb);
	} else {
	    _status_method = new Action(template_tree_node(), newaction);
	}
    } else if (cmd == "startup_method") {
	list<string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _startup_method = new XrlAction(template_tree_node(), newaction,
					    xrldb);
	} else {
	    _startup_method = new Action(template_tree_node(), newaction);
	}
    } else if (cmd == "shutdown_method") {
	list<string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _shutdown_method = new XrlAction(template_tree_node(), newaction,
					     xrldb);
	} else {
	    _shutdown_method = new Action(template_tree_node(), newaction);
	}
    } else {
	string err = "invalid subcommand \"" + cmd + "\" to %modinfo";
	xorp_throw(ParseError, err);
    }
}

#if 0
int
ModuleCommand::execute(TaskManager& taskmgr) const
{
    return taskmgr.add_module(*this);
}
#endif // 0

Validation*
ModuleCommand::startup_validation(TaskManager& taskmgr) const
{
    if ((_status_method != NULL) && (_startup_method != NULL)) {
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusStartupValidation(_module_name, *xa, taskmgr);
	else
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
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusConfigMeValidation(_module_name, *xa, taskmgr);
	else
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
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusReadyValidation(_module_name, *xa, taskmgr);
	else
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
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusShutdownValidation(_module_name, *xa, taskmgr);
	else
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
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_startup_method);
	if (xa != NULL)
	    return new XrlStartup(_module_name, *xa, taskmgr);
	else
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
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_shutdown_method);
	if (xa != NULL)
	    return new XrlShutdown(_module_name, *xa, taskmgr);
	else
	    return NULL;
    } else {
	// We can always kill it from the module manager.
	return NULL;
    }
}

int
ModuleCommand::start_transaction(ConfigTreeNode& ctn,
				 TaskManager& task_manager) const
{
    if (_start_commit == NULL)
	return XORP_OK;

    XrlRouter::XrlCallback cb = callback(this,
					 &ModuleCommand::action_complete,
					 &ctn, _start_commit,
					 string("start transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_start_commit);
    XLOG_ASSERT(xa != NULL);

    return xa->execute(ctn, task_manager, cb);
}

int
ModuleCommand::end_transaction(ConfigTreeNode& ctn,
			       TaskManager& task_manager) const
{
    if (_end_commit == NULL)
	return XORP_OK;

    XrlRouter::XrlCallback cb = callback(this,
					 &ModuleCommand::action_complete,
					 &ctn, _end_commit,
					 string("end transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_end_commit);
    XLOG_ASSERT(xa != NULL);

    return xa->execute(ctn, task_manager, cb);
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

#if 0
bool
ModuleCommand::execute_completed() const
{
    return _execute_done;
}

void
ModuleCommand::exec_complete(const XrlError& /* err */,
			     XrlArgs*)
{
    debug_msg("ModuleCommand::exec_complete\n");
    _execute_done = true;
}
#endif // 0

void
ModuleCommand::action_complete(const XrlError& err,
			       XrlArgs* xrl_args,
			       ConfigTreeNode *ctn,
			       Action* action,
			       string cmd) const
{
    debug_msg("ModuleCommand::action_complete\n");

    if (err != XrlError::OKAY()) {
	UNUSED(cmd);
	// There was an error.  There's nothing we can so here - errors
	// are handled in the TaskManager.
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
}
