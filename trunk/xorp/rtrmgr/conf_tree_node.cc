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

#ident "$XORP: xorp/rtrmgr/conf_tree_node.cc,v 1.18 2003/05/30 23:57:09 mjh Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_VARIABLES
#define DEBUG_COMMIT

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"

#include "conf_tree_node.hh"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "module_command.hh"
#include "command_tree.hh"
#include "util.hh"

extern int booterror(const char *s);

ConfigTreeNode::ConfigTreeNode() 
{
    _parent = NULL;
    _template = NULL;
    _deleted = false;
    _has_value = false;
    _existence_committed = false;
    _value_committed = false;
    _user_id = 0;
    _actions_pending = 0;
    _actions_succeeded = true;
    _cmd_that_failed = NULL;
    _on_parent_path = false;
}

ConfigTreeNode::ConfigTreeNode(const string& nodename,
			       const string &path, 
			       const TemplateTreeNode *ttn,
			       ConfigTreeNode *parent,
			       uid_t user_id)
{
    _template = ttn;
    _deleted = false;
    _has_value = false;
    _value = "";
    _committed_value = "";
    _segname = nodename;
    _path = path;
    _parent = parent;
    _user_id = user_id;
    _committed_user_id = 0;
    TimerList::system_gettimeofday(&_modification_time);
    _committed_modification_time = TimeVal::ZERO();
    _existence_committed = false;
    _value_committed = false;
    _actions_pending = 0;
    _actions_succeeded = true;
    _cmd_that_failed = NULL;
    _on_parent_path = false;
    parent->add_child(this);
    
    //printf("new ConfigTreeNode at %x (%s, %d)\n", this, path.c_str(), type);
}

ConfigTreeNode::ConfigTreeNode(const ConfigTreeNode& ctn) {
    _template = ctn._template;
    _deleted = ctn._deleted;
    _has_value = ctn._has_value;
    _value = ctn._value;
    _committed_value = ctn._committed_value;
    _segname = ctn._segname;
    _path = ctn._path;
    _parent = NULL;
    _user_id = ctn._user_id;
    _committed_user_id = ctn._committed_user_id;
    _modification_time = ctn._modification_time;
    _committed_modification_time = ctn._committed_modification_time;
    _existence_committed = ctn._existence_committed;
    _value_committed = ctn._value_committed;
    _actions_pending = 0;
    _actions_succeeded = true;
    _cmd_that_failed = NULL;
    _on_parent_path = false;
}

ConfigTreeNode::~ConfigTreeNode() {
    while (!_children.empty()) {
	delete _children.front();
	_children.pop_front();
    }
    //detect accidental reuse
    _parent = (ConfigTreeNode *)0xbad;
    _template = (TemplateTreeNode *)0xbad;
}

bool ConfigTreeNode::operator==(const ConfigTreeNode& them) const {
    //this comparison only cares about the declarative state of the
    //node, not about the ephemeral state such as when it was
    //modified, deleted, or who changed it last
    if (_segname != them.segname())
	return false;
    if (_parent != NULL && const_cast<ConfigTreeNode&>(them).parent()!= NULL) {
	if (is_tag() != them.is_tag())
	    return false;
	if (_has_value && (_value != them.value())) {
	    return false;
	}
    }
    if (is_leaf() != them.is_leaf())
	return false;
    if (_template != them.template_node())
	return false;
#ifdef DEBUG_EQUALS
    printf("Equal nodes:\n");
    printf("Node1: %s\n", str().c_str());
    printf("Node2: %s\n\n", them.str().c_str());
#endif
    return true;
}

void 
ConfigTreeNode::add_child(ConfigTreeNode *child) {
    _children.push_back(child);
}

void 
ConfigTreeNode::remove_child(ConfigTreeNode *child) {
    /* it's faster to implement this ourselves than use _children.remove()
       because _children has no duplicates */
    list <ConfigTreeNode*>::iterator i;
    for (i=_children.begin(); i!= _children.end(); i++) {
	if ((*i)==child) {
	    _children.erase(i);
	    return;
	}
    }

    XLOG_UNREACHABLE();
}

void
ConfigTreeNode::add_default_children() {
    if (_template == NULL)
	return;

    //find all the template nodes for our children
    set <const TemplateTreeNode*> childrens_templates;
    list <ConfigTreeNode *>::const_iterator i;
    for (i=_children.begin(); i!=_children.end(); i++) {
	childrens_templates.insert((*i)->template_node());
    }

    //now go through all the children of our template node, and find
    //any that are leaf nodes that have default values, and that we
    //don't already have an instance of among our children
    list <TemplateTreeNode*>::const_iterator tci;
    for (tci = _template->children().begin();
	 tci != _template->children().end();
	 tci++) {
	if ((*tci)->has_default()) {
	    if (childrens_templates.find(*tci) 
		== childrens_templates.end()) {
		string name = (*tci)->segname();
		string path = _path + " " + name;
		ConfigTreeNode *new_node 
		    = new ConfigTreeNode(name, path, *tci, this, _user_id);
		new_node->set_value((*tci)->default_str(), _user_id);
	    }
	}
    }
}

void 
ConfigTreeNode::recursive_add_default_children() {
    list <ConfigTreeNode *>::const_iterator i;
    for (i=_children.begin(); i!=_children.end(); i++) {
	(*i)->recursive_add_default_children();
    }

    add_default_children();
}

