// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#ifdef HAVE_GLOB_H
#include <glob.h>
#elif defined(HOST_OS_WINDOWS)
#include "glob_win32.h"
#endif



#include "command_tree.hh"
#include "conf_tree.hh"
#include "module_command.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"


TemplateTreeNode::TemplateTreeNode(TemplateTree& template_tree,
				   TemplateTreeNode* parent,
				   const string& path,
				   const string& varname)
    : _parent(parent),
      _template_tree(template_tree),
      _module_name(""),
      _default_target_name(""),
      _segname(path),
      _varname(varname),
      _has_default(false),
      _is_tag(false),
      _unique_in_node(""),
      _order(ORDER_UNSORTED),
      _verbose(template_tree.verbose()),
      _is_deprecated(false),
      _is_user_hidden(false),
      _is_mandatory(false),
      _is_read_only(false),
      _is_permanent(false)
{
    if (_parent != NULL) {
	_parent->add_child(this);
	_module_name = _parent->module_name();
	_default_target_name = _parent->default_target_name();
	_child_number = _parent->children().size();
    } else {
	_child_number = 0;
    }
}

TemplateTreeNode::~TemplateTreeNode()
{
    while (! _children.empty()) {
	delete _children.front();
	_children.pop_front();
    }

    map<string, BaseCommand *>::iterator iter1, iter2;
    iter1 = _cmd_map.begin();
    while (iter1 != _cmd_map.end()) {
	iter2 = iter1;
	++iter1;
	delete iter2->second;
	_cmd_map.erase(iter2);
    }
}

bool
TemplateTreeNode::expand_template_tree(string& error_msg)
{
    map<string, BaseCommand*>::iterator cmd_iter;

    //
    // Expand all %allow, %allow-operator and %allow-range filters.
    // XXX: currently the %allow-operator expanding is no-op.
    //
    cmd_iter = _cmd_map.find("%allow");
    if (cmd_iter != _cmd_map.end()) {
	BaseCommand* command = cmd_iter->second;
	AllowOptionsCommand* allow_options_command;
	allow_options_command = dynamic_cast<AllowOptionsCommand*>(command);
	XLOG_ASSERT(allow_options_command != NULL);
	if (allow_options_command->expand_actions(error_msg) != true)
	    return (false);
    }
    cmd_iter = _cmd_map.find("%allow-operator");
    if (cmd_iter != _cmd_map.end()) {
	BaseCommand* command = cmd_iter->second;
	AllowOperatorsCommand* allow_operators_command;
	allow_operators_command = dynamic_cast<AllowOperatorsCommand*>(command);
	XLOG_ASSERT(allow_operators_command != NULL);
	if (allow_operators_command->expand_actions(error_msg) != true)
	    return (false);
    }
    cmd_iter = _cmd_map.find("%allow-range");
    if (cmd_iter != _cmd_map.end()) {
	BaseCommand* command = cmd_iter->second;
	AllowRangeCommand* allow_range_command;
	allow_range_command = dynamic_cast<AllowRangeCommand*>(command);
	XLOG_ASSERT(allow_range_command != NULL);
	if (allow_range_command->expand_actions(error_msg) != true)
	    return (false);
    }

    //
    // Mark all referred mandatory variables
    //
    list<string>::const_iterator string_iter;
    for (string_iter = mandatory_config_nodes().begin();
	 string_iter != mandatory_config_nodes().end();
	 ++string_iter) {
	const string& mandatory_config_node = *string_iter;
	TemplateTreeNode* ttn;
	ttn = find_varname_node(mandatory_config_node);
	if (ttn == NULL) {
	    error_msg = c_format("Invalid template mandatory variable %s: "
				 "not found",
				 mandatory_config_node.c_str());
	    return (false);
	}
	bool is_multi_value = false;
	TemplateTreeNode* ttn_check_multi = ttn;
	do {
	    //
	    // Check if there is a multi-value node between the referred
	    // template node and this node (or the root of the template tree).
	    //
	    if (ttn_check_multi->is_tag()) {
		is_multi_value = true;
		break;
	    }
	    ttn_check_multi = ttn_check_multi->parent();
	    if (ttn_check_multi == this)
		break;
	    if (ttn_check_multi == NULL)
		break;
	} while (true);
	if (is_multi_value) {
	    error_msg = c_format("Invalid template mandatory variable %s: "
				 "cannot specify a multi-value node as "
				 "mandatory",
				 mandatory_config_node.c_str());
	    return (false);
	}

	ttn->set_mandatory(true);
    }

    //
    // Mark all referred unique variables
    //
    if (!unique_in_node().empty()) {
	list<string> inverted_path_to_unique;
	const string& unique_node = unique_in_node();
	TemplateTreeNode* ttn;

	ttn = find_varname_node(unique_node);
	if (ttn == NULL) {
	    error_msg = c_format("Invalid unique-in variable %s: "
		    "not found", unique_node.c_str());
	    return (false);
	}

	//Check if unique-in variable is from the same module as this variable
	if (ttn->module_name() != module_name()) {
	    error_msg = c_format("Invalid unique-in variable %s: "
			    "should be from the module %s as %s",
			    unique_node.c_str(), module_name().c_str(), _segname.c_str());
	    return (false);
	}

	//Check if unique-in variable is parent for this variable
	TemplateTreeNode* parent = _parent;

	do {
	    if (!parent) {
		error_msg = c_format("Invalid unique-in variable %s:\n"
			"it should be parent (or parent of parent, or parent of parent of parent...) of \"%s\"",
			unique_node.c_str(), path().c_str());
		return (false);
	    }
	    if (parent == ttn)
		break;

	    /**
	     * Fill path to node in which it should be unique
	     *
	     * Write names of nodes for tag nodes,
	     * and write "@:="+type for children of tag nodes
	     */
	    if (parent->segname() == "@")
		inverted_path_to_unique.push_front(
			parent->segname() + ":=" + parent->typestr());
	    else
		inverted_path_to_unique.push_front(parent->segname());

	    parent = parent->parent();
	} while (true);

	_unique_in_path = inverted_path_to_unique;
    }

    //
    // Recursively expand all children nodes
    //
    list<TemplateTreeNode*>::iterator iter2;
    for (iter2 = _children.begin(); iter2 != _children.end(); ++iter2) {
	TemplateTreeNode* ttn = *iter2;
	if (ttn->expand_template_tree(error_msg) != true)
	    return (false);
    }

    return (true);
}

