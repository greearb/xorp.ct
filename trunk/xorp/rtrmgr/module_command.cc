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

#ident "$XORP: xorp/rtrmgr/module_command.cc,v 1.6 2003/05/23 00:02:08 mjh Exp $"

//#define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "module_command.hh"
#include "xrldb.hh"
#include "template_tree.hh"
#include "libxipc/xrl_router.hh"
#include "task.hh"

ModuleCommand::ModuleCommand(const string& cmd_name, TemplateTree& tt)
    : Command(cmd_name),
      _tt(tt), 
      _startcommit(NULL), _endcommit(NULL), 
      _status_method(NO_STATUS_METHOD), 
      _execute_done(false)
{
    assert(cmd_name == "%modinfo");
}

void 
ModuleCommand::add_action(const list<string>& action, const XRLdb& xrldb) 
    throw (ParseError)
{
    if ((action.size() == 3) 
	&& ((action.front() == "startcommit")
	    || (action.front() == "endcommit"))) {
	//it's OK 
    } else if (action.size() != 2) {
	fprintf(stderr, "Error in modinfo command:\n");
	list <string>::const_iterator i = action.begin();
	while (i != action.end()) {
	    fprintf(stderr, ">%s< ", i->c_str());
	    ++i;
	}
	fprintf(stderr, "\n");
	if (action.size() > 2)
	    throw ParseError("too many parameters to %modinfo");
	else
	    throw ParseError("too few parameters to %modinfo");
    }
    typedef list<string>::const_iterator CI;
    CI ptr = action.begin();
    string cmd = *ptr;
    ++ptr;
    string value = *ptr;
    if (cmd == "provides") {
	_modname = value;
	_tt.register_module(_modname, this);
    } else if (cmd == "depends") {
	if (_modname.empty()) {
	    throw ParseError("\"depends\" must be preceded by \"provides\"");
	}
	_depends.push_back(value);
    } else if (cmd == "path") {
	if (_modname == "") {
	    throw ParseError("\"path\" must be preceded by \"provides\"");
	}
	if (_modpath != "") {
	    throw ParseError("duplicate \"path\" subcommand");
	}
	if (value[0]=='"') 
	    _modpath = value.substr(1,value.length()-2);
	else
	    _modpath = value;
    } else if (cmd == "startcommit") {
	debug_msg("startcommit:\n");
	list <string>::const_iterator i;
	for (i=action.begin(); i!=action.end(); i++)
	    debug_msg(">%s< ", (*i).c_str());
	debug_msg("\n");
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front()=="xrl")
	    _startcommit = new XrlAction(newaction, xrldb);
	else
	    _startcommit = new Action(newaction);
    } else if (cmd == "endcommit") {
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front()=="xrl")
	    _endcommit = new XrlAction(newaction, xrldb);
	else
	    _endcommit = new Action(newaction);
    } else if (cmd == "statusmethod") {
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _status_method = STATUS_BY_XRL;
	} else {
	    throw ParseError("Unknown statusmethod " + newaction.front());
	}
    } else {
	string err = "invalid subcommand \"" + cmd + "\" to %modinfo";
	throw ParseError(err);
    }
}

int 
ModuleCommand::execute(TaskManager& taskmgr) const 
{
#if 0
    debug_msg("ModuleCommand::execute %s (do_exec %d, do_commit %d)\n",
	      _modname.c_str(), taskmgr.do_exec(), do_commit);
    if (do_commit) {
	debug_msg("do_commit == true\n");
	//OK, we're actually going to do the commit
	//find or create the module in the module manager
	if (module_manager.module_exists(_modname)) {
	    if (module_manager.module_has_started(_modname))
		return XORP_OK;
	} else {
	    if (!module_manager.new_module(_modname, _modpath))
		return XORP_ERROR;
	}

	debug_msg("Starting module\n");
	XCCommandCallback cb = callback(const_cast<ModuleCommand*>(this),
					&ModuleCommand::exec_complete);
	int r = xclient.start_module(tid, module_manager, _modname, cb,
				      do_exec);

	XrlAction* xrl_procready = dynamic_cast<XrlAction*>(_procready);
	if (xrl_procready) {
	    debug_msg("proc ready xrl \"%s\"\n",
		      xrl_procready->request().c_str());
	    int r2 = xclient.send_xrl(tid,
				      UnexpandedXrl(0, xrl_procready),
				      0, do_exec, 30, 250);
	    debug_msg("Send Xrl okay %d\n", r2 == XORP_OK);
	}
	return r;
    } else {
	//this is just the verification pass, if this is successful
	//then a commit pass will occur.
	return XORP_OK;
    }
#endif
    if (_status_method == STATUS_BY_XRL) {
	StatusReadyValidation* sv =
	    new StatusReadyValidation(_modname, taskmgr);
	return taskmgr.add_module(_modname, _modpath, sv);
    } else {
	DelayValidation* dv = new DelayValidation(taskmgr.eventloop(), 2000);
	return taskmgr.add_module(_modname, _modpath, dv);
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
    tmp= "ModuleCommand: provides: " + _modname + "\n";
    tmp+="               path: " + _modpath + "\n";
    typedef list<string>::const_iterator CI;
    CI ptr = _depends.begin();
    while (ptr != _depends.end()) {
	tmp += "               depends: " + *ptr + "\n";
	++ptr;
    }
    return tmp;
}

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
#ifdef NOTDEF
    if (err == XrlError::OKAY()) {
	//XXX does this make sense?
	ctn->command_status_callback(this, true);
    } else {	
	ctn->command_status_callback(this, false);
    }
#endif
}

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
	//There was an error.  There's nothing we can so here - errors
	//are handled in the TaskManager.
	return;
    }
}