void
ConfigTreeNode::set_value(const string &value, uid_t user_id) {
    _value = value;
    _has_value = true;
    _value_committed = false;
    _user_id = user_id;
    TimerList::system_gettimeofday(&_modification_time);
}

void
ConfigTreeNode::command_status_callback(Command *cmd, bool success) {
    debug_msg("command_status_callback node %s cmd %s\n",
	   _segname.c_str(), cmd->str().c_str());
    debug_msg("actions_pending = %d\n", _actions_pending);
    assert(_actions_pending > 0);
    if (_actions_succeeded && success==false) {
	_actions_succeeded = false;
	_cmd_that_failed = cmd;
    }
    _actions_pending--;
}

bool
ConfigTreeNode::merge_deltas(uid_t user_id,
			     const ConfigTreeNode& delta_node, 
			     bool provisional_change,
			     string& response) {
    assert(_segname == delta_node.segname());
    if (_template != NULL) {
	assert(type() == delta_node.type());
	if (type() == NODE_VOID) {
	} else {
	    if (delta_node.is_leaf()) {
		if (_value != delta_node.value()) {
		    _has_value = true;
		    if (provisional_change) {
			_committed_value = _value;
			_value = delta_node.value();
			_committed_user_id = _user_id;
			_user_id = user_id;
			_committed_modification_time = _modification_time;
			TimerList::system_gettimeofday(&_modification_time);
			_value_committed = false;
		    } else {
			_committed_value = delta_node.value();
			_value = delta_node.value();
			_committed_user_id = delta_node.user_id();
			_user_id = delta_node.user_id();
			_committed_modification_time = 
			    delta_node.modification_time();
			_modification_time = 
			    delta_node.modification_time();
			_value_committed = true;
		    }
		    _actions_pending = 0;
		    _actions_succeeded = true;
		    _cmd_that_failed = NULL;
		}
	    }
	}
    }
    
    list <ConfigTreeNode*>::const_iterator dci;
    for (dci = delta_node.const_children().begin();
	 dci != delta_node.const_children().end();
	 dci++) {
	ConfigTreeNode *delta_child = *dci;
	
	bool delta_child_done = false;
	list <ConfigTreeNode*>::iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ci++) {
	    ConfigTreeNode *my_child = *ci;
	    if (my_child->segname() == delta_child->segname()) {
		delta_child_done = true;
		bool success = my_child->merge_deltas(user_id, *delta_child,
						      provisional_change,
						      response);
		if (success == false) {
		    //if something failed, abort the merge
		    return false;
		}
		break;
	    }
	}
	if (delta_child_done == false) {
	    ConfigTreeNode *new_node;
	    new_node = new ConfigTreeNode(delta_child->segname(),
					  delta_child->path(),
					  delta_child->template_node(),
					  this,
					  user_id);
	    if (!provisional_change)
		new_node->set_existence_committed(true);
	    new_node->merge_deltas(user_id, *delta_child, 
				   provisional_change, response);
	}
    }
    return true;
}

bool
ConfigTreeNode::merge_deletions(uid_t user_id,
				const ConfigTreeNode& deletion_node, 
				bool provisional_change,
				string& response) {
    assert(_segname == deletion_node.segname());
    if (_template != NULL) {
	assert(type() == deletion_node.type());
	if (deletion_node.const_children().empty()) {
	    if (provisional_change) {
		_deleted = true;
		_value_committed = false;
	    } else {
		delete_subtree_silently();
	    }
	    return true;
	}
    }
    
    list <ConfigTreeNode*>::const_iterator dci;
    for (dci = deletion_node.const_children().begin();
	 dci != deletion_node.const_children().end();
	 dci++) {
	ConfigTreeNode *deletion_child = *dci;
	
	bool deletion_child_done = false;
	list <ConfigTreeNode*>::iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ci++) {
	    ConfigTreeNode *my_child = *ci;
	    if (my_child->segname() == deletion_child->segname()) {
		deletion_child_done = true;
		bool success = my_child->merge_deletions(user_id, 
							 *deletion_child,
							 provisional_change,
							 response);
		if (success == false) {
		    //if something failed, abort the merge
		    return false;
		}
		break;
	    }
	}
	if (deletion_child_done == false) {
	    response = "Failed to delete node:\n   " + deletion_node.path()
		+ "\nNode does not exist.\n";
	    return false;
	}
    }
    return true;
}

void
ConfigTreeNode::find_changed_modules(set <string>& changed_modules) const {
    if ((_template != NULL)
	&& (!_existence_committed || !_value_committed)) {
	const Command *cmd;
	set <string> modules;
	set <string>::const_iterator i;
	if (_deleted) {
	    cmd = _template->const_command("%delete");
	    if (cmd == NULL)
		//no need to go to children
		return;
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		changed_modules.insert(*i);
	    return;
	}
	if (!_existence_committed) {
	    cmd = _template->const_command("%create");
	    if (cmd != NULL) {
		modules = cmd->affected_xrl_modules();
		for (i = modules.begin(); i!=modules.end(); i++)
		    changed_modules.insert(*i);
	    }
	    cmd = _template->const_command("%activate");
	    if (cmd != NULL) {
		modules = cmd->affected_xrl_modules();
		for (i = modules.begin(); i!=modules.end(); i++)
		    changed_modules.insert(*i);
	    }
	} else if (!_value_committed) {
	    cmd = _template->const_command("%set");
	    if (cmd != NULL) {
		modules = cmd->affected_xrl_modules();
		for (i = modules.begin(); i!=modules.end(); i++)
		    changed_modules.insert(*i);
	    }
	}
    }
    list <ConfigTreeNode*>::const_iterator i;
    for (i=_children.begin(); i!=_children.end(); i++) {
	(*i)->find_changed_modules(changed_modules);
    }
}

