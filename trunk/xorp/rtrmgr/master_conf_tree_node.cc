// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/master_conf_tree_node.cc,v 1.4 2004/12/11 21:29:57 mjh Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "command_tree.hh"
#include "master_conf_tree_node.hh"
#include "module_command.hh"
#include "template_commands.hh"
#include "template_tree_node.hh"
#include "util.hh"


extern int booterror(const char *s) throw (ParseError);

MasterConfigTreeNode::MasterConfigTreeNode(bool verbose)
    : ConfigTreeNode(verbose)
#if 0
    // TODO: temporary commented-out. See the comments at the end
    // of the MasterConfigTreeNode declaration.
      _actions_pending(0),
      _actions_succeeded(true),
      _cmd_that_failed(NULL)
#endif
{

}

MasterConfigTreeNode::MasterConfigTreeNode(const string& nodename,
			       const string& path, 
			       const TemplateTreeNode* ttn,
			       MasterConfigTreeNode* parent,
			       uid_t user_id,
			       bool verbose)
    : ConfigTreeNode(nodename, path, ttn, parent, user_id, verbose)
#if 0
    // TODO: temporary commented-out. See the comments at the end
    // of the MasterConfigTreeNode declaration.
      _actions_pending(0),
      _actions_succeeded(true),
      _cmd_that_failed(NULL)
#endif
{
}

MasterConfigTreeNode::MasterConfigTreeNode(const MasterConfigTreeNode& ctn)
    : ConfigTreeNode(ctn)
#if 0
    // TODO: temporary commented-out. See the comments at the end
    // of the MasterConfigTreeNode declaration.
      _actions_pending(0),
      _actions_succeeded(true),
      _cmd_that_failed(NULL)
#endif
{
}


void
MasterConfigTreeNode::command_status_callback(const Command* cmd, bool success)
{
    debug_msg("command_status_callback node %s cmd %s\n",
	      _segname.c_str(), cmd->str().c_str());
    debug_msg("actions_pending = %d\n", _actions_pending);

    XLOG_ASSERT(_actions_pending > 0);

    if (_actions_succeeded && (success == false)) {
	_actions_succeeded = false;
	_cmd_that_failed = cmd;
    }
    _actions_pending--;
}


void
MasterConfigTreeNode::find_changed_modules(set<string>& changed_modules) const
{
    if ((_template_tree_node != NULL)
	&& (!_existence_committed || !_value_committed)) {
	const BaseCommand *base_cmd = NULL;
	const Command *cmd = NULL;
	set<string> modules;
	set<string>::const_iterator iter;

	if (_deleted) {
	    base_cmd = _template_tree_node->const_command("%delete");
	    if (base_cmd == NULL) {
		// No need to go to children
		return;
	    }
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();

	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		changed_modules.insert(*iter);
	    return;
	}

	if (!_existence_committed) {
	    if (! _template_tree_node->module_name().empty())
		changed_modules.insert(_template_tree_node->module_name());
	    base_cmd = _template_tree_node->const_command("%create");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_xrl_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	    base_cmd = _template_tree_node->const_command("%activate");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_xrl_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	} else if (!_value_committed) {
	    base_cmd = _template_tree_node->const_command("%set");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_xrl_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	    base_cmd = _template_tree_node->const_command("%update");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_xrl_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	}
    }

    list<ConfigTreeNode*>::const_iterator li;
    for (li = _children.begin(); li != _children.end(); ++li) {
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*li);
	child->find_changed_modules(changed_modules);
    }
}

void
MasterConfigTreeNode::find_active_modules(set<string>& active_modules) const
{
    if ((_template_tree_node != NULL) && (!_deleted)) {
	const BaseCommand *base_cmd;
	const Command *cmd;
	set<string> modules;
	set<string>::const_iterator iter;

	base_cmd = _template_tree_node->const_command("%create");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%activate");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%update");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%set");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
    }
    if (!_deleted) {
	list<ConfigTreeNode*>::const_iterator li;
	for (li = _children.begin(); li != _children.end(); ++li) {
	    MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*li);
	    child->find_active_modules(active_modules);
	}
    }
}

void
MasterConfigTreeNode::find_all_modules(set<string>& all_modules) const
{
    if (_template_tree_node != NULL) {
	const BaseCommand *base_cmd = NULL;
	const Command *cmd = NULL;
	set<string> modules;
	set<string>::const_iterator iter;

	base_cmd = _template_tree_node->const_command("%create");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%activate");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%update");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%set");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_xrl_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
    }
    list<ConfigTreeNode*>::const_iterator li;
    for (li = _children.begin(); li != _children.end(); ++li) {
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*li);
	child->find_all_modules(all_modules);
    }
}

void 
MasterConfigTreeNode::initialize_commit()
{
    _actions_pending = 0;
    _actions_succeeded = true;
    _cmd_that_failed = NULL;

    list<ConfigTreeNode *>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*iter);
	child->initialize_commit();
    }
}


