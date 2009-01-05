// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/rtrmgr/master_conf_tree_node.cc,v 1.28 2008/10/02 21:58:23 bms Exp $"

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
    : ConfigTreeNode(verbose),
      _actions_pending(0),
      _actions_succeeded(true),
      _cmd_that_failed(NULL)
{

}

MasterConfigTreeNode::MasterConfigTreeNode(const string& nodename,
					   const string& path, 
					   const TemplateTreeNode* ttn,
					   MasterConfigTreeNode* parent,
					   const ConfigNodeId& node_id,
					   uid_t user_id,
					   bool verbose)
    : ConfigTreeNode(nodename, path, ttn, parent, node_id, user_id, 
		     /* clientid */ 0, verbose),
      _actions_pending(0),
      _actions_succeeded(true),
      _cmd_that_failed(NULL)
{
}

MasterConfigTreeNode::MasterConfigTreeNode(const MasterConfigTreeNode& ctn)
    : ConfigTreeNode(ctn),
      _actions_pending(0),
      _actions_succeeded(true),
      _cmd_that_failed(NULL)
{
}

ConfigTreeNode*
MasterConfigTreeNode::create_node(const string& segment, const string& path,
				  const TemplateTreeNode* ttn, 
				  ConfigTreeNode* parent_node, 
				  const ConfigNodeId& node_id,
				  uid_t user_id,
				  uint32_t clientid,
				  bool verbose)
{
    UNUSED(clientid);
    MasterConfigTreeNode *new_node, *parent;
    parent = dynamic_cast<MasterConfigTreeNode *>(parent_node);

    // sanity check - all nodes in this tree should be Master nodes
    if (parent_node != NULL)
	XLOG_ASSERT(parent != NULL);

    new_node = new MasterConfigTreeNode(segment, path, ttn, parent, 
					node_id, user_id, verbose);
    return reinterpret_cast<ConfigTreeNode*>(new_node);
}

ConfigTreeNode*
MasterConfigTreeNode::create_node(const ConfigTreeNode& ctn) {
    MasterConfigTreeNode *new_node;
    const MasterConfigTreeNode *orig;

    // sanity check - all nodes in this tree should be Master nodes
    orig = dynamic_cast<const MasterConfigTreeNode *>(&ctn);
    XLOG_ASSERT(orig != NULL);

    new_node = new MasterConfigTreeNode(*orig);
    return new_node;
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
    if (_template_tree_node != NULL) {
	// XXX: ignore deprecated subtrees
	if (_template_tree_node->is_deprecated())
	    return;
    }

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
	    modules = cmd->affected_modules();

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
		modules = cmd->affected_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	    base_cmd = _template_tree_node->const_command("%activate");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	} else if (!_value_committed) {
	    base_cmd = _template_tree_node->const_command("%set");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_modules();
		for (iter = modules.begin(); iter != modules.end(); ++iter)
		    changed_modules.insert(*iter);
	    }
	    base_cmd = _template_tree_node->const_command("%update");
	    if (base_cmd != NULL) {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		modules = cmd->affected_modules();
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
    if (_template_tree_node != NULL) {
	// XXX: ignore deprecated subtrees
	if (_template_tree_node->is_deprecated())
	    return;
    }

    if (_deleted) {
	// XXX: ignore deleted subtrees
	return;
    }

    if (_template_tree_node != NULL) {
	const BaseCommand *base_cmd;
	const Command *cmd;
	set<string> modules;
	set<string>::const_iterator iter;

	//
	// XXX: If the module's top-level node is not deleted explicitly, then
	// the module is considered still active.
	//
	if (_template_tree_node->is_module_root_node())
	    active_modules.insert(_template_tree_node->module_name());

	base_cmd = _template_tree_node->const_command("%create");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%activate");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%update");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%set");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		active_modules.insert(*iter);
	}
    }

    //
    // Process the subtree
    //
    list<ConfigTreeNode*>::const_iterator li;
    for (li = _children.begin(); li != _children.end(); ++li) {
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*li);
	child->find_active_modules(active_modules);
    }
}