void
ConfigTreeNode::find_active_modules(set <string>& active_modules) const {
    if ((_template != NULL) && (!_deleted)) {
	const Command *cmd;
	set <string> modules;
	set <string>::const_iterator i;
	cmd = _template->const_command("%create");
	if (cmd != NULL) {
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		active_modules.insert(*i);
	}
	cmd = _template->const_command("%activate");
	if (cmd != NULL) {
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		active_modules.insert(*i);
	}
	cmd = _template->const_command("%set");
	if (cmd != NULL) {
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		active_modules.insert(*i);
	}
    }
    if (!_deleted) {
	list <ConfigTreeNode*>::const_iterator i;
	for (i=_children.begin(); i!=_children.end(); i++) {
	    (*i)->find_active_modules(active_modules);
	}
    }
}

void
ConfigTreeNode::find_all_modules(set <string>& all_modules) const {
    if (_template != NULL) {
	const Command *cmd;
	set <string> modules;
	set <string>::const_iterator i;
	cmd = _template->const_command("%create");
	if (cmd != NULL) {
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		all_modules.insert(*i);
	}
	cmd = _template->const_command("%activate");
	if (cmd != NULL) {
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		all_modules.insert(*i);
	}
	cmd = _template->const_command("%set");
	if (cmd != NULL) {
	    modules = cmd->affected_xrl_modules();
	    for (i = modules.begin(); i!=modules.end(); i++)
		all_modules.insert(*i);
	}
    }
    list <ConfigTreeNode*>::const_iterator i;
    for (i=_children.begin(); i!=_children.end(); i++) {
	(*i)->find_all_modules(all_modules);
    }
}

void 
ConfigTreeNode::initialize_commit() {
    //we don't need to initialize anything, but we're paranoid, so we
    //verify everything's OK first.
    assert(_actions_pending==0);
    assert(_actions_succeeded == true);
    list <ConfigTreeNode *>::iterator i;
    for (i = _children.begin(); i != _children.end(); i++) 
	(*i)->initialize_commit();
}

bool
ConfigTreeNode::commit_changes(TaskManager& task_manager,
			       bool do_commit, 
			       int depth, int last_depth,
			       string& result) {
    bool success = true;
    const Command *cmd;
#ifdef DEBUG_COMMIT
    printf("*****COMMIT CHANGES node >%s< >%s<\n", _path.c_str(),
	   _value.c_str());
    if (do_commit)
	printf("do_commit\n");
    if (_existence_committed == false)
	printf("_existence_committed == false\n");
    if (_value_committed == false)
	printf("_value_committed == false\n");
#endif
    bool changes_made = false;

    //the root node has a NULL template
    if (_template != NULL) {
	//do we have to start any modules to implement this
	//functionality
	cmd = _template->const_command("%modinfo");
	const ModuleCommand* modcmd 
	    = dynamic_cast<const ModuleCommand*>(cmd);
	if (modcmd != NULL) {
	    if (modcmd->start_transaction(*this, task_manager) != XORP_OK) {
		result = "Start Transaction failed for module "
		    + modcmd->name() + "\n";
		return false;
	    }
	}
	if ((_existence_committed == false || _value_committed == false)) {
#ifdef DEBUG_COMMIT
	    printf("we have changes to handle\n");
#endif
	    //first handle deletions
	    if (_deleted) {
		if (do_commit == false) {
		    //no point in checking further
		    return true;
		} else {
		    //We expect a deleted node here to have previously
		    //had its existence committed
		    //(_existence_committed == true) but its
		    //non-existence not previously committed
		    //(_value_committed == false)
		    assert(_existence_committed);
		    assert(!_value_committed);  
		    cmd = _template->const_command("%delete");
		    if (cmd != NULL) {
			int actions = 
			    cmd->execute(*this, task_manager);
			if (actions < 0) {
			    //bad stuff happenned
			    //XXX now what?
			    result = "Something went wrong.\n";
			    result += "The problem was with \"" + path() + "\"\n";
			    result += "WARNING: Partially commited changes exist\n";
#ifdef DEBUG_COMMIT
			    printf("%s\n", result.c_str());
#endif
			    return false;
			}
			_actions_pending += actions;
			//no need to go on to delete children
			return true;
		    } 
		}
	    } else {
		//check any allow commands that might prevent us
		//going any further
		cmd = _template->const_command("%allow");
		if (cmd != NULL) {
#ifdef DEBUG_COMMIT
		    printf("found ALLOW command: %s\n", cmd->str().c_str());
#endif
		    try {
			((const AllowCommand*)cmd)->execute(*this);
		    } catch (ParseError(&pe)) {
			//commit_changes should always be run first
			//with do_commit not set, so there can be no
			//Allow command errors.
			assert(do_commit == false);
			result = "Bad value for \"" + path() + "\"\n";
			result += "No changes have been committed.\n";
			result += "Correct this error and try again.\n";
#ifdef DEBUG_COMMIT
			printf("%s\n", result.c_str());
#endif
			return false;
		    }
		}


		//next, we run any "create" or "set" commands
		cmd = _template->const_command("%create");
		if (cmd == NULL)
		    cmd = _template->const_command("%set");
		if (cmd == NULL) {
#ifdef DEBUG_COMMIT
		    printf("no appropriate command found\n");
#endif
		} else {
#ifdef DEBUG_COMMIT
		    printf("found commands: %s\n", cmd->str().c_str());
#endif
		    int actions = 
			cmd->execute(*this, task_manager);
		    if (actions < 0) {
			result = "Parameter error for \"" + path() + "\"\n";
			result += "No changes have been committed.\n";
			result += "Correct this error and try again.\n";
#ifdef DEBUG_COMMIT
			printf("%s\n", result.c_str());
#endif
			return false;
		    }
		    _actions_pending += actions;
		}
	    }
	}
    }


    list <ConfigTreeNode *>::iterator iter, previter;
    iter = _children.begin();
    if (changes_made)
	last_depth = depth;
    while (iter != _children.end()) {
	previter = iter;
	++iter;
#ifdef DEBUG_COMMIT
	printf("  child: %s\n", (*previter)->path().c_str());
#endif
	string child_response;
	success =
	    (*previter)->commit_changes(task_manager, do_commit,
					depth+1, last_depth, 
					child_response);
	result += child_response;
	if (success == false) {
	    return false;
	}
    }

    if (!_deleted
	&& (_existence_committed == false || _value_committed == false)) {
	if (_template != NULL) {
	    //finally, on the way back out, we run the %activate commands
	    cmd = _template->const_command("%activate");
	    if (cmd != NULL) {
#ifdef DEBUG_COMMIT
		printf("found commands: %s\n", cmd->str().c_str());
#endif
		int actions = 
		    cmd->execute(*this, task_manager);
		if (actions < 0) {
		    result = "Parameter error for \"" + path() + "\"\n";
		    result += "No changes have been committed.\n";
		    result += "Correct this error and try again.\n";
#ifdef DEBUG_COMMIT
		    printf("%s\n", result.c_str());
#endif
		    return false;
		}
		_actions_pending += actions;
	    }
	}
    }
    if (_template != NULL) {
	cmd = _template->const_command("%modinfo");
	if (cmd != NULL) {
	    const ModuleCommand* modcmd 
		= dynamic_cast<const ModuleCommand*>(cmd);
	    if (modcmd != NULL) {
		if (modcmd->end_transaction(*this, task_manager) != XORP_OK) {
		    result = "End Transaction failed for module "
			+ modcmd->name() + "\n";
		    return false;
		}
	    }
	}
    }
#ifdef DEBUG_COMMIT
    printf("Result: %s\n", result.c_str());
    printf("COMMIT, leaving node >%s<\n", _path.c_str());
    printf("final node %s actions_pending = %d\n", _segname.c_str(), _actions_pending);
#endif
    return success;
}