bool
TemplateTreeNode::check_template_tree(string& error_msg) const
{
    const BaseCommand *base_cmd = NULL;
    map<string, BaseCommand*>::const_iterator cmd_iter;

    //
    // Check all refered variables for this node
    //
    for (cmd_iter = _cmd_map.begin(); cmd_iter != _cmd_map.end(); ++cmd_iter) {
	const BaseCommand* command = cmd_iter->second;
	if (! command->check_referred_variables(error_msg))
	    return false;
    }

    //
    // Check all referred mandatory variables
    //
    list<string>::const_iterator string_iter;
    for (string_iter = mandatory_config_nodes().begin();
	 string_iter != mandatory_config_nodes().end();
	 ++string_iter) {
	const string& mandatory_config_node = *string_iter;
	const TemplateTreeNode* ttn;
	ttn = find_const_varname_node(mandatory_config_node);
	if (ttn == NULL) {
	    error_msg = c_format("Invalid template mandatory variable %s: "
				 "not found",
				 mandatory_config_node.c_str());
	    return false;
	}
	bool is_multi_value = false;
	do {
	    //
	    // Check if there is a multi-value node between the referred
	    // template node and this node (or the root of the template tree).
	    //
	    if (ttn->is_tag()) {
		is_multi_value = true;
		break;
	    }
	    ttn = ttn->parent();
	    if (ttn == this)
		break;
	    if (ttn == NULL)
		break;
	} while (true);
	if (is_multi_value) {
	    error_msg = c_format("Invalid template mandatory variable %s: "
				 "cannot specify a multi-value node as "
				 "mandatory",
				 mandatory_config_node.c_str());
	    return false;
	}
    }

    //
    // Check all referred unique variables
    //
    if (!is_leaf_value() && !unique_in_node().empty()) {
	error_msg = c_format("Found %%unique-in command in node \"%s\" that "
		"doesn't expect value",
		path().c_str());
	return false;
    }
    if (!unique_in_node().empty()) {
	const string& unique_node = unique_in_node();
	const TemplateTreeNode* ttn;

	ttn = find_const_varname_node(unique_node);
	if (ttn == NULL) {
	    error_msg = c_format("Invalid unique-in variable %s: "
		    "not found",
		    unique_node.c_str());
	    return (false);
	}

	//Check if unique-in variable is from the same module as this variable
	if (ttn->module_name() != module_name()) {
	    error_msg =
		    c_format("Invalid unique-in variable %s: "
			    "should be from the module %s as %s",
			    unique_node.c_str(), module_name().c_str(), _segname.c_str());
	    return (false);
	}

	//Check if unique-in variable is parent for this variable
	TemplateTreeNode* parent = _parent;

	do {
	    if (!parent) {
		error_msg = c_format("Invalid unique-in variable %s: "
			"it should be (grand)parent of %s",
			unique_node.c_str(), _segname.c_str());
		return (false);
	    }
	    if (parent == ttn)
		break;

	    parent = parent->parent();
	} while (true);
    }


    //
    // Check specific commands for this node
    //
    // XXX: only leaf nodes should have %set command
    base_cmd = const_command("%set");
    if (base_cmd != NULL) {
	if (! is_leaf_value()) {
	    error_msg = c_format("Found %%set command in node \"%s\" that "
				 "doesn't expect value",
			      path().c_str());
	    return false;
	}
    }

    //
    // Recursively check all children nodes
    //
    list<TemplateTreeNode*>::const_iterator iter2;
    for (iter2 = _children.begin(); iter2 != _children.end(); ++iter2) {
	const TemplateTreeNode* ttn = *iter2;
	if (ttn->check_template_tree(error_msg) != true)
	    return false;
    }

    return true;
}

void
TemplateTreeNode::add_child(TemplateTreeNode* child)
{
    _children.push_back(child);
}

