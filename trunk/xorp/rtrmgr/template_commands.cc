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

#ident "$XORP: xorp/rtrmgr/template_commands.cc,v 1.33 2003/12/19 20:30:20 pavlin Exp $"

// #define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_router.hh"
#include "conf_tree_node.hh"
#include "template_commands.hh"
#include "xrldb.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "task.hh"

Action::Action(TemplateTreeNode& template_tree_node,
	       const list<string>& action)
    : _template_tree_node(template_tree_node)
{
    string cur("\n");
    enum char_type { VAR, NON_VAR, QUOTE };
    char_type mode = NON_VAR;
    list<string>::const_iterator iter;

    //
    // We need to split the command into variable names and the rest so
    // that when we later expand the variables, we can do them quickly
    // and with no risk of accidentally doing multiple expansions.
    //

    iter = action.begin();
    while (iter != action.end()) {
	string word = (*iter);
	char c;

	if (word[0] == '"' && word[word.size() - 1] == '"')
	    word = word.substr(1, word.size() - 2);

	for (size_t i = 0; i < word.length(); i++) {
	    c = word[i];
	    switch (mode) {
	    case VAR:
		if (word[i] == ')') {
		    mode = NON_VAR;
		    cur += ')';
		    _split_cmd.push_back(cur);
		    cur = "";
		    break;
		}
		cur += word[i];
		break;
	    case NON_VAR:
		if (word[i] == '$') {
		    mode = VAR;
		    if (cur != "")
			_split_cmd.push_back(cur);
		    cur = c;
		    break;
		}
		if (word[i] == '`') {
		    mode = QUOTE;
		    if (cur != "")
			_split_cmd.push_back(cur);
		    cur = c;
		    break;
		}
		cur += word[i];
		break;
	    case QUOTE:
		if (word[i] == '`') {
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
	if ((cur != "") && (cur != "\n"))
	    _split_cmd.push_back(cur);
	cur = "\n";
	++iter;
    }
}

string
Action::str() const
{
    string str;
    list<string>::const_iterator iter;

    for (iter = _split_cmd.begin(); iter != _split_cmd.end(); ++iter) {
	const string& cmd = *iter;

	if (str == "") {
	    str = cmd.substr(1, cmd.size() - 1);
	} else {
	    if (cmd[0] == '\n')
		str += " " + cmd.substr(1, cmd.size() - 1);
	    else
		str += cmd;
	}
    }
    return str;
}

/***********************************************************************/

XrlAction::XrlAction(TemplateTreeNode& template_tree_node,
		     const list<string>& action, const XRLdb& xrldb)
    throw (ParseError)
    : Action(template_tree_node, action)
{
    list<string> xrl_parts = _split_cmd;

    debug_msg("XrlAction constructor\n");
    XLOG_ASSERT(action.front() == "xrl");

    check_xrl_is_valid(action, xrldb);

    // Print debug info
    {
	list<string>::const_iterator si;
	int j = 0;

	for (si = xrl_parts.begin(); si!= xrl_parts.end(); ++si)
	    debug_msg("Seg %d: >%s< \n", j++, si->c_str());
	debug_msg("\n");
    }

    // Trim off the "xrl" command part.
    xrl_parts.pop_front();
    if (xrl_parts.empty())
	xorp_throw(ParseError, "bad XrlAction syntax\n");

    bool request_done = false;
    size_t seg_count = 0;
    while (xrl_parts.empty() == false) {
	string segment = xrl_parts.front();
	debug_msg("segment: %s\n", segment.c_str());
	string orig_segment = segment;
	if (orig_segment[0] == '\n') {
	    // Strip the magic "\n" off
	    if (seg_count == 0)
		segment = segment.substr(1, segment.size() - 1);
	    else
		segment = " " + segment.substr(1, segment.size() - 1);
	}
	string::size_type start = segment.find("->");
	debug_msg("start=%u\n", (uint32_t)start);
	if (start != string::npos) {
	    debug_msg("found return spec\n");
	    string::size_type orig_start = orig_segment.find("->");
	    if (request_done)
		xorp_throw(ParseError, "Two responses in one XRL\n");
	    request_done = true;
	    _request += segment.substr(0, start);
	    if (orig_start != 0)
		_split_request.push_back(orig_segment.substr(0, orig_start));
	    segment = segment.substr(start + 2, segment.size() - (start + 2));
	    orig_segment = orig_segment.substr(orig_start + 2,
					       orig_segment.size()
					       - (orig_start + 2));
	}
	if (request_done) {
	    _response += segment;
	    if (!orig_segment.empty())
		_split_response.push_back(orig_segment);
	} else {
	    _request += segment;
	    if (!orig_segment.empty())
		_split_request.push_back(orig_segment);
	}
	xrl_parts.pop_front();
	seg_count++;
    }

    // Print debug output
    {
	list<string>::const_iterator iter;

	debug_msg("XrlAction:\n");
	debug_msg("Request: >%s<\n", _request.c_str());
	for (iter = _split_request.begin();
	     iter != _split_request.end();
	     ++iter) {
	    debug_msg(">%s< ", (*iter).c_str());
	}
	debug_msg("\n");

	debug_msg("Response: >%s<\n", _response.c_str());
	for (iter = _split_response.begin();
	     iter != _split_response.end();
	     ++iter) {
	    debug_msg(">%s< ", (*iter).c_str());
	}
	debug_msg("\n");
    }
}

void
XrlAction::check_xrl_is_valid(const list<string>& action, const XRLdb& xrldb)
    const throw (ParseError)
{
    XLOG_ASSERT(action.front() == "xrl");

    list<string>::const_iterator xrl_pos = ++action.begin();
    if (xrl_pos == action.end()) {
	xorp_throw(ParseError, "Expected xrl but none supplied.");
    }

    const string& xrlstr = *xrl_pos;
    debug_msg("checking XRL: %s\n", xrlstr.c_str());

    //
    // We need to go through the XRL template, and remove the
    //  "=$(VARNAME)" instances to produce a generic version of the
    // XRL.  Then we can check it is a valid XRL as known by the
    // XRLdb.
    //
    enum char_type { VAR, NON_VAR, QUOTE, ASSIGN };
    char_type mode = NON_VAR;
    string cleaned_xrl;

    // Trim quotes from around the XRL
    size_t start = 0;
    size_t stop = xrlstr.length();
    if (xrlstr[start] == '"' && xrlstr[stop - 1] == '"') {
	start++;
	stop--;
    }

    //
    // If the target name is a variable, then replace the target name
    // with the default target name (for verification purpose only).
    //
    if (xrlstr[start] == '$') {
	// Find the target name variable
	string::size_type target_name_end = xrlstr.find("/", start);
	if (target_name_end == string::npos) {
	    string err;
	    err = "Syntax error in XRL specification;\n" +
		xrlstr + " the XRL has no target name.\n";
	    xorp_throw(ParseError, err);
	}
	string target_name_var = xrlstr.substr(start, target_name_end - 1);
	if (target_name_var.empty()) {
	    string err;
	    err = "Syntax error in XRL specification;\n" +
		xrlstr + " empty XRL target.\n";
	    xorp_throw(ParseError, err);
	}

	// Find the default target name
	string default_target_name;
	default_target_name = template_tree_node().get_default_target_name_by_variable(target_name_var);
	if (default_target_name.empty()) {
	    string err;
	    err = "Syntax error in XRL default target name expansion;\n" +
		xrlstr + " the module has no default target name.\n";
	    xorp_throw(ParseError, err);
	}

	// Add the default target name to the cleaned XRL
	cleaned_xrl += default_target_name;

	// Advance the start pointer to point to the first symbol after
	// the target name, which should be '/'.
	while (xrlstr[start] != '/') {
	    start++;
	    XLOG_ASSERT(start < stop);
	}
    }

    // Copy the XRL, omitting the "=$(VARNAME)" parts
    for (size_t i = start; i < stop; i++) {
	switch (mode) {
	case VAR:
	    if (xrlstr[i] == '$' || xrlstr[i] == '`') {
		string err;
		err = "Syntax error in XRL variable expansion;\n" +
		    xrlstr + " contains bad variable definition.\n";
		xorp_throw(ParseError, err);
	    }
	    if (xrlstr[i] == ')')
		mode = NON_VAR;
	    break;
	case NON_VAR:
	    if (xrlstr[i] == '=') {
		mode = ASSIGN;
		break;
	    }
	    cleaned_xrl += xrlstr[i];
	    // FALLTHROUGH
	case QUOTE:
	    if (xrlstr[i] == '`')
		mode = NON_VAR;
	    break;
	case ASSIGN:
	    if (xrlstr[i] == '$') {
		mode = VAR;
		break;
	    }
	    if (xrlstr[i] == '`') {
		mode = QUOTE;
		break;
	    }
	    if (xrlstr[i] == '&') {
		mode = NON_VAR;
		cleaned_xrl += xrlstr[i];
		break;
	    }
	    break;
	}
    }

    if (xrldb.check_xrl_syntax(cleaned_xrl) == false) {
	string err;
	err = "Syntax error in XRL;\n" + cleaned_xrl +
	    " is not valid XRL syntax.\n";
	xorp_throw(ParseError, err);
    }
    XRLMatchType match = xrldb.check_xrl_exists(cleaned_xrl);
    switch (match) {
    case MATCH_FAIL:
    case MATCH_RSPEC: {
	string err;
	err = "Error in XRL spec;\n" + cleaned_xrl +
	    " is not specified in the XRL targets directory.\n";
	xorp_throw(ParseError, err);
    }
    case MATCH_XRL: {
	string err;
	err = "Error in XRL spec;\n" + cleaned_xrl +
	    " has different return specification from that in the " +
	    "XRL targets directory.\n";
	xorp_throw(ParseError, err);
    }
    case MATCH_ALL:
	break;
    }
}

int
XrlAction::execute(const ConfigTreeNode& ctn,
		   TaskManager& task_manager,
		   XrlRouter::XrlCallback cb) const
{
    list<string> expanded_cmd;
    list<string>::const_iterator iter;
    string word;

    //
    // First, go back through and merge all the separate words in the
    // command back together.
    //
    for (iter = _split_cmd.begin(); iter != _split_cmd.end(); ++iter) {
	string segment = *iter;
	// "\n" at start of segment indicates start of a word
	if (segment[0] == '\n') {
	    // Store the previous word
	    if (word != "") {
		expanded_cmd.push_back(word);
		word = "";
	    }
	    // Strip the magic "\n" off
	    segment = segment.substr(1, segment.size() - 1);
	}
	word += segment;
    }
    // Store the last word
    if (word != "")
	expanded_cmd.push_back(word);

    // Go through the expanded version and copy to an array
    string args[expanded_cmd.size()];
    size_t words = 0;
    for (iter = expanded_cmd.begin(); iter != expanded_cmd.end(); ++iter) {
	args[words] = *iter;
	++words;
    }
    if (words == 0)
	return (XORP_ERROR);

    // Now we're ready to begin...
    int result;
    if (args[0] == "xrl") {
	if (words < 2) {
	    string err = "XRL command is missing the XRL on node "
		+ ctn.path() + "\n";
	    XLOG_WARNING(err.c_str());
	}

	string xrlstr = args[1];
	if (xrlstr[0] == '"' && xrlstr[xrlstr.size() - 1] == '"')
	    xrlstr = xrlstr.substr(1, xrlstr.size() - 2);
	debug_msg("CALL XRL: %s\n", xrlstr.c_str());

	UnexpandedXrl uxrl(ctn, *this);
	task_manager.add_xrl(affected_module(ctn), uxrl, cb);
	debug_msg("result = %d\n", result);
	result = XORP_OK;
    } else {
	fprintf(stderr, "Bad command: %s\n", args[0].c_str());
	return (XORP_ERROR);
    }
    return result;
}

template<class TreeNode>
string
XrlAction::expand_xrl_variables(const TreeNode& tn) const
    throw (UnexpandedVariable)
{
    string word;
    string expanded_var;
    list<string> expanded_cmd;

    debug_msg(("expand_xrl_variables node " + tn.segname() +
	       " XRL " + _request + "\n").c_str());

    //
    // Go through the split command, doing variable substitution
    // put split words back together, and remove special "\n" characters
    //
    list<string>::const_iterator iter = _split_request.begin();
    while (iter != _split_request.end()) {
	string segment = *iter;
	// "\n" at start of segment indicates start of a word
	if (segment[0] == '\n') {
	    // Store the previous word
	    if (word != "") {
		expanded_cmd.push_back(word);
		word = "";
	    }
	    // Strip the magic "\n" off
	    segment = segment.substr(1, segment.size() - 1);
	}

	// Do variable expansion
	bool expand_done = false;
	if (segment[0] == '`') {
	    expand_done = tn.expand_expression(segment, expanded_var);
	    if (expand_done) {
		size_t len = expanded_var.length();
		// Remove quotes
		if (expanded_var[0] == '"' && expanded_var[len - 1] == '"')
		    word += expanded_var.substr(1, len - 2);
		else
		    word += expanded_var;
	    } else {
		fprintf(stderr, "FATAL ERROR: failed to expand expression %s associated with node \"%s\"\n",
			segment.c_str(), tn.segname().c_str());
		throw UnexpandedVariable(segment, tn.segname());
	    }
	} else if (segment[0] == '$') {
	    expand_done = tn.expand_variable(segment, expanded_var);
	    if (expand_done) {
		size_t len = expanded_var.length();
		// Remove quotes
		if (expanded_var[0] == '"' && expanded_var[len - 1] == '"')
		    word += expanded_var.substr(1, len - 2);
		else
		    word += expanded_var;
	    } else {
		fprintf(stderr, "FATAL ERROR: failed to expand variable %s associated with node \"%s\"\n",
			segment.c_str(), tn.segname().c_str());
		throw UnexpandedVariable(segment, tn.segname());
	    }
	} else {
	    word += segment;
	}
	++iter;
    }
    // Store the last word
    if (word != "")
	expanded_cmd.push_back(word);

    XLOG_ASSERT(expanded_cmd.size() >= 1);

    string xrlstr = expanded_cmd.front();
    if (xrlstr[0] == '"' && xrlstr[xrlstr.size() - 1] == '"')
	xrlstr = xrlstr.substr(1, xrlstr.size() - 2);
    return xrlstr;
}

string
XrlAction::affected_module(const ConfigTreeNode& ctn) const
{
    string::size_type end = _request.find("/");
    if (end == string::npos) {
	XLOG_WARNING("Failed to find XRL target in XrlAction");
	return "";
    }

    string target_name = _request.substr(0, end);
    if (target_name.empty()) {
	XLOG_WARNING("Empty XRL target in XrlAction");
	return "";
    }

    if (target_name[0] == '$') {
	//
	// This is a variable that needs to be expanded to get the
	// real target name. We use this variable name to get the module name.
	//
	return (ctn.get_module_name_by_variable(target_name));
    }

    //
    // XXX: the target name is hardcoded in the XRL, hence we assume that
    // the target name is same as the module name.
    //
    return target_name;
}

Command::Command(TemplateTreeNode& template_tree_node, const string& cmd_name)
    : _template_tree_node(template_tree_node)
{
    _cmd_name = cmd_name;
}

Command::~Command()
{
    list<Action*>::const_iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	delete *iter;
    }
}

void
Command::add_action(const list<string>& action, const XRLdb& xrldb)
{
    if (action.front() == "xrl") {
	_actions.push_back(new XrlAction(_template_tree_node, action, xrldb));
    } else {
	_actions.push_back(new Action(_template_tree_node, action));
    }
}

int
Command::execute(ConfigTreeNode& ctn, TaskManager& task_manager) const
{
    int result = 0;
    int actions = 0;

    list<Action*>::const_iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	const XrlAction* xa = dynamic_cast<const XrlAction*>(*iter);
	if (xa != NULL) {
	    result = xa->execute(ctn, task_manager,
				 callback(const_cast<Command*>(this),
					  &Command::action_complete, &ctn));
	} else {
	    // Current we only implement XRL commands
	    XLOG_FATAL("execute on unimplemented action type\n");
	}
	if (result < 0) {
	    debug_msg("command execute returning %d\n", result);
	    // XXX: how do we communicate this error back up
	    // return result;
	}
	actions++;
    }
    debug_msg("command execute returning XORP_OK\n");
    return actions;
}

void
Command::action_complete(const XrlError& err,
			 XrlArgs*,
			 ConfigTreeNode* ctn)
{
    printf("Command::action_complete\n");

    if (err == XrlError::OKAY()) {
	ctn->command_status_callback(this, true);
    } else {
	ctn->command_status_callback(this, false);
    }
}

set<string>
Command::affected_xrl_modules(const ConfigTreeNode& ctn) const
{
    set<string> affected_modules;
    list<Action*>::const_iterator iter;

    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	XrlAction* xa = dynamic_cast<XrlAction*>(*iter);
	if (xa != NULL) {
	    string affected = xa->affected_module(ctn);
	    affected_modules.insert(affected);
	}
    }
    return affected_modules;
}

