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

// $XORP: xorp/rtrmgr/template_commands.hh,v 1.39 2008/10/02 21:58:25 bms Exp $

#ifndef __RTRMGR_TEMPLATE_COMMANDS_HH__
#define __RTRMGR_TEMPLATE_COMMANDS_HH__






#include "libxorp/callback.hh"
#include "libxorp/run_command.hh"

#include "template_base_command.hh"
#include "master_conf_tree_node.hh"
#include "rtrmgr_error.hh"


class XorpClient;
class XRLdb;

class Action {
public:
    Action(TemplateTreeNode& template_tree_node, const list<string>& action);
    virtual ~Action() {};

    enum ActionCharType { VAR, NON_VAR, QUOTE, ASSIGN };

    string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }
    const TemplateTreeNode& template_tree_node() const { return _template_tree_node; }
    virtual bool expand_action(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;

protected:
    list<string> _action;
    list<string> _split_cmd;
    list<string> _referred_variables;
    TemplateTreeNode& _template_tree_node;
};

class XrlAction : public Action {
public:
    XrlAction(TemplateTreeNode& template_tree_node, const list<string>& action,
	      const XRLdb* xrldb) throw (ParseError);

    virtual bool expand_action(string& error_msg);
    int execute(const MasterConfigTreeNode& ctn, TaskManager& task_manager,
		XrlRouter::XrlCallback cb) const;
#if 0    
    template<class TreeNode> int expand_xrl_variables(const TreeNode& tn,
						      string& result,
						      string& error_msg) const;
#endif
    template<class TreeNode> Xrl* expand_xrl_variables(const TreeNode& tn,
						       string& error_msg) const;
    const string& request() const { return _request; }
    const string& xrl_return_spec() const { return _response; }
    string related_module() const;
    string affected_module() const;

private:
#ifdef DEBUG_XRLDB
    bool check_xrl_is_valid(const list<string>& action,
			    const XRLdb* xrldb, string& error_msg);
#endif
    template<class TreeNode> bool expand_vars(const TreeNode& tn,
					      const string& s, 
					      string& result) const;

    string		_module_name;
    list<string>	_split_request;
    string		_request;
    list<string>	_split_response;
    string		_response;
    const XRLdb*	_xrldb;
};

class ProgramAction : public Action {
public:
    ProgramAction(TemplateTreeNode& template_tree_node,
		  const list<string>& action) throw (ParseError);

    virtual bool expand_action(string& error_msg);
    int execute(const MasterConfigTreeNode&	ctn,
		TaskManager&			task_manager,
		TaskProgramItem::ProgramCallback program_cb) const;
    template<class TreeNode> int expand_program_variables(const TreeNode& tn,
							  string& result,
							  string& error_msg) const;
    string related_module() const;
    string affected_module() const;
    const string& request() const { return _request; }
    const string& stdout_variable_name() const { return _stdout_variable_name; }
    const string& stderr_variable_name() const { return _stderr_variable_name; }

private:
    bool check_program_is_valid(const list<string>& action, string& error_msg);
    void parse_program_response(const string& part) throw (ParseError);

    string		_module_name;
    list<string>	_split_request;
    string		_request;
    list<string>	_split_response;
    string		_response;
    string		_stdout_variable_name;
    string		_stderr_variable_name;
};

class Command : public BaseCommand {
public:
    Command(TemplateTreeNode& template_tree_node, const string& cmd_name);
    virtual ~Command();

    void add_action(const list<string>& action, const XRLdb* xrldb)
	throw (ParseError);
    int execute(MasterConfigTreeNode& ctn, TaskManager& task_manager) const;
    void xrl_action_complete(const XrlError& err,
			     XrlArgs* xrl_args,
			     MasterConfigTreeNode* ctn,
			     Action* action) const;
    bool process_xrl_action_return_arguments(XrlArgs* xrl_args,
					     MasterConfigTreeNode* ctn,
					     Action* action) const;
    void program_action_complete(bool success,
				 const string& command_stdout,
				 const string& command_stderr,
				 bool do_exec,
				 MasterConfigTreeNode* ctn,
				 string stdout_variable_name,
				 string stderr_variable_name) const;
    set<string> affected_modules() const;
    bool affects_module(const string& module) const;
    virtual string str() const;
    virtual bool expand_actions(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;

protected:
    list<Action*>	_actions;
};


#endif // __RTRMGR_TEMPLATE_COMMANDS_HH__
