// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/template_commands.cc,v 1.6 2003/02/22 00:46:10 mjh Exp $"

#define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "template_commands.hh"
#include "xrldb.hh"
#include "template_tree.hh"
#include "libxipc/xrl_router.hh"
#include "libxipc/xrl_router.hh"

Action::Action(const list<string> &action) 
    throw (ParseError) 
{

    //we need to split the command into variable names and the rest so
    //that when we later expand the variables, we can do them quickly
    //and with no risk of accidentally doing multiple expansions.
    string cur("\n");
    char c;
    enum char_type {VAR, NON_VAR, QUOTE};
    char_type mode = NON_VAR;
    typedef list<string>::const_iterator CI;
    CI ptr = action.begin();
    while (ptr != action.end()) {
	string word = (*ptr);
	if (word[0]=='"' && word[word.size()-1]=='"')
	    word = word.substr(1, word.size()-2);
	for(u_int i = 0; i<word.length(); i++) {
	    c = word[i];
	    switch (mode) {
	    case VAR:
		if (word[i]==')') {
		    mode = NON_VAR;
		    cur += ')';
		    _split_cmd.push_back(cur);
		    cur = "";
		    break;
		}
		cur += word[i];
		break;
	    case NON_VAR:
		if (word[i]=='$') {
		    mode = VAR;
		    if (cur!="")
			_split_cmd.push_back(cur);
		    cur = c;
		    break;
		}
		if (word[i]=='`') {
		    mode = QUOTE;
		    if (cur!="")
			_split_cmd.push_back(cur);
		    cur = c;
		    break;
		}
		cur += word[i];
		break;
	    case QUOTE:
		if (word[i]=='`') {
		    mode = NON_VAR;
		    cur += '`';
		    _split_cmd.push_back(cur);
		    cur = "";
		    break;
		}
		cur += word[i];
		break;
	    }
	}
	if ((cur != "") && (cur != "\n")) {
	    _split_cmd.push_back(cur);
	}
	cur = "\n";
	++ptr;
    }
}

string
Action::str() const {
    string str;
    typedef list<string>::const_iterator CI;
    CI ptr = _split_cmd.begin();
    while (ptr != _split_cmd.end()) {
	if (str == "") {
	    str = (*ptr).substr(1,(*ptr).size()-1);
	} else {
	    if ((*ptr)[0]=='\n')
		str += " " + (*ptr).substr(1,(*ptr).size()-1);
	    else
		str += *ptr;
	}
	++ptr;
    }
    return str;
}

/***********************************************************************/

XrlAction::XrlAction(const list<string> &action, const XRLdb& xrldb) 
     throw (ParseError) : Action(action)
{
    printf("XrlAction constructor\n");
    assert(action.front()=="xrl");
    check_xrl_is_valid(action, xrldb);
    
    list <string> xrlparts = _split_cmd;
    list <string>::const_iterator si;
    int j=0;
    for (si = xrlparts.begin(); si!= xrlparts.end(); si++) {
	printf("Seg %d: >%s< \n", j++, si->c_str());
    }
    printf("\n");
    //trim off the "xrl" command part.
    xrlparts.pop_front();
    if (xrlparts.empty())
	throw ParseError("bad XrlAction syntax\n");

    bool request_done = false;
    int segcount = 0;
    while (xrlparts.empty()==false) {
	string segment = xrlparts.front();
	printf("segment: %s\n", segment.c_str());
	string origsegment = segment;
	if (origsegment[0]=='\n') {
	    //strip the magic "\n" off
	    if (segcount == 0)
		segment = segment.substr(1,segment.size()-1);
	    else
		segment = " " + segment.substr(1,segment.size()-1);
	}
	string::size_type start = segment.find("->");
	printf("start=%u\n", (uint32_t)start);
	if (start != string::npos) {
	    printf("found return spec\n");
	    string::size_type origstart = origsegment.find("->");
	    if (request_done)
		throw ParseError("Two responses in one XRL\n");
	    request_done = true;
	    _request += segment.substr(0, start);
	    if (origstart != 0)
		_split_request.push_back(origsegment.substr(0, origstart));
	    segment = segment.substr(start+2, segment.size()-(start+2));
	    origsegment = origsegment.substr(origstart+2, 
					     origsegment.size()-(origstart+2));
	}
	if (request_done) {
	    _response += segment;
	    if (!origsegment.empty())
		_split_response.push_back(origsegment);
	} else {
	    _request += segment;
	    if (!origsegment.empty())
		_split_request.push_back(origsegment);
	}
	xrlparts.pop_front();
	segcount++;
    }
    printf("XrlAction:\n");
    printf("Request: >%s<\n", _request.c_str());
    list <string>::const_iterator i;
    for (i = _split_request.begin(); i != _split_request.end(); i++) {
	printf(">%s< ", (*i).c_str());
    }
    printf("\n");
    printf("Response: >%s<\n", _response.c_str());
    for (i = _split_response.begin(); i != _split_response.end(); i++) {
	printf(">%s< ", (*i).c_str());
    }
    printf("\n");
}

