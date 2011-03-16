// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



//#define DEBUG_LOGGING
#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#include "command_tree.hh"
#include "conf_tree_node.hh"
#include "module_command.hh"
#include "template_commands.hh"
#include "template_tree_node.hh"
#include "util.hh"


extern int booterror(const char *s) throw (ParseError);

bool
CTN_Compare::operator() (ConfigTreeNode* a, ConfigTreeNode *b)
{
    //
    // If the nodes have the same template, then we sort according to their
    // own sort order, otherwise we sort them into template tree order.
    //
    if (a->template_tree_node() == b->template_tree_node()) {
	TTSortOrder order = a->template_tree_node()->order();
	switch (order) {
	case ORDER_UNSORTED:
	    // Don't sort
	    return false;
	case ORDER_SORTED_NUMERIC:
	    if (a->is_tag()) {
		XLOG_ASSERT(!a->const_children().empty());
		XLOG_ASSERT(!b->const_children().empty());
		const ConfigTreeNode *child_a = *(a->const_children().begin());
		const ConfigTreeNode *child_b = *(b->const_children().begin());
		return (strtoll(child_a->segname().c_str(), (char **)NULL, 10)
			< strtoll(child_b->segname().c_str(), (char **)NULL, 10));
	    } else {
		return (strtoll(a->segname().c_str(), (char **)NULL, 10)
			< strtoll(b->segname().c_str(), (char **)NULL, 10));
	    }
	case ORDER_SORTED_ALPHABETIC:
	    if (a->is_tag()) {
		XLOG_ASSERT(!a->const_children().empty());
		XLOG_ASSERT(!b->const_children().empty());
		const ConfigTreeNode *child_a = *(a->const_children().begin());
		const ConfigTreeNode *child_b = *(b->const_children().begin());
		return (child_a->str() < child_b->str());
	    } else {
		return (a->str() < b->str());
	    }
	}
    }

    // Nodes have different templates, so sort by template file order
    return (a->child_number() < b->child_number());
}


ConfigTreeNode::ConfigTreeNode(bool verbose)
    : _template_tree_node(NULL),
      _has_value(false),
      _operator(OP_NONE),
      _committed_operator(OP_NONE),
      _parent(NULL),
      _node_id(ConfigNodeId::ZERO()),
      _node_id_generator(ConfigNodeId::ZERO()),
      _user_id(0),
      _committed_user_id(0),
      _clientid(0),
      _modification_time(TimeVal::ZERO()),
      _committed_modification_time(TimeVal::ZERO()),
      _existence_committed(false),
      _value_committed(false),
      _deleted(false),
      _on_parent_path(false),
      _verbose(verbose)
{
}

ConfigTreeNode::ConfigTreeNode(const string& nodename,
			       const string& path, 
			       const TemplateTreeNode* ttn,
			       ConfigTreeNode* parent,
			       const ConfigNodeId& node_id,
			       uid_t user_id,
			       uint32_t clientid,
			       bool verbose)
    : _template_tree_node(ttn),
      _has_value(false),
      _operator(OP_NONE),
      _committed_operator(OP_NONE),
      _segname(nodename),
      _path(path),
      _parent(parent),
      _node_id(node_id),
      _node_id_generator(ConfigNodeId::ZERO()),
      _user_id(user_id),
      _committed_user_id(0),
      _clientid(clientid),
      _modification_time(TimeVal::ZERO()),
      _committed_modification_time(TimeVal::ZERO()),
      _existence_committed(false),
      _value_committed(false),
      _deleted(false),
      _on_parent_path(false),
      _verbose(verbose)
{
    TimerList::system_gettimeofday(&_modification_time);
    _node_id_generator.set_instance_id(_clientid);
    parent->add_child(this);
    if (node_id.is_empty()) {
	allocate_unique_node_id();
    }
}

ConfigTreeNode::ConfigTreeNode(const ConfigTreeNode& ctn)
    : _template_tree_node(ctn._template_tree_node),
      _has_value(ctn._has_value),
      _value(ctn._value),
      _committed_value(ctn._committed_value),
      _operator(ctn._operator),
      _committed_operator(ctn._committed_operator),
      _segname(ctn._segname),
      _path(ctn._path),
      _parent(NULL),
      _node_id(ctn._node_id),
      _node_id_generator(ctn._node_id_generator),
      _user_id(ctn._user_id),
      _committed_user_id(ctn._committed_user_id),
      _clientid(ctn._clientid),
      _modification_time(ctn._modification_time),
      _committed_modification_time(ctn._committed_modification_time),
      _existence_committed(ctn._existence_committed),
      _value_committed(ctn._value_committed),
      _deleted(ctn._deleted),
      _on_parent_path(false),
      _verbose(ctn._verbose)
{
}

ConfigTreeNode::~ConfigTreeNode()
{
    while (!_children.empty()) {
	delete _children.front();
	_children.pop_front();
    }

    // Detect accidental reuse
    _template_tree_node = reinterpret_cast<TemplateTreeNode *>(0xbad);
    _parent = reinterpret_cast<ConfigTreeNode *>(0xbad);
}

bool
ConfigTreeNode::operator==(const ConfigTreeNode& them) const
{
    return (is_same(them, false));
}

bool
ConfigTreeNode::is_same(const ConfigTreeNode& them, bool ignore_node_id) const
{
    //
    // This comparison only cares about the declarative state of the node,
    // not about the ephemeral state such as when it was modified, deleted,
    // or who changed it last.
    //
    if (_segname != them.segname())
	return false;
    if (_parent != NULL && them.const_parent() != NULL) {
	if (is_tag() != them.is_tag())
	    return false;
	if (_has_value && (_value != them.value())) {
	    return false;
	}
	if (!is_tag() && !them.is_tag()) {
	    if (_operator != them.get_operator()) {
		return false;
	    }
	}
	if ((! ignore_node_id) && (_node_id != them.node_id())) {
	    return false;
	}
    }
    if (is_leaf_value() != them.is_leaf_value())
	return false;
    if (_template_tree_node != them.template_tree_node())
	return false;

    debug_msg("Equal nodes:\n");
    debug_msg("Node1: %s\n", str().c_str());
    debug_msg("Node2: %s\n\n", them.str().c_str());

    return true;
}


void
ConfigTreeNode::add_child(ConfigTreeNode* child)
{
    _children.push_back(child);
    list<ConfigTreeNode *> sorted_children = _children;
    sort_by_template(sorted_children);
    _children = sorted_children;
}

void
ConfigTreeNode::remove_child(ConfigTreeNode* child)
{
    //
    // It's faster to implement this ourselves than use _children.remove()
    // because _children has no duplicates.
    //
    list<ConfigTreeNode*>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	if ((*iter) == child) {
	    _children.erase(iter);
	    return;
	}
    }

    XLOG_UNREACHABLE();
}

void
ConfigTreeNode::add_default_children()
{
    if (_template_tree_node == NULL)
	return;

    // Find all the template nodes for our children
    set<const TemplateTreeNode*> childrens_templates;
    list<ConfigTreeNode *>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	childrens_templates.insert((*iter)->template_tree_node());
    }

    //
    // Now go through all the children of our template node, and find
    // any that are leaf nodes that have default values, and that we
    // don't already have an instance of among our children.
    //
    list<TemplateTreeNode*>::const_iterator tci;
    for (tci = _template_tree_node->children().begin();
	 tci != _template_tree_node->children().end();
	 ++tci) {
	TemplateTreeNode* ttn = *tci;
	if (ttn->has_default()) {
	    if (childrens_templates.find(ttn) == childrens_templates.end()) {
		string error_msg;
		string name = ttn->segname();
		string path = _path + " " + name;
		ConfigTreeNode *new_node = create_node(name, path, ttn,
						       this, 
						       ConfigNodeId::ZERO(),
						       _user_id,
						       _clientid,
						       _verbose);
		if (new_node->set_value(ttn->default_str(), _user_id,
					error_msg)
		    != true) {
		    XLOG_FATAL("Cannot set default value: %s",
			       error_msg.c_str());
		}
		if (new_node->set_operator(OP_ASSIGN, _user_id, error_msg)
		    != true) {
		    XLOG_FATAL("Cannot set default operator: %s",
			       error_msg.c_str());
		}
	    }
	}
    }
}

