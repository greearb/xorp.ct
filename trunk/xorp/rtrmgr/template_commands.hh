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

// $XORP: xorp/rtrmgr/template_commands.hh,v 1.29 2005/07/11 23:11:45 pavlin Exp $

#ifndef __RTRMGR_TEMPLATE_COMMANDS_HH__
#define __RTRMGR_TEMPLATE_COMMANDS_HH__


#include <map>
#include <list>
#include <set>

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
    virtual bool expand_action(string& errmsg);
    bool check_referred_variables(string& errmsg) const;

protected:
    list<string> _action;
    list<string> _split_cmd;
    list<string> _referred_variables;
    TemplateTreeNode& _template_tree_node;
};

class XrlAction : public Action {
public:
    XrlAction(TemplateTreeNode& template_tree_node, const list<string>& action,
	      const XRLdb& xrldb) throw (ParseError);

    bool expand_action(string& errmsg);
    int execute(const MasterConfigTreeNode& ctn, TaskManager& task_manager,
		XrlRouter::XrlCallback cb) const;
#if 0    
    template<class TreeNode> int expand_xrl_variables(const TreeNode& tn,
						      string& result,
						      string& errmsg) const;
#endif
    template<class TreeNode> Xrl* expand_xrl_variables(const TreeNode& tn,
						       string& errmsg) const;
    inline const string& request() const { return _request; }
    const string& xrl_return_spec() const { return _response; }
    string related_module() const;
    string affected_module() const;

private:
    bool check_xrl_is_valid(const list<string>& action,
			    const XRLdb& xrldb, string& errmsg);
    template<class TreeNode> bool expand_vars(const TreeNode& tn,
					      const string& s, 
					      string& result) const;

    const XRLdb&	_xrldb;
    string		_module_name;
    list<string>	_split_request;
    string		_request;
    list<string>	_split_response;
    string		_response;
};

class ProgramAction : public Action {
public:
    ProgramAction(TemplateTreeNode& template_tree_node,
		  const list<string>& action) throw (ParseError);

    bool expand_action(string& errmsg);
    int execute(const MasterConfigTreeNode&	ctn,
		TaskManager&			task_manager,
		TaskProgramItem::ProgramCallback program_cb) const;
    template<class TreeNode> int expand_program_variables(const TreeNode& tn,
							  string& result,
							  string& errmsg) const;
    string related_module() const;
    string affected_module() const;
    inline const string& request() const { return _request; }
    const string& stdout_variable_name() const { return _stdout_variable_name; }
    const string& stderr_variable_name() const { return _stderr_variable_name; }

private:
    bool check_program_is_valid(const list<string>& action, string& errmsg);
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

    void add_action(const list<string>& action, const XRLdb& xrldb);
    int execute(MasterConfigTreeNode& ctn, TaskManager& task_manager) const;
    void xrl_action_complete(const XrlError& err,
			     XrlArgs* xrl_args,
			     MasterConfigTreeNode* ctn) const;
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
    bool expand_actions(string& errmsg);
    bool check_referred_variables(string& errmsg) const;

protected:
    list<Action*>	_actions;
};


#endif // __RTRMGR_TEMPLATE_COMMANDS_HH__
