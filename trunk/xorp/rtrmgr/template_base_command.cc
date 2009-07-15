// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#include <sstream>

#include "libxipc/xrl_router.hh"

#include "conf_tree_node.hh"
#include "task.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"
#include "config_operators.hh"

BaseCommand::BaseCommand(TemplateTreeNode& template_tree_node, 
			 const string& cmd_name)
    : _template_tree_node(template_tree_node)
{
    debug_msg("BaseCommand constructor: %s\n", cmd_name.c_str());
    _cmd_name = cmd_name;
}

BaseCommand::~BaseCommand()
{
}

string
BaseCommand::str() const
{
    string tmp = _cmd_name;
    return tmp;
}


// ----------------------------------------------------------------------------
// AllowCommand implementation

AllowCommand::AllowCommand(TemplateTreeNode& template_tree_node,
			   const string& cmd_name)
    : BaseCommand(template_tree_node, cmd_name)
{
}


// ----------------------------------------------------------------------------
// AllowOptionsCommand implementation

AllowOptionsCommand::AllowOptionsCommand(TemplateTreeNode& 	ttn,
					 const string&		cmd_name)
    : AllowCommand(ttn, cmd_name)
{
}

bool
AllowOptionsCommand::expand_actions(string& error_msg)
{
    map<string, Filter>::const_iterator iter;

    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;

	TemplateTreeNode* ttn;
	ttn = template_tree_node().find_varname_node(varname);
	if (ttn == NULL) {
	    // Error: invalid variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}

	const Filter& filter = iter->second;
	Filter::const_iterator iter2;
	for (iter2 = filter.begin(); iter2 != filter.end(); ++iter2) {
	    const string& value = iter2->first;
	    const string& help = iter2->second;
	    ttn->add_allowed_value(value, help);
	}
    }

    return (true);
}

bool
AllowOptionsCommand::check_referred_variables(string& error_msg) const
{
    map<string, Filter>::const_iterator iter;

    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;

	const TemplateTreeNode* ttn;
	ttn = template_tree_node().find_const_varname_node(varname);
	if (ttn == NULL) {
	    // Error: invalid variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}
    }

    return (true);
}

void
AllowOptionsCommand::add_action(const list<string>& action) throw (ParseError)
{
    string error_msg;
    string new_varname, new_value, new_help_keyword, new_help_str;
    size_t expected_parameters_n = 4;
    list<string> unparsed_action = action;

    //
    // Check the number of parameters
    //
    if (action.size() != expected_parameters_n) {
	error_msg = c_format("%%allow command with invalid number of "
			     "parameters: %u (expected %u)",
			     XORP_UINT_CAST(action.size()),
			     XORP_UINT_CAST(expected_parameters_n));
	xorp_throw(ParseError, error_msg);
    }

    //
    // Extract each parameter
    //
    new_varname = unparsed_action.front();
    unparsed_action.pop_front();
    new_value = unquote(unparsed_action.front());
    unparsed_action.pop_front();
    new_help_keyword = unparsed_action.front();
    unparsed_action.pop_front();
    new_help_str = unquote(unparsed_action.front());
    unparsed_action.pop_front();

    //
    // Verify all parameters
    //
    if (new_help_keyword != "%help") {
	error_msg = c_format("Invalid %%allow argument: %s "
			     "(expected \"%%help:\")",
			     new_help_keyword.c_str());
	xorp_throw(ParseError, error_msg);
    }

    //
    // Insert the new entry
    //
    map<string, Filter>::iterator iter;
    iter = _filters.find(new_varname);
    if (iter == _filters.end()) {
	// Insert a new map
	Filter new_filter;
	_filters.insert(make_pair(new_varname, new_filter));
	iter = _filters.find(new_varname);
    }
    XLOG_ASSERT(iter != _filters.end());
    Filter& filter = iter->second;

    // XXX: insert the new pair even if we overwrite an existing one
    filter.insert(make_pair(new_value, new_help_str));
}