void
ConfigTreeNode::recursive_add_default_children()
{
    list<ConfigTreeNode *>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	(*iter)->recursive_add_default_children();
    }

    add_default_children();
}

bool
ConfigTreeNode::check_allowed_value(string& error_msg) const
{
    return (check_allowed_value(_value, error_msg));
}

bool 
ConfigTreeNode::check_allowed_value(const string& value,
				    string& error_msg) const
{
    if (_template_tree_node == NULL)
	return true;		// XXX: the root node has a NULL template

    return (_template_tree_node->check_allowed_value(value, error_msg));
}

bool 
ConfigTreeNode::check_allowed_operator(const string& value,
				       string& error_msg) const
{
    if (_template_tree_node == NULL)
	return true;		// XXX: the root node has a NULL template

    const BaseCommand* base_cmd;
    const AllowOperatorsCommand* cmd;
    base_cmd = _template_tree_node->const_command("%allow-operator");
    cmd = dynamic_cast<const AllowOperatorsCommand *>(base_cmd);
    if (cmd == NULL)
	return true;

    return (cmd->verify_variable_by_value(*this, value, error_msg));
}

bool
ConfigTreeNode::set_value(const string& value, uid_t user_id,
			  string& error_msg)
{
    if (check_allowed_value(value, error_msg) != true)
	return false;

    set_value_without_verification(value, user_id);

    return true;
}

void
ConfigTreeNode::set_value_without_verification(const string& value,
					       uid_t user_id)
{
    _value = value;
    _has_value = true;
    _value_committed = false;
    _user_id = user_id;
    TimerList::system_gettimeofday(&_modification_time);
}

bool
ConfigTreeNode::set_operator(ConfigOperator op, uid_t user_id,
			     string& error_msg)
{
    if (op != OP_NONE) {
	string value = operator_to_str(op);

	if (check_allowed_operator(value, error_msg) != true)
	    return false;
    }

    set_operator_without_verification(op, user_id);

    return true;
}

void
ConfigTreeNode::set_operator_without_verification(ConfigOperator op,
						  uid_t user_id)
{
    if (_operator != op) {
	_operator = op;
	_value_committed = false;
	_user_id = user_id;
	TimerList::system_gettimeofday(&_modification_time);
    }
}

bool
ConfigTreeNode::merge_deltas(uid_t user_id,
			     const ConfigTreeNode& delta_node, 
			     bool provisional_change,
			     bool preserve_node_id,
			     string& error_msg)
{
    XLOG_ASSERT(_segname == delta_node.segname());

    if (_template_tree_node != NULL) {
	XLOG_ASSERT(type() == delta_node.type());

	if (delta_node.is_leaf_value()) {
	    if (_value != delta_node.value() 
		|| _operator != delta_node.get_operator()) {
		_has_value = true;
		if (provisional_change) {
		    _committed_value = _value;
		    _value = delta_node.value();
		    _committed_operator = _operator;
		    _operator = delta_node.get_operator();
		    if (preserve_node_id)
			_node_id = delta_node.node_id();
		    else {
			_node_id = ConfigNodeId::ZERO();
			allocate_unique_node_id();
		    }
		    _committed_user_id = _user_id;
		    _user_id = user_id;
		    _clientid = delta_node.clientid();
		    _committed_modification_time = _modification_time;
		    TimerList::system_gettimeofday(&_modification_time);
		    _value_committed = false;
		} else {
		    _committed_value = delta_node.value();
		    _value = delta_node.value();
		    _committed_operator = delta_node.get_operator();
		    _operator = delta_node.get_operator();
		    if (preserve_node_id)
			_node_id = delta_node.node_id();
		    else
			allocate_unique_node_id();
		    _committed_user_id = delta_node.user_id();
		    _user_id = delta_node.user_id();
		    _clientid = delta_node.clientid();
		    _committed_modification_time =
			delta_node.modification_time();
		    _modification_time = delta_node.modification_time();
		    _value_committed = true;
		}
#if 0
XXXXXXX to be copied to MasterConfigTreeNode
		_actions_pending = 0;
		_actions_succeeded = true;
		_cmd_that_failed = NULL;
#endif
	    }
	}
    }

    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = delta_node.const_children().begin();
	 iter != delta_node.const_children().end();
	 ++iter) {
	ConfigTreeNode *delta_child = *iter;
	
	bool delta_child_done = false;
	list<ConfigTreeNode*>::iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ++ci) {
	    ConfigTreeNode *my_child = *ci;
	    if (my_child->segname() == delta_child->segname()) {
		delta_child_done = true;
		bool success = my_child->merge_deltas(user_id, *delta_child,
						      provisional_change,
						      preserve_node_id,
						      error_msg);
		if (success == false) {
		    // If something failed, abort the merge
		    return false;
		}
		break;
	    }
	}
	if (delta_child_done == false) {
	    ConfigTreeNode* new_node;
	    ConfigNodeId new_node_id = ConfigNodeId::ZERO();
	    if (preserve_node_id)
		new_node_id = delta_child->node_id();
	    new_node = create_node(delta_child->segname(),
				   delta_child->path(),
				   delta_child->template_tree_node(),
				   this,
				   new_node_id,
				   user_id,
				   delta_child->clientid(),
				   _verbose);
	    if (!provisional_change)
		new_node->set_existence_committed(true);
	    new_node->merge_deltas(user_id, *delta_child, provisional_change,
				   preserve_node_id, error_msg);
	}
    }
    return true;
}

bool
ConfigTreeNode::merge_deletions(uid_t user_id,
				const ConfigTreeNode& deletion_node, 
				bool provisional_change,
				string& error_msg)
{
    XLOG_ASSERT(_segname == deletion_node.segname());

    if (_template_tree_node != NULL) {
	XLOG_ASSERT(type() == deletion_node.type());
	if (deletion_node.const_children().empty()) {
	    if (provisional_change) {
		_deleted = true;
		_value_committed = false;
		//
		// XXX: Mark the whole subtree for deletion so later
		// the scanning of the tree will invoke the appropriate
		// method for each node.
		//
		mark_subtree_for_deletion(user_id);
	    } else {
		delete_subtree_silently();
	    }
	    return true;
	}
    }
    
    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = deletion_node.const_children().begin();
	 iter != deletion_node.const_children().end();
	 ++iter) {
	ConfigTreeNode *deletion_child = *iter;

	bool deletion_child_done = false;
	list<ConfigTreeNode*>::iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ++ci) {
	    ConfigTreeNode *my_child = *ci;
	    if (my_child->segname() == deletion_child->segname()) {
		deletion_child_done = true;
		bool success = my_child->merge_deletions(user_id, 
							 *deletion_child,
							 provisional_change,
							 error_msg);
		if (success == false) {
		    // If something failed, abort the merge
		    return false;
		}
		break;
	    }
	}
	if (deletion_child_done == false) {
	    error_msg = c_format("Failed to delete node:\n"
				 "   %s\n"
				 "Node does not exist.\n",
				 deletion_node.path().c_str());
	    return false;
	}
    }
    return true;
}