void
MasterConfigTreeNode::find_all_modules(set<string>& all_modules) const
{
    if (_template_tree_node != NULL) {
	// XXX: ignore deprecated subtrees
	if (_template_tree_node->is_deprecated())
	    return;
    }

    if (_template_tree_node != NULL) {
	const BaseCommand *base_cmd = NULL;
	const Command *cmd = NULL;
	set<string> modules;
	set<string>::const_iterator iter;

        //
        // XXX: If the module's top-level node is in the configuration, then
        // the module is added to the list.
        //
        if (_template_tree_node->is_module_root_node())
            all_modules.insert(_template_tree_node->module_name());

	base_cmd = _template_tree_node->const_command("%create");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%activate");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%update");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
	    for (iter = modules.begin(); iter != modules.end(); ++iter)
		all_modules.insert(*iter);
	}
	base_cmd = _template_tree_node->const_command("%set");
	if (base_cmd != NULL) {
	    cmd = reinterpret_cast<const Command*>(base_cmd);
	    modules = cmd->affected_modules();
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
MasterConfigTreeNode::children_changed()
{
    if (_existence_committed == false || _value_committed == false)
	return true;
    list<ConfigTreeNode *>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*iter);
	if (child->children_changed()) {
	    return true;
	}
    }
    return false;
}