bool 
ConfigTreeNode::check_commit_status(string &response) const {
#ifdef DEBUG_COMMIT
    printf("ConfigTreeNode::check_commit_status %s\n",
	   _segname.c_str());
#endif
    if ((_existence_committed == false) || (_value_committed == false)) {
	assert(_actions_pending == 0);
	if (_actions_succeeded == false) {
	    response = "WARNING: Commit Failed\n";
	    response += "  Error in " + _cmd_that_failed->str() + 
		" command for " + _path + "\n";
	    response += "  State may be partially committed - suggest reverting to previous state\n";
	    return false;
	}
	if (_deleted) {
	    const Command *cmd = _template->const_command("%delete");
	    if (cmd != NULL) {
		//no need to check the children if we succeeded in
		//deleting this node
		return true;
	    }
	}
    }

    list <ConfigTreeNode *>::const_iterator iter;
    bool result = true;
    for (iter = _children.begin(); iter != _children.end(); iter++) {
#ifdef DEBUG_COMMIT
	printf("  child: %s\n", (*iter)->path().c_str());
#endif
	result = (*iter)->check_commit_status(response);
	if (result == false)
	    return false;
    }

    return result;
}

void 
ConfigTreeNode::finalize_commit() {
#ifdef DEBUG_COMMIT
    printf("ConfigTreeNode::finalize_commit %s\n", _segname.c_str());
#endif
    if (_deleted) {
#ifdef DEBUG_COMMIT
	printf("node deleteded\n");
#endif
	//Delete the entire subtree. We expect a deleted node here
	//to have previously had its existence committed
	//(_existence_committed == true) but its non-existence not
	//previously committed (_value_committed == false)
	assert(_existence_committed);
	assert(!_value_committed);  
	delete_subtree_silently();
	//no point in going further
	return;
    }
    if ((_existence_committed == false) || (_value_committed == false)) {
#ifdef DEBUG_COMMIT
	printf("node finalized\n");
#endif
	assert(_actions_pending == 0);
	assert(_actions_succeeded);
	_existence_committed = true;
	_value_committed = true;
	_committed_value = _value;
	_committed_user_id = _user_id;
	_committed_modification_time = _modification_time;
    }

    //note: finalize_commit may delete the child, so we need to be
    //careful the iterator stays valid
    list <ConfigTreeNode *>::iterator iter, previter;
    iter = _children.begin(); 
    while(iter != _children.end()) {
	previter = iter;
	iter++;
	(*previter)->finalize_commit();
    }
}

