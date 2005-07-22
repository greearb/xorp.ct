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

#ident "$XORP: xorp/rtrmgr/template_tree_node.cc,v 1.51 2005/07/22 19:38:53 pavlin Exp $"


#include <glob.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

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
      _order(ORDER_UNSORTED),
      _verbose(template_tree.verbose()),
      _is_deprecated(false)
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

void
TemplateTreeNode::add_child(TemplateTreeNode* child)
{
    _children.push_back(child);
}

void
TemplateTreeNode::add_cmd(const string& cmd)
    throw (ParseError)
{
    string errmsg;
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
	    if (! is_leaf()) {
		errmsg = c_format("Invalid command \"%s\".\n", cmd.c_str());
		errmsg += "This command only applies to leaf nodes that have ";
		errmsg += "values and only if the value is allowed to be ";
		errmsg += "changed.\n";
		xorp_throw(ParseError, errmsg);
	    }
	}

	//
	// If the command already exists, no need to create it again.
	// The command action will simply be added to the existing command.
	//
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    // we just create a placeholder here - if we were a master
	    // template tree node we'd create a real command.
	    command = new BaseCommand(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%mandatory") {
	// Nothing to do
    } else {
	errmsg = c_format("Invalid command \"%s\".\n", cmd.c_str());
	errmsg += "Valid commands are %create, %delete, %set, %unset, %get, ";
	errmsg += "%default, %modinfo, %activate, %update, %allow, ";
	errmsg += "%allow-range, %mandatory, %deprecated, %order\n";
	xorp_throw(ParseError, errmsg);
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
{
    BaseCommand* command;
    map<string, BaseCommand*>::iterator iter;

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
		XLOG_WARNING(string("Ignored help descriptor "
				    + action_list.front()
				    + " in template file - \"short\" "
				    + "or \"long\" expectted").c_str());
	    }
	} else {
	    // XXX really should say why it's bad.
	    XLOG_WARNING("Bad help specification in template file ignored\n");
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
	    // XXX really should say why it's bad.
	    XLOG_WARNING("Bad %%deprecated specification in template file ignored\n");
	}
    } else if (cmd == "%order") {
	if (action_list.size() == 1) {
	    list<string>::const_iterator li = action_list.begin();
	    TTSortOrder order;
	    if (*li == "unsorted") {
		order = ORDER_UNSORTED;
	    } else if (*li == "sorted-numeric") {
		order = ORDER_SORTED_NUMERIC;
	    } else if (*li == "sorted-alphabetic") {
		order = ORDER_SORTED_ALPHABETIC;
	    } else {
		XLOG_WARNING("Bad %%order specification in template file ignored - should be unsorted, sorted-numeric, or sorted-alphabetic");
	    }
	    set_order(order);

	    if ((_parent != NULL) && (_parent->is_tag())) {
		_parent->set_order(order);
	    }
	} else {
	    // XXX really should say why it's bad.
	    XLOG_WARNING("Bad %%order specification in template file ignored\n");
	}
    } else if (cmd == "%mandatory") {
	// Add all new mandatory variables
	list<string>::const_iterator li;
	for (li = action_list.begin(); li != action_list.end(); ++li) {
	    const string& varname = *li;
	    if (find(_mandatory_children.begin(),
		     _mandatory_children.end(),
		     varname)
		== _mandatory_children.end()) {
		_mandatory_children.push_back(varname);
	    }
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
TemplateTreeNode::is_leaf() const
{
    if ((type() != NODE_VOID) && (_parent != NULL) && (!_parent->is_tag()))
	return true;

    return false;
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
TemplateTreeNode::expand_variable(const string& varname, string& value) const
{
    const TemplateTreeNode* varname_node;
    varname_node = find_varname_node(varname);

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
    if (expand_variable(tmp_expr, tmp_value) != true)
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

    ttn = find_varname_node(varname);
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

    ttn = find_varname_node(varname);
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
TemplateTreeNode::find_varname_node(const string& varname) const
{
    if (varname == "$(@)" /* current value of a node */
	|| (varname == "$(DEFAULT)") /* default value of a node */
	|| (varname == "$(<>)") /* operator for a terminal node */
	|| (varname == "$(#)") /* the node number of a node */
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
	return find_child_varname_node(var_parts);
    }

    if (var_parts.size() > 1) {
	// It's a parent node, or a child of a parent node
	return find_parent_varname_node(var_parts);
    }

    //
    // XXX: if not referring to this node, then size of 0 or 1
    // is not valid syntax.
    //
    return NULL;
}

const TemplateTreeNode*
TemplateTreeNode::find_parent_varname_node(const list<string>& var_parts) const
{
    if (_parent == NULL) {
	//
	// We have reached the root node.
	// Search among the children.
	//
	list<TemplateTreeNode* >::const_iterator iter;
	for (iter = _children.begin(); iter != _children.end(); ++iter) {
	    const TemplateTreeNode* found_child;
	    found_child = (*iter)->find_child_varname_node(var_parts);
	    if (found_child != NULL)
		return found_child;
	}
	return NULL;
    }
    if (is_tag() || (type() == NODE_VOID)) {
	// When naming a parent node variable, you must start with a tag
	if (_segname == var_parts.front()) {
	    // We've found the right place to start
	    return find_child_varname_node(var_parts);
	}
    }

    return _parent->find_parent_varname_node(var_parts);
}

const TemplateTreeNode*
TemplateTreeNode::find_child_varname_node(const list<string>& var_parts) const
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
	    return this;
	}
    }

    // The name might refer to the default value of this node
    if ((var_parts.size() == 2) && (var_parts.back() == "DEFAULT")) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    // The name refers to the default value of this node
	    if (! has_default())
		return NULL;	// The template tree node has no default value
	    return this;
	}
    }

    // The name might refer to the operator value of this node
    if ((var_parts.size() == 2) && (var_parts.back() == "<>")) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    return this;
	}
    }

    // The name might refer to the node number of this node
    if ((var_parts.size() == 2) && (var_parts.back() == "#")) {
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
	    return this;
	}
    }

    // The name might refer to a child of ours
    list<string> child_var_parts = var_parts;
    child_var_parts.pop_front();

    list<TemplateTreeNode* >::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	const TemplateTreeNode* found_child;
	found_child = (*iter)->find_child_varname_node(child_var_parts);
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

