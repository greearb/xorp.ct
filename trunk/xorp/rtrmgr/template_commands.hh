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

// $XORP: xorp/rtrmgr/template_commands.hh,v 1.15 2003/11/19 23:04:51 pavlin Exp $

#ifndef __RTRMGR_TEMPLATE_COMMANDS_HH__
#define __RTRMGR_TEMPLATE_COMMANDS_HH__

#include <map>
#include <list>
#include <set>
#include "libxorp/xorp.h"
#include "libxorp/ipvxnet.hh"
#include "libxorp/mac.hh"
#include "libxorp/callback.hh"
#include "parse_error.hh"
#include "conf_tree_node.hh"


class TemplateTree;
class TemplateTreeNode;
class XorpClient;
class XRLdb;

class UnexpandedVariable {
public:
    UnexpandedVariable(const string& varname, const string& node)
	: _varname(varname), _node(node) {};
    string str() { return "Unexpanded variable " + _varname +
		       " on node " + _node + "."; }

private:
    string _varname;
    string _node;
};

class Action {
public:
    Action(TemplateTreeNode& template_tree_node,
	   const list<string>& cmd) throw (ParseError);
    virtual ~Action() {};

    string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }
    const TemplateTreeNode& template_tree_node() const { return _template_tree_node; }

protected:
    list<string> _split_cmd;

private:
    TemplateTreeNode& _template_tree_node;
};

class XrlAction : public Action {
public:
    XrlAction(TemplateTreeNode& template_tree_node, const list<string>& cmd,
	      const XRLdb& xrldb) throw (ParseError);

    int execute(const ConfigTreeNode& ctn, TaskManager& task_manager,
		XrlRouter::XrlCallback cb) const;
    template<class TreeNode> string expand_xrl_variables(const TreeNode& tn) const;
    string xrl_return_spec() const { return _response; }
    string affected_module(const ConfigTreeNode& ctn) const;
    inline const string& request() const { return _request; }

private:
    void check_xrl_is_valid(const list<string>& cmd,
			    const XRLdb& xrldb) const throw (ParseError);
    list<string>	_split_request;
    string		_request;
    list<string>	_split_response;
    string		_response;
};

class Command {
public:
    Command(TemplateTreeNode& template_tree_node, const string &cmd_name);
    virtual ~Command();

    void add_action(const list<string>& action, const XRLdb& xrldb);
    int execute(ConfigTreeNode& ctn, TaskManager& task_manager) const;
    void action_complete(const XrlError& err, XrlArgs* xrl_args,
			 ConfigTreeNode* ctn);
    set<string> affected_xrl_modules(const ConfigTreeNode& ctn) const;
    bool affects_module(const ConfigTreeNode& ctn, const string& module) const;
    virtual string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }

protected:
    string		_cmd_name;
    list<Action*>	_actions;

private:
    TemplateTreeNode&	_template_tree_node;
};

class AllowCommand : public Command {
public:
    AllowCommand(TemplateTreeNode& template_tree_node, const string& cmd_name);

    void add_action(const list<string>& action) throw (ParseError);
    int execute(const ConfigTreeNode& ctn) const throw (ParseError);
    const list<string>& allowed_values() const { return _allowed_values; }

    string str() const;

private:
    string		_varname;
    list<string>	_allowed_values;
};

#endif // __RTRMGR_TEMPLATE_COMMANDS_HH__
