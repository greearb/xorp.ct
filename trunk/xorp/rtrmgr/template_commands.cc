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

#ident "$XORP: xorp/rtrmgr/template_commands.cc,v 1.34 2004/01/05 23:43:04 pavlin Exp $"

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

bool
Action::check_referred_variables(string& errmsg) const
{
    list<string>::const_iterator iter;

    for (iter = _referred_variables.begin();
	 iter != _referred_variables.end();
	 ++iter) {
	const string& varname = *iter;
	if (_template_tree_node.find_varname_node(varname) == NULL) {
	    errmsg = c_format("Syntax error in action %s: "
			      "invalid variable name %s",
			      str().c_str(), varname.c_str());
	    return false;
	}
    }

    return true;
}

/***********************************************************************/

XrlAction::XrlAction(TemplateTreeNode& template_tree_node,
		     const list<string>& action, const XRLdb& xrldb)
    throw (ParseError)
    : Action(template_tree_node, action)
{
    string errmsg;
    list<string> xrl_parts = _split_cmd;

    debug_msg("XrlAction constructor\n");
    XLOG_ASSERT(action.front() == "xrl");

    if (check_xrl_is_valid(action, xrldb, errmsg) != true)
	xorp_throw(ParseError, errmsg);

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
	xorp_throw(ParseError, "bad XrlAction syntax");

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
		xorp_throw(ParseError, "Two responses in one XRL");
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

bool
XrlAction::check_xrl_is_valid(const list<string>& action, const XRLdb& xrldb,
			      string& errmsg)
{
    XLOG_ASSERT(action.front() == "xrl");

    list<string>::const_iterator xrl_pos = ++action.begin();
    if (xrl_pos == action.end()) {
	errmsg = "Expected XRL but none supplied";
	return false;
    }

    const string& xrlstr = *xrl_pos;
    debug_msg("checking XRL: %s\n", xrlstr.c_str());

    //
    // We need to go through the XRL template, and remove the "=$(VARNAME)"
    // instances to produce a generic version of the XRL.
    // Then we can check it is a valid XRL as known by the XRLdb.
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
	    errmsg = c_format("Syntax error in XRL %s: no target name",
			      xrlstr.c_str());
	    return false;
	}
	string target_name_var = xrlstr.substr(start, target_name_end - 1);
	if (target_name_var.empty()) {
	    errmsg = c_format("Syntax error in XRL %s: empty XRL target",
			      xrlstr.c_str());
	    return false;
	}

	// Find the default target name
	string default_target_name;
	default_target_name = template_tree_node().get_default_target_name_by_variable(target_name_var);
	if (default_target_name.empty()) {
	    errmsg = c_format("Syntax error in XRL %s: "
			      "the module has no default target name",
			      xrlstr.c_str());
	    return false;
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

    //
    // Copy the XRL, omitting the "=$(VARNAME)" parts.
    // In the mean time, build the list of encountered "$(VARNAME)" variables.
    //
    for (size_t i = start; i < stop; i++) {
	switch (mode) {
	case VAR:
	    if (xrlstr[i] == '$' || xrlstr[i] == '`') {
		errmsg = c_format("Syntax error in XRL %s: "
				  "bad variable definition",
				  xrlstr.c_str());
		return false;
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
		//
		// Get the variable name and add it to the list of referred
		// variables.
		//
		{
		    string varname;
		    bool varname_end_found = false;
		    for (size_t j = i; j < stop; j++) {
			varname += xrlstr[j];
			if (xrlstr[j] == ')') {
			    varname_end_found = true;
			    break;
			}
		    }
		    if (! varname_end_found) {
			errmsg = c_format("Syntax error in XRL %s: "
					  "bad variable syntax",
					  xrlstr.c_str());
			return false;
		    }
		    list<string>::const_iterator iter;
		    iter = find(_referred_variables.begin(),
				_referred_variables.end(),
				varname);
		    if (iter == _referred_variables.end()) {
			_referred_variables.push_back(varname);
		    }
		}
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
	errmsg = c_format("Syntax error in XRL %s: invalid XRL syntax",
			  cleaned_xrl.c_str());
	return false;
    }
    XRLMatchType match = xrldb.check_xrl_exists(cleaned_xrl);
    switch (match) {
    case MATCH_FAIL:
    case MATCH_RSPEC: {
	errmsg = c_format("Error in XRL %s: "
			  "the XRL is not specified in the XRL targets "
			  "directory",
			  cleaned_xrl.c_str());
	return false;
    }
    case MATCH_XRL: {
	errmsg = c_format("Error in XRL %s: "
			  "the XRL has different return specification from "
			  "that in the XRL targets directory",
			  cleaned_xrl.c_str());
	return false;
    }
    case MATCH_ALL:
	break;
    }

    return true;
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
	    string err = c_format("XRL command is missing the XRL on node %s",
				  ctn.path().c_str());
	    XLOG_WARNING(err.c_str());
	}

	string xrlstr = args[1];
	if (xrlstr[0] == '"' && xrlstr[xrlstr.size() - 1] == '"')
	    xrlstr = xrlstr.substr(1, xrlstr.size() - 2);
	debug_msg("CALL XRL: %s\n", xrlstr.c_str());

	UnexpandedXrl uxrl(ctn, *this);
	task_manager.add_xrl(affected_module(), uxrl, cb);
	debug_msg("result = %d\n", result);
	result = XORP_OK;
    } else {
	fprintf(stderr, "Bad command: %s\n", args[0].c_str());
	return (XORP_ERROR);
    }
    return result;
}

template<class TreeNode>
int
XrlAction::expand_xrl_variables(const TreeNode& tn,
				string& result,
				string& errmsg) const
{
    string word;
    string expanded_var;
    list<string> expanded_cmd;

    debug_msg("expand_xrl_variables() node %s XRL %s\n",
	      tn.segname().c_str(), _request.c_str());

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
		// Error
		errmsg = c_format("failed to expand expression \"%s\" "
				  "associated with node \"%s\"",
				  segment.c_str(), tn.segname().c_str());
		return (XORP_ERROR);
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
		// Error
		errmsg = c_format("failed to expand variable \"%s\" "
				  "associated with node \"%s\"",
				  segment.c_str(), tn.segname().c_str());
		return (XORP_ERROR);
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
    result = xrlstr;

    return (XORP_OK);
}

string
XrlAction::affected_module() const
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
	return (template_tree_node().get_module_name_by_variable(target_name));
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
				 callback(this,
					  &Command::action_complete, &ctn));
	} else {
	    // Current we only implement XRL commands
	    XLOG_FATAL("execute on unimplemented action type");
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
			 XrlArgs* ,
			 ConfigTreeNode* ctn) const
{
    printf("Command::action_complete\n");

    if (err == XrlError::OKAY()) {
	ctn->command_status_callback(this, true);
    } else {
	ctn->command_status_callback(this, false);
    }
}