ConfigTreeNode*
ConfigTreeNode::find_config_module(const string& module_name)
{
    if (module_name.empty())
	return (NULL);

    // Check if this node matches
    if (_template_tree_node != NULL) {
	if (_template_tree_node->module_name() == module_name)
	    return (this);
    }

    // Check all children nodes
    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	ConfigTreeNode* ctn = (*iter)->find_config_module(module_name);
	if (ctn != NULL)
	    return (ctn);
    }

    return (NULL);
}

bool
ConfigTreeNode::check_config_tree(string& error_msg) const
{
    list<ConfigTreeNode *>::const_iterator iter;

    //
    // Check that the node is not deprecated or read-only
    //
    if (_template_tree_node != NULL) {
	if (_template_tree_node->is_deprecated()) {
	    error_msg = c_format("Node \"%s\" is deprecated: %s\n",
				 _path.c_str(),
				 _template_tree_node->deprecated_reason().c_str());
	    return false;
	}
	if (_template_tree_node->is_read_only()
	    && (is_leaf_value())
	    && (! is_default_value())) {
	    string reason = read_only_reason();
	    error_msg = c_format("Node \"%s\" is read-only", _path.c_str());
	    if (! reason.empty())
		error_msg += c_format(": %s", reason.c_str());
	    error_msg += "\n";
	    return false;
	}
    }
    //
    // An early check that the child nodes are not deprecated or read-only.
    // Note: we need to check the child nodes before the check for
    // mandatory configuration nodes so we will print a better error
    // message if, say, a mandatory node has been renamed and the old
    // one has been deprecated.
    //
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	const TemplateTreeNode* ttn = (*iter)->template_tree_node();
	if (ttn == NULL)
	    continue;
	if (ttn->is_deprecated()) {
	    error_msg = c_format("Node \"%s\" is deprecated: %s\n",
				 (*iter)->path().c_str(),
				 ttn->deprecated_reason().c_str());
	    return false;
	}
	if (ttn->is_read_only()
	    && (is_leaf_value())
	    && (! is_default_value())) {
	    string reason = ttn->read_only_reason();
	    error_msg = c_format("Node \"%s\" is read-only", _path.c_str());
	    if (! reason.empty())
		error_msg += c_format(": %s", reason.c_str());
	    error_msg += "\n";
	    return false;
	}
    }

    //
    // Check that all mandatory configuration nodes are configured in place
    //
    if ((! deleted()) && (_template_tree_node != NULL)) {
	list<string>::const_iterator li;
	for (li = _template_tree_node->mandatory_config_nodes().begin();
	     li != _template_tree_node->mandatory_config_nodes().end();
	     ++li) {
	    const string& mandatory_config_node = *li;
	    string value;
	    if (expand_variable(mandatory_config_node, value, true) != true) {
		error_msg = c_format("Missing mandatory configuration node "
				     "\"%s\" required by node \"%s\"\n",
				     mandatory_config_node.c_str(),
				     _path.c_str());
		return false;
	    }
	}
    }

    //
    // Verify the allowed configuration values
    //
    if (_template_tree_node != NULL) {
	if (has_value()) {
	    if (_template_tree_node->check_allowed_value(value(), error_msg)
		!= true) {
		return false;
	    }
	}
	if (_template_tree_node->verify_variables(*this, error_msg) != true)
	    return false;
    }

    //
    // Recursively check all child nodes
    //
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	ConfigTreeNode* ctn = *iter;
	if (ctn->check_config_tree(error_msg) != true)
	    return false;
    }

    return true;
}


string
ConfigTreeNode::discard_changes(int depth, int last_depth)
{
    string result;
    bool changes_made = false;

    debug_msg("DISCARD CHANGES node >%s<\n", _path.c_str());

    // The root node has a NULL template
    if (_template_tree_node != NULL) {
	if (_existence_committed == false) {
	    bool show_top = (depth > 0);
	    result = show_subtree(show_top, depth, /* XXX */ depth * 2, true, 
				  /* numbered */ false, false, false);
	    delete_subtree_silently();
	    return result;
	} else if (_value_committed == false) {
	    debug_msg("discarding changes from node %s\n",
		      _path.c_str());
	    _value_committed = true;
	    _deleted = false;
	    _value = _committed_value;
	    _operator = _committed_operator;
	    _user_id = _committed_user_id;
	    _modification_time = _committed_modification_time;
	    result = node_str();
	    if (is_leaf_value()) 
		result += "\n";
	    else
		result += " {\n";
	    changes_made = true;
	} 
    }

    list<ConfigTreeNode *>::iterator iter, prev_iter;
    iter = _children.begin();
    if (changes_made)
	last_depth = depth;
    while (iter != _children.end()) {
	// Be careful, or discard_changes can invalidate the iterator
	prev_iter = iter;
	++iter;
	debug_msg("  child: %s\n", (*prev_iter)->path().c_str());
	result += (*prev_iter)->discard_changes(depth + 1, last_depth);
    }

    return result;
}

int
ConfigTreeNode::type() const
{
    return _template_tree_node->type();
}

bool
ConfigTreeNode::is_tag() const
{
    //
    // Only the root node does not have a template tree node, and it is
    //
    if (_template_tree_node == NULL)
	return false;

    return _template_tree_node->is_tag();
}

bool
ConfigTreeNode::is_multi_value_node() const
{
    if (_parent == NULL)
	return false;
    return (_parent->is_tag());
}

bool
ConfigTreeNode::is_leaf_value() const
{
    if (_has_value)
	return true;
    if (_template_tree_node == NULL)
	return false;
    if (_template_tree_node->is_leaf_value())
	return true;
    return false;
}

bool
ConfigTreeNode::is_read_only() const
{
    if (_template_tree_node == NULL)
	return false;
    return (_template_tree_node->is_read_only());
}

const string&
ConfigTreeNode::read_only_reason() const
{
    if (_template_tree_node == NULL) {
	static const string empty_reason;
	return empty_reason;
    }
    return (_template_tree_node->read_only_reason());
}

bool
ConfigTreeNode::is_permanent() const
{
    if (_template_tree_node == NULL)
	return false;
    return (_template_tree_node->is_permanent());
}

const string&
ConfigTreeNode::permanent_reason() const
{
    if (_template_tree_node == NULL) {
	static const string empty_reason;
	return empty_reason;
    }
    return (_template_tree_node->permanent_reason());
}

bool
ConfigTreeNode::is_user_hidden() const
{
    if (_template_tree_node == NULL)
	return false;
    return (_template_tree_node->is_user_hidden());
}

const string&
ConfigTreeNode::user_hidden_reason() const
{
    if (_template_tree_node == NULL) {
	static const string empty_reason;
	return empty_reason;
    }
    return (_template_tree_node->user_hidden_reason());
}

unsigned int
ConfigTreeNode::depth() const
{
    if (is_root_node())
	return 0;
    else
	return (1 + _parent->depth());
}

const string&
ConfigTreeNode::value() const
{
    if (_has_value)
	return _value;

    if (is_tag()) {
	// We should never ask a tag for its value
	XLOG_UNREACHABLE();
    }

    return _segname;
}

ConfigOperator
ConfigTreeNode::get_operator() const
{
    if (is_tag()) {
	// We should never ask a tag for its operator
	XLOG_UNREACHABLE();
    }

    return _operator;
}

void
ConfigTreeNode::undelete_node_and_ancestors()
{
    ConfigTreeNode* ctn;

    //
    // Undelete this node and all its ancestors
    //
    for (ctn = this; ctn != NULL; ctn = ctn->parent()) {
	ctn->undelete();
    }
}

void
ConfigTreeNode::undelete_subtree()
{
    list<ConfigTreeNode*>::iterator iter;

    undelete();

    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	ConfigTreeNode* ctn = *iter;
	ctn->undelete_subtree();
    }
}