void 
XrlAction::check_xrl_is_valid(list<string> action, const XRLdb& xrldb) 
    const throw (ParseError)
{
    if (action.front()!= "xrl")
	abort();
    action.pop_front();
    string xrlstr =  action.front();
    printf("checking XRL: %s\n", xrlstr.c_str());

    /*
     * we need to go through the XRL template, and remove the
     * "=$(VARNAME)" instances to produce a generic version of the
     * XRL.  Then we can check it is a valid XRL as known by the
     * XRLdb.  
     */
    enum char_type {VAR, NON_VAR, QUOTE, ASSIGN};
    char_type mode = NON_VAR;
    char *cleaned_xrl = new char[xrlstr.length()];
    char *cp = cleaned_xrl;

    /*trim quotes from around the XRL*/
    u_int start=0;
    u_int stop = xrlstr.length();
    if (xrlstr[0]=='"' && xrlstr[xrlstr.length()-1]=='"') {
	start++; stop--;
    }

    /*copy the XRL, omitting the "=$(VARNAME)" parts */
    for(u_int i=start; i<stop; i++) {
	switch (mode) {
	case VAR:
	    if ((xrlstr[i]=='$') || (xrlstr[i]=='`')) {
		delete[] cleaned_xrl;
		string err;
		err = "Syntax error in XRL variable expansion;\n" +
		    xrlstr + " contains bad variable definition.\n";
		throw ParseError(err);
	    }
	    if (xrlstr[i]==')')
		mode = NON_VAR;
	    break;
	case NON_VAR:
	    if (xrlstr[i]=='=') {
		mode = ASSIGN;
		break;
	    }
	    *cp = xrlstr[i];
	    cp++;
	case QUOTE:
	    if (xrlstr[i]=='`') 
		mode = NON_VAR;
	    break;
	case ASSIGN:
	    if (xrlstr[i]=='$') {
		mode = VAR;
		break;
	    }
	    if (xrlstr[i]=='`') {
		mode = QUOTE;
		break;
	    }
	    if (xrlstr[i]=='&') {
		mode = NON_VAR;
		*cp = xrlstr[i];
		cp++;
		break;
	    }
	    break;
	}
    }

    *cp = '\0';
    if (xrldb.check_xrl_syntax(cleaned_xrl) == false) {
	string err;
	err = "Syntax error in XRL;\n" + string(cleaned_xrl) + 
	    " is not valid XRL syntax.\n";
	delete[] cleaned_xrl;
	throw ParseError(err);
    }
    XRLMatchType match = xrldb.check_xrl_exists(cleaned_xrl);
    switch (match) 
	{
	case MATCH_FAIL:
	case MATCH_RSPEC: {
	    string err;
	    err = "Error in XRL spec;\n" + string(cleaned_xrl) + 
		" is not specified in the XRL targets directory.\n";
	    delete[] cleaned_xrl;
	    throw ParseError(err);
	}
	case MATCH_XRL: {
	    string err;
	    err = "Error in XRL spec;\n" + string(cleaned_xrl) + 
		" has different return specification from that in the " +
		"XRL targets directory.\n";
	    delete[] cleaned_xrl;
	    throw ParseError(err);
	}
	case MATCH_ALL: {
	    delete[] cleaned_xrl;
	}
    }
}