bool
Command::affects_module(const ConfigTreeNode& ctn, const string& module) const
{
    // XXX: if we don't specify a module, we mean any module
    if (module == "")
	return true;

    set<string> affected_modules = affected_xrl_modules(ctn);
    if (affected_modules.find(module)==affected_modules.end()) {
	return false;
    }
    return true;
}

string
Command::str() const
{
    string tmp = _cmd_name + ": ";
    list<Action*>::const_iterator iter;

    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	tmp += (*iter)->str() + ", ";
    }
   return tmp;
}


AllowCommand::AllowCommand(TemplateTreeNode& template_tree_node,
			   const string& cmd_name)
    : Command(template_tree_node, cmd_name)
{
}

void
AllowCommand::add_action(const list<string>& action) throw (ParseError)
{
    debug_msg("AllowCommand::add_action\n");

    if (action.size() < 2) {
	xorp_throw(ParseError, "Allow command with less than two parameters");
    }

    list<string>::const_iterator iter;
    iter = action.begin();
    if ((_varname.size() != 0) && (_varname != *iter)) {
	xorp_throw(ParseError,
		   "Currently only one variable per node can be specified using allow commands");
    }
    _varname = *iter;
    ++iter;
    for ( ; iter != action.end(); ++iter) {
	const string& value = *iter;
	if (value[0] == '"')
	    _allowed_values.push_back(value.substr(1, value.size() - 2));
	else
	    _allowed_values.push_back(value);
    }
}

