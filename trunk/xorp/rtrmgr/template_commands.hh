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

// $XORP: xorp/rtrmgr/template_commands.hh,v 1.17 2004/01/05 23:43:04 pavlin Exp $

#ifndef __RTRMGR_TEMPLATE_COMMANDS_HH__
#define __RTRMGR_TEMPLATE_COMMANDS_HH__

#include <map>
#include <list>
#include <set>
#include "libxorp/xorp.h"
#include "libxorp/ipvxnet.hh"
#include "libxorp/mac.hh"
#include "libxorp/callback.hh"
#include "rtrmgr_error.hh"
#include "conf_tree_node.hh"


class TemplateTree;
class TemplateTreeNode;
class XorpClient;
class XRLdb;

class Action {
public:
    Action(TemplateTreeNode& template_tree_node, const list<string>& action);
    virtual ~Action() {};

    string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }
    const TemplateTreeNode& template_tree_node() const { return _template_tree_node; }
    bool check_referred_variables(string& errmsg) const;

protected:
    list<string> _split_cmd;
    list<string> _referred_variables;

private:
    TemplateTreeNode& _template_tree_node;
};

class XrlAction : public Action {
public:
    XrlAction(TemplateTreeNode& template_tree_node, const list<string>& action,
	      const XRLdb& xrldb) throw (ParseError);

    int execute(const ConfigTreeNode& ctn, TaskManager& task_manager,
		XrlRouter::XrlCallback cb) const;
    template<class TreeNode> int expand_xrl_variables(const TreeNode& tn,
						      string& result,
						      string& errmsg) const;
    string xrl_return_spec() const { return _response; }
    string affected_module() const;
    inline const string& request() const { return _request; }

private:
    bool check_xrl_is_valid(const list<string>& action,
			    const XRLdb& xrldb, string& errmsg);
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
			 ConfigTreeNode* ctn) const;
    set<string> affected_xrl_modules() const;
    bool affects_module(const string& module) const;
    virtual string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }
    bool check_referred_variables(string& errmsg) const;

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
    bool verify_variable_value(const ConfigTreeNode& ctn,
			       string& errmsg) const;

    string str() const;

private:
    string		_varname;
    list<string>	_allowed_values;
};

#endif // __RTRMGR_TEMPLATE_COMMANDS_HH__