set<string>
Command::affected_xrl_modules() const
{
    set<string> affected_modules;
    list<Action*>::const_iterator iter;

    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	XrlAction* xa = dynamic_cast<XrlAction*>(*iter);
	if (xa != NULL) {
	    string affected = xa->affected_module();
	    affected_modules.insert(affected);
	}
    }
    return affected_modules;
}

bool
Command::affects_module(const string& module) const
{
    // XXX: if we don't specify a module, we mean any module
    if (module == "")
	return true;

    set<string> affected_modules = affected_xrl_modules();
    if (affected_modules.find(module) == affected_modules.end()) {
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

bool
Command::check_referred_variables(string& errmsg) const
{
    list<Action *>::const_iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	const Action* action = *iter;
	if (! action->check_referred_variables(errmsg))
	    return false;
    }

    return true;
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
		   "Currently only one variable per node can be specified "
		   "using \"allow\" commands");
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

bool
AllowCommand::verify_variable_value(const ConfigTreeNode& ctn,
				    string& errmsg) const
{
    string value;

    if (ctn.expand_variable(_varname, value) == false) {
	// Error: cannot expand the variable
	errmsg = c_format("Variable %s is not defined.", _varname.c_str());
	return false;
    }

    list<string>::const_iterator iter;
    for (iter = _allowed_values.begin();
	iter != _allowed_values.end();
	++iter) {
	if (value == *iter)
	    return true;		// OK
    }

    // Error: variable value is not allowed
    errmsg = c_format("Value \"%s\" is not a valid value for variable %s. ",
		      value.c_str(), _varname.c_str());
    list<string> values = _allowed_values;
    if (values.size() == 1) {
	errmsg += c_format("The only value allowed is %s.",
			   values.front().c_str());
    } else {
	errmsg += "Allowed values are: ";
	errmsg += values.front();
	values.pop_front();
	while (! values.empty()) {
	    if (values.size() == 1)
		errmsg += " and ";
	    else
		errmsg += ", ";
	    errmsg += values.front();
	    values.pop_front();
	}
	errmsg += ".";
    }
    return false;
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
template int XrlAction::expand_xrl_variables<class ConfigTreeNode>(
    const ConfigTreeNode& ctn,
    string& result,
    string& errmsg) const;
template int XrlAction::expand_xrl_variables<class TemplateTreeNode>(
    const TemplateTreeNode& ttn,
    string& result,
    string& errmsg) const;