bool
AllowOptionsCommand::verify_variables(const ConfigTreeNode& ctn,
				      string& error_msg) const
{
    string value;
    map<string, Filter>::const_iterator iter;

    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;
	if (ctn.expand_variable(varname, value, false) != true) {
	    // Error: cannot expand the variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}

	Filter filter = iter->second;	// XXX: create a copy we can manipulate
	if (filter.find(value) != filter.end())
	    continue;		// This variable is OK

	// Error: variable value is not allowed
	string full_varname;
	if (ctn.expand_variable_to_full_varname(varname, full_varname)
	    != true) {
	    // Error: cannot expand the variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}
	error_msg = c_format("Value \"%s\" is not a valid value for "
			     "variable \"%s\". ",
			     value.c_str(), full_varname.c_str());
	if (filter.size() == 1) {
	    error_msg += c_format("The only value allowed is %s.",
				  filter.begin()->first.c_str());
	} else {
	    error_msg += "Allowed values are: ";
	    bool is_first = true;
	    Filter::iterator iter2;
	    while (! filter.empty()) {
		if (is_first) {
		    is_first = false;
		} else {
		    if (filter.size() == 1)
			error_msg += " and ";
		    else
			error_msg += ", ";
		}
		iter2 = filter.begin();
		error_msg += iter2->first;
		filter.erase(iter2);
	    }
	    error_msg += ".";
	}
	return (false);
    }

    return (true);
}

string
AllowOptionsCommand::str() const
{
    string tmp;

    tmp = c_format("AllowOptionsCommand:\n");

    map<string, Filter>::const_iterator iter;
    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;
	const Filter& filter = iter->second;
	tmp += c_format("\tVarname: %s Allowed values:\n",
			varname.c_str());

	Filter::const_iterator iter2;
	for (iter2 = filter.begin(); iter2 != filter.end(); ++iter2) {
	    tmp += c_format("\t\tvalue: \"%s\"\thelp: \"%s\"\n",
			    iter2->first.c_str(), iter2->second.c_str());
	}
    }
    return tmp;
}


// ----------------------------------------------------------------------------
// AllowOperatorsCommand implementation

AllowOperatorsCommand::AllowOperatorsCommand(TemplateTreeNode& 	ttn,
					 const string&		cmd_name)
    : AllowCommand(ttn, cmd_name)
{
}

bool
AllowOperatorsCommand::expand_actions(string& error_msg)
{
    UNUSED(error_msg);
    return (true);	// XXX: nothing to do
}

bool
AllowOperatorsCommand::check_referred_variables(string& error_msg) const
{
    UNUSED(error_msg);
    return (true);	// XXX: nothing to do
}

void
AllowOperatorsCommand::add_action(const list<string>& action)
    throw (ParseError)
{
    string error_msg;
    size_t min_expected_parameters_n = 1;

    //
    // Check the number of parameters
    //
    if (action.size() < min_expected_parameters_n) {
	error_msg = c_format("%%allow-operator command with invalid number of "
			     "parameters: %u (expected at least %u)",
			     XORP_UINT_CAST(action.size()),
			     XORP_UINT_CAST(min_expected_parameters_n));
	xorp_throw(ParseError, error_msg);
    }

    //
    // Extract the parameters and add them
    //
    list<string>::const_iterator iter;
    for (iter = action.begin(); iter != action.end(); ++iter) {
	ConfigOperator op;
	string op_str = unquote(*iter);
	try {
	    op = lookup_operator(op_str);
	} catch (const ParseError& e) {
	    error_msg = c_format("%%allow-operator command with invalid "
				 "operator: %s", op_str.c_str());
	    xorp_throw(ParseError, error_msg);		 
	}
	if (find(_allowed_operators.begin(), _allowed_operators.end(), op)
	    == _allowed_operators.end()) {
	    _allowed_operators.push_back(op);
	}
    }
}

bool
AllowOperatorsCommand::verify_variables(const ConfigTreeNode& ctn,
					string& error_msg) const
{
    if (ctn.get_operator() == OP_NONE) {
	error_msg = c_format("Missing operator");
	return (false);
    }

    string op_str = ctn.show_operator();

    return (verify_variable_by_value(ctn, op_str, error_msg));
}

