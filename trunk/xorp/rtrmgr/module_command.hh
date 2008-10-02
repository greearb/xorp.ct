// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rtrmgr/module_command.hh,v 1.28 2008/07/23 05:11:42 pavlin Exp $

#ifndef __RTRMGR_MODULE_COMMAND_HH__
#define __RTRMGR_MODULE_COMMAND_HH__


#include "template_commands.hh"


class TaskManager;
class TemplateTreeNode;
class Validation;
class MasterConfigTreeNode;

class ModuleCommand : public Command {
public:
    ModuleCommand(TemplateTree& template_tree,
		  TemplateTreeNode& template_tree_node,
		  const string& cmd_name);
    ~ModuleCommand();

    void add_action(const list<string>& action,
		    const XRLdb& xrldb) throw (ParseError);
    virtual bool expand_actions(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;

    Validation* startup_validation(TaskManager& taskmgr) const;
    Validation* config_validation(TaskManager& taskmgr) const;
    Validation* ready_validation(TaskManager& taskmgr) const;
    Validation* shutdown_validation(TaskManager& taskmgr) const;
    Startup*	startup_method(TaskManager& taskmgr) const;
    Shutdown*	shutdown_method(TaskManager& taskmgr) const;

    const string& module_name() const { return _module_name; }
    const string& module_exec_path() const { return _module_exec_path; }
    const list<string>& depends() const { return _depends; }
    int start_transaction(MasterConfigTreeNode& ctn,
			  TaskManager& task_manager) const;
    int end_transaction(MasterConfigTreeNode& ctn,
			TaskManager& task_manager) const;
    string str() const;

    typedef XorpCallback3<void, bool, const string&, const string&>::RefPtr ProgramCallback;

protected:
    void xrl_action_complete(const XrlError& err,
			     XrlArgs* xrl_args,
			     MasterConfigTreeNode *ctn,
			     Action *action,
			     string cmd) const;

    void program_action_complete(bool success,
				 const string& stdout_output,
				 const string& stderr_output,
				 bool do_exec,
				 MasterConfigTreeNode *ctn,
				 Action *action,
				 string cmd) const;

private:
    TemplateTree&	_tt;
    string		_module_name;
    string		_module_exec_path;
    string		_default_target_name;
    list<string>	_depends;
    Action*		_start_commit;
    Action*		_end_commit;
    Action*		_status_method;
    Action*		_startup_method;
    Action*		_shutdown_method;
    bool		_execute_done;
    bool		_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_MODULE_COMMAND_HH__