string
ConfigTreeNode::discard_changes(int depth, int last_depth) {
#ifdef DEBUG_COMMIT
    printf("DISCARD CHANGES node >%s<\n", _path.c_str());
#endif
    string result;
    bool changes_made = false;
    //the root node has a NULL template
    if (_template != NULL) {
	if (_existence_committed == false) {
	    result =  show_subtree(depth, /*XXX*/depth*2, true, false);
	    delete_subtree_silently();
	    return result;
	} else if (_value_committed == false) {
#ifdef DEBUG_COMMIT
	    printf("discarding changes from node %s\n",
		   _path.c_str());
#endif
	    _value_committed = true;
	    _user_id = _committed_user_id;
	    _value = _committed_value;
	    _modification_time = _committed_modification_time;
	    _deleted = false;
	    result = s();
	    if (is_leaf()) 
		result += "\n";
	    else
		result += " {\n";
	    changes_made = true;
	} 
    }


    list <ConfigTreeNode *>::iterator iter, previter;
    iter = _children.begin();
    if (changes_made)
	last_depth = depth;
    while (iter != _children.end()) {
	//be careful, or discard_changes can invalidate the iterator
	previter = iter;
	++iter;
#ifdef DEBUG_COMMIT
	printf("  child: %s\n", (*previter)->path().c_str());	
#endif
	result != (*previter)->discard_changes(depth+1, last_depth);
    }

    return result;
}

int 
ConfigTreeNode::type() const {
    return _template->type();
}

bool 
ConfigTreeNode::is_tag() const {
    return _template->is_tag();
}

bool 
ConfigTreeNode::is_leaf() const {
    return _has_value;
}

const string& 
ConfigTreeNode::value() const {
    if (is_leaf()) {
	return _value;
    } else if (is_tag()) {
	//we should never ask a tag for its value
	XLOG_UNREACHABLE();
    } else {
	return _segname;
    }
}

string 
ConfigTreeNode::show_subtree(int depth, int indent, bool do_indent,
			     bool annotate) const {
    if (_deleted) return string("");

    string s;
    string my_in;
    bool show_me = (depth > 0);
    bool is_a_tag = false;
    if (_template != NULL)
	is_a_tag = is_tag();
    int new_indent;
    for (int i=0; i<indent; i++) 
	my_in += " ";
    
    if (is_a_tag && show_me) {
	new_indent = indent;
	list <ConfigTreeNode*>::const_iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ci++) {
	    if ((*ci)->deleted()) {
		//skip deleted children
	    } else {
		if (annotate) {
		    if ((*ci)->existence_committed())
			s += "  ";
		    else
			s += "> ";
		}
		s += my_in + _segname + " " +
		    (*ci)->show_subtree(depth, indent, false, annotate);
	    }
	}
    } else if (is_a_tag && (show_me==false)) {
	s = "ERROR";
    } else if ((is_a_tag==false) && show_me) {
	new_indent = indent + 2;

	string s2;
	//annotate modified config lines
	if (annotate) {
	    if ((_has_value && !_value_committed) || (!_existence_committed))
		s2 = "> ";
	    else
		s2 = "  ";
	}

	if (do_indent) {
	    s = s2 + my_in + _segname;
	} else
	    s += _segname; 
	if ((type() != NODE_VOID) && (_has_value)) {
	    string value;
	    if ((type() == NODE_TEXT)/* && annotate */) {
		value = "\"" + _value + "\"";
	    } else {
		value = _value;
	    }
	    if (_parent!=NULL && _parent->is_tag()) {
		s += " " + value;
	    } else {
		s += ": " + value;
	    }
	}
	if (_children.size() == 0) {
	    s += "\n";
	    return s;
	}
	s += " {\n";
	list <ConfigTreeNode*>::const_iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ci++) {
	    s += (*ci)->show_subtree(depth+1, new_indent, true, annotate);
	}
	s += s2 + my_in + "}\n";
    } else if ((is_a_tag==false) && (show_me==false)) {
	new_indent = indent;
	list <ConfigTreeNode*>::const_iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ci++) {
	    s += (*ci)->show_subtree(depth+1, new_indent, true, annotate);
	}
    }
    return s;
}

string
ConfigTreeNode::str() const {
    string s;
    s = _path;
    if (_has_value) {
	s+= ": " + _value;
    }
    return s;
}

void
ConfigTreeNode::mark_subtree_as_committed() {
    _existence_committed = true;
    _value_committed = true;
    list <ConfigTreeNode*>::iterator i;
    i = _children.begin();
    while(i!= _children.end()) {
	(*i)->mark_subtree_as_committed();
	i++;
    }
}

void
ConfigTreeNode::mark_subtree_for_deletion(uid_t user_id) {
    /* we delete all the children of this node, then delete the node
       itself */
    debug_msg("Mark subtree for deletion: %s\n", _segname.c_str());

    if (_existence_committed == false) {
	//this node was never committed
	delete_subtree_silently();
	return;
    }
    
    /* delete_subtree calls remove_child, so we just iterate until no
       children are left */
    debug_msg("Node has %u children\n", (uint32_t)_children.size());
    list <ConfigTreeNode*>::iterator i, previ;
    i = _children.begin();
    while(i!= _children.end()) {
	/* be careful to avoid the iterator becoming invalid when
           calling mark_subtree_for_deletion on an uncommitted node*/
	previ = i;
	i++;
	(*previ)->mark_subtree_for_deletion(user_id);
    }

    _user_id = user_id;
    TimerList::system_gettimeofday(&_modification_time);
    _deleted = true;
    _value_committed = false;
}
				