bool
AllowOperatorsCommand::verify_variable_by_value(const ConfigTreeNode& ctn,
						const string& value,
						string& error_msg) const
{
    ConfigOperator op;
    string op_str = unquote(value);

    try {
	op = lookup_operator(op_str);
    } catch (const ParseError& e) {
	error_msg = c_format("Invalid operator: %s", op_str.c_str());
	return (false);
    }

    list<ConfigOperator>::const_iterator iter;
    for (iter = _allowed_operators.begin();
	iter != _allowed_operators.end();
	++iter) {
	if (op == *iter)
	    return (true);		// OK
    }

    // Error: variable value is not allowed
    error_msg = c_format("Operator \"%s\" is not a valid value for node %s. ",
			 value.c_str(), ctn.segname().c_str());
    list<ConfigOperator> values = _allowed_operators;
    if (values.size() == 1) {
	error_msg += c_format("The only value allowed is %s.",
			      operator_to_str(values.front()).c_str());
    } else {
	error_msg += "Allowed values are: ";
	error_msg += operator_to_str(values.front()).c_str();
	values.pop_front();
	bool is_first = true;
	while (! values.empty()) {
	    if (is_first) {
		is_first = false;
	    } else {
		if (values.size() == 1)
		    error_msg += " and ";
		else
		    error_msg += ", ";
	    }
	    error_msg += operator_to_str(values.front()).c_str();
	    values.pop_front();
	}
	error_msg += ".";
    }
    return (false);
}

list<ConfigOperator>
AllowOperatorsCommand::allowed_operators() const
{
    list<ConfigOperator> result;

    list<ConfigOperator>::const_iterator iter;
    for (iter = _allowed_operators.begin();
	 iter != _allowed_operators.end();
	 ++iter) {
	const ConfigOperator& config_operator = *iter;
	result.push_back(config_operator);
    }

    return result;
}

string
AllowOperatorsCommand::str() const
{
    string tmp;

    tmp = "AllowOperatorsCommand: Allowed values: ";

    list<ConfigOperator>::const_iterator iter;
    for (iter = _allowed_operators.begin();
	 iter != _allowed_operators.end();
	 ++iter) {
	tmp += "  " + operator_to_str(*iter);
    }
    tmp += "\n";
    return tmp;
}

// ----------------------------------------------------------------------------
// AllowRangeCommand implementation

AllowRangeCommand::AllowRangeCommand(TemplateTreeNode& 	ttn,
				     const string&	cmd_name)
    : AllowCommand(ttn, cmd_name)
{
}

bool
AllowRangeCommand::expand_actions(string& error_msg)
{
    map<string, Filter>::const_iterator iter;

    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;

	TemplateTreeNode* ttn;
	ttn = template_tree_node().find_varname_node(varname);
	if (ttn == NULL) {
	    // Error: invalid variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}

	const Filter& filter = iter->second;
	Filter::const_iterator iter2;
	for (iter2 = filter.begin(); iter2 != filter.end(); ++iter2) {
	    const pair<int64_t, int64_t>& pair = iter2->first;
	    int64_t lower_value = pair.first;
	    int64_t upper_value = pair.second;
	    const string& help = iter2->second;
	    ttn->add_allowed_range(lower_value, upper_value, help);
	}
    }

    return (true);
}

bool
AllowRangeCommand::check_referred_variables(string& error_msg) const
{
    map<string, Filter>::const_iterator iter;

    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;

	const TemplateTreeNode* ttn;
	ttn = template_tree_node().find_const_varname_node(varname);
	if (ttn == NULL) {
	    // Error: invalid variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}
    }

    return (true);
}

void
AllowRangeCommand::add_action(const list<string>& action) throw (ParseError)
{
    string error_msg;
    string new_varname, new_lower_str, new_upper_str;
    string new_help_keyword, new_help_str;
    size_t expected_parameters_n = 5;
    list<string> unparsed_action = action;
    int64_t new_lower_value, new_upper_value;

    //
    // Check the number of parameters
    //
    if (action.size() != expected_parameters_n) {
	error_msg = c_format("%%allow-range command with invalid number of "
			     "parameters: %u (expected %u)",
			     XORP_UINT_CAST(action.size()),
			     XORP_UINT_CAST(expected_parameters_n));
	xorp_throw(ParseError, error_msg);
    }

    //
    // Extract each parameter
    //
    new_varname = unparsed_action.front();
    unparsed_action.pop_front();
    new_lower_str = unquote(unparsed_action.front());
    unparsed_action.pop_front();
    new_upper_str = unquote(unparsed_action.front());
    unparsed_action.pop_front();
    new_help_keyword = unparsed_action.front();
    unparsed_action.pop_front();
    new_help_str = unquote(unparsed_action.front());
    unparsed_action.pop_front();

    //
    // Verify all parameters
    //
    if (new_help_keyword != "%help") {
	error_msg = c_format("Invalid %%allow-range argument: %s "
			     "(expected \"%%help:\")",
			     new_help_keyword.c_str());
	xorp_throw(ParseError, error_msg);
    }

    //
    // Insert the new entry
    //
    map<string, Filter>::iterator iter;
    iter = _filters.find(new_varname);
    if (iter == _filters.end()) {
	// Insert a new map
	Filter new_filter;
	_filters.insert(make_pair(new_varname, new_filter));
	iter = _filters.find(new_varname);
    }
    XLOG_ASSERT(iter != _filters.end());
    Filter& filter = iter->second;

    new_lower_value = strtoll(new_lower_str.c_str(), (char **)NULL, 10);
    new_upper_value = strtoll(new_upper_str.c_str(), (char **)NULL, 10);
    if (new_lower_value > new_upper_value)
	swap(new_lower_value, new_upper_value);

    pair<int64_t, int64_t> new_range(new_lower_value, new_upper_value);

    // XXX: insert the new pair even if we overwrite an existing one
    filter.insert(make_pair(new_range, new_help_str));
}