int
AllowCommand::execute(const ConfigTreeNode& ctn) const throw (ParseError)
{
    string found;

    if (ctn.expand_variable(_varname, found) == false) {
	string err = "\n AllowCommand: variable " + _varname +
	    " is not defined.\n";
	xorp_throw(ParseError, err);
    }

    list<string>::const_iterator iter;
    for (iter = _allowed_values.begin();
	iter != _allowed_values.end();
	++iter) {
	if (found == *iter)
	    return 0;
    }
    string err = "value " + found + " is not a valid value for " + _varname + "\n";
    xorp_throw(ParseError, err);
}

string
AllowCommand::str() const
{
    string tmp;

    tmp = "AllowCommand: varname = " + _varname + " \n       Allowed values: ";

    list<string>::const_iterator iter;
    for (iter = _allowed_values.begin();
	 iter != _allowed_values.end();
	 ++iter) {
	tmp += "  " + *iter;
    }
    tmp += "\n";
    return tmp;
}

//
// Template explicit instatiation
//
template string XrlAction::expand_xrl_variables<class ConfigTreeNode>(const ConfigTreeNode&) const throw (UnexpandedVariable);
template string XrlAction::expand_xrl_variables<class TemplateTreeNode>(const TemplateTreeNode&) const throw (UnexpandedVariable);