void
ConfigTreeNode::delete_subtree_silently() {
    /* delete_subtree calls remove_child, so we just iterate until no
       children are left */
    while (!_children.empty()) {
	_children.front()->delete_subtree_silently();
    }

    if (_parent != NULL)
	_parent->remove_child(this);

    delete this;
}	

void 
ConfigTreeNode::clone_subtree(const ConfigTreeNode& orig_node) {
    list <ConfigTreeNode*>::const_iterator i;
    for (i = orig_node.const_children().begin();
	 i != orig_node.const_children().end();
	 i++) {
	ConfigTreeNode *new_node;
	new_node = new ConfigTreeNode(**i);
	new_node->set_parent(this);
	add_child(new_node);
	new_node->clone_subtree(**i);
    }
}

string 
ConfigTreeNode::typestr() const{
    return _template->typestr();
}

string 
ConfigTreeNode::s() const {
    string tmp;
    if (_template == NULL) {
	tmp = "ROOT";
    } else {
	tmp = _path + " " + typestr();
	if (!_value.empty()) {
	    tmp += " = " + _value;
	}
    }
    return tmp;
}

void ConfigTreeNode::print_tree() const {
    printf("%s\n", s().c_str());
    list <ConfigTreeNode*>::const_iterator ci;
    for (ci = _children.begin(); ci != _children.end(); ci++) {
	(*ci)->print_tree();
    }
}

ConfigTreeNode* 
ConfigTreeNode::find_node(list <string>& path) {

    //are we looking for the root node?
    if (path.empty()) return this;

    if (_template != NULL) {
	if (path.front() == _segname) {
	    if (path.size()==1) {
		return this;
	    } else {
		path.pop_front();
	    }
	} else {
	    //we must have screwed up
	    XLOG_UNREACHABLE();
	}
    }
	
    list <ConfigTreeNode *>::const_iterator ci;
    for (ci = _children.begin(); ci != _children.end(); ci++) {
	if ((*ci)->segname() == path.front()) {
	    return ((*ci)->find_node(path));
	}
    }

    //none of the nodes match the path
    return NULL;
}

bool 
ConfigTreeNode::expand_expression(const string& expr, 
				  string& value) const {
    if (expr[0]!='`')
	return false;
    if (expr[expr.size()-1]!='`')
	return false;

    //trim quotes
    string expression = expr.substr(1, expr.size()-2);

    //XXXXXX quick and very dirty hack
    if (expression != "~$(@)")
	return false;
    string tmpvalue;
    if (expand_variable("$(@)", tmpvalue)) {
	if (tmpvalue == "false")
	    value = "true";
	else if (tmpvalue == "true")
	    value = "false";
	else
	    return false;
    } else {
	return false;
    }
    return true;
}

const string&
ConfigTreeNode::named_value(const string& varname) const {
    map <string, string>::const_iterator i;
    i = _variables.find(varname);
    assert(i!=_variables.end());
    return i->second;
}

bool 
ConfigTreeNode::expand_variable(const string& varname, 
				string& value) const {
#ifdef DEBUG_VARIABLES
    printf("ConfigTreeNode::expand_variable at %s: >%s<\n", _segname.c_str(), varname.c_str());
#endif

    VarType type = NONE;
    const ConfigTreeNode *varname_node;
    varname_node = find_const_varname_node(varname, type);

    if (varname_node == NULL)
	return false;

    switch (type) {
    case NONE:
	//this can't happen
	XLOG_UNREACHABLE();
    case NODE_VALUE:
	value = varname_node->value();
	return true;
    case NAMED: {
	list <string> var_parts;
	split_up_varname(varname, var_parts);
	value = varname_node->named_value(var_parts.back());
	return true;
    }
    case TEMPLATE_DEFAULT: {
	list <string> var_parts;
	split_up_varname(varname, var_parts);
	string nodename = var_parts.back();
	const TemplateTreeNode *tplt = varname_node->template_node();
	list <TemplateTreeNode *>::const_iterator i;
	for (i=tplt->children().begin(); i != tplt->children().end(); i++) {
	    if ((*i)->segname() == nodename) {
		assert((*i)->has_default());
		value = (*i)->default_str();
		return true;
	    }
	}
	//this can't happen 
	XLOG_UNREACHABLE();
    }
    }
    XLOG_UNREACHABLE();
}

const ConfigTreeNode* 
ConfigTreeNode::find_const_varname_node(const string& varname, 
					VarType& type) const {
    //we need both const and non-const versions of find_varname_node,
    //but don't want to write it all twice.
    return (const_cast<ConfigTreeNode*>(this))
	->find_varname_node(varname, type);
}

ConfigTreeNode* 
ConfigTreeNode::find_varname_node(const string& varname, VarType& type) {
#ifdef DEBUG_VARIABLES
    printf("ConfigTreeNode::expand_variable at %s: >%s<\n", _segname.c_str(), varname.c_str());
#endif
    if (varname == "$(@)" || (varname == "$("+_segname+")") ) {
	assert(!_template->is_tag());
	type = NODE_VALUE;
#ifdef DEBUG_VARIABLES
	printf("varname node is >%s<\n", _segname.c_str());
#endif
	return this;
    }
    list <string> var_parts;
    split_up_varname(varname, var_parts);
    if (var_parts.front() == "@") {
	_on_parent_path = true;
	ConfigTreeNode* found_node = find_child_varname_node(var_parts, type);
	_on_parent_path = false;
	return found_node;
    } else if (var_parts.size()>1) {
	//it's a parent node, or a child of a parent node
	return find_parent_varname_node(var_parts, type);
    }

    //size of 0 or 1, and not $(@) is not valid syntax
#ifdef DEBUG_VARIABLES
    printf("expand failed\n");
#endif
    return NULL;
}