string
ConfigTreeNode::show_subtree(bool show_top, int depth, int indent,
			     bool do_indent, bool numbered, bool annotate,
			     bool suppress_default_values) const
{
    string s;
    string my_in;
    bool is_a_tag = false;
    int new_indent;

    if (suppress_default_values && is_default_value())
	return string("");

    if (_template_tree_node != NULL) {
	// XXX: ignore deprecated subtrees
	if (_template_tree_node->is_deprecated())
	    return string("");
	// XXX: ignore user-hidden subtrees
	if (_template_tree_node->is_user_hidden())
	    return string("");
    }

    if (_template_tree_node != NULL)
	is_a_tag = is_tag();

    for (int i = 0; i < indent; i++) 
	my_in += "  ";

    if (is_a_tag && show_top) {
	new_indent = indent;
	list<ConfigTreeNode*>::const_iterator iter;
	for (iter = _children.begin(); iter != _children.end(); ++iter) {
	    const ConfigTreeNode* child_ctn = *iter;

	    if (suppress_default_values && child_ctn->is_default_value()) {
		//
		// Skip children with default values if we don't care about
		// them.
		//
		continue;
	    }

	    //
	    // Add the annotation prefix
	    //
	    if (annotate) {
		do {
		    if (child_ctn->deleted()) {
			s += "-   ";
			break;
		    }
		    if (! child_ctn->existence_committed()) {
			s += ">   ";
			break;
		    }
		    s += "    ";
		    break;
		} while (false);
	    }
	    s += my_in + show_node_id(numbered, child_ctn->node_id()) 
		+ _segname + " " +
		child_ctn->show_subtree(show_top, depth, indent, false, 
					numbered, annotate,
					suppress_default_values);
	}
    } else if (is_a_tag && (show_top == false)) {
	s = "ERROR";
    } else if ((is_a_tag == false) && show_top) {
	string s2;

	new_indent = indent + 2;

	//
	// Add the annotation prefix
	//
	if (annotate) {
	    do {
		if (deleted()) {
		    s2 = "-   ";
		    break;
		}
		if (is_uncommitted()) {
		    s2 = ">   ";
		    break;
		}
		s2 = "    ";
		break;
	    } while (false);
	}

	if (do_indent) {
	    s = s2 + my_in + show_node_id(numbered, node_id()) + _segname;
	} else {
	    // no need to show the node ID here
	    XLOG_ASSERT(_parent->is_tag());
	    if ((type() == NODE_TEXT) && is_quotable_string(_segname)) {
		s += quoted_value(_segname);
	    } else {
		s += _segname; 
	    }
	}
	if ((type() != NODE_VOID) && (_has_value)) {
	    string value;
	    if ((type() == NODE_TEXT) /* && annotate */ ) {
		value = quoted_value(_value);
	    } else {
		value = _value;
	    }
	    if (_parent != NULL && _parent->is_tag()) {
		s += " " + value;
	    } else {
		if (_operator == OP_NONE) {
		    // no-op
		} else if (_operator == OP_ASSIGN) {
		    s += show_operator() + " " + value;
		} else {
		    s += " " + show_operator() + " " + value;
		}
	    }
	}
	if (_children.empty() && (type() != NODE_VOID || _has_value)) {
	    //
	    // Normally if a node has no children, we don't want the
	    // braces, but certain grouping nodes, with type NODE_VOID
	    // (e.g. "protocols") won't parse correctly if we print
	    // them without the braces.
	    //
	    s += "\n";
	    return s;
	}
	s += " {\n";
	list<ConfigTreeNode*>::const_iterator iter;
	for (iter = _children.begin(); iter != _children.end(); ++iter) {
	    s += (*iter)->show_subtree(true, depth + 1, new_indent, true, 
				       numbered, annotate,
				       suppress_default_values);
	}
	s += s2 + my_in + "}\n";
    } else if ((is_a_tag == false) && (show_top == false)) {
	new_indent = indent;
	list<ConfigTreeNode*>::const_iterator iter;
	for (iter = _children.begin(); iter != _children.end(); ++iter) {
	    s += (*iter)->show_subtree(true, depth + 1, new_indent, true, 
				       numbered, annotate,
				       suppress_default_values);
	}
    }
    return s;
}

void
ConfigTreeNode::mark_subtree_as_committed()
{
    _existence_committed = true;
    _value_committed = true;
    _deleted = false;
    _committed_value = _value;
    _committed_operator = _operator;
    _committed_user_id = _user_id;
    _committed_modification_time = _modification_time;

    list<ConfigTreeNode*>::iterator iter;
    iter = _children.begin();
    while (iter != _children.end()) {
	(*iter)->mark_subtree_as_committed();
	++iter;
    }
}

void
ConfigTreeNode::mark_subtree_as_uncommitted()
{
    _existence_committed = false;
    _value_committed = false;

    list<ConfigTreeNode*>::iterator iter;
    iter = _children.begin();
    while (iter != _children.end()) {
	(*iter)->mark_subtree_as_uncommitted();
	++iter;
    }
}

void
ConfigTreeNode::mark_subtree_for_deletion(uid_t user_id)
{
    // We delete all the children of this node, then delete the node itself
    debug_msg("Mark subtree for deletion: %s\n", _segname.c_str());

    if (_existence_committed == false) {
	// this node was never committed
	delete_subtree_silently();
	return;
    }

    //
    // Delete_subtree calls remove_child, so we just iterate until no
    // children are left.
    //
    debug_msg("Node has %u children\n", XORP_UINT_CAST(_children.size()));
    list<ConfigTreeNode*>::iterator iter, prev_iter;
    iter = _children.begin();
    while (iter != _children.end()) {
	//
	// Be careful to avoid the iterator becoming invalid when
	// calling mark_subtree_for_deletion on an uncommitted node.
	//
	prev_iter = iter;
	++iter;
	(*prev_iter)->mark_subtree_for_deletion(user_id);
    }

    _user_id = user_id;
    TimerList::system_gettimeofday(&_modification_time);
    _deleted = true;
    _value_committed = false;
#if 0
XXXXX to be copied to MasterConfigTreeNode??
    _actions_succeeded = true;
#endif
}

void
ConfigTreeNode::delete_subtree_silently()
{
    //
    // Delete_subtree calls remove_child, so we just iterate until no
    // children are left.
    //
    while (!_children.empty()) {
	_children.front()->delete_subtree_silently();
    }

    if (_parent != NULL)
	_parent->remove_child(this);

    if (! is_root_node())
	delete this;
}

void
ConfigTreeNode::clone_subtree(const ConfigTreeNode& orig_node)
{
    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = orig_node.const_children().begin();
	 iter != orig_node.const_children().end();
	 ++iter) {
	ConfigTreeNode* new_node = create_node(**iter);
	new_node->set_parent(this);
	add_child(new_node);
	new_node->clone_subtree(**iter);
    }
}

string
ConfigTreeNode::typestr() const
{
    return _template_tree_node->typestr();
}

string
ConfigTreeNode::node_str() const
{
    string tmp;

    if (_template_tree_node == NULL) {
	tmp = "ROOT";
    } else {
	tmp = _path + " " + typestr();
	if (!_value.empty()) {
	    tmp += " = " + _value;
	}
    }
    return tmp;
}

//
// TODO: this method is not used. What is the difference between
// str() and node_str() ??
// 
string
ConfigTreeNode::str() const
{
    string s = _path;

    if (_has_value) {
	s+= ": " + _value;
    }
    return s;
}

string
ConfigTreeNode::subtree_str() const
{
    string s;

    s = c_format("%s\n", node_str().c_str());

    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	s += (*iter)->subtree_str();
    }

    return s;
}

const ConfigTreeNode*
ConfigTreeNode::find_const_node(const list<string>& path) const
{
    //
    // We need both const and non-const versions of find_node,
    // but don't want to write it all twice.
    //
    return (const_cast<ConfigTreeNode*>(this))->find_node(path);
}