#ifdef NOTDEF
int
Action::execute(const ConfigTreeNode& ctn,
		XorpClient *xclient,  uint tid,
		bool no_execute, XCCommandCallback cb) const {

    //go through the split command, doing variable substitution
    //put split words back together, and remove special "\n" characters
    list<string>::const_iterator ptr = _split_cmd.begin();
    //    map<string,string>::const_iterator var_iter;
    string expanded_var;
    list <string> expanded_cmd;
    string word;
    while (ptr != _split_cmd.end()) {
	string segment = *ptr;
	//"\n" at start of segment indicates start of a word
	if (segment[0]=='\n') {
	    //store the previous word
	    if (word != "") {
		expanded_cmd.push_back(word);
		word = "";
	    }
	    //strip the magic "\n" off
	    segment = segment.substr(1,segment.size()-1);
	}

	//do variable expansion
	bool expand_done = false;
	if (segment[0]=='`') {
	    expand_done = ctn.expand_expression(segment, expanded_var);
	    if (expand_done) {
		int len = expanded_var.length();
		//remove quotes
		if (expanded_var[0]=='"' && expanded_var[len-1]=='"')
		    word += expanded_var.substr(1,len-2);
		else
		    word += expanded_var;
	    } else {
		fprintf(stderr, "FATAL ERROR: failed to expand expression %s associated with node \"%s\"\n", segment.c_str(), ctn.segname().c_str());
		exit(1);
	    }
	} else if (segment[0]=='$') {
	    expand_done = ctn.expand_variable(segment, expanded_var);
	    if (expand_done) {
		int len = expanded_var.length();
		//remove quotes
		if (expanded_var[0]=='"' && expanded_var[len-1]=='"')
		    word += expanded_var.substr(1,len-2);
		else
		    word += expanded_var;
	    } else {
		fprintf(stderr, "FATAL ERROR: failed to expand variable %s associated with node \"%s\"\n", segment.c_str(), ctn.segname().c_str());
		//exit(1);
		abort();
	    }
	} else {
	    word += segment;
	}
	++ptr;
    }
    //store the last word
    if (word != "") 
	expanded_cmd.push_back(word);

    assert(expanded_cmd.size() >= 1);

    //go through the expanded version and extract some things we want
    string cmd_str, args[expanded_cmd.size()];
    ptr = expanded_cmd.begin();
    int i=0;
    while (ptr != expanded_cmd.end()) {
	args[i] = *ptr;
	if (cmd_str == "")
	    cmd_str = *ptr;
	else
	    cmd_str += " " + *ptr;
	++ptr;
	++i;
    }

    int result;
    if (args[0] == "xrl") {
	if (args[1].empty()) {
	    fprintf(stderr, "Bad XRL command: %s\n", cmd_str.c_str());
	    return (XORP_ERROR);
	}
	if ((args[1])[0]=='"' && args[1][args[1].size()-1]=='"')
	    args[1] = args[1].substr(1,args[1].size()-2);
	printf("CALL XRL: %s\n", args[1].c_str());
	result = xclient->run_command(tid, ctn, args[1], cb, no_execute);
	if ((i>=4) && !(args[3].empty()))
	    printf("Need to do something here to get the response back\n");
    } else {
	fprintf(stderr, "Bad command: %s\n", args[0].c_str());
	return (XORP_ERROR);
    }
	       
    printf("action execute returning %d\n", result);
    return result;
}

#endif

int
XrlAction::execute(const ConfigTreeNode& ctn,
		   XorpClient *xclient,  uint tid,
		   bool no_execute, XCCommandCallback cb) const {

    //first, go back through and merge all the separate words in the
    //command back together.
    list <string> expanded_cmd;
    list <string>::const_iterator ptr;
    string word;
    for (ptr = _split_cmd.begin(); ptr != _split_cmd.end(); ptr++) {
	string segment = *ptr;
	//"\n" at start of segment indicates start of a word
	if (segment[0]=='\n') {
	    //store the previous word
	    if (word != "") {
		expanded_cmd.push_back(word);
		word = "";
	    }
	    //strip the magic "\n" off
	    segment = segment.substr(1,segment.size()-1);
	}
	word += segment;
    }
    //store the last word
    if (word != "") 
	expanded_cmd.push_back(word);


    //go through the expanded version and copy to an array
    string args[expanded_cmd.size()];
    int words=0;
    for (ptr = expanded_cmd.begin(); ptr != expanded_cmd.end(); ptr++) {
	args[words] = *ptr;
	++words;
    }
    if (words==0)
	return (XORP_ERROR);

    
    //now we're ready to begin...
    int result;
    if (args[0] == "xrl") {
	if (words<2) {
	    string err = "XRL command is missing the XRL on node " 
		+ ctn.path() + "\n";
	    XLOG_WARNING(err.c_str());
	}

	string xrlstr = args[1];
	if (xrlstr[0]=='"' && xrlstr[xrlstr.size()-1]=='"')
	    xrlstr = xrlstr.substr(1,xrlstr.size()-2);
	printf("CALL XRL: %s\n", xrlstr.c_str());

	UnexpandedXrl x(&ctn, this);
	result = xclient->send_xrl(tid, x, cb, no_execute);
	printf("result = %d\n", result);
    } else {
	fprintf(stderr, "Bad command: %s\n", args[0].c_str());
	return (XORP_ERROR);
    }
    return result;
}