#if 0
bool
TemplateTreeNode::check_template_tree(string& errmsg) const
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
	    if (! command->check_referred_variables(errmsg))
		return false;
	}
    }

    //
    // Recursively check all children nodes
    //
    list<TemplateTreeNode*>::const_iterator iter2;
    for (iter2 = _children.begin(); iter2 != _children.end(); ++iter2) {
	const TemplateTreeNode* ttn = *iter2;
	if (ttn->check_template_tree(errmsg) != true)
	    return false;
    }

    return true;
}
#endif

bool
TemplateTreeNode::check_command_tree(const list<string>& cmd_names,
				     bool include_intermediates,
				     size_t depth) const
{
    bool instantiated = false;

    // XXX: ignore deprecated subtrees
    if (is_deprecated())
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
    if (_is_tag || ((type() == NODE_VOID) && include_intermediates)) {
	list<TemplateTreeNode*>::const_iterator tti;
	for (tti = _children.begin(); tti != _children.end(); ++tti) {
	    if ((*tti)->check_command_tree(cmd_names, include_intermediates,
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
    if (initializer == "")
	return;

    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad Text type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
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
    if (initializer == "")
	return;

    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad arith type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
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
    if (initializer == "")
	return;

    string s = strip_quotes(initializer);
    string errmsg;
    if (! type_match(s, errmsg)) {
	string err = "Bad UInt type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
    }
    _default = atoi(s.c_str());
    set_has_default();
}

bool
UIntTemplate::type_match(const string& orig, string& errmsg) const
{
    string s = strip_quotes(orig);

    for (size_t i = 0; i < s.length(); i++)
	if (s[i] < '0' || s[i] > '9') {
	    if (s[i]=='-') {
		errmsg = "Value cannot be negative.";
	    } if (s[i]=='.') {
		errmsg = "Value must be an integer.";
	    } else {
		errmsg = "Value must be numeric.";
	    }
	    return false;
	}
    return true;
}

string
UIntTemplate::default_str() const
{
    return c_format("%u", _default);
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
    if (initializer == "")
	return;

    string s = strip_quotes(initializer);
    string errmsg;
    if (! type_match(s, errmsg)) {
	string err = "Bad Int type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
    }
    _default = atoi(s.c_str());
    set_has_default();
}

bool
IntTemplate::type_match(const string& orig, string& errmsg) const
{
    string s = strip_quotes(orig);
    size_t start = 0;
    if (s[0] == '-') {
	if (s.length() == 1) {
	    errmsg = "Value must be an integer.";
	    return false;
	}
	start = 1;
    }
    for (size_t i = start; i < s.length(); i++)
	if (s[i] < '0' || s[i] > '9') {
	    if (s[i]=='.') {
		errmsg = "Value must be an integer.";
	    } else {
		errmsg = "Value must be numeric.";
	    }	    
	    return false;
	}
    return true;
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
    if (initializer == "")
	return;
    
    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad Bool type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
    }
    if (initializer == string("false"))
	_default = false;
    else
	_default = true;
    set_has_default();
}

bool
BoolTemplate::type_match(const string& s, string& errmsg) const
{
    if (s == "true")
	return true;
    else if (s == "false")
	return true;
    errmsg = "Value must be \"true\" or \"false\".";
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
    if (! initializer.empty()) {
	try {
	    _default = new IPv4(initializer.c_str());
	} catch (InvalidString) {
	    string err = "Bad IPv4 type value: " + initializer;
	    xorp_throw(ParseError, err);
	}
	set_has_default();
    }
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
IPv4Template::type_match(const string& s, string& errmsg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	errmsg = "Value must be an IP address in dotted decimal form.";
	return false;
    }

    try {
	IPv4* ipv4 = new IPv4(tmp.c_str());
	delete ipv4;
    } catch (InvalidString) {
	errmsg = "Value must be an IP address in dotted decimal form.";
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
    if (! initializer.empty()) {
	try {
	    _default = new IPv4Net(initializer.c_str());
	} catch (InvalidString) {
	    string err = "Bad IPv4Net type value: " + initializer;
	    xorp_throw(ParseError, err);
	} catch (InvalidNetmaskLength) {
	    string err = "Illegal IPv4 prefix length in subnet: " 
		+ initializer; 
	    xorp_throw(ParseError, err);
	}
	set_has_default();
    }
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
IPv4NetTemplate::type_match(const string& s, string& errmsg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	errmsg = "Value must be a subnet in address/prefix-length form.";
	return false;
    }

    try {
	IPv4Net* ipv4net = new IPv4Net(tmp.c_str());
	delete ipv4net;
    } catch (InvalidString) {
	errmsg = "Value must be a subnet in address/prefix-length form.";
	return false;
    } catch (InvalidNetmaskLength) {
	errmsg = c_format("Prefix length must be an integer between 0 and %u.",
			  IPv4::addr_bitlen());
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
    if (! initializer.empty()) {
	try {
	    _default = new IPv6(initializer.c_str());
	} catch (InvalidString) {
	    string err = "Bad IPv6 type value: " + initializer;
	    xorp_throw(ParseError, err);
	}
	set_has_default();
    }
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
IPv6Template::type_match(const string& s, string& errmsg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	errmsg = "Value must be an IPv4 address.";
	return false;
    }

    try {
	IPv6* ipv6 = new IPv6(tmp.c_str());
	delete ipv6;
    } catch (InvalidString) {
	errmsg = "Value must be an IPv6 address.";
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
    if (! initializer.empty()) {
	try {
	    _default = new IPv6Net(initializer.c_str());
	} catch (InvalidString) {
	    string err = "Bad IPv6Net type value: " + initializer;
	    xorp_throw(ParseError, err);
	} catch (InvalidNetmaskLength) {
	    string err = "Illegal IPv6 prefix length in subnet: " 
		+ initializer; 
	    xorp_throw(ParseError, err);
	}
	set_has_default();
    }
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
IPv6NetTemplate::type_match(const string& s, string& errmsg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	errmsg = "Value must be an IPv6 subnet in address/prefix-length form.";
	return false;
    }

    try {
	IPv6Net* ipv6net = new IPv6Net(tmp.c_str());
	delete ipv6net;
    } catch (InvalidString) {
	errmsg = "Value must be an IPv6 subnet in address/prefix-length form.";
	return false;
    } catch (InvalidNetmaskLength) {
	errmsg = c_format("Prefix length must be an integer between 0 and %u.",
			  IPv6::addr_bitlen());
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
    if (! initializer.empty()) {
	try {
	    _default = new EtherMac(initializer.c_str());
	} catch (InvalidString) {
	    string err = "Bad MacAddr type value: " + initializer;
	    xorp_throw(ParseError, err);
	}
	set_has_default();
    }
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
MacaddrTemplate::type_match(const string& s, string& errmsg) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty()) {
	errmsg = "Value must be an MAC address (six hex digits separated by colons)";
	return false;
    }

    try {
	EtherMac* mac = new EtherMac(tmp.c_str());
	delete mac;
    } catch (InvalidString) {
	errmsg = "Value must be an MAC address (six hex digits separated by colons)";
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
    if (initializer == "")
	return;

    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad UrlFile type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
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
    if (initializer == "")
	return;

    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad UrlFtp type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
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
    if (initializer == "")
	return;

    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad UrlHttp type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
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
    if (initializer == "")
	return;

    string errmsg;
    if (! type_match(initializer, errmsg)) {
	string err = "Bad UrlTftp type value: " + initializer;
	if (!errmsg.empty()) {
	    err += "\n";
	    err += errmsg;
	}
	xorp_throw(ParseError, err);
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