ConfigTreeNode*
ConfigTreeNode::find_node(const list<string>& path)
{
    list<string> path_copy = path;

    // Are we looking for the root node?
    if (path_copy.empty())
	return this;

    if (_template_tree_node != NULL) {
	// XXX: not a root node
	if (path_copy.front() == _segname) {
	    if (path_copy.size() == 1) {
		return this;
	    } else {
		path_copy.pop_front();
	    }
	} else {
	    // We must have screwed up
	    XLOG_UNREACHABLE();
	}
    }

    list<ConfigTreeNode *>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	if ((*iter)->segname() == path_copy.front()) {
	    return ((*iter)->find_node(path_copy));
	}
    }

    // None of the nodes match the path
    return NULL;
}


bool
ConfigTreeNode::retain_different_nodes(const ConfigTreeNode& them,
				       bool retain_value_changed)
{
    list<ConfigTreeNode*>::iterator my_iter;
    list<ConfigTreeNode*>::const_iterator their_iter;
    bool retained_children = false;

    XLOG_ASSERT(_segname == them.segname());

    for (my_iter = _children.begin(); my_iter != _children.end(); ) {
	bool retain_child = true;
	ConfigTreeNode *my_child = *my_iter;

	// Be careful not to invalidate the iterator when we remove children
	++my_iter;

	for (their_iter = them.const_children().begin();
	     their_iter != them.const_children().end();
	     ++their_iter) {
	    ConfigTreeNode* their_child = *their_iter;

	    if (their_child->deleted())
		continue;	// XXX: ignore deleted nodes

	    // Are the nodes the same?
	    if (my_child->is_same(*their_child, true)) {
		if (!my_child->retain_different_nodes(*their_child,
						      retain_value_changed)) {
		    retain_child = false;
		}
		break;
	    }
	    // Are the nodes the same leaf node, but with a changed value
	    if (!retain_value_changed
		&& my_child->is_leaf_value() && their_child->is_leaf_value()
		&& (my_child->segname() == their_child->segname())) {
		retain_child = false;
		break;
	    }
	}

	if (retain_child == false) {
	    my_child->delete_subtree_silently();
	} else {
	    retained_children = true;
	}
    }
    return retained_children;
}

bool
ConfigTreeNode::retain_deletion_nodes(const ConfigTreeNode& them,
				      bool retain_value_changed)
{
    list<ConfigTreeNode*>::iterator my_iter;
    list<ConfigTreeNode*>::const_iterator their_iter;
    bool found_deletion_children = false;

    XLOG_ASSERT(_segname == them.segname());

    for (my_iter = _children.begin(); my_iter != _children.end(); ) {
	bool retain_child = true;
	bool node_found = false;
	bool found_deletion_child = false;
	ConfigTreeNode *my_child = *my_iter;

	// Be careful not to invalidate the iterator when we remove children
	++my_iter;

	for (their_iter = them.const_children().begin();
	     their_iter != them.const_children().end();
	     ++their_iter) {
	    ConfigTreeNode* their_child = *their_iter;

	    if (their_child->deleted())
		continue;	// XXX: ignore deleted nodes

	    // Are the nodes the same?
	    if (my_child->is_same(*their_child, true)) {
		if (my_child->retain_deletion_nodes(*their_child,
						    retain_value_changed)) {
		    found_deletion_child = true;
		}
		node_found = true;
		break;
	    }
	    // Are the nodes the same leaf node, but with a changed value
	    if (!retain_value_changed
		&& my_child->is_leaf_value() && their_child->is_leaf_value()
		&& (my_child->segname() == their_child->segname())) {
		// retain_child = false;
		node_found = true;
		break;
	    }
	}

	//
	// Remove the children of my_child
	//
	if (! node_found) {
	    for (their_iter = my_child->children().begin();
		 their_iter != my_child->children().end(); ) {
		ConfigTreeNode* del_node = *their_iter;
		++their_iter;
		del_node->delete_subtree_silently();
		found_deletion_child = true;
	    }
	}

	if (retain_child == false) {
	    my_child->delete_subtree_silently();
	    found_deletion_child = true;
	}
	if ((! found_deletion_child)
	    && node_found
	    && (! my_child->has_undeleted_children())) {
	    // XXX: common child among both trees, hence just prune it
	    my_child->delete_subtree_silently();
	}

	if (found_deletion_child)
	    found_deletion_children = true;
    }

    return (found_deletion_children);
}

void
ConfigTreeNode::retain_common_nodes(const ConfigTreeNode& them)
{
    list<ConfigTreeNode*>::iterator my_iter;
    list<ConfigTreeNode*>::const_iterator their_iter;
    bool retained_children = false;

    XLOG_ASSERT(_segname == them.segname());

    for (my_iter = _children.begin(); my_iter != _children.end(); ) {
	bool retain_child = false;
	ConfigTreeNode *my_child = *my_iter;

	// Be careful not to invalidate the iterator when we remove children
	++my_iter;

	for (their_iter = them.const_children().begin();
	     their_iter != them.const_children().end();
	     ++their_iter) {
	    ConfigTreeNode* their_child = *their_iter;
	    if ((*my_child) == (*their_child)) {
		my_child->retain_common_nodes(*their_child);
		retain_child = true;
		retained_children = true;
		break;
	    }
	}
	if (retain_child == false) {
	    my_child->delete_subtree_silently();
	}
    }
    if (_parent != NULL && retained_children == false && is_tag()) {
	delete_subtree_silently();
	return;
    }
}


bool
ConfigTreeNode::expand_variable(const string& varname, string& value,
				bool ignore_deleted_nodes) const
{

    VarType type = NONE;
    const ConfigTreeNode *varname_node;

    debug_msg("ConfigTreeNode::expand_variable at %s: >%s<\n",
	      _segname.c_str(), varname.c_str());

    varname_node = find_const_varname_node(varname, type);

    if (ignore_deleted_nodes && (varname_node != NULL)) {
	if (varname_node->deleted())
	    return false;
    }

    switch (type) {
    case NONE:
	return false;
    case NODE_VALUE:
	XLOG_ASSERT(varname_node != NULL);
	value = varname_node->value();
	return true;
    case NODE_OPERATOR:
	XLOG_ASSERT(varname_node != NULL);
	value = varname_node->show_operator();
	debug_msg("variable %s at %s, value is \"%s\"\n",
		  varname.c_str(), _segname.c_str(), value.c_str());
	return true;
    case NODE_ID:
	XLOG_ASSERT(varname_node != NULL);
	value = varname_node->node_id().str();
	debug_msg("variable %s at %s, value is \"%s\"\n",
		  varname.c_str(), _segname.c_str(), value.c_str());
	return true;
    case NAMED:
    {
	list<string> var_parts;

	XLOG_ASSERT(varname_node != NULL);
	if (split_up_varname(varname, var_parts) == false) {
	    return false;
	}
	value = varname_node->named_value(var_parts.back());
	return true;
    }
    case TEMPLATE_DEFAULT:
    {
	const TemplateTreeNode* ttn;
	ttn = _template_tree_node->find_const_varname_node(varname);
	XLOG_ASSERT(ttn != NULL);
	XLOG_ASSERT(ttn->has_default());
	value = ttn->default_str();
	return true;
    }
    }
    XLOG_UNREACHABLE();
}