void
TemplateTreeNode::add_cmd(const string& cmd) throw (ParseError)
{
    string error_msg;
    BaseCommand* command;

    if (cmd == "%modinfo") {
	// only the MasterTemplateTree cares about this
    } else if (cmd == "%allow") {
	// If the command already exists, no need to create it again.
	// The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new AllowOptionsCommand(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%allow-range") {
	// If the command already exists, no need to create it again.
	// The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new AllowRangeCommand(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%allow-operator") {
	debug_msg("%%allow-operator\n");
	// If the command already exists, no need to create it again.
	// The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    debug_msg("creating new AllowOperatorsCommand\n");
	    command = new AllowOperatorsCommand(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%help") {
	// Nothing to do - the work is done by add_action
    } else if (cmd == "%deprecated") {
	// Nothing to do - the work is done by add_action
    } else if (cmd == "%user-hidden") {
	// Nothing to do - the work is done by add_action
    } else if (cmd == "%read-only") {
	// XXX: only leaf nodes should have %read-only command
	if (! is_leaf_value()) {
	    error_msg = c_format("Invalid command \"%s\".\n", cmd.c_str());
	    error_msg += "This command only applies to leaf nodes that have ";
	    error_msg += "values.\n";
	    xorp_throw(ParseError, error_msg);
	}
	_is_read_only = true;
	_is_permanent = true;	// XXX: read-only also implies permanent node
    } else if (cmd == "%permanent") {
	_is_permanent = true;
    } else if (cmd == "%order") {
	// Nothing to do - the work is done by add_action
    } else if ((cmd == "%create")
	       || (cmd == "%activate")
	       || (cmd == "%update")
	       || (cmd == "%list")
	       || (cmd == "%delete")
	       || (cmd == "%set")
	       || (cmd == "%unset")
	       || (cmd == "%get")
	       || (cmd == "%default")) {
	//
	// Check if we are allowed to add this command
	//
	if (cmd == "%set") {
	    // XXX: only leaf nodes should have %set command
	    if (! is_leaf_value()) {
		error_msg = c_format("Invalid command \"%s\".\n", cmd.c_str());
		error_msg += "This command only applies to leaf nodes that ";
		error_msg += "have values and only if the value is allowed ";
		error_msg += "to be changed.\n";
		xorp_throw(ParseError, error_msg);
	    }
	}

	//
	// XXX: We just create a placeholder here - if we were a master
	// template tree node we'd create a real command.
	//
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new DummyBaseCommand(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%unique-in") {
	if (!is_leaf_value()) {
	    error_msg = c_format("Invalid command \"%s\".\n", cmd.c_str());
	    error_msg += "This command only applies to leaf nodes that ";
	    error_msg += "have values and only if the value is allowed ";
	    error_msg += "to be changed.\n";
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%mandatory") {
	// Nothing to do
    } else {
	error_msg = c_format("Invalid command \"%s\".\n", cmd.c_str());
	error_msg += "Valid commands are %create, %delete, %set, %unset, ";
	error_msg += "%get, %default, %modinfo, %activate, %update, %allow, ";
	error_msg += "%allow-range, %mandatory, %deprecated, %user-hidden, ";
	error_msg += "%read-only, %permanent, %order, %unique-in\n";
	xorp_throw(ParseError, error_msg);
    }
}

set<string>
TemplateTreeNode::commands() const
{
    set<string> cmds;
    map<string, BaseCommand *>::const_iterator iter;

    for (iter = _cmd_map.begin(); iter != _cmd_map.end(); ++iter) {
	cmds.insert(iter->first);
    }
    return cmds;
}

void
TemplateTreeNode::add_action(const string& cmd,
			     const list<string>& action_list)
    throw (ParseError)
{
    BaseCommand* command;
    map<string, BaseCommand*>::iterator iter;
    string error_msg;

    if (cmd == "%modinfo") {
	// only the Master tree cares about this
    } else if (cmd == "%allow") {
	iter = _cmd_map.find("%allow");
	XLOG_ASSERT(iter != _cmd_map.end());
	command = iter->second;
	AllowCommand* allow_command = dynamic_cast<AllowCommand*>(command);
	XLOG_ASSERT(allow_command != NULL);
	allow_command->add_action(action_list);
    } else if (cmd == "%allow-range") {
	iter = _cmd_map.find("%allow-range");
	XLOG_ASSERT(iter != _cmd_map.end());
	command = iter->second;
	AllowCommand* allow_command = dynamic_cast<AllowCommand*>(command);
	XLOG_ASSERT(allow_command != NULL);
	allow_command->add_action(action_list);
    } else if (cmd == "%allow-operator") {
	iter = _cmd_map.find("%allow-operator");
	XLOG_ASSERT(iter != _cmd_map.end());
	command = iter->second;
	AllowCommand* allow_command = dynamic_cast<AllowCommand*>(command);
	XLOG_ASSERT(allow_command != NULL);
	allow_command->add_action(action_list);
    } else if (cmd == "%help") {
	if (action_list.size() == 2) {
	    list<string>::const_iterator li = action_list.begin();
	    li++;
	    // Trim off quotes if present
	    string help = unquote(*li);
	    if (action_list.front() == "short") {
		_help = help;
	    } else if (action_list.front() == "long") {
		_help_long = help;
	    } else {
		error_msg = c_format("Invalid %%help descriptor %s: "
				     "\"short\" or \"long\" expectted",
				     action_list.front().c_str());
		xorp_throw(ParseError, error_msg);
	    }
	} else {
	    error_msg = c_format("Invalid number of %%help arguments: "
				 "%u (expected 2)",
				 XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%deprecated") {
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    // Trim off quotes if present
	    string reason = unquote(*li);
	    _is_deprecated = true;
	    _deprecated_reason = reason;
	    if ((_parent != NULL) && (_parent->is_tag())) {
		_parent->set_deprecated(true);
		_parent->set_deprecated_reason(reason);
	    }
	} else {
	    error_msg = c_format("Invalid number of %%deprecated arguments: "
				 "%u (expected 1)",
				 XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%user-hidden") {
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    // Trim off quotes if present
	    string reason = unquote(*li);
	    _is_user_hidden = true;
	    _user_hidden_reason = reason;
	    if ((_parent != NULL) && (_parent->is_tag())) {
		_parent->set_user_hidden(true);
		_parent->set_user_hidden_reason(reason);
	    }
	} else {
	    error_msg = c_format("Invalid number of %%user-hidden arguments: "
				 "%u (expected 1)",
				 XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%read-only") {
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    // Trim off quotes if present
	    string reason = unquote(*li);
	    _is_read_only = true;
	    _read_only_reason = reason;
	    _is_permanent = true; // XXX: read-only also implies permanent node
	} else {
	    error_msg = c_format("Invalid number of %%user-hidden arguments: "
				 "%u (expected 1)",
				 XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%permanent") {
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    // Trim off quotes if present
	    string reason = unquote(*li);
	    _is_permanent = true;
	    _permanent_reason = reason;
	} else {
	    error_msg = c_format("Invalid number of %%permanent arguments: "
				 "%u (expected 1)",
				 XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%order") {
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    TTSortOrder order = ORDER_UNSORTED;
	    if (*li == "unsorted") {
		order = ORDER_UNSORTED;
	    } else if (*li == "sorted-numeric") {
		order = ORDER_SORTED_NUMERIC;
	    } else if (*li == "sorted-alphabetic") {
		order = ORDER_SORTED_ALPHABETIC;
	    } else {
		error_msg = c_format("Bad %%order specification in template "
				     "file ignored - should be unsorted, "
				     "sorted-numeric, or sorted-alphabetic");
		xorp_throw(ParseError, error_msg);
	    }
	    set_order(order);

	    if ((_parent != NULL) && (_parent->is_tag())) {
		_parent->set_order(order);
	    }
	} else {
	    error_msg = c_format("Invalid number of %%order arguments: "
				 "%u (expected 1)",
				 XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else if (cmd == "%mandatory") {
	// Add all new mandatory variables
	list<string>::const_iterator li;
	for (li = action_list.begin(); li != action_list.end(); ++li) {
	    const string& varname = *li;
	    if (find(_mandatory_config_nodes.begin(),
		     _mandatory_config_nodes.end(),
		     varname)
		== _mandatory_config_nodes.end()) {
		_mandatory_config_nodes.push_back(varname);
	    }
	}
    } else if (cmd == "%unique-in") {
	// Add all new  variables
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    if (!_unique_in_node.empty()) {
		error_msg = c_format("There can be only one declaration of %%unique-in argument"
			"for node. Previous was %s in node \"%s\"",
			_unique_in_node.c_str(), path().c_str());
		xorp_throw(ParseError, error_msg);
	    }
	    _unique_in_node = *li;
	} else {
	    error_msg = c_format("Invalid number of %%unique-in arguments: "
		    "%u (expected 1)",
		    XORP_UINT_CAST(action_list.size()));
	    xorp_throw(ParseError, error_msg);
	}
    } else {
	// the master tree will deal with these
    }
}

map<string, string>
TemplateTreeNode::create_variable_map(const list<string>& segments) const
{
    map<string,string> varmap;
    const TemplateTreeNode* ttn = this;
    list<string>::const_reverse_iterator iter;

    for (iter = segments.rbegin(); iter != segments.rend(); ++iter) {
	if (ttn->name_is_variable())
	    varmap[ttn->segname()] = *iter;
	ttn = ttn->parent();
    }
    return varmap;
}

string
TemplateTreeNode::path() const
{
    string path;

    if (_parent != NULL) {
	string parent_path = _parent->path();
	if (parent_path == "")
	    path = _segname;
	else
	    path = parent_path + " " + _segname;
    } else {
	path = "";
    }
    return path;
}

bool
TemplateTreeNode::is_module_root_node() const
{
    string my_module_name, parent_module_name;

    my_module_name = module_name();
    if (my_module_name.empty())
	return (false);		// XXX: this node doesn't belong to any module

    if (parent() != NULL)
	parent_module_name = parent()->module_name();

    return (my_module_name != parent_module_name);
}

bool
TemplateTreeNode::is_leaf_value() const
{
    if ((type() != NODE_VOID) && (_parent != NULL) && (!_parent->is_tag()))
	return true;

    return false;
}

list<ConfigOperator>
TemplateTreeNode::allowed_operators() const
{
    list<ConfigOperator> result;

    const BaseCommand* c = const_command("%allow-operator");
    if (c == NULL)
	return result;

    const AllowOperatorsCommand* cmd;
    cmd = dynamic_cast<const AllowOperatorsCommand *>(c);
    if (cmd == NULL)
	return result;

    result = cmd->allowed_operators();
    return result;
}

string
TemplateTreeNode::str() const
{
    string tmp;

    tmp = path() + " Type:" + typestr();
    if (has_default()) {
	tmp += " = " + default_str();
    }
    if (!_cmd_map.empty()) {
	map<string, BaseCommand*>::const_iterator rpair;
	rpair = _cmd_map.begin();
	while (rpair != _cmd_map.end()) {
	    tmp += "\n    " + rpair->second->str();
	    ++rpair;
	}
    }
    return tmp;
}

string
TemplateTreeNode::subtree_str() const
{
    string s;

    s = c_format("%s\n", str().c_str());

    list<TemplateTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	s += (*iter)->subtree_str();
    }

    return s;
}

string
TemplateTreeNode::encoded_typestr() const
{
    return "<" + typestr() + ">";
}

bool
TemplateTreeNode::type_match(const string&, string& ) const
{
    return true;
}

BaseCommand*
TemplateTreeNode::command(const string& cmd_name)
{
    map<string, BaseCommand *>::iterator iter;

    iter = _cmd_map.find(cmd_name);
    if (iter == _cmd_map.end())
	return NULL;
    return iter->second;
}

const BaseCommand*
TemplateTreeNode::const_command(const string& cmd_name) const
{
    map<string, BaseCommand *>::const_iterator iter;

    iter = _cmd_map.find(cmd_name);
    if (iter == _cmd_map.end())
	return NULL;
    return iter->second;
}

string
TemplateTreeNode::strip_quotes(const string& s) const
{
    size_t len = s.length();

    if (len < 2)
	return s;
    if ((s[0] == '"') && (s[len - 1] == '"'))
	return s.substr(1, len - 2);
    else
	return s;
}

bool
TemplateTreeNode::name_is_variable() const
{
    if (_segname.size() < 4)
	return false;
    if (_segname[0] != '$' || _segname[1] != '(')
	return false;
    return true;
}

bool
TemplateTreeNode::expand_variable(const string& varname, string& value,
				  bool ignore_deleted_nodes) const
{
    const TemplateTreeNode* varname_node;

    UNUSED(ignore_deleted_nodes);

    varname_node = find_const_varname_node(varname);
    if (varname_node == NULL)
	return false;

    list<string> var_parts;
    if (split_up_varname(varname, var_parts) == false) {
	return false;
    }
    const string& nodename = var_parts.back();

    XLOG_ASSERT((nodename == varname_node->segname())
		|| (nodename == "DEFAULT"));
    if (! varname_node->has_default())
	return false;		// XXX: the variable has no default value
    value = varname_node->default_str();
    return true;
}

bool
TemplateTreeNode::expand_expression(const string& expression,
				    string& value) const
{
    // Expecting at least "`~" + a_name + "`"
    if (expression.size() < 4) {
	return false;
    }

    if ((expression[0] != '`') || (expression[expression.size() - 1] != '`'))
	return false;

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

void
TemplateTreeNode::set_subtree_module_name(const string& module_name)
{
    //
    // Recursively set the module name to the subtree below this node
    //
    _module_name = module_name;

    list<TemplateTreeNode *>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	TemplateTreeNode* child = *iter;
	child->set_subtree_module_name(module_name);
    }
}

void
TemplateTreeNode::set_subtree_default_target_name(const string& default_target_name)
{
    //
    // Recursively set the default target name to the subtree below this node
    //
    _default_target_name = default_target_name;

    list<TemplateTreeNode *>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	TemplateTreeNode* child = *iter;
	child->set_subtree_default_target_name(default_target_name);
    }
}

string
TemplateTreeNode::get_module_name_by_variable(const string& varname) const
{
    const TemplateTreeNode* ttn;

    ttn = find_const_varname_node(varname);
    //
    // Search all template tree nodes toward the root
    // to find the module name.
    //
    while (ttn != NULL) {
	string module_name = ttn->module_name();
	if (module_name.length() > 0)
	    return (module_name);
	ttn = ttn->parent();
    }

    return "";		// XXX: nothing found
}

string
TemplateTreeNode::get_default_target_name_by_variable(const string& varname) const
{
    const TemplateTreeNode* ttn;

    ttn = find_const_varname_node(varname);
    if (ttn != NULL) {
	//
	// Search all template tree nodes toward the root
	// to find the default target name.
	//
	for ( ; ttn != NULL; ttn = ttn->parent()) {
	    string default_target_name = ttn->default_target_name();
	    if (default_target_name.length() > 0)
		return (default_target_name);
	}
    }

    return "";		// XXX: nothing found
}

bool
TemplateTreeNode::split_up_varname(const string& varname,
				   list<string>& var_parts) const
{
    if (varname.size() < 4)
	return false;

    if (varname[0] != '$'
	|| varname[1] != '('
	|| varname[varname.size() - 1] != ')') {
	return false;
    }

    string trimmed = varname.substr(2, varname.size() - 3);
    var_parts = split(trimmed, '.');

    return true;
}

const TemplateTreeNode*
TemplateTreeNode::find_const_varname_node(const string& varname) const
{
    //
    // We need both const and non-const versions of find_varname_node,
    // but don't want to write it all twice.
    //
    return (const_cast<TemplateTreeNode*>(this))->find_varname_node(varname);
}

TemplateTreeNode*
TemplateTreeNode::find_varname_node(const string& varname)
{
    if (varname == "$(@)" /* current value of a node */
	|| (varname == "$(DEFAULT)") /* default value of a node */
	|| (varname == "$(<>)") /* operator for a terminal node */
	|| (varname == "$(#)") /* the node ID of a node */
	|| (varname == "$(" + _segname + ")") ) {
	XLOG_ASSERT(! is_tag());
	if (varname == "$(DEFAULT)") {
	    if (! has_default())
		return NULL;	// The template tree node has no default value
	}
	return this;
    }

    // Split varname
    list<string> var_parts;
    if (split_up_varname(varname, var_parts) == false) {
	return NULL;
    }

    if (var_parts.front() == "@") {
	return find_child_varname_node(var_parts, type());
    }

    if (var_parts.back() == "@" && var_parts.size() > 2) {
	/**
	 * If we entered here, we want to find children
	 * of some of our parents
	 */
	TTNodeType _type = (_parent && _parent->segname() == "@") ? _parent->type() : NODE_VOID;
	list<string> parent_node_parts;

	parent_node_parts.push_back(var_parts.front());
	parent_node_parts.push_back(var_parts.back());

	var_parts.pop_front();
	var_parts.pop_back();
	var_parts.push_front("@");

	TemplateTreeNode* parent = find_parent_varname_node(parent_node_parts, _type);

	if (parent) {
	    return parent->find_child_varname_node(var_parts, parent->type());
	}
	return NULL;
    }

    if (var_parts.size() > 1) {
	TTNodeType _type = (_parent && _parent->segname() == "@") ? _parent->type() : NODE_VOID;
	// It's a parent node, or a child of a parent node
	return find_parent_varname_node(var_parts, _type);
    }

    //
    // XXX: if not referring to this node, then size of 0 or 1
    // is not valid syntax.
    //
    return NULL;
}

TemplateTreeNode*
TemplateTreeNode::find_parent_varname_node(const list<string>& var_parts, const TTNodeType& _type)
{
    if (_parent == NULL) {
	//
	// We have reached the root node.
	// Search among the children.
	//
	list<TemplateTreeNode* >::iterator iter;
	for (iter = _children.begin(); iter != _children.end(); ++iter) {
	    TemplateTreeNode* found_child;
	    found_child = (*iter)->find_child_varname_node(var_parts, NODE_VOID);
	    if (found_child != NULL)
		return found_child;
	}
	return NULL;
    }

    if (is_tag() || (type() == NODE_VOID)) {

	// When naming a parent node variable, you must start with a tag
	if (_segname == var_parts.front()) {
	    // We've found the right place to start
	    return find_child_varname_node(var_parts, (type() == NODE_VOID) ? _type : type());
	}
    }
    return _parent->find_parent_varname_node(var_parts,
	    (_parent && _parent->segname() == "@") ? _parent->type() : _type);
}

TemplateTreeNode*
TemplateTreeNode::find_child_varname_node(const list<string>& var_parts, const TTNodeType& _type)
{
    if ((var_parts.front() != "@") && (var_parts.front() != _segname)) {
	// varname doesn't match us
	return NULL;
    }
    // The name might refer to this node
    if (var_parts.size() == 1) {
	if ((var_parts.front() == "@")
	    || (var_parts.front() == _segname)
	    || (var_parts.front() == "<>")
	    || (var_parts.front() == "#")) {
	    if (_type != type() && _segname == "@") {
		debug_msg("We are searching for node with different type. Skip this node.\n");
		return NULL;
	    }
	    return this;
	}
    }

    // The name might refer to the default value of this node
    if ((var_parts.size() == 2) && (var_parts.back() == "DEFAULT")) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    // The name refers to the default value of this node
	    if (! has_default())
		return NULL;	// The template tree node has no default value
	    if (_type != type() && _segname == "@") {
		debug_msg("We are searching for node with different type. Skip this node.\n");
		return NULL;
	    }
	    return this;
	}
    }

    // The name might refer to the operator value of this node
    if ((var_parts.size() == 2) && (var_parts.back() == "<>")) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    if (_type != type() && _segname == "@") {
		debug_msg("We are searching for node with different type. Skip this node.\n");
		return NULL;
	    }
	    return this;
	}
    }

    // The name might refer to the node ID of this node
    if ((var_parts.size() == 2) && (var_parts.back() == "#")) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    if (_type != type() && _segname == "@") {
		debug_msg("We are searching for node with different type. Skip this node.\n");
		return NULL;
	    }
	    return this;
	}
    }

    // The name might refer to a child of ours
    list<string> child_var_parts = var_parts;
    child_var_parts.pop_front();

    list<TemplateTreeNode* >::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	TemplateTreeNode* found_child;
	found_child = (*iter)->find_child_varname_node(child_var_parts, (type() == NODE_VOID) ? _type : type());
	if (found_child != NULL)
	    return found_child;
    }

    return NULL;
}

const TemplateTreeNode*
TemplateTreeNode::find_first_deprecated_ancestor() const
{
    const TemplateTreeNode* ttn = _parent;
    const TemplateTreeNode* deprecated_ttn = NULL;

    if (is_deprecated())
	deprecated_ttn = this;

    while (ttn != NULL) {
	if (ttn->is_deprecated())
	    deprecated_ttn = ttn;
	ttn = ttn->parent();
    }

    return (deprecated_ttn);
}

const TemplateTreeNode*
TemplateTreeNode::find_first_user_hidden_ancestor() const
{
    const TemplateTreeNode* ttn = _parent;
    const TemplateTreeNode* user_hidden_ttn = NULL;

    if (is_user_hidden())
	user_hidden_ttn = this;

    while (ttn != NULL) {
	if (ttn->is_user_hidden())
	    user_hidden_ttn = ttn;
	ttn = ttn->parent();
    }

    return (user_hidden_ttn);
}

void
TemplateTreeNode::add_allowed_value(const string& value, const string& help)
{
    // XXX: insert the new pair even if we overwrite an existing one
    _allowed_values.insert(make_pair(value, help));
}

void
TemplateTreeNode::add_allowed_range(int64_t lower_value, int64_t upper_value,
				    const string& help)
{
    pair<int64_t, int64_t> range(lower_value, upper_value);

    // XXX: insert the new pair even if we overwrite an existing one
    _allowed_ranges.insert(make_pair(range, help));
}

bool
TemplateTreeNode::check_allowed_value(const string& value,
				      string& error_msg) const
{
    //
    // Check the allowed values
    //
    if ((! _allowed_values.empty())
	&& (_allowed_values.find(value) == _allowed_values.end())) {
	map<string, string> values = _allowed_values;

	if (values.size() == 1) {
	    error_msg = c_format("The only value allowed is %s.",
				 values.begin()->first.c_str());
	} else {
	    error_msg = "Allowed values are: ";
	    bool is_first = true;
	    while (! values.empty()) {
		if (is_first) {
		    is_first = false;
		} else {
		    if (values.size() == 1)
			error_msg += " and ";
		    else
			error_msg += ", ";
		}
		map<string, string>::iterator iter = values.begin();
		error_msg += iter->first;
		values.erase(iter);
	    }
	    error_msg += ".";
	}
	return (false);
    }

    //
    // Check the allowed ranges
    //
    bool is_accepted = true;
    int64_t lower_value = 0;
    int64_t upper_value = 0;
    //
    // XXX: it is sufficient for the variable's value to belong to any
    // of the allowed ranges.
    //
    if (! _allowed_ranges.empty()) {
	is_accepted = false;
	map<pair<int64_t, int64_t>, string>::const_iterator iter;
	int64_t ival = strtoll(value.c_str(), (char **)NULL, 10);
	for (iter = _allowed_ranges.begin();
	     iter != _allowed_ranges.end();
	     ++iter) {
	    const pair<int64_t, int64_t>& range = iter->first;
	    lower_value = range.first;
	    upper_value = range.second;
	    if ((ival >= lower_value) && (ival <= upper_value)) {
		is_accepted = true;
		break;
	    }
	}
    }

    if (! is_accepted) {
	map<pair<int64_t, int64_t>, string> ranges = _allowed_ranges;

	if (ranges.size() == 1) {
	    const pair<int64_t, int64_t>& range = ranges.begin()->first;
	    ostringstream ost;
	    ost << "[" << range.first << ".." << range.second << "]";
	    error_msg = c_format("The only range allowed is %s.",
				 ost.str().c_str());
	} else {
	    error_msg = "Allowed ranges are: ";
	    bool is_first = true;
	    while (! ranges.empty()) {
		if (is_first) {
		    is_first = false;
		} else {
		    if (ranges.size() == 1)
			error_msg += " and ";
		    else
			error_msg += ", ";
		}
		map<pair<int64_t, int64_t>, string>::iterator iter;
		iter = ranges.begin();
		const pair<int64_t, int64_t>& range = iter->first;
		ostringstream ost;
		ost << "[" << range.first << ".." << range.second << "]";
		error_msg += c_format("%s", ost.str().c_str());
		ranges.erase(iter);
	    }
	    error_msg += ".";
	}
	return (false);
    }

    return (true);
}

bool
TemplateTreeNode::verify_variables(const ConfigTreeNode& ctn,
				   string& error_msg) const
{
    map<string, BaseCommand *>::const_iterator iter;

    for (iter = _cmd_map.begin(); iter != _cmd_map.end(); ++iter) {
	const BaseCommand* command = iter->second;
	const AllowCommand* allow_command;
	allow_command = dynamic_cast<const AllowCommand *>(command);
	if (allow_command == NULL)
	    continue;
	if (allow_command->verify_variables(ctn, error_msg) != true)
	    return (false);
    }

    return (true);
}

const string&
TemplateTreeNode::help() const
{
    // If the node is a tag, the help is held on the child
    if (is_tag())
	return children().front()->help();
    return _help;
}

const string&
TemplateTreeNode::help_long() const
{
    if (is_tag())
	return children().front()->help_long();
    if (_help_long == "")
	return help();
    return _help_long;
}

#if 0
bool
TemplateTreeNode::check_template_tree(string& error_msg) const
{
    //
    // First check this node, then check recursively all children nodes
    //

    //
    // Check whether all referred variable names are valid
    //
    map<string, BaseCommand *>::const_iterator iter1;
    for (iter1 = _cmd_map.begin(); iter1 != _cmd_map.end(); ++iter1) {
	const Command* command;
	command = dynamic_cast<Command*>(iter1->second);
	if (command) {
	    if (! command->check_referred_variables(error_msg))
		return false;
	}
    }

    //
    // Recursively check all children nodes
    //
    list<TemplateTreeNode*>::const_iterator iter2;
    for (iter2 = _children.begin(); iter2 != _children.end(); ++iter2) {
	const TemplateTreeNode* ttn = *iter2;
	if (ttn->check_template_tree(error_msg) != true)
	    return false;
    }

    return true;
}
#endif // 0

bool
TemplateTreeNode::check_command_tree(const list<string>& cmd_names,
				     bool include_intermediate_nodes,
				     bool include_read_only_nodes,
				     bool include_permanent_nodes,
				     bool include_user_hidden_nodes,
				     size_t depth) const
{
    bool instantiated = false;

    // XXX: ignore deprecated subtrees
    if (is_deprecated())
	return false;

    // XXX: ignore read-only nodes, permanent nodes and user-hidden nodes
    if (is_read_only() && (! include_read_only_nodes))
	return false;
    if (is_permanent() && (! include_permanent_nodes))
	return false;
    if (is_user_hidden() && (! include_user_hidden_nodes))
	return false;

    debug_msg("TTN:check_command_tree %s type %s depth %u\n",
	      _segname.c_str(), typestr().c_str(), XORP_UINT_CAST(depth));

    if (_parent != NULL && _parent->is_tag() && (depth == 0)) {
	//
	// If the parent is a tag, then this node must be a pure
	// variable. We don't want to instantiate any pure variable
	// nodes in the command tree.
	//
	debug_msg("pure variable\n");
	return false;
    }

    list<string>::const_iterator iter;
    for (iter = cmd_names.begin(); iter != cmd_names.end(); ++iter) {
	if (const_command(*iter) != NULL) {
	    debug_msg("node has command %s\n", (*iter).c_str());
	    instantiated = true;
	    break;
	} else {
	    debug_msg("node doesn't have command %s\n", (*iter).c_str());
	}
    }

    //
    // When we're building a command tree we only want to go one level
    // into the template tree beyond the part of the tree that is
    // instantiated by the config tree.  The exception is if the first
    // level node is a tag or a VOID node, then we need to check
    // further to find a real node.
    //
    if (_is_tag || ((type() == NODE_VOID) && include_intermediate_nodes)) {
	list<TemplateTreeNode*>::const_iterator tti;
	for (tti = _children.begin(); tti != _children.end(); ++tti) {
	    if ((*tti)->check_command_tree(cmd_names,
					   include_intermediate_nodes,
					   include_read_only_nodes,
					   include_permanent_nodes,
					   include_user_hidden_nodes,
					   depth + 1)) {
		instantiated = true;
		break;
	    }
	}
    }
    return instantiated;
}

bool
TemplateTreeNode::check_variable_name(const vector<string>& parts,
				      size_t part) const
{
    bool ok = false;

    debug_msg("check_variable_name: us>%s< match>%s<, %d\n",
	      _segname.c_str(), parts[part].c_str(), static_cast<int>(part));

    do {
	if (_parent == NULL) {
	    // Root node
	    ok = true;
	    part--; // Prevent increment of part later
	    break;
	}

	if (_parent->is_tag()) {
	    // We're the varname after a tag
	    if (parts[part] == "*") {
		ok = true;
	    } else {
		debug_msg("--varname but not wildcarded\n");
	    }
	    break;
	}

	if (parts[part] != _segname) {
	    debug_msg("--mismatch\n");
	    return false;
	}

	ok = true;
	break;
    } while (false);

    if (! ok)
	return false;

    if (part == parts.size() - 1) {
	// Everything that we were looking for matched
	// Final check that we haven't finished at a tag
	if (is_tag())
	    return (false);
	debug_msg("**match successful**\n");
	return true;
    }

    if (_children.empty()) {
	// No more children, but the search string still has components
	debug_msg("--no children\n");
	return false;
    }

    list<TemplateTreeNode*>::const_iterator tti;
    for (tti = _children.begin(); tti != _children.end(); ++tti) {
	if ((*tti)->check_variable_name(parts, part + 1)) {
	    return true;
	}
    }
    debug_msg("--no successful children, %d\n", static_cast<int>(part));

    return false;
}

/**************************************************************************
 * TextTemplate
 **************************************************************************/

TextTemplate::TextTemplate(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default("")
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad Text type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
TextTemplate::type_match(const string&, string&) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // text value.
    //
    return true;
}


/**************************************************************************
 * ArithTemplate
 **************************************************************************/

ArithTemplate::ArithTemplate(TemplateTree& template_tree,
			     TemplateTreeNode* parent,
			     const string& path, const string& varname,
			     const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default("")
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad arith type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
ArithTemplate::type_match(const string&, string&) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // arith value.
    //
    return true;
}


/**************************************************************************
 * UIntTemplate
 **************************************************************************/

UIntTemplate::UIntTemplate(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname)
{
    string error_msg;

    if (initializer.empty())
	return;

    string s = strip_quotes(initializer);
    if (! type_match(s, error_msg)) {
	error_msg = c_format("Bad UInt type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }
    _default = strtoll(s.c_str(), (char **)NULL, 10);
    set_has_default();
}

bool
UIntTemplate::type_match(const string& orig, string& error_msg) const
{
    string s = strip_quotes(orig);

    for (size_t i = 0; i < s.length(); i++) {
	if (s[i] < '0' || s[i] > '9') {
	    if (s[i]=='-') {
		error_msg = "value cannot be negative";
	    } else if (s[i]=='.') {
		error_msg = "value must be an integer";
	    } else {
		error_msg = "value must be numeric";
	    }
	    return false;
	}
    }
    return check_allowed_value(orig, error_msg);
}

string
UIntTemplate::default_str() const
{
    return c_format("%u", _default);
}


/**************************************************************************
 * UIntRangeTemplate
 **************************************************************************/

UIntRangeTemplate::UIntRangeTemplate(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new U32Range(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad U32Range type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

UIntRangeTemplate::~UIntRangeTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
UIntRangeTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
UIntRangeTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be a valid range of unsigned 32-bit integers";
	return false;
    }

    try {
	U32Range* u32range = new U32Range(tmp.c_str());
	delete u32range;
    } catch (InvalidString&) {
	error_msg = "value must be a valid range of unsigned 32-bit integers";
	return false;
    }
    return true;
}


/**************************************************************************
 * ULongTemplate
 **************************************************************************/

ULongTemplate::ULongTemplate(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname)
{
    string error_msg;

    if (initializer.empty())
	return;

    string s = strip_quotes(initializer);
    if (! type_match(s, error_msg)) {
	error_msg = c_format("Bad ULong type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }
    _default = strtoll(s.c_str(), (char **)NULL, 10);
    set_has_default();
}

bool
ULongTemplate::type_match(const string& orig, string& error_msg) const
{
    string s = strip_quotes(orig);

    for (size_t i = 0; i < s.length(); i++) {
	if (s[i] < '0' || s[i] > '9') {
	    if (s[i]=='-') {
		error_msg = "value cannot be negative";
	    } else if (s[i]=='.') {
		error_msg = "value must be an integer";
	    } else {
		error_msg = "value must be numeric";
	    }
	    return false;
	}
    }
    return check_allowed_value(orig, error_msg);
}

string
ULongTemplate::default_str() const
{
    ostringstream oss;
    oss << _default;
    return oss.str();
}


/**************************************************************************
 * ULongRangeTemplate
 **************************************************************************/

ULongRangeTemplate::ULongRangeTemplate(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new U64Range(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad U64Range type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

ULongRangeTemplate::~ULongRangeTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
ULongRangeTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
ULongRangeTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be a valid range of unsigned 64-bit integers";
	return false;
    }

    try {
	U64Range* u64range = new U64Range(tmp.c_str());
	delete u64range;
    } catch (InvalidString&) {
	error_msg = "value must be a valid range of unsigned 64-bit integers";
	return false;
    }
    return true;
}


/**************************************************************************
 * IntTemplate
 **************************************************************************/

IntTemplate::IntTemplate(TemplateTree& template_tree,
			 TemplateTreeNode* parent,
			 const string& path, const string& varname,
			 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname)
{
    string error_msg;

    if (initializer.empty())
	return;

    string s = strip_quotes(initializer);
    if (! type_match(s, error_msg)) {
	error_msg = c_format("Bad Int type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }
    _default = strtoll(s.c_str(), (char **)NULL, 10);
    set_has_default();
}

bool
IntTemplate::type_match(const string& orig, string& error_msg) const
{
    string s = strip_quotes(orig);
    size_t start = 0;
    if (s[0] == '-') {
	if (s.length() == 1) {
	    error_msg = "value must be an integer";
	    return false;
	}
	start = 1;
    }
    for (size_t i = start; i < s.length(); i++)
	if (s[i] < '0' || s[i] > '9') {
	    if (s[i]=='.') {
		error_msg = "value must be an integer";
	    } else {
		error_msg = "value must be numeric";
	    }
	    return false;
	}
    return check_allowed_value(orig, error_msg);
}

string
IntTemplate::default_str() const
{
    return c_format("%d", _default);
}


/**************************************************************************
 * BoolTemplate
 **************************************************************************/

BoolTemplate::BoolTemplate(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname)
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad Bool type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }
    if (initializer == string("false"))
	_default = false;
    else
	_default = true;
    set_has_default();
}

bool
BoolTemplate::type_match(const string& s, string& error_msg) const
{
    if (s == "true")
	return true;
    else if (s == "false")
	return true;
    error_msg = "value must be \"true\" or \"false\"";
    return false;
}

string
BoolTemplate::default_str() const
{
    if (_default)
	return "true";
    else
	return "false";
}


/**************************************************************************
 * IPv4Template
 **************************************************************************/

IPv4Template::IPv4Template(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new IPv4(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad IPv4 type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

IPv4Template::~IPv4Template()
{
    if (_default != NULL)
	delete _default;
}

string
IPv4Template::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
IPv4Template::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be an IP address in dotted decimal form";
	return false;
    }

    try {
	IPv4* ipv4 = new IPv4(tmp.c_str());
	delete ipv4;
    } catch (InvalidString&) {
	error_msg = "value must be an IP address in dotted decimal form";
	return false;
    }
    return true;
}


/**************************************************************************
 * IPv4NetTemplate
 **************************************************************************/

IPv4NetTemplate::IPv4NetTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new IPv4Net(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad IPv4Net type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    } catch (InvalidNetmaskLength&) {
	error_msg = c_format("Illegal IPv4 prefix length in subnet \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

IPv4NetTemplate::~IPv4NetTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
IPv4NetTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
IPv4NetTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be an IPv4 subnet in address/prefix-length form";
	return false;
    }

    try {
	IPv4Net ipv4net = IPv4Net(tmp.c_str());
	string::size_type slash = tmp.find('/');
	XLOG_ASSERT(slash != string::npos);
	IPv4 ipv4(tmp.substr(0, slash).c_str());
	if (ipv4 != ipv4net.masked_addr()) {
	    error_msg = "there is a mismatch between the masked address value and the prefix length";
	    return false;
	}
    } catch (InvalidString&) {
	error_msg = "value must be an IPv4 subnet in address/prefix-length form";
	return false;
    } catch (InvalidNetmaskLength&) {
	error_msg = c_format("prefix length must be an integer between "
			     "0 and %u",
			     IPv4::addr_bitlen());
	return false;
    }
    return true;
}


/**************************************************************************
 * IPv4RangeTemplate
 **************************************************************************/

IPv4RangeTemplate::IPv4RangeTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new IPv4Range(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad IPv4Range type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

IPv4RangeTemplate::~IPv4RangeTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
IPv4RangeTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
IPv4RangeTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "invalid format";
	return false;
    }

    try {
	IPv4Range* ipv4range = new IPv4Range(tmp.c_str());
	delete ipv4range;
    } catch (InvalidString&) {
	error_msg = "invalid format";
	return false;
    }
    return true;
}


/**************************************************************************
 * IPv6Template
 **************************************************************************/

IPv6Template::IPv6Template(TemplateTree& template_tree,
			   TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new IPv6(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad IPv6 type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

IPv6Template::~IPv6Template()
{
    if (_default != NULL)
	delete _default;
}

string
IPv6Template::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
IPv6Template::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be an IPv6 address";
	return false;
    }

    try {
	IPv6* ipv6 = new IPv6(tmp.c_str());
	delete ipv6;
    } catch (InvalidString&) {
	error_msg = "value must be an IPv6 address";
	return false;
    }
    return true;
}


/**************************************************************************
 * IPv6NetTemplate
 **************************************************************************/

IPv6NetTemplate::IPv6NetTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new IPv6Net(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad IPv6Net type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    } catch (InvalidNetmaskLength&) {
	error_msg = c_format("Illegal IPv6 prefix length in subnet \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

IPv6NetTemplate::~IPv6NetTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
IPv6NetTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
IPv6NetTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be an IPv6 subnet in address/prefix-length "
	    "form";
	return false;
    }

    try {
	IPv6Net ipv6net = IPv6Net(tmp.c_str());
	string::size_type slash = tmp.find('/');
	XLOG_ASSERT(slash != string::npos);
	IPv6 ipv6(tmp.substr(0, slash).c_str());
	if (ipv6 != ipv6net.masked_addr()) {
	    error_msg = "there is a mismatch between the masked address value and the prefix length";
	    return false;
	}
    } catch (InvalidString&) {
	error_msg = "value must be an IPv6 subnet in address/prefix-length "
	    "form";
	return false;
    } catch (InvalidNetmaskLength&) {
	error_msg = c_format("prefix length must be an integer between "
			     "0 and %u",
			     IPv6::addr_bitlen());
	return false;
    }
    return true;
}


/**************************************************************************
 * IPv6RangeTemplate
 **************************************************************************/

IPv6RangeTemplate::IPv6RangeTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new IPv6Range(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad IPv6Range type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

IPv6RangeTemplate::~IPv6RangeTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
IPv6RangeTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
IPv6RangeTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "invalid format";
	return false;
    }

    try {
	IPv6Range* ipv6range = new IPv6Range(tmp.c_str());
	delete ipv6range;
    } catch (InvalidString&) {
	error_msg = "invalid format";
	return false;
    }
    return true;
}


/**************************************************************************
 * MacaddrTemplate
 **************************************************************************/

MacaddrTemplate::MacaddrTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default(NULL)
{
    string error_msg;

    if (initializer.empty())
	return;

    try {
	_default = new Mac(initializer.c_str());
    } catch (InvalidString&) {
	error_msg = c_format("Bad MacAddr type value \"%s\".",
			     initializer.c_str());
	xorp_throw(ParseError, error_msg);
    }
    set_has_default();
}

MacaddrTemplate::~MacaddrTemplate()
{
    if (_default != NULL)
	delete _default;
}

string
MacaddrTemplate::default_str() const
{
    if (_default != NULL)
	return _default->str();

    return "";
}

bool
MacaddrTemplate::type_match(const string& s, string& error_msg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	error_msg = "value must be an MAC address (six hex digits separated "
	    "by colons)";
	return false;
    }

    try {
	Mac* mac = new Mac(tmp.c_str());
	delete mac;
    } catch (InvalidString&) {
	error_msg = "value must be an MAC address (six hex digits separated "
	    "by colons)";
	return false;
    }
    return true;
}


/**************************************************************************
 * UrlFileTemplate
 **************************************************************************/

UrlFileTemplate::UrlFileTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default("")
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad UrlFile type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
UrlFileTemplate::type_match(const string&, string&) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // text value.
    //
    return true;
}


/**************************************************************************
 * UrlFtpTemplate
 **************************************************************************/

UrlFtpTemplate::UrlFtpTemplate(TemplateTree& template_tree,
			       TemplateTreeNode* parent,
			       const string& path, const string& varname,
			       const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default("")
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad UrlFtp type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
UrlFtpTemplate::type_match(const string&, string&) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // text value.
    //
    return true;
}


/**************************************************************************
 * UrlHttpTemplate
 **************************************************************************/

UrlHttpTemplate::UrlHttpTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default("")
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad UrlHttp type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
UrlHttpTemplate::type_match(const string&, string&) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // text value.
    //
    return true;
}


/**************************************************************************
 * UrlTftpTemplate
 **************************************************************************/

UrlTftpTemplate::UrlTftpTemplate(TemplateTree& template_tree,
				 TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer) throw (ParseError)
    : TemplateTreeNode(template_tree, parent, path, varname),
      _default("")
{
    string error_msg;

    if (initializer.empty())
	return;

    if (! type_match(initializer, error_msg)) {
	error_msg = c_format("Bad UrlTftp type value \"%s\": %s.",
			     initializer.c_str(), error_msg.c_str());
	xorp_throw(ParseError, error_msg);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
UrlTftpTemplate::type_match(const string&, string&) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // text value.
    //
    return true;
}
