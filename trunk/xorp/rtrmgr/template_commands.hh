// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/template_commands.hh,v 1.4 2003/02/22 00:41:37 mjh Exp $

#ifndef __RTRMGR_TEMPLATE_COMMANDS_HH__
#define __RTRMGR_TEMPLATE_COMMANDS_HH__

#include <map>
#include <list>
#include <set>
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/ipvxnet.hh"
#include "libxorp/mac.hh"
#include "libxorp/callback.hh"
#include "parse_error.hh"
#include "conf_tree_node.hh"

class XRLdb;
class XorpClient;
class TemplateTree;
class ModuleManager;

class UnexpandedVariable {
public:
    UnexpandedVariable(const string& varname, const string& node)
	: _varname(varname), _node(node) {};
    string str() {return "Unexpanded variable " + _varname +
		      " on node " + _node + "."; }
private:
    string _varname, _node;
};


class Action {
public:
    Action(const list<string> &cmd) throw (ParseError);
    virtual ~Action() {};
    string str() const;
protected:
    list<string> _split_cmd;
};

class XrlAction : public Action {
public:
    XrlAction(const list<string> &cmd, const XRLdb& xrldb) throw (ParseError);
    int execute(const ConfigTreeNode& ctn,
		XorpClient *xclient, uint tid, bool no_execute,
		XCCommandCallback cb) const;
    string expand_xrl_variables(const ConfigTreeNode& ctn) const;
    string xrl_return_spec() const {return _response;}
    string affected_module() const;
    
private:
    void check_xrl_is_valid(list<string> cmd, 
			    const XRLdb& xrldb) const throw (ParseError);
    list <string> _split_request;
    string _request;
    list <string> _split_response;
    string _response;
};

class Command {
public:
    Command(const string &cmd_name);
    virtual ~Command();
    void add_action(const list <string> &action,
			    const XRLdb& xrldb);
    int execute(ConfigTreeNode& ctn,
		XorpClient *xclient, uint tid, bool no_execute) const ;
    void action_complete(const XrlError& err, 
			 XrlArgs* xrlargs,
			 ConfigTreeNode *ctn);
    set <string> affected_xrl_modules() const;
    bool affects_module(const string& module) const;
    virtual string str() const;
protected:
    string _cmd_name;
    list <Action*> _actions;
};

class ModuleCommand : public Command {
public:
    ModuleCommand(const string &cmd_name, TemplateTree *ct);
    ~ModuleCommand() {}
    void add_action(const list <string> &action,
		    const XRLdb& xrldb) throw (ParseError);
    void set_path(const string &path);
    void set_depends(const string &depends);
    int execute(XorpClient *xclient, uint tid,
		ModuleManager *module_manager, 
		bool no_execute, 
		bool no_commit) const;
    void exec_complete(const XrlError& err, 
		       XrlArgs* xrlargs);
    void action_complete(const XrlError& err, 
			 XrlArgs* args,
			 ConfigTreeNode *ctn,
			 Action *action,
			 string cmd);
    const string& name() const {return _modname;}
    const string& path() const {return _modpath;}
    const list <string>& depends() const {return _depends;}
    int start_transaction(ConfigTreeNode& ctn,
			  XorpClient *xclient,  uint tid, 
			  bool no_execute, bool no_commit) const;
    int end_transaction(ConfigTreeNode& ctn,
			XorpClient *xclient,  uint tid, 
			bool no_execute, bool no_commit) const;
    string str() const;
private:
    TemplateTree *_tt;
    string _modname;
    string _modpath;
    list <string> _depends;
    Action *_startcommit;
    Action *_endcommit;
};

class AllowCommand : public Command {
public:
    AllowCommand(const string &cmd_name);

    void add_action(const list <string> &action) throw (ParseError);

    int execute(const ConfigTreeNode& ctn) const 
	throw (ParseError);

    const list <string>& allowed_values() const {
	return _allowed_values;
    }

    string str() const;
private:
    string _varname;
    list <string> _allowed_values;
};

#endif // __RTRMGR_TEMPLATE_COMMANDS_HH__
