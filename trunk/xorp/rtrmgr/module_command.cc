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

#ident "$XORP: xorp/rtrmgr/module_command.cc,v 1.18 2003/11/18 23:03:57 pavlin Exp $"

// #define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "libxorp/xlog.h"

#include "libxipc/xrl_router.hh"

#include "module_command.hh"
#include "xrldb.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "task.hh"
#include "parse_error.hh"

ModuleCommand::ModuleCommand(TemplateTree& template_tree,
			     TemplateTreeNode& template_tree_node,
			     const string& cmd_name)
    : Command(template_tree_node, cmd_name),
      _tt(template_tree),
      _startcommit(NULL),
      _endcommit(NULL),
      _status_method(NULL),
      _shutdown_method(NULL),
      _execute_done(false)
{
    XLOG_ASSERT(cmd_name == "%modinfo");
}

ModuleCommand::~ModuleCommand()
{
    if (_startcommit != NULL)
	delete _startcommit;
    if (_endcommit != NULL)
	delete _endcommit;
    if (_status_method != NULL)
	delete _status_method;
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
    if ((subcommand == "startcommit")
	|| (subcommand == "endcommit")
	|| (subcommand == "statusmethod")
	|| (subcommand == "shutdownmethod")) {
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
	_module_name = value;
	_tt.register_module(_module_name, this);
	template_tree_node().set_module_name(_module_name);
    } else if (cmd == "depends") {
	if (_module_name.empty()) {
	    xorp_throw(ParseError, "\"depends\" must be preceded by \"provides\"");
	}
	_depends.push_back(value);
    } else if (cmd == "path") {
	if (_module_name == "") {
	    xorp_throw(ParseError, "\"path\" must be preceded by \"provides\"");
	}
	if (_module_exec_path != "") {
	    xorp_throw(ParseError, "duplicate \"path\" subcommand");
	}
	if (value[0]=='"')
	    _module_exec_path = value.substr(1, value.length() - 2);
	else
	    _module_exec_path = value;
    } else if (cmd == "default_targetname") {
	if (_module_name == "") {
	    xorp_throw(ParseError, "\"default_targetname\" must be preceded by \"provides\"");
	}
	if (_default_target_name != "") {
	    xorp_throw(ParseError, "duplicate \"default_targetname\" subcommand");
	}
	if (value[0] == '"')
	    _default_target_name = value.substr(1, value.length() - 2);
	else
	    _default_target_name = value;
	if (_default_target_name.empty()) {
	    xorp_throw(ParseError, "Empty \"default_targetname\" subcommand");
	}
	template_tree_node().set_default_target_name(_default_target_name);
    } else if (cmd == "startcommit") {
	debug_msg("startcommit:\n");
	list <string>::const_iterator i;
	for (i=action.begin(); i!=action.end(); i++)
	    debug_msg(">%s< ", (*i).c_str());
	debug_msg("\n");
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front()=="xrl")
	    _startcommit = new XrlAction(template_tree_node(), newaction,
					 xrldb);
	else
	    _startcommit = new Action(template_tree_node(), newaction);
    } else if (cmd == "endcommit") {
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front()=="xrl")
	    _endcommit = new XrlAction(template_tree_node(), newaction, xrldb);
	else
	    _endcommit = new Action(template_tree_node(), newaction);
    } else if (cmd == "statusmethod") {
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front()=="xrl")
	    _status_method = new XrlAction(template_tree_node(), newaction,
					   xrldb);
	else
	    _status_method = new Action(template_tree_node(), newaction);
    } else if (cmd == "shutdownmethod") {
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front()=="xrl")
	    _shutdown_method = new XrlAction(template_tree_node(), newaction,
					     xrldb);
	else
	    _shutdown_method = new Action(template_tree_node(), newaction);
    } else {
	string err = "invalid subcommand \"" + cmd + "\" to %modinfo";
	xorp_throw(ParseError, err);
    }
}

#ifdef NOTDEF
int
ModuleCommand::execute(TaskManager& taskmgr) const
{
    return taskmgr.add_module(*this);
}
#endif

Validation*
ModuleCommand::startup_validation(TaskManager &taskmgr) const
{
    if (_status_method != NULL) {
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusConfigMeValidation(_module_name, *xa, taskmgr);
	else
	    return NULL;
    } else {
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000);
    }
}

Validation*
ModuleCommand::ready_validation(TaskManager &taskmgr) const
{
    if (_status_method != NULL) {
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusReadyValidation(_module_name, *xa, taskmgr);
	else
	    return NULL;
    } else {
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000);
    }
}

