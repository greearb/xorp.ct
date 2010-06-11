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

// $XORP: xorp/rtrmgr/template_base_command.hh,v 1.13 2008/10/02 21:58:25 bms Exp $

#ifndef __RTRMGR_TEMPLATE_BASE_COMMAND_HH__
#define __RTRMGR_TEMPLATE_BASE_COMMAND_HH__






#include "libxorp/callback.hh"
#include "rtrmgr_error.hh"
#include "config_operators.hh"

class TemplateTree;
class TemplateTreeNode;
class ConfigTreeNode;

class BaseCommand {
public:
    BaseCommand(TemplateTreeNode& template_tree_node, const string& cmd_name);
    virtual ~BaseCommand();

    virtual bool expand_actions(string& error_msg) = 0;
    virtual bool check_referred_variables(string& error_msg) const = 0;

    virtual string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }
    const TemplateTreeNode& template_tree_node() const { return _template_tree_node; }

protected:
    string		_cmd_name;
    TemplateTreeNode&	_template_tree_node;
};

class DummyBaseCommand : public BaseCommand {
public:
    DummyBaseCommand(TemplateTreeNode& template_tree_node,
		     const string& cmd_name)
	: BaseCommand(template_tree_node, cmd_name) {}
    virtual ~DummyBaseCommand() {}

    virtual bool expand_actions(string& error_msg) {
	UNUSED(error_msg);
	return (true);
    }

    virtual bool check_referred_variables(string& error_msg) const {
	UNUSED(error_msg);
	return (true);
    }

private:
};

class AllowCommand : public BaseCommand {
public:
    AllowCommand(TemplateTreeNode& template_tree_node, const string& cmd_name);

    virtual void add_action(const list<string>& action) throw (ParseError) = 0;
    virtual bool verify_variables(const ConfigTreeNode& ctn,
				  string& error_msg) const = 0;

    virtual string str() const = 0;
};

class AllowOptionsCommand : public AllowCommand {
public:
    AllowOptionsCommand(TemplateTreeNode& template_tree_node,
			const string& cmd_name);

    virtual bool expand_actions(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;
    virtual void add_action(const list<string>& action) throw (ParseError);
    virtual bool verify_variables(const ConfigTreeNode& 	ctn,
				  string& error_msg) const;

    virtual string str() const;

private:
    typedef map<string, string> Filter;	// Map between a value and help string
    map<string, Filter>	_filters;	// Map between a varname and a filter
};

class AllowOperatorsCommand : public AllowCommand {
public:
    AllowOperatorsCommand(TemplateTreeNode& template_tree_node,
			  const string& cmd_name);

    virtual bool expand_actions(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;
    virtual void add_action(const list<string>& action) throw (ParseError);
    virtual bool verify_variables(const ConfigTreeNode& 	ctn,
				  string& error_msg) const;
    virtual bool verify_variable_by_value(const ConfigTreeNode& ctn,
					  const string& value,
					  string& error_msg) const;
    virtual list<ConfigOperator> allowed_operators() const;

    virtual string str() const;

private:
    string		_varname;
    list<ConfigOperator>	_allowed_operators;
};

class AllowRangeCommand : public AllowCommand {
public:
    AllowRangeCommand(TemplateTreeNode&	template_tree_node,
		      const string& cmd_name);

    virtual bool expand_actions(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;
    virtual void add_action(const list<string>& action) throw (ParseError);
    virtual bool verify_variables(const ConfigTreeNode& ctn,
				  string& error_msg) const;

    virtual string str() const;

private:
    // Map between the pair of [lower, upper] boundaries and the help string
    typedef map<pair<int64_t, int64_t>, string> Filter;
    map<string, Filter> _filters;	// Map between a varname and a filter
};

#endif // __RTRMGR_TEMPLATE_BASE_COMMAND_HH__