string XrlAction::expand_xrl_variables(const ConfigTreeNode& ctn) const {
    //go through the split command, doing variable substitution
    //put split words back together, and remove special "\n" characters
    debug_msg(("expand_xrl_variables node " + ctn.segname() +
	      " XRL " + _request + "\n").c_str());
    list<string>::const_iterator ptr = _split_request.begin();
    string expanded_var;
    list <string> expanded_cmd;
    string word;
    while (ptr != _split_request.end()) {
	string segment = *ptr;
	//"\n" at start of segment indicates start of a word
	if (segment[0]=='\n') {
	    //store the previous word
	    if (word != "") {
		expanded_cmd.push_back(word);
		word = "";
	    }
	    //strip the magic "\n" off
	    segment = segment.substr(1,segment.size()-1);
	}

	//do variable expansion
	bool expand_done = false;
	if (segment[0]=='`') {
	    expand_done = ctn.expand_expression(segment, expanded_var);
	    if (expand_done) {
		int len = expanded_var.length();
		//remove quotes
		if (expanded_var[0]=='"' && expanded_var[len-1]=='"')
		    word += expanded_var.substr(1,len-2);
		else
		    word += expanded_var;
	    } else {
		fprintf(stderr, "FATAL ERROR: failed to expand expression %s associated with node \"%s\"\n", segment.c_str(), ctn.segname().c_str());
		throw UnexpandedVariable(segment, ctn.segname());
	    }
	} else if (segment[0]=='$') {
	    expand_done = ctn.expand_variable(segment, expanded_var);
	    if (expand_done) {
		int len = expanded_var.length();
		//remove quotes
		if (expanded_var[0]=='"' && expanded_var[len-1]=='"')
		    word += expanded_var.substr(1,len-2);
		else
		    word += expanded_var;
	    } else {
		fprintf(stderr, "FATAL ERROR: failed to expand variable %s associated with node \"%s\"\n", segment.c_str(), ctn.segname().c_str());
		throw UnexpandedVariable(segment, ctn.segname());
	    }
	} else {
	    word += segment;
	}
	++ptr;
    }
    //store the last word
    if (word != "") 
	expanded_cmd.push_back(word);

    assert(expanded_cmd.size() >= 1);

    string xrlstr = expanded_cmd.front();
    if (xrlstr[0]=='"' && xrlstr[xrlstr.size()-1]=='"')
	xrlstr = xrlstr.substr(1,xrlstr.size()-2);
    return xrlstr;
}

string XrlAction::affected_module() const {
    string::size_type end = _request.find("/");
    if (end == string::npos) {
	XLOG_WARNING("Failed to find XRL target in XrlAction");
	return "";
    }
    return _request.substr(0, end);
}

Command::Command(const string &cmd_name) {
    _cmd_name = cmd_name;
}

Command::~Command() {
    list <Action*>::const_iterator i;
    for (i= _actions.begin(); i != _actions.end(); i++) {
	delete *i;
    }
}

void 
Command::add_action(const list <string> &action, const XRLdb& xrldb) {
    if (action.front()=="xrl") {
	_actions.push_back(new XrlAction(action, xrldb));
    } else {
	_actions.push_back(new Action(action));
    }
}