ConfigTreeNode* 
ConfigTreeNode::find_parent_varname_node(const list <string>& var_parts, 
					 VarType& type) {
    debug_msg("find parent at node %s\n", _segname.c_str());
    if (_parent == NULL) {
	debug_msg("reached root node\n");
	type = NONE;
	return NULL;
    }
    if (is_tag() || (this->type()==NODE_VOID)) {
	//when naming a parent node variable, you must start with a tag
	if (_segname == var_parts.front()) {
	    //we've found the right place to start
	    return find_child_varname_node(var_parts, type);
	}
    }
    _on_parent_path = true;
    ConfigTreeNode *found = _parent->find_parent_varname_node(var_parts, type);
    _on_parent_path = false;
    return found;
}

ConfigTreeNode* 
ConfigTreeNode::find_child_varname_node(const list <string>& var_parts, 
					VarType& type) {

    string s;
    s = "find child at node " + _segname + " varname: ";
    list <string>::const_iterator i;
    for (i=var_parts.begin(); i!=var_parts.end(); i++) {
	s += ">" + (*i) + "< ";
    }
    s += "\n";
    debug_msg(s.c_str());

    //@ can only expand when we've already gone through this node
    //heading towards the parent.
    if ((var_parts.front() == "@") && _on_parent_path==false) {
	type = NONE;
	debug_msg("no on parent path\n");
	return NULL;
    }

    if ((var_parts.front() != "@")
	&& (var_parts.front() != _segname) 
	&& ((!_has_value) || (var_parts.front() != _value))) {
	//varname doesn't match us.
	type = NONE;
	debug_msg("varname doesn't match\n");
	return NULL;
    }

    //the name might refer to this node
    if (var_parts.size()==1) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    type = NODE_VALUE;
#ifdef DEBUG_VARIABLES
	    printf("varname V node is >%s<\n", _segname.c_str());
#endif
	    return this;
	}
    }

    if (var_parts.size()==2) {
	//the name might refer to a named variable on this node
	if (_variables.find(var_parts.back()) != _variables.end()) {
	    type = NAMED;
#ifdef DEBUG_VARIABLES
	    printf("varname N node is >%s<\n", _segname.c_str());
#endif
	    return this;
	}
    }

    //the name might refer to a child of ours
    list <string> child_var_parts = var_parts;
    child_var_parts.pop_front();
    ConfigTreeNode *found_child;
    list <ConfigTreeNode *>::iterator ci;
    for (ci = _children.begin(); ci != _children.end(); ci++) {
	found_child = (*ci)->find_child_varname_node(child_var_parts, type);
	if (found_child != NULL)
	    return found_child;
    }

    debug_msg("it's not a child\n");
    if (var_parts.size()==2) {
	//the name might refer to a child of our template node;
	list <TemplateTreeNode *>::const_iterator ti;
	for (ti = _template->children().begin(); 
	     ti != _template->children().end(); 
	     ti++) {
	    if ((*ti)->segname() == var_parts.back()) {
		//it refers to a child of a template
		type = TEMPLATE_DEFAULT;
#ifdef DEBUG_VARIABLES
		printf("varname TD node is >%s<\n", _segname.c_str());
#endif
		return this;
	    }
	}
    }
    debug_msg("it can't be a template either\n");
    type = NONE;
    return NULL;
}

void 
ConfigTreeNode::split_up_varname(const string& varname, 
				 list <string>& var_parts) const {
#ifdef DEBUG_VARIABLES
    printf("split up varname >%s<\n", varname.c_str());
#endif
    assert(varname[0]=='$');
    assert(varname[1]=='(');
    assert(varname[varname.size()-1]==')');
    string trimmed = varname.substr(2, varname.size()-3);
    var_parts = split(trimmed, '.');
#ifdef DEBUG_VARIABLES
    list <string>::iterator i;
    for (i=var_parts.begin(); i!=var_parts.end(); i++) {
	printf(">%s<, ", i->c_str());
    }
    printf("\n");
#endif
}

string ConfigTreeNode::join_up_varname(const list <string>& var_parts) const {
    string s;
    s = "$(";
    list <string>::const_iterator i;
    for (i = var_parts.begin(); i!= var_parts.end(); i++) {
	if (i != var_parts.begin()) {
	    s += ".";
	}
	s += *i;
    }
    s += ")";
    return s;
}

void 
ConfigTreeNode::expand_varname_to_matchlist(const vector<string>& parts, 
					    uint part,
					    list <string>& matches) const {
    //don't expand unless the node has been committed
    if (!_existence_committed)
	return;

    bool ok = false;
    if (is_root_node()) {
	//root node
	ok = true;
	part--; //prevent increment of part later
    } else if ((!_parent->is_root_node()) && _parent->is_tag()) {
	//we're the varname after a tag
	if (parts[part] == "*") {
	    ok = true;
	} else {
	}
    } else {
	if (parts[part]!=_segname) {
	    return;
	}
	ok = true;
    }
    if (ok) {
	if (part == parts.size()-1) {
	    //everything that we were looking for matched
	    assert(parts[part]=="*");
	    matches.push_back(_segname);
	    return;
	} 
	if (_children.empty()) {
	    //no more children, but the search string still has components
	    return;
	}
	list <ConfigTreeNode*>::const_iterator cti;
	for (cti=_children.begin(); cti!= _children.end(); ++cti) {
	    (*cti)->expand_varname_to_matchlist(parts, part+1, matches);
	}
    }
}

