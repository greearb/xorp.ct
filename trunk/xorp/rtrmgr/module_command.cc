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

#ident "$XORP: xorp/rtrmgr/template_commands.cc,v 1.18 2003/04/25 03:39:02 mjh Exp $"

//#define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "module_command.hh"
#include "xrldb.hh"
#include "template_tree.hh"
#include "libxipc/xrl_router.hh"
#include "task.hh"

ModuleCommand::ModuleCommand(const string& cmd_name, TemplateTree& tt)
    : Command(cmd_name),
      _tt(tt), _procready(NULL), _startcommit(NULL), _endcommit(NULL),
      _execute_done(false)
{
    assert(cmd_name == "%modinfo");
}

void 
ModuleCommand::add_action(const list<string>& action, const XRLdb& xrldb) 
    throw (ParseError)
{
    if ((action.size() == 3) 
	&& ((action.front() == "ready")
	    ||(action.front() == "startcommit")
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
    } else if (cmd == "ready") {
	list <string> newaction = action;
	newaction.pop_front();
	if (newaction.front() == "xrl") {
	    _procready = new XrlAction(newaction, xrldb);
	} else {
	    _procready = new Action(newaction);
	}
    } else {
	string err = "invalid subcommand \"" + cmd + "\" to %modinfo";
	throw ParseError(err);
    }
}

int 
ModuleCommand::execute(TaskManager& taskmgr,
		       bool do_commit) const {
    debug_msg("ModuleCommand::execute %s (do_exec %d, do_commit %d)\n",
	      _modname.c_str(), taskmgr.do_exec(), do_commit);
    if (do_commit) {
#if 0
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
#endif
	//XXX need to add a validation here
	return taskmgr.add_module(_modname, _modpath, NULL);
    } else {
	//this is just the verification pass, if this is successful
	//then a commit pass will occur.
	return XORP_OK;
    }
}

int 
ModuleCommand::start_transaction(ConfigTreeNode& ctn,
				 XorpClient& xclient,  uint tid, 
				 bool do_exec, 
				 bool do_commit) const {
    if (_startcommit == NULL || do_commit == false)
	return XORP_OK;
    debug_msg("\n\n****! start_transaction on %s \n", ctn.segname().c_str());
    XCCommandCallback cb = callback(const_cast<ModuleCommand*>(this),
				    &ModuleCommand::action_complete,
				    &ctn, _startcommit,
				    string("end transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_startcommit);
    if (xa != NULL) {
	return xa->execute(ctn, xclient, tid, do_exec, cb);
    } 
    abort();
}

int 
ModuleCommand::end_transaction(ConfigTreeNode& ctn,
			       XorpClient& xclient,  uint tid, 
			       bool do_exec, 
			       bool do_commit) const {
    if (_endcommit == NULL || do_commit == false)
	return XORP_OK;
    XCCommandCallback cb = callback(const_cast<ModuleCommand*>(this),
				    &ModuleCommand::action_complete,
				    &ctn, _endcommit,
				    string("end transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_endcommit);
    if (xa != NULL) {
	return xa->execute(ctn, xclient, tid, do_exec, cb);
    } 
    abort();
}

string
ModuleCommand::str() const {
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
			     XrlArgs*) {
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
			       string cmd) {
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
			abort();
		    }
		    string value = returned_atom.value();
		    debug_msg("found atom = %s\n", returned_atom.str().c_str());
		    debug_msg("found value = %s\n", value.c_str());
		    ctn->set_variable(varname, value);
		}
	    }
	}
	return;
    } else if (err == XrlError::BAD_ARGS()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());
	//XXX 
    } else if (err == XrlError::COMMAND_FAILED()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());
	//XXX 
    } else if (err == XrlError::RESOLVE_FAILED()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());	
	//XXX 
    } else if (err == XrlError::NO_FINDER()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());
	//XXX 
    } else if (err == XrlError::SEND_FAILED()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());
	//XXX 
    } else if (err == XrlError::REPLY_TIMED_OUT()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());	
	//XXX 
    } else if (err == XrlError::NO_SUCH_METHOD()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());	
	//XXX 
    } else if (err == XrlError::INTERNAL_ERROR()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());	
	//XXX 
    } else if (err == XrlError::SYSCALL_FAILED()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());	
	//XXX 
    } else if (err == XrlError::FAILED_UNKNOWN()) {
	fprintf(stderr, "%s: %s\n", err.str().c_str(), cmd.c_str());	
	//XXX 
    }
    abort();
}