bool
ConfigTreeNode::expand_variable_to_full_varname(const string& varname,
						string& full_varname) const
{

    VarType type = NONE;
    const ConfigTreeNode *varname_node;

    debug_msg("ConfigTreeNode::expand_variable_to_full_varname at %s: >%s<\n",
	      _segname.c_str(), varname.c_str());

    varname_node = find_const_varname_node(varname, type);

    switch (type) {
    case NONE:
	return false;
    case NODE_VALUE:
    case NODE_OPERATOR:
    case NODE_ID:
	XLOG_ASSERT(varname_node != NULL);
	full_varname = varname_node->path();
	return true;
    case NAMED:
    {
	list<string> var_parts;

	XLOG_ASSERT(varname_node != NULL);
	if (split_up_varname(varname, var_parts) == false) {
	    return false;
	}
	full_varname = varname_node->path() + " " + var_parts.back();
	return true;
    }
    case TEMPLATE_DEFAULT:
    {
	const TemplateTreeNode* ttn;
	ttn = _template_tree_node->find_const_varname_node(varname);
	XLOG_ASSERT(ttn != NULL);
	full_varname = ttn->path();
	return true;
    }
    }
    XLOG_UNREACHABLE();
}

bool
ConfigTreeNode::expand_expression(const string& expression,
				  string& value) const
{
    // Expect string of form: "`" opchar + name + "`"
    // and the only definition of opchar supported is "~".
    //
    if (expression.size() < 4) {
	return false;
    }
    if (expression[0] != '`' ||
	expression[expression.size() - 1] != '`') {
	return false;
    }

    // Trim the back-quotes
    string tmp_expr = expression.substr(1, expression.size() - 2);

    //
    // XXX: for now the only expression we can expand is the "~" boolean
    // negation operator.
    //
    if (tmp_expr[0] != '~')
	return false;

    // Trim the operator
    tmp_expr = tmp_expr.substr(1, expression.size() - 1);

    // Expand the variable
    string tmp_value;
    if (expand_variable(tmp_expr, tmp_value, true) != true)
	return false;

    if (tmp_value == "false")
	value = "true";
    else if (tmp_value == "true")
	value = "false";
    else
	return false;

    return true;
}

const ConfigTreeNode*
ConfigTreeNode::find_const_varname_node(const string& varname,
					VarType& type) const
{
    //
    // We need both const and non-const versions of find_varname_node,
    // but don't want to write it all twice.
    //
    return (const_cast<ConfigTreeNode*>(this))
	->find_varname_node(varname, type);
}

ConfigTreeNode*
ConfigTreeNode::find_varname_node(const string& varname, VarType& type)
{
    debug_msg("ConfigTreeNode::find_varname_node at %s: >%s<\n",
	      _segname.c_str(), varname.c_str());

    if (varname == "$(@)" || (varname == "$(" + _segname + ")") ) {
	XLOG_ASSERT(!_template_tree_node->is_tag());
	type = NODE_VALUE;
	debug_msg("varname node is >%s<\n", _segname.c_str());
	return this;
    }

    if (varname == "$(<>)") {
	XLOG_ASSERT(!_template_tree_node->is_tag());
	type = NODE_OPERATOR;
	debug_msg("varname node is >%s<\n", _segname.c_str());
	return this;
    }

    if (varname == "$(#)") {
	XLOG_ASSERT(!_template_tree_node->is_tag());
	type = NODE_ID;
	debug_msg("varname node is >%s<\n", _segname.c_str());
	return this;
    }

    list<string> var_parts;
    if (split_up_varname(varname, var_parts) == false) {
	return NULL;
    }

    if (var_parts.back() == "DEFAULT") {
	// XXX: use the template tree to get the default value
	const TemplateTreeNode* ttn = NULL;
	if ((var_parts.size() > 1) && (_template_tree_node != NULL)) {
	    ttn = _template_tree_node->find_const_varname_node(varname);
	} else {
	    ttn = _template_tree_node;
	}
	if ((ttn != NULL) && ttn->has_default()) {
	    type = TEMPLATE_DEFAULT;
	    return NULL;
	}
	type = NONE;
	return NULL;
    }

    ConfigTreeNode* found_node = NULL;
    if ((var_parts.front() == "@") || (_parent == NULL)) {
	_on_parent_path = true;
	found_node = find_child_varname_node(var_parts, type);
	_on_parent_path = false;
    } else if (var_parts.size() > 1) {
	// It's a parent node, or a child of a parent node
	found_node = find_parent_varname_node(var_parts, type);
    }
    if (found_node != NULL)
	return (found_node);

    // Test if we can match a template node
    if (_template_tree_node != NULL) {
	const TemplateTreeNode* ttn;
	ttn = _template_tree_node->find_const_varname_node(varname);
	if ((ttn != NULL) && ttn->has_default()) {
	    type = TEMPLATE_DEFAULT;
	    return NULL;
	}
    }

    //
    // XXX: if not referring to this node, then size of 0 or 1
    // is not valid syntax.
    //
    type = NONE;
    return NULL;
}

ConfigTreeNode*
ConfigTreeNode::find_parent_varname_node(const list<string>& var_parts, 
					 VarType& type)
{
    debug_msg("find parent at node %s\n", _segname.c_str());

    if (_parent == NULL) {
	debug_msg("reached root node\n");
	type = NONE;
	return NULL;
    }

    if (is_tag() || (this->type() == NODE_VOID)) {
	// When naming a parent node variable, you must start with a tag
	if (_segname == var_parts.front()) {
	    // We've found the right place to start
	    return find_child_varname_node(var_parts, type);
	}
    }
    _on_parent_path = true;
    ConfigTreeNode *parent, *found;
    parent = (ConfigTreeNode*)_parent;
    found = parent->find_parent_varname_node(var_parts, type);
    _on_parent_path = false;
    return found;
}

ConfigTreeNode*
ConfigTreeNode::find_child_varname_node(const list<string>& var_parts, 
					VarType& type)
{
    string s;

    s = "find child at node " + _segname + " varname: ";

    list<string>::const_iterator iter;
    for (iter = var_parts.begin(); iter != var_parts.end(); ++iter) {
	s += ">" + (*iter) + "< ";
    }
    s += "\n";
    debug_msg("%s", s.c_str());

    //
    // @ can only expand when we've already gone through this node
    // heading towards the parent.
    //
    if ((var_parts.front() == "@") && _on_parent_path == false) {
	type = NONE;
	debug_msg("no on parent path\n");
	return NULL;
    }

    if (_parent == NULL) {
	// We have reached the root node
	// The name should refer to a child of ours
	ConfigTreeNode *found_child, *child;
	list<ConfigTreeNode *>::iterator ci;
	for (ci = _children.begin(); ci != _children.end(); ++ci) {
	    child = (ConfigTreeNode*)(*ci);
	    found_child = child->find_child_varname_node(var_parts, type);
	    if (found_child != NULL)
		return found_child;
	}
	type = NONE;
	return NULL;
    }

    if ((var_parts.front() != "@")
	&& (var_parts.front() != _segname) 
	&& (var_parts.front() != "<>") 
	&& (var_parts.front() != "#") 
	&& ((!_has_value) || (var_parts.front() != _value))) {
	// varname doesn't match us.
	type = NONE;
	debug_msg("varname doesn't match\n");
	return NULL;
    }

    // The name might refer to this node
    if (var_parts.size() == 1) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    type = NODE_VALUE;
	    debug_msg("varname V node is >%s<\n", _segname.c_str());
	    return this;
	}
    }

    // The name might refer to this node for an operator
    if (var_parts.size() == 1) {
	if ((var_parts.front() == "<>")) {
	    type = NODE_OPERATOR;
	    debug_msg("varname O node is >%s<\n", _segname.c_str());
	    return this;
	}
    }

    // The name might refer to this node for a node ID
    if (var_parts.size() == 1) {
	if ((var_parts.front() == "#")) {
	    type = NODE_ID;
	    debug_msg("varname # node is >%s<\n", _segname.c_str());
	    return this;
	}
    }

    if (var_parts.size() == 2) {
	// The name might refer to a named variable on this node
	if (_variables.find(var_parts.back()) != _variables.end()) {
	    type = NAMED;
	    debug_msg("varname N node is >%s<\n", _segname.c_str());
	    return this;
	}
    }

    // The name might refer to a child of ours
    list<string> child_var_parts = var_parts;
    child_var_parts.pop_front();
    ConfigTreeNode *found_child, *child;
    list<ConfigTreeNode *>::iterator ci;
    for (ci = _children.begin(); ci != _children.end(); ++ci) {
	child = (ConfigTreeNode*)(*ci);
	found_child = child->find_child_varname_node(child_var_parts, type);
	if (found_child != NULL)
	    return found_child;
    }

    type = NONE;
    return NULL;
}