bool
MasterConfigTreeNode::commit_changes(TaskManager& task_manager,
				     bool do_commit,
				     int depth, int last_depth,
				     string& result,
				     bool& needs_update)
{
    bool success = true;
    const BaseCommand *base_cmd = NULL;
    const Command *cmd = NULL;
    bool changes_made = false;

    debug_msg("*****COMMIT CHANGES node >%s< >%s<\n",
	      _path.c_str(), _value.c_str());
    if (do_commit)
	debug_msg("do_commit\n");
    if (_existence_committed == false)
	debug_msg("_existence_committed == false\n");
    if (_value_committed == false)
	debug_msg("_value_committed == false\n");

    // The root node has a NULL template
    if (_template_tree_node != NULL) {
	// Do we have to start any modules to implement this functionality
	base_cmd = _template_tree_node->const_command("%modinfo");
	const ModuleCommand* modcmd 
	    = dynamic_cast<const ModuleCommand*>(base_cmd);
	if (modcmd != NULL) {
	    if (modcmd->start_transaction(*this, task_manager) != XORP_OK) {
		result = "Start Transaction failed for module "
		    + modcmd->module_name() + "\n";
		return false;
	    }
	}
	if ((_existence_committed == false || _value_committed == false)) {
	    debug_msg("we have changes to handle\n");
	    // First handle deletions
	    if (_deleted) {
		if (do_commit == false) {
		    // No point in checking further
		    return true;
		} else {
		    //
		    // We expect a deleted node here to have previously
		    // had its existence committed.
		    // (_existence_committed == true) but its
		    // non-existence not previously committed
		    // (_value_committed == false)
		    //
		    XLOG_ASSERT(_existence_committed);
		    XLOG_ASSERT(!_value_committed);  
		    base_cmd = _template_tree_node->const_command("%delete");
		    if (base_cmd != NULL) {
			cmd = reinterpret_cast<const Command*>(base_cmd);
			int actions = cmd->execute(*this, task_manager);
			if (actions < 0) {
			    // Bad stuff happenned
			    // XXX now what?
			    result = "Something went wrong.\n";
			    result += "The problem was with \"" + path() + "\"\n";
			    result += "WARNING: Partially commited changes exist\n";
			    XLOG_WARNING("%s\n", result.c_str());
			    return false;
			}
			_actions_pending += actions;
			// No need to go on to delete children
			return true;
		    }
		}
	    } else {
		// Check any %allow commands that might prevent us
		// going any further
		base_cmd = _template_tree_node->const_command("%allow");
		if (base_cmd == NULL) {
		    // Try allow-range
		    base_cmd 
			= _template_tree_node->const_command("%allow-range");
		}
		if (base_cmd != NULL) {
		    const AllowCommand* allow_cmd;
		    debug_msg("found ALLOW command: %s\n",
			      cmd->str().c_str());
		    allow_cmd = dynamic_cast<const AllowCommand*>(base_cmd);
		    XLOG_ASSERT(allow_cmd != NULL);
		    string errmsg;
		    if (allow_cmd->verify_variable_value(*this, errmsg)
			!= true) {
			//
			// Commit_changes should always be run first
			// with do_commit not set, so there can be no
			// Allow command errors.
			//
			// XLOG_ASSERT(do_commit == false);
			result = c_format("Bad value for \"%s\": %s; ",
					  path().c_str(), errmsg.c_str());
			result += "No changes have been committed. ";
			result += "Correct this error and try again.";
			XLOG_WARNING("%s\n", result.c_str());
			return false;
		    }
		}

		//
		// Next, we run any "create" or "set" commands.
		// Note that if there is no "create" command, then we run
		// the "set" command instead.
		//
		base_cmd = NULL;
		if (_existence_committed == false)
		    base_cmd = _template_tree_node->const_command("%create");
		if (base_cmd == NULL)
		    base_cmd = _template_tree_node->const_command("%set");
		if (base_cmd == NULL) {
		    debug_msg("no appropriate command found\n");
		} else {
		    cmd = reinterpret_cast<const Command*>(base_cmd);
		    debug_msg("found commands: %s\n",
			      cmd->str().c_str());
		    int actions = cmd->execute(*this, task_manager);
		    if (actions < 0) {
			result = "Parameter error for \"" + path() + "\"\n";
			result += "No changes have been committed.\n";
			result += "Correct this error and try again.\n";
			XLOG_WARNING("%s\n", result.c_str());
			return false;
		    }
		    _actions_pending += actions;
		}
	    }
	}
    }

    list<ConfigTreeNode *>::iterator iter, prev_iter;
    iter = _children.begin();
    if (changes_made)
	last_depth = depth;
    while (iter != _children.end()) {
	prev_iter = iter;
	++iter;
	debug_msg("  child: %s\n", (*prev_iter)->path().c_str());
	string child_response;
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*prev_iter);
	success = child->commit_changes(task_manager, do_commit,
					       depth + 1, last_depth, 
					       child_response,
					       needs_update);
	result += child_response;
	if (success == false) {
	    return false;
	}
    }

    //
    // Take care of %activate and %update commands on the way back out.
    //
    do {
	if (_deleted)
	    break;
	if (_template_tree_node == NULL)
	    break;

	// The %activate command
	if (_existence_committed == false) {
	    base_cmd = _template_tree_node->const_command("%activate");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		debug_msg("found commands: %s\n", cmd->str().c_str());
		int actions = cmd->execute(*this, task_manager);
		if (actions < 0) {
		    result = "Parameter error for \"" + path() + "\"\n";
		    result += "No changes have been committed.\n";
		    result += "Correct this error and try again.\n";
		    XLOG_WARNING("%s\n", result.c_str());
		    return false;
		}
		_actions_pending += actions;
	    }
	    break;
	}

	// The %update command
	if (needs_update || (_value_committed == false)) {
	    base_cmd = _template_tree_node->const_command("%update");
	    if (base_cmd == NULL) {
		if (_value_committed == false)
		    needs_update = true;
	    } else {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		debug_msg("found commands: %s\n", cmd->str().c_str());
		needs_update = false;
		int actions = cmd->execute(*this, task_manager);
		if (actions < 0) {
		    result = "Parameter error for \"" + path() + "\"\n";
		    result += "No changes have been committed.\n";
		    result += "Correct this error and try again.\n";
		    XLOG_WARNING("%s\n", result.c_str());
		    return false;
		}
		_actions_pending += actions;
	    }
	    break;
	}

	break;
    } while (false);

    if (_template_tree_node != NULL) {
	base_cmd = _template_tree_node->const_command("%modinfo");
	if (base_cmd != NULL) {
	    const ModuleCommand* modcmd
		= dynamic_cast<const ModuleCommand*>(base_cmd);
	    if (modcmd != NULL) {
		if (modcmd->end_transaction(*this, task_manager) != XORP_OK) {
		    result = "End Transaction failed for module "
			+ modcmd->module_name() + "\n";
		    return false;
		}
	    }
	}
    }

    debug_msg("Result: %s\n", result.c_str());
    debug_msg("COMMIT, leaving node >%s<\n", _path.c_str());
    debug_msg("final node %s actions_pending = %d\n",
	      _segname.c_str(), _actions_pending);

    return success;
}

