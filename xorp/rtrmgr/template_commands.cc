// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
#include "libxorp/utils.hh"

#include "libxipc/xrl_router.hh"

#include "conf_tree_node.hh"
#include "task.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"

#ifdef DEBUG_XRLDB
#include "xrldb.hh"
#endif

#include "libxipc/xrl_atom_encoding.hh"

Action::Action(TemplateTreeNode& template_tree_node,
	       const list<string>& action)
    : _action(action),
      _template_tree_node(template_tree_node)
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
	string word = unquote(*iter);
	char c;

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
Action::expand_action(string& error_msg)
{
    // XXX: nothing to do in the default case
    UNUSED(error_msg);
    return (true);
}

bool
Action::check_referred_variables(string& error_msg) const
{
    list<string>::const_iterator iter;

    for (iter = _referred_variables.begin();
	 iter != _referred_variables.end();
	 ++iter) {
	const string& varname = *iter;
	if (_template_tree_node.find_varname_node(varname) == NULL) {
	    error_msg = c_format("Syntax error in action %s: "
				 "invalid variable name %s",
				 str().c_str(), varname.c_str());
	    return false;
	}
    }

    return true;
}

/***********************************************************************/

XrlAction::XrlAction(TemplateTreeNode& template_tree_node,
		     const list<string>& action, const XRLdb* xrldb)
    throw (ParseError)
    : Action(template_tree_node, action),
      _xrldb(0)
{
#ifdef DEBUG_XRLDB
    XLOG_ASSERT(0 != dynamic_cast<const XRLdb*>(xrldb));
    _xrldb = xrldb;
#else
    UNUSED(xrldb);
#endif

    list<string> xrl_parts = _split_cmd;

    debug_msg("XrlAction constructor\n");
    XLOG_ASSERT(action.front() == "xrl");

    // Print debug info
    {
	list<string>::const_iterator si;
	int j = 0;
	UNUSED(j);

	for (si = xrl_parts.begin(); si != xrl_parts.end(); ++si)
	    debug_msg("Seg %d: >%s< \n", j++, si->c_str());
	debug_msg("\n");
    }

    // Trim off the "xrl" command part
    xrl_parts.pop_front();
    if (xrl_parts.empty())
	xorp_throw(ParseError, "bad XrlAction syntax");

    bool request_done = false;
    size_t seg_count = 0;
    while (xrl_parts.empty() == false) {
	if (xrl_parts.front().size() == 0) {
	    xrl_parts.pop_front();
	    continue;
	}

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
	debug_msg("start=%u\n", XORP_UINT_CAST(start));
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
XrlAction::expand_action(string& error_msg)
{
    debug_msg("XrlAction::expand_action()\n");
    XLOG_ASSERT(_action.front() == "xrl");

    _module_name = _template_tree_node.module_name();
    if (_module_name.empty()) {
	error_msg = c_format("Empty module name for action in template %s",
			     _template_tree_node.path().c_str());
	return false;
    }

#ifdef DEBUG_XRLDB
    if (check_xrl_is_valid(_action, _xrldb, error_msg) != true)
	return false;
#endif

    return true;
}

#ifdef DEBUG_XRLDB
bool
XrlAction::check_xrl_is_valid(const list<string>& action,
			      const XRLdb* xrldb,
			      string& error_msg)
{
    const string module_name = template_tree_node().module_name();

    XLOG_ASSERT(action.front() == "xrl");
    XLOG_ASSERT(0 != dynamic_cast<const XRLdb*>(xrldb));

    list<string>::const_iterator xrl_pos = ++action.begin();
    if (xrl_pos == action.end()) {
	error_msg = c_format("Expected XRL in module %s but none supplied",
			     module_name.c_str());
	return false;
    }

    const string& xrl_str = *xrl_pos;
    debug_msg("checking XRL: %s\n", xrl_str.c_str());

    //
    // We need to go through the XRL template, and remove the "=$(VARNAME)"
    // instances to produce a generic version of the XRL.
    // Then we can check it is a valid XRL as known by the XRLdb.
    //
    string cleaned_xrl;

    // Trim quotes from around the XRL
    size_t start = 0;
    size_t stop = xrl_str.length();
    if (xrl_str[start] == '"' && xrl_str[stop - 1] == '"') {
	start++;
	stop--;
    }

    //
    // If the target name is a variable, then replace the target name
    // with the default target name (for verification purpose only).
    //
    if (xrl_str[start] == '$') {
	// Find the target name variable
	string::size_type target_name_end = xrl_str.find("/", start);
	if (target_name_end == string::npos) {
	    error_msg = c_format("Syntax error in module %s XRL %s: "
				 "no target name",
				 module_name.c_str(), xrl_str.c_str());
	    return false;
	}
	string target_name_var = xrl_str.substr(start, target_name_end - 1);
	if (target_name_var.empty()) {
	    error_msg = c_format("Syntax error in module %s XRL %s: "
				 "empty XRL target",
				 module_name.c_str(), xrl_str.c_str());
	    return false;
	}

	// Find the default target name
	string default_target_name;
	default_target_name = template_tree_node().get_default_target_name_by_variable(target_name_var);
	if (default_target_name.empty()) {
	    error_msg = c_format("Syntax error in module %s XRL %s: "
				 "the module has no default target name",
				 module_name.c_str(), xrl_str.c_str());
	    return false;
	}

	// Add the default target name to the cleaned XRL
	cleaned_xrl += default_target_name;

	// Advance the start pointer to point to the first symbol after
	// the target name, which should be '/'.
	while (xrl_str[start] != '/') {
	    start++;
	    XLOG_ASSERT(start < stop);
	}
    }

    debug_msg("XrlAction before cleaning:\n%s\n", xrl_str.c_str());
    //
    // Copy the XRL, omitting the "=$(VARNAME)" parts.
    // In the mean time, build the list of encountered "$(VARNAME)" variables.
    //
    list<ActionCharType> mode_stack;
    mode_stack.push_front(NON_VAR);
    for (size_t i = start; i < stop; i++) {
	switch (mode_stack.front()) {
	case VAR:
	    if (xrl_str[i] == '$' || xrl_str[i] == '`') {
		error_msg = c_format("Syntax error in module %s XRL %s: "
				     "bad variable definition",
				     module_name.c_str(), xrl_str.c_str());
		return false;
	    }
	    if (xrl_str[i] == ')') {
		mode_stack.pop_front();
	    }
	    break;
	case NON_VAR:
	    if (xrl_str[i] == '=') {
		mode_stack.push_front(ASSIGN);
		break;
	    }
	    cleaned_xrl += xrl_str[i];
	    // FALLTHROUGH
	case QUOTE:
	    if (xrl_str[i] == '`') {
		mode_stack.pop_front();
	    }
	    break;
	case ASSIGN:
	    if (xrl_str[i] == '$') {
		//
		// Get the variable name and add it to the list of referred
		// variables.
		//
		{
		    string varname;
		    bool varname_end_found = false;
		    for (size_t j = i; j < stop; j++) {
			varname += xrl_str[j];
			if (xrl_str[j] == ')') {
			    varname_end_found = true;
			    break;
			}
		    }
		    if (! varname_end_found) {
			error_msg = c_format("Syntax error in module %s XRL %s: "
					     "bad variable syntax",
					     module_name.c_str(),
					     xrl_str.c_str());
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
		mode_stack.push_front(VAR);
		break;
	    }
	    if (xrl_str[i] == '`') {
		mode_stack.push_front(QUOTE);
		break;
	    }
	    if (xrl_str[i] == '&') {
		mode_stack.pop_front();
		if (mode_stack.front() != NON_VAR) {
		    error_msg = c_format("Syntax error in module %s XRL %s: "
					 "invalid XRL syntax, char-idx: %i (rxl_str[i] == &, mode_stack.front() != NON_VAR)",
					 module_name.c_str(), xrl_str.c_str(), (int)(i));
		    return false;
		}
		cleaned_xrl += xrl_str[i];
		break;
	    }
	    if ((xrl_str.size() > i)
		&& (xrl_str[i] == '-') && (xrl_str[i+1] == '>')) {
		/* it's the start of the return spec */
		cleaned_xrl += xrl_str[i];
		mode_stack.pop_front();
		if (mode_stack.front() != NON_VAR) {
		    error_msg = c_format("Syntax error in module %s XRL %s: "
					 "invalid XRL syntax, char-idx: %i (start of return spec)",
					 module_name.c_str(), xrl_str.c_str(), (int)(i));
		    return false;
		}
	    }
	    break;
	}
    }
    debug_msg("XrlAction after cleaning:\n%s\n", cleaned_xrl.c_str());

    if (xrldb->check_xrl_syntax(cleaned_xrl) == false) {
	error_msg = c_format("Syntax error in module %s XRL %s: "
			     "invalid XRL syntax (check_xrl_syntax failed)",
			     module_name.c_str(), cleaned_xrl.c_str());
	return false;
    }
    XRLMatchType match = xrldb->check_xrl_exists(cleaned_xrl);
    switch (match) {
    case MATCH_FAIL:
    case MATCH_RSPEC: {
	error_msg = c_format("Error in module %s XRL %s: "
			     "the XRL is not specified in the XRL targets "
			     "directory",
			     module_name.c_str(), cleaned_xrl.c_str());
	return false;
    }
    case MATCH_XRL: {
	error_msg = c_format("Error in module %s XRL %s: "
			     "the XRL has different return specification from "
			     "that in the XRL targets directory",
			     module_name.c_str(), cleaned_xrl.c_str());
	return false;
    }
    case MATCH_ALL:
	break;
    }

    return true;
}
#endif // DEBUG_XRLDB

int
XrlAction::execute(const MasterConfigTreeNode& ctn,
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
    vector<string> args;
    for (iter = expanded_cmd.begin(); iter != expanded_cmd.end(); ++iter) {
	args.push_back(*iter);
    }
    if (args.empty())
	return (XORP_ERROR);

    // Now we're ready to begin...
    int result;
    if (args[0] == "xrl") {
	if (args.size() < 2) {
	    string err = c_format("XRL command is missing the XRL on node %s",
				  ctn.path().c_str());
	    XLOG_WARNING("%s", err.c_str());
	}

	string xrl_str = unquote(args[1]);

	debug_msg("CALL XRL: %s\n", xrl_str.c_str());

	UnexpandedXrl uxrl(ctn, *this);
	task_manager.add_xrl(related_module(), uxrl, cb);
	result = XORP_OK;
	debug_msg("result = %d\n", result);
    } else {
	XLOG_ERROR("Bad command: %s\n", args[0].c_str());
	return XORP_ERROR;
    }
    return result;
}


template<class TreeNode>
Xrl*
XrlAction::expand_xrl_variables(const TreeNode& tn,
				string& error_msg) const
{
    string word;
    string expanded_var;
    string command;
    list<string> args;

    debug_msg("expand_xrl_variables() node %s XRL %s\n",
	   tn.segname().c_str(), _request.c_str());


    /* split the request into command and separate args */
    bool escaped = false;
    for (size_t i = 0; i < _request.size(); i++) {
	if (escaped == false) {
	    if (_request[i] == '\\') {
		escaped = true;
		word += '\\';
	    } else if (_request[i] == '?') {
		if (command == "") {
		    command = word;
		    word = "";
		} else {
		    error_msg = c_format("unescaped '?' in XRL args \"%s\" "
					 "associated with node \"%s\"",
					 _request.c_str(), tn.path().c_str());
		    return NULL;
		}
	    } else if (_request[i] == '&') {
		args.push_back(word);
		word = "";
	    } else {
		word += _request[i];
	    }
	} else {
	    word += _request[i];
	}
    }
    if (word != "") {
	if (command.empty()) {
	    command = word;
	} else {
	    args.push_back(word);
	}
    }

    debug_msg("target/command part: %s\n", command.c_str());
    list <string>::const_iterator i2;
    for (i2 = args.begin(); i2 != args.end(); ++i2) {
	debug_msg("arg part: %s\n", (*i2).c_str());
    }

    string expanded_command;
    if (!expand_vars(tn, command, expanded_command)) {
	    error_msg = expanded_command;
	    return NULL;
	}

    debug_msg("expanded target/command part: %s\n", expanded_command.c_str());

    // find the command name
    list <string> cmd_parts = split(expanded_command, '/');
    if (cmd_parts.size() < 2) {
	    error_msg = c_format("bad XRL \"%s\" "
				 "associated with node \"%s\"",
				 _request.c_str(), tn.path().c_str());
	    return NULL;
    }
    command = cmd_parts.back();
    cmd_parts.pop_back();

    // put the target name back together again
    string target;
    while (!cmd_parts.empty()) {
	if (!target.empty()) {
	    target += '/';
	}
	target += cmd_parts.front();
	cmd_parts.pop_front();
    }

    // now process the args.

    XrlArgs xrl_args;

    list<string>::const_iterator iter;
    for (iter = args.begin(); iter != args.end(); ++iter) {

	// split each arg into argname, argtype and argvalue
	string arg = *iter;
	string name, type, value;
	for (size_t i = 0; i < arg.size(); i++) {
	    if (arg[i] == ':') {
		name = strip_empty_spaces(arg.substr(0,i));
		arg = arg.substr(i + 1, arg.size() - (i + 1));
		break;
	    }
	}
	for (size_t i = 0; i < arg.size(); i++) {
	    if (arg[i] == '=') {
		type = strip_empty_spaces(arg.substr(0,i));
		value = arg.substr(i + 1, arg.size() - (i + 1));
		break;
	    }
	}

	// check that this is a legal XrlAtom type
	// it really shouldn't be possible for this to fail given
	// earlier checks
	XrlAtomType arg_type = XrlAtom::lookup_type(type.c_str());
	if (arg_type == xrlatom_no_type) {
	    error_msg = c_format("bad XRL syntax \"%s\" "
				 "associated with node \"%s\"",
				 _request.c_str(), tn.path().c_str());
	    return NULL;
	}

	string expanded_value;
	if (!expand_vars(tn, value, expanded_value)) {
	    error_msg = expanded_value;
	    return NULL;
	}

	// At this point we've expanded all the variables.
	// Now it's time to build an XrlAtom
	try {
	    debug_msg("Atom: %s\n", expanded_value.c_str());
	    XrlAtom atom(name, arg_type,
			 xrlatom_encode_value(expanded_value.c_str(),
					      expanded_value.size()));
	    xrl_args.add(atom);
	} catch (InvalidString&) {
	    error_msg = c_format("Bad xrl arg \"%s\" "
				 "associated with node \"%s\"",
				 name.c_str(), tn.path().c_str());
	    return NULL;
	}
    }

    // Now we've got a arg list.  Time to build an Xrl
    Xrl* xrl = new Xrl(target, command, xrl_args);
    debug_msg("Xrl expanded to %s\n", xrl->str().c_str());
    return xrl;
}

template<class TreeNode>
bool
XrlAction::expand_vars(const TreeNode& tn,
		       const string& value, string& result) const {
    debug_msg("expand_vars: %s\n", value.c_str());
    bool escaped = false;
    string varname;
    for (size_t i = 0; i < value.size(); i++) {
	if (escaped) {
	    escaped = false;
	    result += value[i];
	} else if (value[i] == '\\') {
	    if (varname.empty()) {
		result += '\\';
		escaped = true;
	    } else {
		// can't escape in a varname
		result = c_format("bad varname \"%s\" "
					  "associated with node \"%s\"",
					  _request.c_str(), tn.path().c_str());
		return false;
	    }
	} else if ((i != 0) && (! varname.empty())
		   && varname[0] == '$' && value[i] == ')') {
	    varname += ')';
	    // expand variable
	    string expanded_var;
	    debug_msg("expanding varname: %s\n", varname.c_str());
	    bool expand_done = tn.expand_variable(varname, expanded_var,
						  false);
	    if (expand_done) {
		debug_msg("expanded to: %s\n", expanded_var.c_str());
		// expanded_var = xrlatom_encode_value(expanded_var);
		result += unquote(expanded_var);
	    } else {
		// Error
		result = c_format("failed to expand variable \"%s\" "
				  "associated with node \"%s\"",
				  varname.c_str(), tn.segname().c_str());
		return false;
	    }
	    varname = "";
	} else if ((i != 0) && (! varname.empty())
		   && varname[0] == '`' && value[i] == '`' ) {
	    varname += '`';
	    // expand expression
	    string expanded_var;
	    bool expand_done = tn.expand_expression(varname, expanded_var);
	    if (expand_done) {
		// expanded_var = xrlatom_encode_value(expanded_var);
		result += unquote(expanded_var);
	    } else {
		// Error
		result = c_format("failed to expand expression \"%s\" "
				  "associated with node \"%s\"",
				  varname.c_str(), tn.path().c_str());
		return false;
	    }
	    varname = "";
	} else if (value[i] == '$') {
	    varname += '$';
	} else if (value[i] == '`') {
	    varname += '`';
	} else if (varname.empty()) {
	    // we're not building up a varname
	    result += value[i];
	} else {
	    varname += value[i];
	}
    }
    return true;
}

string
XrlAction::related_module() const
{
    return (_module_name);
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

/***********************************************************************/

ProgramAction::ProgramAction(TemplateTreeNode& template_tree_node,
			     const list<string>& action) throw (ParseError)
    : Action(template_tree_node, action)
{
    list<string> program_parts = _split_cmd;

    debug_msg("ProgramAction constructor\n");
    XLOG_ASSERT(action.front() == "program");

    // Print debug info
    {
	list<string>::const_iterator si;
	int j = 0;
	UNUSED(j);

	for (si = program_parts.begin(); si != program_parts.end(); ++si)
	    debug_msg("Seg %d: >%s< \n", j++, si->c_str());
	debug_msg("\n");
    }

    // Trim off the "program" command part
    program_parts.pop_front();
    if (program_parts.empty())
	xorp_throw(ParseError, "bad ProgramAction syntax");

    bool request_done = false;
    size_t seg_count = 0;
    while (program_parts.empty() == false) {
	if (program_parts.front().size() == 0) {
	    program_parts.pop_front();
	    continue;
	}

	string segment = program_parts.front();
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
	debug_msg("start=%u\n", XORP_UINT_CAST(start));
	if (start != string::npos) {
	    debug_msg("found return spec\n");
	    string::size_type orig_start = orig_segment.find("->");
	    if (request_done)
		xorp_throw(ParseError, "Two responses in one program");
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
	program_parts.pop_front();
	seg_count++;
    }

    //
    // Find the variable names that will contain the stdout and the stderr
    // of the program.
    //
    if (! _response.empty()) {
	string part1, part2;
	string::size_type pos = _response.find("&");
	part1 = _response.substr(0, pos);
	if (pos != string::npos)
	    part2 = _response.substr(pos + 1);
	part1 = strip_empty_spaces(part1);
	part2 = strip_empty_spaces(part2);
	if (part2.find("&") != string::npos) {
	    xorp_throw(ParseError,
		       "Too many components in the program response");
	}
	parse_program_response(part1);
	parse_program_response(part2);
    }

    // Print debug output
    {
	list<string>::const_iterator iter;

	debug_msg("ProgramAction:\n");
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
ProgramAction::expand_action(string& error_msg)
{
    debug_msg("ProgramAction::expand_action()\n");
    XLOG_ASSERT(_action.front() == "program");

    _module_name = _template_tree_node.module_name();
    if (_module_name.empty()) {
	error_msg = c_format("Empty module name for action in template %s",
			     _template_tree_node.path().c_str());
	return false;
    }

    if (check_program_is_valid(_action, error_msg) != true)
	return false;

    return true;
}

void
ProgramAction::parse_program_response(const string& part) throw (ParseError)
{
    string::size_type pos;

    if (part.empty())
	return;

    pos = part.find("=");
    if (pos == string::npos) {
	xorp_throw(ParseError,
		   "Missing '=' in program response specification");
    }

    string l, r;
    l = part.substr(0, pos);
    r = part.substr(pos + 1);

    if ((l != "stdout") && (l != "stderr")) {
	string error_msg = c_format("Unrecognized keyword in program "
				    "response specification: %s", l.c_str());
	xorp_throw(ParseError, error_msg);
    }

    if (l == "stdout") {
	if (! _stdout_variable_name.empty()) {
	    xorp_throw(ParseError,
		       "Repeated \"stdout\" keyword in program response "
		       "specification");
	}
	_stdout_variable_name = r;
    }
    if (l == "stderr") {
	if (! _stderr_variable_name.empty()) {
	    xorp_throw(ParseError,
		       "Repeated \"stderr\" keyword in program response "
		       "specification");
	}
	_stderr_variable_name = r;
    }
}

bool
ProgramAction::check_program_is_valid(const list<string>& action,
				      string& error_msg)
{
    XLOG_ASSERT(action.front() == "program");

    list<string>::const_iterator program_pos = action.begin();
    program_pos++;
    if (program_pos == action.end()) {
	error_msg = "Expected program but none supplied";
	return false;
    }

    const string& program_str = *program_pos;
    debug_msg("checking program: %s\n", program_str.c_str());

    //
    // We need to go through the program template and perform some basic
    // validations.
    // Note that we cannot verify that the program is valid and exists,
    // because it may contain variable names and we don't know the values
    // of those variables yet.
    //
    string cleaned_program;

    // Trim quotes from around the program
    size_t start = 0;
    size_t stop = program_str.length();
    if (program_str[start] == '"' && program_str[stop - 1] == '"') {
	start++;
	stop--;
    }

    debug_msg("ProgramAction before cleaning:\n%s\n", program_str.c_str());

    //
    // Copy the program and perform some basic validations.
    // In the mean time, build the list of encountered "$(VARNAME)" variables.
    //
    list<ActionCharType> mode_stack;
    mode_stack.push_front(NON_VAR);
    for (size_t i = start; i < stop; i++) {
	switch (mode_stack.front()) {
	case VAR:
	    if (program_str[i] == '$' || program_str[i] == '`') {
		error_msg = c_format("Syntax error in program %s: "
				     "bad variable definition",
				     program_str.c_str());
		return false;
	    }
	    if (program_str[i] == ')') {
		mode_stack.pop_front();
	    }
	    break;
	case NON_VAR:
	    if (program_str[i] == '$') {
		//
		// Get the variable name and add it to the list of referred
		// variables.
		//
		{
		    string varname;
		    bool varname_end_found = false;
		    for (size_t j = i; j < stop; j++) {
			varname += program_str[j];
			if (program_str[j] == ')') {
			    varname_end_found = true;
			    break;
			}
		    }
		    if (! varname_end_found) {
			error_msg = c_format("Syntax error in program %s: "
					     "bad variable syntax",
					     program_str.c_str());
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
		mode_stack.push_front(VAR);
		break;
	    }
	    if (program_str[i] == '`') {
		mode_stack.push_front(QUOTE);
		break;
	    }
	    cleaned_program += program_str[i];
	    break;
	case QUOTE:
	    if (program_str[i] == '`') {
		mode_stack.pop_front();
	    }
	    break;
	case ASSIGN:
	    // XXX: not used
	    XLOG_UNREACHABLE();
	    break;
	}
    }
    debug_msg("ProgramAction after cleaning:\n%s\n", cleaned_program.c_str());

    if (cleaned_program.empty()) {
	error_msg = c_format("Syntax error in program specification %s: "
			     "empty program",
			     program_str.c_str());
	return false;
    }

    return true;
}

int
ProgramAction::execute(const MasterConfigTreeNode&	ctn,
		       TaskManager&			task_manager,
		       TaskProgramItem::ProgramCallback	program_cb) const
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
    vector<string> args;
    for (iter = expanded_cmd.begin(); iter != expanded_cmd.end(); ++iter) {
	args.push_back(*iter);
    }
    if (args.empty())
	return (XORP_ERROR);

    // Now we're ready to begin...
    int result;
    if (args[0] == "program") {
	if (args.size() < 2) {
	    string err = c_format("Program command is missing the program "
				  "on node %s",
				  ctn.path().c_str());
	    XLOG_WARNING("%s", err.c_str());
	}

	string program_str = unquote(args[1]);

	debug_msg("CALL program: %s\n", program_str.c_str());

	UnexpandedProgram uprogram(ctn, *this);
	task_manager.add_program(related_module(), uprogram, program_cb);
	result = XORP_OK;
	debug_msg("result = %d\n", result);
    } else {
	XLOG_ERROR("Bad command: %s\n", args[0].c_str());
	return XORP_ERROR;
    }
    return result;
}

template<class TreeNode>
int
ProgramAction::expand_program_variables(const TreeNode& tn,
					string& result,
					string& error_msg) const
{
    string word;
    string expanded_var;
    list<string> expanded_cmd;

    debug_msg("expand_program_variables() node %s program %s\n",
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
	    if (segment.empty()) {
		++iter;
		continue;
	    }
	}

	// Do variable expansion
	bool expand_done = false;
	if (segment[0] == '`') {
	    expand_done = tn.expand_expression(segment, expanded_var);
	    if (expand_done) {
		word += unquote(expanded_var);
	    } else {
		// Error
		error_msg = c_format("failed to expand expression \"%s\" "
				     "associated with node \"%s\"",
				     segment.c_str(), tn.path().c_str());
		return (XORP_ERROR);
	    }
	} else if (segment[0] == '$') {
	    expand_done = tn.expand_variable(segment, expanded_var, false);
	    if (expand_done) {
		word += unquote(expanded_var);
	    } else {
		// Error
		error_msg = c_format("failed to expand variable \"%s\" "
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

    result = unquote(expanded_cmd.front());

    return (XORP_OK);
}

string
ProgramAction::related_module() const
{
    return (_module_name);
}

string
ProgramAction::affected_module() const
{
    return (_module_name);
}

/***********************************************************************/

Command::Command(TemplateTreeNode& template_tree_node, const string& cmd_name)
    : BaseCommand(template_tree_node, cmd_name)
{
    debug_msg("Command constructor: %s\n", cmd_name.c_str());
}

Command::~Command()
{
    list<Action*>::const_iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	delete *iter;
    }
}

void
Command::add_action(const list<string>& action, const XRLdb* xrldb)
    throw (ParseError)
{
    string action_type;
    string error_msg;

    if (action.empty())
	return;		// XXX: no action to perform

    action_type = action.front();

    if (action_type == "xrl") {
	_actions.push_back(new XrlAction(_template_tree_node, action, xrldb));
	return;
    }

    if (action_type == "program") {
	_actions.push_back(new ProgramAction(_template_tree_node, action));
	return;
    }

    // Unknown action
    error_msg = c_format("Unknown action \"%s\". Expected actions: "
			 "\"%s\", \"%s\".",
			 action_type.c_str(), "xrl", "program");
    xorp_throw(ParseError, error_msg);
}

int
Command::execute(MasterConfigTreeNode& ctn, TaskManager& task_manager) const
{
    int result = XORP_OK;
    int actions = 0;

    list<Action*>::const_iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	do {
	    const XrlAction* xa = dynamic_cast<const XrlAction*>(*iter);
	    if (xa != NULL) {
		result = xa->execute(
		    ctn, task_manager,
		    callback(this, &Command::xrl_action_complete, &ctn, *iter));
		break;
	    }
	    const ProgramAction* pa = dynamic_cast<const ProgramAction*>(*iter);
	    if (pa != NULL) {
		result = pa->execute(
		    ctn, task_manager,
		    callback(this, &Command::program_action_complete, &ctn,
			     pa->stdout_variable_name(),
			     pa->stderr_variable_name()));
		break;
	    }
	    // Unrecognized command
	    XLOG_FATAL("Execute on unimplemented action type on node %s",
		       ctn.str().c_str());
	    break;
	} while (false);
	if (result != XORP_OK) {
	    debug_msg("command execute returning %d\n", result);
	    // XXX: how do we communicate this error back up
	    // return (-1);
	}
	actions++;
    }
    debug_msg("command execute returning XORP_OK\n");
    return actions;
}

void
Command::xrl_action_complete(const XrlError& err,
			     XrlArgs* xrl_args,
			     MasterConfigTreeNode* ctn,
			     Action* action) const
{
    debug_msg("Command::xrl_action_complete\n");

    if (err != XrlError::OKAY()) {
	ctn->command_status_callback(this, false);
	return;
    }

    if (process_xrl_action_return_arguments(xrl_args, ctn, action) != true) {
	ctn->command_status_callback(this, false);
	return;
    }

    ctn->command_status_callback(this, true);
}

bool
Command::process_xrl_action_return_arguments(XrlArgs* xrl_args,
					     MasterConfigTreeNode *ctn,
					     Action* action) const
{
    if ((xrl_args == NULL) || xrl_args->empty())
	return (true);

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

	if (returned_atom.has_fake_args()) {
	    ctn->value_to_node_existing_value(varname, value);
	    returned_atom.using_real_args();
	}
	ctn->set_variable(varname, value);
    }

    return (true);
}

void
Command::program_action_complete(bool success,
				 const string& command_stdout,
				 const string& command_stderr,
				 bool do_exec,
				 MasterConfigTreeNode* ctn,
				 string stdout_variable_name,
				 string stderr_variable_name) const
{
    debug_msg("Command::program_action_complete\n");

    if (do_exec) {
	if (! stdout_variable_name.empty())
	    if (ctn->set_variable(stdout_variable_name, command_stdout)
		!= true) {
		XLOG_ERROR("Failed to write the stdout of a program action "
			   "to variable %s", stdout_variable_name.c_str());
	    }
	if (! stderr_variable_name.empty()) {
	    if (ctn->set_variable(stderr_variable_name, command_stderr)
		!= true) {
		XLOG_ERROR("Failed to write the stderr of a program action "
			   "to variable %s", stderr_variable_name.c_str());
	    }
	}
    }

    if (success) {
	ctn->command_status_callback(this, true);
    } else {
	ctn->command_status_callback(this, false);
    }
}

set<string>
Command::affected_modules() const
{
    set<string> modules_set;
    list<Action*>::const_iterator iter;

    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	Action *a = (*iter);
	XrlAction* xa = dynamic_cast<XrlAction*>(a);
	if (xa != NULL) {
	    string affected = xa->affected_module();
	    modules_set.insert(affected);
	    continue;
	}
	ProgramAction* pa = dynamic_cast<ProgramAction*>(a);
	if (pa != NULL) {
	    string affected = pa->affected_module();
	    modules_set.insert(affected);
	    continue;
	}
    }
    return modules_set;
}

bool
Command::affects_module(const string& module) const
{
    // XXX: if we don't specify a module, we mean any module
    if (module == "")
	return true;

    set<string> modules_set = affected_modules();
    if (modules_set.find(module) == modules_set.end()) {
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
Command::expand_actions(string& error_msg)
{
    list<Action *>::iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	Action* action = *iter;
	if (action->expand_action(error_msg) != true)
	    return false;
    }

    return true;
}

bool
Command::check_referred_variables(string& error_msg) const
{
    list<Action *>::const_iterator iter;
    for (iter = _actions.begin(); iter != _actions.end(); ++iter) {
	const Action* action = *iter;
	if (action->check_referred_variables(error_msg) != true)
	    return false;
    }

    return true;
}



//
// Template explicit instatiation
//
template Xrl* XrlAction::expand_xrl_variables<class MasterConfigTreeNode>(
    const MasterConfigTreeNode& ctn,
    string& error_msg) const;
template Xrl* XrlAction::expand_xrl_variables<class TemplateTreeNode>(
    const TemplateTreeNode& ttn,
    string& error_msg) const;
template bool XrlAction::expand_vars<class MasterConfigTreeNode>(
    const MasterConfigTreeNode& ctn,
    const string& value, string& result) const;
template bool XrlAction::expand_vars<class TemplateTreeNode>(
    const TemplateTreeNode& ctn,
    const string& value, string& result) const;


template int ProgramAction::expand_program_variables<class MasterConfigTreeNode>(
    const MasterConfigTreeNode& ctn,
    string& result,
    string& error_msg) const;
template int ProgramAction::expand_program_variables<class TemplateTreeNode>(
    const TemplateTreeNode& ttn,
    string& result,
    string& error_msg) const;