bool
ConfigTreeNode::split_up_varname(const string& varname, 
				       list<string>& var_parts) const
{
    debug_msg("split up varname >%s<\n", varname.c_str());

    if (varname.size() < 4 || varname[0] != '$'
	|| varname[1] != '(' || varname[varname.size() - 1] != ')') {
	XLOG_ERROR("Bad variable name: %s", varname.c_str());
	return false;
    }

    string trimmed = varname.substr(2, varname.size() - 3);
    var_parts = split(trimmed, '.');

    if (_verbose) {
	string debug_output;
	list<string>::iterator iter;
	for (iter = var_parts.begin(); iter != var_parts.end(); ++iter) {
	    debug_output += c_format(">%s<, ", iter->c_str());
	}
	debug_output += "\n";
	debug_msg("%s", debug_output.c_str());
    }
    return true;
}

string
ConfigTreeNode::join_up_varname(const list<string>& var_parts) const
{
    string s;

    s = "$(";
    list<string>::const_iterator iter;
    for (iter = var_parts.begin(); iter != var_parts.end(); ++iter) {
	if (iter != var_parts.begin()) {
	    s += ".";
	}
	s += *iter;
    }
    s += ")";
    return s;
}

void
ConfigTreeNode::expand_varname_to_matchlist(const vector<string>& parts,
						  size_t part,
						  list<string>& matches) const
{
    bool ok = false;

    // Don't expand unless the node has been committed
    if (! _existence_committed)
	return;

    if (is_root_node()) {
	// The root node
	ok = true;
	//
	// XXX: note that even if "part" is 0, this is compensated later by
	// the usage of "part + 1" later.
	//
	part--;		// Prevent increment of part later
    } else if ((!_parent->is_root_node()) && _parent->is_tag()) {
	// We're the varname after a tag
	if (parts[part] == "*") {
	    ok = true;
	} else {
	}
    } else {
	if (parts[part] != _segname) {
	    return;
	}
	ok = true;
    }

    if (! ok)
	return;

    if (part == parts.size() - 1) {
	// Everything that we were looking for matched
	XLOG_ASSERT(parts[part] == "*");
	matches.push_back(_segname);
	return;
    } 

    //
    // Search the children. If no more children, return the result so far
    //
    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	ConfigTreeNode *child = (ConfigTreeNode*)(*iter);
	child->expand_varname_to_matchlist(parts, part + 1, matches);
    }
}

bool
ConfigTreeNode::set_variable(const string& varname, const string& value)
{
    //
    // The variable name should be in the form "nodename.varname".
    // Nodename itself can be complex, such as "vif.@" to indicate the
    // current vif.  The general idea is that we're setting a variable
    // on a config tree node. However the config tree node that we're
    // setting the variable on doesn't have to be "this" node - this
    // node only scopes the variable name.
    //

    // First search for an existing variable to set
    VarType type;
    ConfigTreeNode* node = find_varname_node(varname, type);
    string errmsg;
    if (node != NULL) {
	switch (type) {
	case NONE:
	    // This can't happen
	    XLOG_UNREACHABLE();
	    break;
	case NODE_VALUE: 
	    if (node->is_read_only()) {
		string reason = node->read_only_reason();
		errmsg = c_format("Attempt to set node \"%s\" "
				  "which is read-only",
				  varname.c_str());
		if (! reason.empty())
		    errmsg += c_format(": %s", reason.c_str());
		XLOG_ERROR("%s", errmsg.c_str());
		return false;
	    }
	    if (node->set_value(value, _user_id, errmsg) != true) {
		errmsg = c_format("Error setting the value of node \"%s\": %s",
				  varname.c_str(), errmsg.c_str());
		XLOG_ERROR("%s", errmsg.c_str());
		return false;
	    }
	    return true;
	case NODE_OPERATOR: 
	    errmsg = c_format("Attempt to set variable operator \"%s\" "
			      "which is the name of a configuration node",
			      varname.c_str());
	    XLOG_ERROR("%s", errmsg.c_str());
	    return false;
	case NODE_ID: 
	    errmsg = c_format("Attempt to set variable node ID \"%s\" "
			      "which is not user-settable",
			      varname.c_str());
	    XLOG_ERROR("%s", errmsg.c_str());
	    return false;
	case NAMED:
	{
	    list<string> var_parts;

	    if (split_up_varname(varname, var_parts) == false) {
		return false;
	    }
	    node->set_named_value(var_parts.back(), value);
	    return true;
	}
	case TEMPLATE_DEFAULT: 
	    // XXX: Ignore, and attempt to set a named variable below.
	    break;
	}
    }

    //
    // We found no existing variable, see if there's a node on which we
    // can set a named variable.
    //
    list<string> var_parts;
    if (split_up_varname(varname, var_parts) == false) {
	return false;
    }
    list<string>::iterator iter;
    for (iter = var_parts.begin(); iter != var_parts.end(); ++iter) {
	debug_msg("VP: %s\n", iter->c_str());
    }
    if (var_parts.size() < 2) {
	errmsg = c_format("Attempt to set incomplete variable \"%s\"",
			  varname.c_str());
	XLOG_ERROR("%s", errmsg.c_str());
	return false;
    }
    var_parts.pop_back();
    if ((var_parts.front() == "@") || (_parent == NULL)) {
        _on_parent_path = true;
        node = find_child_varname_node(var_parts, type);
        _on_parent_path = false;
    } else {
        // It's a parent node, or a child of a parent node
        node = find_parent_varname_node(var_parts, type);
    }
    if (node != NULL) {
	switch (type) {
	case NONE:
	    // This can't happen
	    XLOG_UNREACHABLE();
	    break;
	case NODE_VALUE:
	    if (split_up_varname(varname, var_parts) == false) {
		return false;
	    }
	    node->set_named_value(var_parts.back(), value);
	    return true;
	case NODE_OPERATOR:
	    errmsg = c_format("Attempt to set operator \"%s\" "
			      "which is the child of a named variable",
			      varname.c_str());
	    XLOG_ERROR("%s", errmsg.c_str());
	    return false;
	case NODE_ID:
	    errmsg = c_format("Attempt to set node ID \"%s\" "
			      "which is not user-settable",
			      varname.c_str());
	    XLOG_ERROR("%s", errmsg.c_str());
	    return false;
	case NAMED:
	    errmsg = c_format("Attempt to set variable \"%s\" "
			      "which is the child of a named variable",
			      varname.c_str());
	    XLOG_ERROR("%s", errmsg.c_str());
	    return false;
	case TEMPLATE_DEFAULT:
	    errmsg = c_format("Attempt to set variable \"%s\" "
			      "which is on a non-existent node",
			      varname.c_str());
	    XLOG_ERROR("%s", errmsg.c_str());
	    return false;
	}
    }

    errmsg = c_format("Attempt to set unknown variable \"%s\"",
		      varname.c_str());
    XLOG_ERROR("%s", errmsg.c_str());
    return false;
}