int
Command::execute(ConfigTreeNode& ctn,
		 XorpClient *xclient, uint tid, bool no_execute) const {
    int result = 0;
    int actions = 0;
    if (xclient==NULL)
	return 0;
    list <Action*>::const_iterator i;
    for (i= _actions.begin(); i != _actions.end(); i++) {
	const XrlAction *xa = dynamic_cast<const XrlAction*>(*i);
	if (xa!=NULL) {
	    result = xa->execute(ctn, xclient, tid, no_execute,
				 callback(const_cast<Command*>(this), 
					  &Command::action_complete, &ctn));
	} else {
	    //current we only implement XRL commands
	    XLOG_FATAL("execute on unimplemented action type\n");
	}
	if (result < 0) {
	    printf("command execute returning %d\n", result);
	    //XXXX how do we communicate this error back up
	    //return result;
	}
	actions++;
    } 
    printf("command execute returning XORP_OK\n");
    return actions;
}

void 
Command::action_complete(const XrlError& err, 
			 XrlArgs*,
			 ConfigTreeNode *ctn) {
    if (err == XrlError::OKAY()) {
	ctn->command_status_callback(this, true);
    } else {	
	ctn->command_status_callback(this, false);
    }
}

set <string> 
Command::affected_xrl_modules() const {
    set <string> affected_modules;
    list <Action*>::const_iterator i;
    for (i = _actions.begin(); i != _actions.end(); i++) {
	XrlAction* xa = dynamic_cast<XrlAction*>(*i);
	if (xa != NULL) {
	    string affected = xa->affected_module();
	    affected_modules.insert(affected);
	}
    }
    return affected_modules;
}

bool 
Command::affects_module(const string& module) const {
    //if we don't specify a module, we mean any module.
    if (module=="")
	return true;

    set <string> affected_modules = affected_xrl_modules();
    if (affected_modules.find(module)==affected_modules.end()) {
	return false;
    }
    return true;
}

string 
Command::str() const {
    string tmp = _cmd_name + ": ";
    list <Action*>::const_iterator i;
    for (i = _actions.begin(); i != _actions.end(); i++) {
	tmp += (*i)->str() + ", ";
    }
   return tmp;
}

ModuleCommand::ModuleCommand(const string &cmd_name, TemplateTree *tt) :
    Command(cmd_name)
{
    _tt = tt;
    assert(cmd_name == "%modinfo");
    _startcommit = NULL;
    _endcommit = NULL;
}

void 
ModuleCommand::add_action(const list<string> &action, const XRLdb& xrldb) 
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
	_tt->register_module(_modname, this);
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
	printf("startcommit:\n");
	list <string>::const_iterator i;
	for (i=action.begin(); i!=action.end(); i++)
	    printf(">%s< ", (*i).c_str());
	printf("\n");
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
    } else {
	string err = "invalid subcommand \"" + cmd + "\" to %modinfo";
	throw ParseError(err);
    }
}

int 
ModuleCommand::execute(XorpClient *xclient, uint tid,
		       ModuleManager *module_manager, 
		       bool no_execute, 
		       bool no_commit) const {
    printf("ModuleCommand::execute %s\n", _modname.c_str());
    if (no_commit == false) {
	printf("no_commit == false\n");
	//OK, we're actually going to do the commit

	//find or create the module in the module manager
	Module *m = module_manager->find_module(_modname);
	if (m == NULL) {
	    m = module_manager->new_module(this);
	    if (m == NULL)
		return XORP_ERROR;
	} else {
	    if (m->is_running())
		return XORP_OK;
	}
	XCCommandCallback cb = callback(const_cast<ModuleCommand*>(this),
					&ModuleCommand::exec_complete);
	return xclient->start_module(tid, module_manager, m, cb,
				     no_execute);
    } else {
	//this is just the verification pass, if this is successful
	//then a commit pass will occur.
	return XORP_OK;
    }
}

