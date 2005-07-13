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

#ident "$XORP: xorp/rtrmgr/template_base_command.cc,v 1.6 2005/07/11 22:09:44 pavlin Exp $"

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

string BaseCommand::str() const
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
    debug_msg("AllowOptionsCommand::add_action\n");

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
	_allowed_values.push_back(unquote(*iter));
    }
}

bool
AllowOptionsCommand::verify_variable(const ConfigTreeNode& ctn,
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
AllowOptionsCommand::str() const
{
    string tmp;

    tmp = "AllowOptionsCommand: varname = " + _varname + " \n       Allowed values: ";

    list<string>::const_iterator iter;
    for (iter = _allowed_values.begin();
	 iter != _allowed_values.end();
	 ++iter) {
	tmp += "  " + *iter;
    }
    tmp += "\n";
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
				       string& errmsg) const
{
    ConfigOperator op = ctn.get_operator();
    list<ConfigOperator>::const_iterator iter;
    for (iter = _allowed_operators.begin();
	iter != _allowed_operators.end();
	++iter) {
	if (op == *iter)
	    return true;		// OK
    }

    // Error: variable value is not allowed
    string opstr = ctn.show_operator();
    errmsg = c_format("Operator \"%s\" is not a valid value for node %s. ",
		      opstr.c_str(), ctn.segname().c_str());
    list<ConfigOperator> values = _allowed_operators;
    if (values.size() == 1) {
	errmsg += c_format("The only value allowed is %s.",
			   operator_to_str(values.front()).c_str());
    } else {
	errmsg += "Allowed values are: ";
	errmsg += operator_to_str(values.front()).c_str();
	values.pop_front();
	while (! values.empty()) {
	    if (values.size() == 1)
		errmsg += " and ";
	    else
		errmsg += ", ";
	    errmsg += operator_to_str(values.front()).c_str();
	    values.pop_front();
	}
	errmsg += ".";
    }
    return false;
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
	xorp_throw(ParseError, "Allow range command with less than three parameters");
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
AllowRangeCommand::verify_variable(const ConfigTreeNode&	ctn,
					 string& errmsg) const
{
    string value;

    if (ctn.expand_variable(_varname, value) == false) {
	// Error: cannot expand the variable
	errmsg = c_format("Variable %s is not defined.", _varname.c_str());
	return false;
    }

    int32_t ival = atoi(value.c_str());
    if (ival < _lower || ival > _upper) {
	errmsg = c_format("Value %s is outside valid range, %d...%d,  for variable %s.",
			  value.c_str(),
			  XORP_INT_CAST(_lower),
			  XORP_INT_CAST(_upper),
			  _varname.c_str());
	return false;
    }

    return true;
}

string
AllowRangeCommand::str() const
{
    return c_format("AllowRangeCommand: varname = %s\n       Allowed range: %d...%d",
		    _varname.c_str(),
		    XORP_INT_CAST(_lower),
		    XORP_INT_CAST(_upper));
}