bool 
MasterConfigTreeNode::check_commit_status(string& response) const
{
    debug_msg("ConfigTreeNode::check_commit_status %s\n",
	      _segname.c_str());

    if ((_existence_committed == false) || (_value_committed == false)) {
	XLOG_ASSERT(_actions_pending == 0);
	if (_actions_succeeded == false) {
	    response = "WARNING: Commit Failed\n";
	    response += "  Error in " + _cmd_that_failed->str() + 
		" command for " + _path + "\n";
	    response += "  State may be partially committed - suggest reverting to previous state\n";
	    return false;
	}
	if (_deleted) {
	    const BaseCommand* cmd 
		= _template_tree_node->const_command("%delete");
	    if (cmd != NULL) {
		//
		// No need to check the children if we succeeded in
		// deleting this node.
		//
		return true;
	    }
	}
    }

    list<ConfigTreeNode *>::const_iterator iter;
    bool result = true;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*iter);
	debug_msg("  child: %s\n", child->path().c_str());
	result = child->check_commit_status(response);
	if (result == false)
	    return false;
    }

    return result;
}

void 
MasterConfigTreeNode::finalize_commit()
{
    debug_msg("MasterConfigTreeNode::finalize_commit %s\n",
	      _segname.c_str());

    if (_deleted) {
	debug_msg("node deleted\n");

	//
	// Delete the entire subtree. We expect a deleted node here
	// to have previously had its existence committed
	// (_existence_committed == true) but its non-existence not
	// previously committed (_value_committed == false)
	//
	XLOG_ASSERT(_existence_committed);
	XLOG_ASSERT(!_value_committed);  
	delete_subtree_silently();
	// No point in going further
	return;
    }
    if ((_existence_committed == false) || (_value_committed == false)) {
	debug_msg("node finalized\n");

	XLOG_ASSERT(_actions_pending == 0);
	XLOG_ASSERT(_actions_succeeded);
	_existence_committed = true;
	_value_committed = true;
	_committed_value = _value;
	_committed_user_id = _user_id;
	_committed_modification_time = _modification_time;
    }

    //
    // Note: finalize_commit may delete the child, so we need to be
    // careful the iterator stays valid.
    //
    list<ConfigTreeNode *>::iterator iter, prev_iter;
    iter = _children.begin(); 
    while (iter != _children.end()) {
	prev_iter = iter;
	++iter;
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*prev_iter);
	child->finalize_commit();
    }
}