void
ConfigTreeNode::set_named_value(const string& varname, const string& value)
{
    debug_msg("set_named_value %s=%s\n", varname.c_str(), value.c_str());

    _variables[varname] = value;
}


const string&
ConfigTreeNode::named_value(const string& varname) const
{
    map<string, string>::const_iterator iter;

    iter = _variables.find(varname);
    XLOG_ASSERT(iter != _variables.end());
    return iter->second;
}

int
ConfigTreeNode::child_number() const
{
    if (_template_tree_node) {
	return _template_tree_node->child_number();
    } else {
	return 0;
    }
}

void
ConfigTreeNode::sort_by_template(list<ConfigTreeNode*>& children) const
{
#ifdef XORP_USE_USTL
    sort(children, CTN_Compare());
#else
    children.sort(CTN_Compare());
#endif
} 

string
ConfigTreeNode::show_node_id(bool numbered, const ConfigNodeId& node_id) const 
{
    string s;
    if (numbered) {
	s = c_format("%%%s%% ", node_id.str().c_str());
    }
    return s;
}

void
ConfigTreeNode::allocate_unique_node_id() 
{
    ConfigTreeNode *prev = NULL, *next = NULL;
    ConfigTreeNode *effective_parent = _parent;

    XLOG_ASSERT(_node_id.is_empty());

    if (_parent == NULL)
	return;

    //
    // Node IDs are guaranteed to be unique among the children of
    // the same parent node.  If the parent node is a tag, they're
    // guaranteed to be unique among the children of the tag's parent
    // node.
    //
    debug_msg("finding order (phase 1)...\n");
    list<ConfigTreeNode *> sorted_children = _parent->children();
    sort_by_template(sorted_children);
    bool found_this = false;
    list<ConfigTreeNode*>::iterator iter;
    for (iter = sorted_children.begin(); 
	 iter != sorted_children.end();
	 ++iter) {
	if ((*iter) == this) {
	    // We found this node
	    found_this = true;
	    debug_msg("found this: %s\n", _segname.c_str());
	    continue;
	}
	if (found_this) {
	    if (next == NULL)
		next = *iter;
	    debug_msg("next: %s %s\n", next->segname().c_str(),
		      next->node_id().str().c_str());
	} else {
	    prev = *iter;
	    debug_msg("prev: %s %s\n", prev->segname().c_str(),
		      prev->node_id().str().c_str());
	}
    }

    //
    // Found the previous and the next (ordered) sibling
    //
    bool found_prev = false;
    if (prev != NULL)
	found_prev = true;
    if (_parent->is_tag() && (!is_tag()) && (_parent->parent() != NULL)) {
	effective_parent = _parent->parent();
	XLOG_ASSERT(effective_parent != NULL);

	debug_msg("node: %s effective parent: %s\n", _segname.c_str(),
		  effective_parent->segname().c_str());
	found_this = false;
	debug_msg("finding order (phase 2)...\n");
	list<ConfigTreeNode *> sorted_children = effective_parent->children();
	sort_by_template(sorted_children);
	for (iter = sorted_children.begin(); 
	     iter != sorted_children.end();
	     ++iter) {
	    if ((*iter) == _parent) {
		// We found the tag parent
		found_this = true;
		debug_msg("found this: %s\n", _segname.c_str());
		if (next != NULL)
		    break;
		continue;
	    }
	    if (found_this) {
		next = *iter;
		debug_msg("next: %s %s\n", next->segname().c_str(),
			  next->node_id().str().c_str());
		break;
	    } else {
		if (!found_prev) {
		    prev = *iter;
		    debug_msg("prev: %s %s\n", prev->segname().c_str(),
			      prev->node_id().str().c_str());
		}
	    }
	}
    }

    //
    // Calculate the unique part of the node ID
    //
    _node_id = effective_parent->node_id_generator().generate_unique_node_id();
    if (prev == NULL) {
	set_node_id_position(0);	// XXX: the first node
    } else {
	set_node_id_position(prev->node_id().unique_node_id());
    }
    debug_msg("node %s node_id %s", _segname.c_str(), _node_id.str().c_str());

    XLOG_ASSERT(! _node_id.is_empty());
}

//
// Update the position part of each node ID in case some of the
// nodes were deleted.
// Note that we update the position part only for those nodes
// that were just created.
//
void
ConfigTreeNode::update_node_id_position()
{
    ConfigTreeNode *prev = NULL;
    ConfigTreeNode *effective_parent = _parent;
    list<ConfigTreeNode*>::iterator iter;
    list<ConfigTreeNode *> sorted_children;

    if (_parent == NULL)
	goto process_subtree;

    XLOG_ASSERT(! _node_id.is_empty());

    // Update the position part only if the node was just created
    if (this->existence_committed())
	goto process_subtree;

    //
    // Found the previous node by excluding the nodes that were deleted.
    // If the parent node is a tag, then we need to consider the children
    // of the tag's parent node.
    //
    sorted_children = _parent->children();
    sort_by_template(sorted_children);
    for (iter = sorted_children.begin();
	 iter != sorted_children.end();
	 ++iter) {
	ConfigTreeNode* ctn = *iter;
	if (ctn == this)
	    break;		// We reached this node

	// Ignore nodes that were deleted
	if (ctn->deleted())
	    continue;

	// The previous node
	prev = ctn;
    }

    //
    // Found the previous (ordered) sibling
    //
    if ((prev == NULL) && _parent->is_tag() && (!is_tag())
	&& (_parent->parent() != NULL)) {
	effective_parent = _parent->parent();
	XLOG_ASSERT(effective_parent != NULL);
	sorted_children = effective_parent->children();
	sort_by_template(sorted_children);
	for (iter = sorted_children.begin(); 
	     iter != sorted_children.end();
	     ++iter) {
	    ConfigTreeNode* ctn = *iter;
	    if (ctn == _parent)
		break;		// We reached the tag parent for this node

	    // Ignore nodes that were deleted
	    if (ctn->deleted())
		continue;

	    // The previous node
	    prev = ctn;
	}
    }

    if (prev == NULL)
	set_node_id_position(0);	// XXX: the first node
    else
	set_node_id_position(prev->node_id().unique_node_id());

    //
    // Apply recursively the operation
    //
 process_subtree:
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	(*iter)->update_node_id_position();
    }
}

string
ConfigTreeNode::show_operator() const
{
    return operator_to_str(_operator);
}

bool
ConfigTreeNode::is_committed() const
{
    return (! is_uncommitted());
}

bool
ConfigTreeNode::is_uncommitted() const
{
    return ((_has_value && !_value_committed) || (!_existence_committed));
}

bool
ConfigTreeNode::is_default_value() const
{
    if (! _has_value)
	return (false);

    if (_template_tree_node == NULL)
	return (false);

    if (! _template_tree_node->has_default())
	return (false);

    return (value() == _template_tree_node->default_str());
}

bool
ConfigTreeNode::is_default_value(const string& test_value) const
{
    if (test_value.empty())
	return (false);

    if (_template_tree_node == NULL)
	return (false);

    if (! _template_tree_node->has_default())
	return (false);

    return (test_value == _template_tree_node->default_str());
}

bool
ConfigTreeNode::has_undeleted_children() const
{
    list<ConfigTreeNode *>::const_iterator iter;

    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	const ConfigTreeNode* child = *iter;
	if ((child != NULL) && (! child->deleted()))
	    return (true);
    }

    return (false);
}

string
ConfigTreeNode::quoted_value(const string& value) const
{
    string result;

    // XXX: re-escape quotes
    string escaped("");

    for (string::const_iterator i = value.begin(); i != value.end(); ++i) {
	const char x = *i;
	if (x == '"')
	    escaped += "\\\"";
	else
	    escaped += x;
    }

    result = "\"" + escaped + "\"";

    return result;
}