bool ConfigTreeNode::set_variable(const string& varname, string& value) {
    /* the variable name should be in the form "nodename.varname".
     * nodename itself can be complex, such as "vif.@" to indicate the
     * current vif.  The general idea is that we're setting a variable
     * on a config tree node. However the config tree node that we're
     * setting the variable on doesn't have to be "this" node - this
     * node only scopes the variable name. */

    //first search for an existing variable to set
    VarType type;
    ConfigTreeNode *node = find_varname_node(varname, type);
    string err;
    if (node != NULL) {
	switch (type) {
	case NONE:
	    //this can't happen
	    XLOG_UNREACHABLE();
	case NODE_VALUE: 
	    err = "Attempt to set variable \"" + varname 
		+ "\" which is the name of a configuration node\n";
	    XLOG_ERROR(err.c_str());
	    return false;
	case NAMED: {
	    list <string> var_parts;
	    split_up_varname(varname, var_parts);
	    node->set_named_value(var_parts.back(), value);
	    return true;
	}
	case TEMPLATE_DEFAULT: 
	    err = "Attempt to set variable \"" + varname 
		+ "\" which is the name of a configuration node\n";
	    XLOG_ERROR(err.c_str());
	    return false;
	}
    }

    //we found no existing variable, see if there's a node on which we
    //can set a named variable
    list <string> var_parts;
    split_up_varname(varname, var_parts);
    list <string>::iterator i;
    for (i=var_parts.begin(); i!=var_parts.end(); i++) {
	debug_msg("VP: %s\n", i->c_str());
    }
    if (var_parts.size()<2) {
	string err = "Attempt to set unknown variable \"" + varname + "\"\n";
	XLOG_ERROR(err.c_str());
	return false;
    }
    var_parts.pop_back();
    string parent_varname = join_up_varname(var_parts);
    node = find_varname_node(parent_varname, type);
    if (node != NULL) {
	switch (type) {
	case NONE:
	    //this can't happen
	    XLOG_UNREACHABLE();
	case NODE_VALUE:
	    split_up_varname(varname, var_parts);
	    node->set_named_value(var_parts.back(), value);
	    return true;
	case NAMED: 
	    err = "Attempt to set variable \"" + varname 
		+ "\" which is the child of a named variable\n";
	    XLOG_ERROR(err.c_str());
	    return false;
	case TEMPLATE_DEFAULT:
	    err = "Attempt to set variable \"" + varname 
		+ "\" which is on a non-existent node\n";
	    XLOG_ERROR(err.c_str());
	    return false;
	}
    }
    err = "Attempt to set unknown variable \"" + varname + "\"\n";
    XLOG_ERROR(err.c_str());
    return false;
}

void 
ConfigTreeNode::set_named_value(const string& varname, const string& value) {
    debug_msg("set_named_value %s=%s\n", varname.c_str(), value.c_str());
    _variables[varname] = value;
}

bool
ConfigTreeNode::retain_different_nodes(const ConfigTreeNode& them,
				       bool retain_value_changed) {
    assert(_segname == them.segname());
    list <ConfigTreeNode*>::iterator my_it;
    list <ConfigTreeNode*>::const_iterator their_it;
    bool retained_children = false;
    for (my_it = _children.begin(); my_it != _children.end();) {
	bool retain_child = true;
	ConfigTreeNode *my_child = *my_it;

	//be careful not to invalidate the iterator when we remove children
	my_it++;

	for (their_it = them.const_children().begin();
	     their_it != them.const_children().end();
	     their_it++) {
	    ConfigTreeNode *their_child = *their_it;
	    //are the nodes the same?
	    if ((*my_child) == (*their_child)) {
		if (!my_child->retain_different_nodes(*their_child,
						      retain_value_changed)) {
		    retain_child = false;
		}
		break;
	    }
	    //are the nodes the same leaf node, but with a changed value
	    if (!retain_value_changed 
		&& my_child->is_leaf() && their_child->is_leaf() 
		&& (my_child->segname() == their_child->segname())) {
		retain_child = false;
		break;
	    }
	}
	if (retain_child == false) {
	    remove_child(my_child);
	    delete my_child;
	} else {
	    retained_children = true;
	}
    }
    return retained_children;
}

void
ConfigTreeNode::retain_common_nodes(const ConfigTreeNode& them) {
    assert(_segname == them.segname());
    list <ConfigTreeNode*>::iterator my_it;
    list <ConfigTreeNode*>::const_iterator their_it;
    bool retained_children = false;
    for (my_it = _children.begin(); my_it != _children.end();) {
	bool retain_child = false;
	ConfigTreeNode *my_child = *my_it;

	//be careful not to invalidate the iterator when we remove children
	my_it++;

	for (their_it = them.const_children().begin();
	     their_it != them.const_children().end();
	     their_it++) {
	    ConfigTreeNode *their_child = *their_it;
	    if ((*my_child) == (*their_child)) {
		my_child->retain_common_nodes(*their_child);
		retain_child = true;
		retained_children = true;
		break;
	    }
	}
	if (retain_child == false) {
	    remove_child(my_child);
	    delete my_child;
	}
    }
    if (_parent != NULL && retained_children == false && is_tag()) {
	_parent->remove_child(this);
	delete this;
	return;
    }
}