bool
MasterConfigTreeNode::commit_changes(TaskManager& task_manager,
				     bool do_commit,
				     int depth, int last_depth,
				     string& error_msg,
				     bool& needs_activate,
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

    if (_template_tree_node != NULL) {
	// XXX: ignore deprecated subtrees
	if (_template_tree_node->is_deprecated())
	    return success;
    }

    // Don't bother to recurse if no child node has any changes to commit.
    // Calling this every time is rather inefficient, but for the
    // scales we're talking about, it's not a big deal.
    if (children_changed() == false) {
	debug_msg("No children changed\n");
	return success;
    } else {
	debug_msg("Children have changed\n");
    }


    // The root node has a NULL template
    if (_template_tree_node != NULL) {
	// Do we have to start any modules to implement this functionality
	base_cmd = _template_tree_node->const_command("%modinfo");
	const ModuleCommand* modcmd 
	    = dynamic_cast<const ModuleCommand*>(base_cmd);
	if (modcmd != NULL) {
	    if (modcmd->start_transaction(*this, task_manager) != XORP_OK) {
		error_msg = c_format("Start Transaction failed for "
				     "module %s\n",
				     modcmd->module_name().c_str());
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
			    error_msg = "Something went wrong.\n";
			    error_msg += c_format("The problem was with \"%s\"\n",
						  path().c_str());
			    error_msg += "WARNING: Partially commited changes exist\n";
			    XLOG_WARNING("%s\n", error_msg.c_str());
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
		    if (allow_cmd->verify_variables(*this, error_msg)
			!= true) {
			//
			// Commit_changes should always be run first
			// with do_commit not set, so there can be no
			// Allow command errors.
			//
			// XLOG_ASSERT(do_commit == false);
			error_msg = c_format("Bad value for \"%s\": %s; ",
					     path().c_str(),
					     error_msg.c_str());
			error_msg += "No changes have been committed. ";
			error_msg += "Correct this error and try again.";
			XLOG_WARNING("%s\n", error_msg.c_str());
			return false;
		    }
		}
		// Check that the operator is OK
		base_cmd = 
		    _template_tree_node->const_command("%allow-operator");
		if (base_cmd == NULL) {
		    /* no explicit command, so only ":" is allowed */
		    if (_operator != OP_NONE && _operator != OP_ASSIGN) {
			error_msg = c_format("Bad operator for \"%s\": "
					     "operator %s was specified, "
					     "only ':' is allowed\n",
					     path().c_str(), 
					     operator_to_str(_operator).c_str());
			error_msg += "No changes have been committed. ";
			error_msg += "Correct this error and try again.";
			XLOG_WARNING("%s\n", error_msg.c_str());
			return false;
		    }
		} else {
		    const AllowCommand* allow_cmd;
		    debug_msg("found ALLOW command: %s\n",
			      cmd->str().c_str());
		    allow_cmd = dynamic_cast<const AllowCommand*>(base_cmd);
		    XLOG_ASSERT(allow_cmd != NULL);
		    if (allow_cmd->verify_variables(*this, error_msg)
			!= true) {
			error_msg = c_format("Bad operator for \"%s\": %s; ",
					     path().c_str(),
					     error_msg.c_str());
			error_msg += "No changes have been committed. ";
			error_msg += "Correct this error and try again.";
			XLOG_WARNING("%s\n", error_msg.c_str());
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
			error_msg = c_format("Parameter error for \"%s\"\n",
					     path().c_str());
			error_msg += "No changes have been committed.\n";
			error_msg += "Correct this error and try again.\n";
			XLOG_WARNING("%s\n", error_msg.c_str());
			return false;
		    }
		    _actions_pending += actions;
		}
	    }
	}
    }

    list<ConfigTreeNode *> sorted_children = _children;
    sort_by_template(sorted_children);

    list<ConfigTreeNode *>::iterator iter, prev_iter;
    iter = sorted_children.begin();
    if (changes_made)
	last_depth = depth;
    while (iter != sorted_children.end()) {
	prev_iter = iter;
	++iter;
	debug_msg("  child: %s\n", (*prev_iter)->path().c_str());
	string child_error_msg;
	MasterConfigTreeNode *child = (MasterConfigTreeNode*)(*prev_iter);
	success = child->commit_changes(task_manager, do_commit,
					depth + 1, last_depth, 
					child_error_msg,
					needs_activate,
					needs_update);
	error_msg += child_error_msg;
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
	if (needs_activate || (_existence_committed == false)) {
	    base_cmd = _template_tree_node->const_command("%activate");
	    if (base_cmd == NULL) {
		if (_existence_committed == false)
		    needs_activate = true;
	    } else {
		cmd = reinterpret_cast<const Command*>(base_cmd);
		debug_msg("found commands: %s\n", cmd->str().c_str());
		needs_activate = false;
		int actions = cmd->execute(*this, task_manager);
		if (actions < 0) {
		    error_msg = c_format("Parameter error for \"%s\"\n",
					 path().c_str());
		    error_msg += "No changes have been committed.\n";
		    error_msg += "Correct this error and try again.\n";
		    XLOG_WARNING("%s\n", error_msg.c_str());
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
		    error_msg = c_format("Parameter error for \"%s\"\n",
					 path().c_str());
		    error_msg += "No changes have been committed.\n";
		    error_msg += "Correct this error and try again.\n";
		    XLOG_WARNING("%s\n", error_msg.c_str());
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
		    error_msg = c_format("End Transaction failed for module %s\n",
					 modcmd->module_name().c_str());
		    return false;
		}
	    }
	}
    }

    debug_msg("Result: %s\n", error_msg.c_str());
    debug_msg("COMMIT, leaving node >%s<\n", _path.c_str());
    debug_msg("final node %s actions_pending = %d\n",
	      _segname.c_str(), _actions_pending);

    return success;
}

bool 
MasterConfigTreeNode::check_commit_status(string& error_msg) const
{
    debug_msg("ConfigTreeNode::check_commit_status %s\n",
	      _segname.c_str());

    if ((_existence_committed == false) || (_value_committed == false)) {
	XLOG_ASSERT(_actions_pending == 0);
	if (_actions_succeeded == false) {
	    error_msg = "WARNING: Commit Failed\n";
	    error_msg += c_format("  Error in %s command for %s\n",
				  _cmd_that_failed->str().c_str(),
				  _path.c_str());
	    error_msg += c_format("  State may be partially committed - "
				  "suggest reverting to previous state\n");
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
	result = child->check_commit_status(error_msg);
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
	_deleted = false;
	_committed_value = _value;
	_committed_operator = _operator;
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


