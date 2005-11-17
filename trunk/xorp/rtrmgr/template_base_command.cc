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

#ident "$XORP: xorp/rtrmgr/template_base_command.cc,v 1.10 2005/11/10 23:55:40 pavlin Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "conf_tree_node.hh"
#include "task.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"
#include "config_operators.hh"
extern "C" ConfigOperator lookup_comparator(const string& s);
extern "C" ConfigOperator lookup_modifier(const string& s);

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

void
AllowOptionsCommand::add_action(const list<string>& action) throw (ParseError)
{
    string error_msg;
    string new_varname, new_value, new_help_keyword, new_help_string;
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
    new_help_string = unquote(unparsed_action.front());
    unparsed_action.pop_front();

    //
    // Verify all parameters
    //
    if ((_varname.size() != 0) && (_varname != new_varname)) {
	error_msg = c_format("Currently only one variable per node can be "
			     "specified using \"%%allow\" commands");
	xorp_throw(ParseError, error_msg);
    }
    if (new_help_keyword != "%help") {
	error_msg = c_format("Invalid %%allow argument: %s "
			     "(expected \"%%help:\")",
			     new_help_keyword.c_str());
	xorp_throw(ParseError, error_msg);
    }

    //
    // Insert the new entry
    //
    if (_allowed_values.find(new_value) == _allowed_values.end()) {
	_varname = new_varname;
	_allowed_values.insert(make_pair(new_value, new_help_string));
    }
}

bool
AllowOptionsCommand::verify_variable(const ConfigTreeNode& ctn,
				     string& error_msg) const
{
    string value;

    if (ctn.expand_variable(_varname, value) == false) {
	// Error: cannot expand the variable
	error_msg = c_format("Variable \"%s\" is not defined.",
			     _varname.c_str());
	return false;
    }

    return (verify_variable_by_value(ctn, value, error_msg));
}

bool
AllowOptionsCommand::verify_variable_by_value(const ConfigTreeNode& ctn,
					      const string& value,
					      string& error_msg) const
{
    map<string, string>::iterator iter;

    if (_allowed_values.find(value) != _allowed_values.end())
	return true;			// OK

    // Error: variable value is not allowed
    string full_varname;
    if (ctn.expand_variable_to_full_varname(_varname, full_varname) == false) {
	// Error: cannot expand the variable
	error_msg = c_format("Variable \"%s\" is not defined.",
			     _varname.c_str());
	return false;
    }
    error_msg = c_format("Value \"%s\" is not a valid value for "
			 "variable \"%s\". ",
			 value.c_str(), full_varname.c_str());
    map<string, string> values = _allowed_values;
    if (values.size() == 1) {
	error_msg += c_format("The only value allowed is %s.",
			      values.begin()->first.c_str());
    } else {
	error_msg += "Allowed values are: ";
	iter = values.begin();
	error_msg += iter->first;
	values.erase(iter);
	while (! values.empty()) {
	    if (values.size() == 1)
		error_msg += " and ";
	    else
		error_msg += ", ";
	    iter = values.begin();
	    error_msg += iter->first;
	    values.erase(iter);
	}
	error_msg += ".";
    }
    return false;
}

string
AllowOptionsCommand::str() const
{
    string tmp;

    tmp = c_format("AllowOptionsCommand: varname = %s\n"
		   "\tAllowed values:\n",
		   _varname.c_str());

    map<string, string>::const_iterator iter;
    for (iter = _allowed_values.begin();
	 iter != _allowed_values.end();
	 ++iter) {
	tmp += c_format("\tvalue: \"%s\"\thelp: \"%s\"\n",
			iter->first.c_str(), iter->second.c_str());
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

void
AllowOperatorsCommand::add_action(const list<string>& action) throw (ParseError)
{
    debug_msg("AllowOperatorsCommand::add_action\n");

    if (action.size() < 1) {
	xorp_throw(ParseError, "Allow command with less than one parameter");
    }

    list<string>::const_iterator iter;
    
    for (iter = action.begin(); iter != action.end(); iter++) {
	ConfigOperator op;
	op = lookup_operator(unquote(*iter));
	_allowed_operators.push_back(op);
    }
}

bool
AllowOperatorsCommand::verify_variable(const ConfigTreeNode& ctn,
				       string& error_msg) const
{
    string opstr = ctn.show_operator();

    return (verify_variable_by_value(ctn, opstr, error_msg));
}

bool
AllowOperatorsCommand::verify_variable_by_value(const ConfigTreeNode& ctn,
						const string& value,
						string& error_msg) const
{
    ConfigOperator op;
    op = lookup_operator(unquote(value));

    list<ConfigOperator>::const_iterator iter;
    for (iter = _allowed_operators.begin();
	iter != _allowed_operators.end();
	++iter) {
	if (op == *iter)
	    return true;		// OK
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
	while (! values.empty()) {
	    if (values.size() == 1)
		error_msg += " and ";
	    else
		error_msg += ", ";
	    error_msg += operator_to_str(values.front()).c_str();
	    values.pop_front();
	}
	error_msg += ".";
    }
    return false;
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

void
AllowRangeCommand::add_action(const list<string>& action) throw (ParseError)
{
    debug_msg("AllowRangeCommand::add_action\n");

    if (action.size() < 3) {
	xorp_throw(ParseError,
		   "Allow range command with less than three parameters");
    }

    list<string>::const_iterator iter;
    iter = action.begin();
    if ((_varname.size() != 0) && (_varname != *iter)) {
	xorp_throw(ParseError,
		   "Currently only one variable per node can be specified "
		   "using \"allow range\" commands");
    }
    _varname = *iter;
    ++iter;
    _lower = atoi(unquote(*iter).c_str());
    ++iter;
    _upper = atoi(unquote(*iter).c_str());

    if (_lower > _upper)
	swap(_lower, _upper);
}

bool
AllowRangeCommand::verify_variable(const ConfigTreeNode& ctn,
				   string& error_msg) const
{
    string value;

    if (ctn.expand_variable(_varname, value) == false) {
	// Error: cannot expand the variable
	error_msg = c_format("Variable \"%s\" is not defined.",
			     _varname.c_str());
	return false;
    }

    return (verify_variable_by_value(ctn, value, error_msg));
}

bool
AllowRangeCommand::verify_variable_by_value(const ConfigTreeNode& ctn,
					    const string& value,
					    string& error_msg) const
{
    int32_t ival = atoi(value.c_str());

    if ((ival >= _lower) && (ival <= _upper)) {
	return true;
    }

    // Error: variable value is not allowed
    string full_varname;
    if (ctn.expand_variable_to_full_varname(_varname, full_varname) == false) {
	// Error: cannot expand the variable
	error_msg = c_format("Variable \"%s\" is not defined.",
			     _varname.c_str());
	return false;
    }
    error_msg = c_format("Value %s is outside valid range, %d...%d, "
			 "for variable \"%s\".",
			 value.c_str(),
			 XORP_INT_CAST(_lower),
			 XORP_INT_CAST(_upper),
			 full_varname.c_str());
    return false;
}

string
AllowRangeCommand::str() const
{
    return c_format("AllowRangeCommand: varname = \"%s\"\n       Allowed range: %d...%d",
		    _varname.c_str(),
		    XORP_INT_CAST(_lower),
		    XORP_INT_CAST(_upper));
}