Validation*
ModuleCommand::shutdown_validation(TaskManager &taskmgr) const
{
    if (_status_method != NULL) {
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_status_method);
	if (xa != NULL)
	    return new StatusShutdownValidation(_module_name, *xa, taskmgr);
	else
	    return NULL;
    } else {
	return new DelayValidation(_module_name, taskmgr.eventloop(), 2000);
    }
}

Shutdown*
ModuleCommand::shutdown_method(TaskManager &taskmgr) const
{
    if (_shutdown_method != NULL) {
	// TODO: for now we can handle only XRL actions
	XrlAction* xa = dynamic_cast<XrlAction*>(_shutdown_method);
	if (xa != NULL)
	    return new XrlShutdown(_module_name, *xa, taskmgr);
	else
	    return NULL;
    } else {
	//we can always kill it from the module manager.
	return NULL;
    }
}

int
ModuleCommand::start_transaction(ConfigTreeNode& ctn,
				 TaskManager& task_manager) const
{
    if (_startcommit == NULL)
	return XORP_OK;
    XrlRouter::XrlCallback cb
	= callback(const_cast<ModuleCommand*>(this),
		   &ModuleCommand::action_complete,
		   &ctn, _startcommit,
		   string("start transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_startcommit);
    assert(xa != NULL);

    return xa->execute(ctn, task_manager, cb);
}

int
ModuleCommand::end_transaction(ConfigTreeNode& ctn,
			       TaskManager& task_manager) const
{
    if (_endcommit == NULL)
	return XORP_OK;
    XrlRouter::XrlCallback cb
	= callback(const_cast<ModuleCommand*>(this),
		   &ModuleCommand::action_complete,
		   &ctn, _endcommit,
		   string("end transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_endcommit);
    assert(xa != NULL);

    return xa->execute(ctn, task_manager, cb);
}

string
ModuleCommand::str() const
{
    string tmp;
    tmp= "ModuleCommand: provides: " + _module_name + "\n";
    tmp+="               path: " + _module_exec_path + "\n";
    typedef list<string>::const_iterator CI;
    CI ptr = _depends.begin();
    while (ptr != _depends.end()) {
	tmp += "               depends: " + *ptr + "\n";
	++ptr;
    }
    return tmp;
}

#ifdef NOTDEF
bool
ModuleCommand::execute_completed() const
{
    return _execute_done;
}

void
ModuleCommand::exec_complete(const XrlError& /*err*/,
			     XrlArgs*)
{
    debug_msg("ModuleCommand::exec_complete\n");
    _execute_done = true;
}
#endif

void
ModuleCommand::action_complete(const XrlError& err,
			       XrlArgs* args,
			       ConfigTreeNode *ctn,
			       Action* action,
			       string cmd)
{
    debug_msg("ModuleCommand::action_complete\n");
    if (err == XrlError::OKAY()) {
	//XXX does this make sense?
	if (!args->empty()) {
	    debug_msg("ARGS: %s\n", args->str().c_str());
	    list <string> specargs;
	    XrlAction* xa = dynamic_cast<XrlAction*>(action);
	    assert(xa != NULL);
	    string s = xa->xrl_return_spec();
	    while (1) {
		string::size_type start = s.find("&");
		if (start == string::npos) {
		    specargs.push_back(s);
		    break;
		}
		specargs.push_back(s.substr(0, start));
		debug_msg("specargs: %s\n", s.substr(0, start).c_str());
		s = s.substr(start+1, s.size()-(start+1));
	    }
	    list <string>::const_iterator i;
	    for(i = specargs.begin(); i!= specargs.end(); i++) {
		string::size_type eq = i->find("=");
		if (eq == string::npos) {
		    continue;
		} else {
		    XrlAtom atom(i->substr(0, eq).c_str());
		    debug_msg("atom name=%s\n", atom.name().c_str());
		    string varname = i->substr(eq+1, i->size()-(eq+1));
		    debug_msg("varname=%s\n", varname.c_str());
		    XrlAtom returned_atom;
		    try {
			returned_atom = args->item(atom.name());
		    } catch (XrlArgs::XrlAtomNotFound& x) {
			//XXX
			XLOG_UNFINISHED();
		    }
		    string value = returned_atom.value();
		    debug_msg("found atom = %s\n", returned_atom.str().c_str());
		    debug_msg("found value = %s\n", value.c_str());
		    ctn->set_variable(varname, value);
		}
	    }
	}
	return;
    } else {
	UNUSED(cmd);
	//There was an error.  There's nothing we can so here - errors
	//are handled in the TaskManager.
	return;
    }
}