bool
AllowRangeCommand::verify_variables(const ConfigTreeNode& ctn,
				    string& error_msg) const
{
    string value;
    map<string, Filter>::const_iterator iter;

    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;
	if (ctn.expand_variable(varname, value, false) != true) {
	    // Error: cannot expand the variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}

	Filter filter = iter->second;	// XXX: create a copy we can manipulate

	bool is_accepted = true;
	Filter::iterator iter2;
	int64_t ival = strtoll(value.c_str(), (char **)NULL, 10);
	int64_t lower_value = 0;
	int64_t upper_value = 0;
	if (! filter.empty())
	    is_accepted = false;
	//
	// XXX: it is sufficient for the variable's value to belong to any
	// of the allowed ranges.
	//
	for (iter2 = filter.begin(); iter2 != filter.end(); ++iter2) {
	    const pair<int64_t, int64_t>& range = iter2->first;
	    lower_value = range.first;
	    upper_value = range.second;
	    if ((ival >= lower_value) && (ival <= upper_value)) {
		is_accepted = true;
		break;
	    }
	}

	if (is_accepted)
	    continue;		// This variable is OK

	// Error: variable value is not allowed
	string full_varname;
	if (ctn.expand_variable_to_full_varname(varname, full_varname)
	    != true) {
	    // Error: cannot expand the variable
	    error_msg = c_format("Variable \"%s\" is not defined.",
				 varname.c_str());
	    return (false);
	}
	error_msg = c_format("Value \"%s\" is not a valid value for "
			     "variable \"%s\". ",
			     value.c_str(), full_varname.c_str());
	if (filter.size() == 1) {
	    const pair<int64_t, int64_t>& range = filter.begin()->first;
	    error_msg += c_format("The only range allowed is ");
	    ostringstream ost;
	    ost << "[" << range.first << ".." << range.second << "]";
	    error_msg += c_format("%s.", ost.str().c_str());
	} else {
	    error_msg += "Allowed ranges are: ";
	    bool is_first = true;
	    while (! filter.empty()) {
		if (is_first) {
		    is_first = false;
		} else {
		    if (filter.size() == 1)
			error_msg += " and ";
		    else
			error_msg += ", ";
		}
		map<pair<int64_t, int64_t>, string>::iterator iter2;
		iter2 = filter.begin();
		const pair<int64_t, int64_t>& range = iter2->first;
		ostringstream ost;
		ost << "[" << range.first << ".." << range.second << "]";
		error_msg += c_format("%s.", ost.str().c_str());
		filter.erase(iter2);
	    }
	    error_msg += ".";
	}
	return (false);
    }

    return (true);
}

string
AllowRangeCommand::str() const
{
    string tmp;

    tmp = c_format("AllowRangeCommand:\n");

    map<string, Filter>::const_iterator iter;
    for (iter = _filters.begin(); iter != _filters.end(); ++iter) {
	const string& varname = iter->first;
	const Filter& filter = iter->second;
	tmp += c_format("\tVarname: %s Allowed ranges:\n",
			varname.c_str());

	Filter::const_iterator iter2;
	for (iter2 = filter.begin(); iter2 != filter.end(); ++iter2) {
	    const pair<int64_t, int64_t>& range = iter2->first;
	    ostringstream ost;
	    ost << "[" << range.first << ".." << range.second << "]";
	    tmp += c_format("\t\trange: %s\thelp: \"%s\"\n",
			    ost.str().c_str(),
			    iter2->second.c_str());
	}
    }

    return (tmp);
}