int 
ModuleCommand::start_transaction(ConfigTreeNode &ctn,
				 XorpClient *xclient,  uint tid, 
				 bool no_execute, 
				 bool no_commit) const {
    if (_startcommit == NULL || no_commit)
	return XORP_OK;
    printf("\n\n****! start_transaction on %s \n", ctn.segname().c_str());
    XCCommandCallback cb = callback(const_cast<ModuleCommand*>(this),
				    &ModuleCommand::action_complete,
				    &ctn, _startcommit,
				    string("end transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_startcommit);
    if (xa != NULL) {
	return xa->execute(ctn, xclient, tid, no_execute, cb);
    } 
    abort();
}

int 
ModuleCommand::end_transaction(ConfigTreeNode &ctn,
			       XorpClient *xclient,  uint tid, 
			       bool no_execute, 
			       bool no_commit) const {
    if (_endcommit == NULL || no_commit)
	return XORP_OK;
    XCCommandCallback cb = callback(const_cast<ModuleCommand*>(this),
				    &ModuleCommand::action_complete,
				    &ctn, _endcommit,
				    string("end transaction"));
    XrlAction *xa = dynamic_cast<XrlAction*>(_endcommit);
    if (xa != NULL) {
	return xa->execute(ctn, xclient, tid, no_execute, cb);
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

void 
ModuleCommand::exec_complete(const XrlError& /*err*/, 
			     XrlArgs*) {
    printf("ModuleCommand::exec_complete\n");
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
    printf("ModuleCommand::action_complete\n");
    if (err == XrlError::OKAY()) {
	//XXX does this make sense?
	if (!args->empty()) {
	    printf("ARGS: %s\n", args->str().c_str());
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
		printf("specargs: %s\n", s.substr(0, start).c_str());
		s = s.substr(start+1, s.size()-(start+1));
	    }
	    list <string>::const_iterator i;
	    for(i = specargs.begin(); i!= specargs.end(); i++) {
		string::size_type eq = i->find("=");
		if (eq == string::npos) {
		    continue;
		} else {
		    XrlAtom atom(i->substr(0, eq).c_str());
		    printf("atom name=%s\n", atom.name().c_str());
		    string varname = i->substr(eq+1, i->size()-(eq+1));
		    printf("varname=%s\n", varname.c_str());
		    XrlAtom returned_atom;
		    try {
			returned_atom = args->item(atom.name());
		    } catch (XrlArgs::XrlAtomNotFound& x) {
			//XXX
			abort();
		    }
		    string value = returned_atom.value();
		    printf("found atom = %s\n", returned_atom.str().c_str());
		    printf("found value = %s\n", value.c_str());
		    ctn->set_variable(varname, value);
		}
	    }
	}
	return;
    } else if (err == XrlError::COMMAND_FAILED()) {	
	fprintf(stderr, "COMMAND_FAILED: %s\n", cmd.c_str());
	//XXX 
    }
    abort();
}


AllowCommand::AllowCommand(const string &cmd_name)
    : Command(cmd_name)
{
}

void 
AllowCommand::add_action(const list <string> &action) 
    throw (ParseError)
{
#ifdef DEBUG_TEMPLATE_PARSER
    printf("AllowCommand::add_action\n");
#endif
    if (action.size() < 2) {
	throw ParseError("Allow command with less than two parameters");
    }
    list<string>::const_iterator i;
    i = action.begin();
    if ((_varname.size()!=0) && (_varname != *i)) {
	throw ParseError("Currently only one variable per node can be specified using allow commands");
    }
    _varname = *i;
    ++i;
    for(; i!= action.end(); ++i) {
	if ((*i)[0] == '"')
	    _allowed_values.push_back((*i).substr(1, (*i).size()-2));
	else
	    _allowed_values.push_back(*i);
    }
}

int 
AllowCommand::execute(const ConfigTreeNode& ctn) const 
    throw (ParseError)
{
    string found;
    if (ctn.expand_variable(_varname, found)== false) {
	string tmp = "\n AllowCommand: variable " + _varname + 
	    " is not defined.\n";
	throw ParseError(tmp);
    }

    list<string>::const_iterator i;
    for(i = _allowed_values.begin(); i!= _allowed_values.end(); ++i) {
	if (found == *i)
	    return 0;
    }
    string tmp = "value " + found + " is not a valid value for " + _varname + "\n";
    throw ParseError(tmp);
}

string 
AllowCommand::str() const {
    string tmp;
    tmp = "AllowCommand: varname = " + _varname + " \n       Allowed values: ";
    list<string>::const_iterator i;
    for(i = _allowed_values.begin(); i!= _allowed_values.end(); ++i) {
	tmp += "  " + *i;
    }
    tmp += "\n";
    return tmp;
}

