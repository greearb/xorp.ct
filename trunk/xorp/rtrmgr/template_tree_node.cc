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

#ident "$XORP: xorp/rtrmgr/template_tree_node.cc,v 1.14 2004/01/06 02:55:28 pavlin Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include <glob.h>
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "module_command.hh"
#include "template_tree.hh"
#include "conf_tree.hh"
#include "command_tree.hh"
#include "util.hh"


TemplateTreeNode::TemplateTreeNode(TemplateTree& template_tree,
				   TemplateTreeNode* parent,
				   const string& path,
				   const string& varname)
    : _template_tree(template_tree),
      _parent(parent),
      _module_name(""),
      _default_target_name(""),
      _segname(path),
      _varname(varname),
      _has_default(false),
      _is_tag(false)
{
    if (_parent != NULL) {
	_parent->add_child(this);
	_module_name = _parent->module_name();
	_default_target_name = _parent->default_target_name();
    }
}

TemplateTreeNode::~TemplateTreeNode()
{
    while (! _children.empty()) {
	delete _children.front();
	_children.pop_front();
    }

    map<string, Command *>::iterator iter1, iter2;
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
TemplateTreeNode::add_cmd(const string& cmd, TemplateTree& tt)
    throw (ParseError)
{
    Command* command;
    map<string, Command*>::iterator iter;

    if (cmd == "%modinfo") {
	iter = _cmd_map.find("%modinfo");
	if (iter == _cmd_map.end()) {
	    command = new ModuleCommand(tt, *this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%allow") {
	// If the command already exists, no need to create it again.
	// The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new AllowCommand(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if ((cmd == "%create")
	       || (cmd == "%activate")
	       || (cmd == "%list")
	       || (cmd == "%delete")
	       || (cmd == "%set")
	       || (cmd == "%unset")
	       || (cmd == "%get")
	       || (cmd == "%default")) {
	// If the command already exists, no need to create it again.
	// The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new Command(*this, cmd);
	    _cmd_map[cmd] = command;
	}
    } else if (cmd == "%mandatory") {
	// Nothing to do
    } else {
	string err = "Invalid command \"" + cmd + "\"\n";
	err += "Valid commands are %create, %delete, %set, %unset, %get, ";
	err += "%default, %modinfo, %activate, %allow, %mandatory\n";
	xorp_throw(ParseError, err);
    }
}

set<string>
TemplateTreeNode::commands() const
{
    set<string> cmds;
    map<string, Command *>::const_iterator iter;

    for (iter = _cmd_map.begin(); iter != _cmd_map.end(); ++iter) {
	cmds.insert(iter->first);
    }
    return cmds;
}

void
TemplateTreeNode::add_action(const string& cmd,
			     const list<string>& action_list,
			     const XRLdb& xrldb)
{
    Command* command;
    map<string, Command*>::iterator iter;

    if (cmd == "%modinfo") {
	iter = _cmd_map.find("%modinfo");
	XLOG_ASSERT(iter != _cmd_map.end());
	command = iter->second;
	ModuleCommand* module_command = dynamic_cast<ModuleCommand*>(command);
	XLOG_ASSERT(module_command != NULL);
	module_command->add_action(action_list, xrldb);
    } else if (cmd == "%allow") {
	iter = _cmd_map.find("%allow");
	XLOG_ASSERT(iter != _cmd_map.end());
	command = iter->second;
	AllowCommand* allow_command = dynamic_cast<AllowCommand*>(command);
	XLOG_ASSERT(allow_command != NULL);
	allow_command->add_action(action_list);
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
	iter = _cmd_map.find(cmd);
	XLOG_ASSERT(iter != _cmd_map.end());
	command = iter->second;
	command->add_action(action_list, xrldb);
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

string
TemplateTreeNode::s() const
{
    string tmp;

    tmp = path() + " Type:" + typestr();
    if (has_default()) {
	tmp += " = " + default_str();
    }
    if (!_cmd_map.empty()) {
	map<string, Command*>::const_iterator rpair;
	rpair = _cmd_map.begin();
	while (rpair != _cmd_map.end()) {
	    tmp += "\n    " + rpair->second->str();
	    ++rpair;
	}
    }
    return tmp;
}

void
TemplateTreeNode::print() const
{
    printf("%s\n", s().c_str());

    list<TemplateTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	(*iter)->print();
    }
}

bool
TemplateTreeNode::type_match(const string& ) const
{
    return true;
}

Command*
TemplateTreeNode::command(const string& cmd_name)
{
    map<string, Command *>::iterator iter;

    iter = _cmd_map.find(cmd_name);
    if (iter == _cmd_map.end())
	return NULL;
    return iter->second;
}

const Command*
TemplateTreeNode::const_command(const string& cmd_name) const
{
    map<string, Command *>::const_iterator iter;

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
    split_up_varname(varname, var_parts);
    string nodename = var_parts.back();

    XLOG_ASSERT((nodename == varname_node->segname())
		|| (nodename == "DEFAULT"));
    if (! varname_node->has_default())
	return false;		// XXX: the variable has no default value
    value = varname_node->default_str();
    return true;
}

bool
TemplateTreeNode::expand_expression(const string& expr, string& value) const
{
    //
    // XXX: for now we cannot expand expressions like "~$(@)" using
    // the template tree only.
    //
    UNUSED(expr);
    UNUSED(value);

    return false;
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

void
TemplateTreeNode::split_up_varname(const string& varname,
				   list<string>& var_parts) const
{
    XLOG_ASSERT(varname[0] == '$');
    XLOG_ASSERT(varname[1] == '(');
    XLOG_ASSERT(varname[varname.size() - 1] == ')');

    string trimmed = varname.substr(2, varname.size() - 3);
    var_parts = split(trimmed, '.');
}

const TemplateTreeNode*
TemplateTreeNode::find_varname_node(const string& varname) const
{
    if (varname == "$(@)"
	|| (varname == "$(DEFAULT)")
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
    split_up_varname(varname, var_parts);

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
	if ((var_parts.front() == "@") || (var_parts.front() == _segname)) {
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

bool
TemplateTreeNode::check_template_tree(string& errmsg) const
{
    //
    // First check this node, then check recursively all children nodes
    //

    //
    // Check whether all referred variable names are valid
    //
    map<string, Command *>::const_iterator iter1;
    for (iter1 = _cmd_map.begin(); iter1 != _cmd_map.end(); ++iter1) {
	const Command* command = iter1->second;
	if (! command->check_referred_variables(errmsg))
	    return false;
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

bool
TemplateTreeNode::check_command_tree(const list<string>& cmd_names,
				     bool include_intermediates,
				     size_t depth) const
{
    bool instantiated = false;

#ifdef DEBUG
    printf("TTN:check_command_tree %s type %s depth %u\n", _segname.c_str(), typestr().c_str(), (uint32_t)depth);
#endif

    if (_parent != NULL && _parent->is_tag() && (depth == 0)) {
	//
	// If the parent is a tag, then this node must be a pure
	// variable. We don't want to instantiate any pure variable
	// nodes in the command tree.
	//
#ifdef DEBUG
	printf("pure variable\n");
#endif
	return false;
    }

    list<string>::const_iterator iter;
    for (iter = cmd_names.begin(); iter != cmd_names.end(); ++iter) {
	if (const_command(*iter) != NULL) {
#ifdef DEBUG
	    printf("node has command %s\n", (*iter).c_str());
#endif
	    instantiated = true;
	    break;
	} else {
#ifdef DEBUG
	    printf("node doesn't have command %s\n", (*iter).c_str());
#endif
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
    // printf("check_variable_name: us>%s< match>%s<, %d\n",
    //   _segname.c_str(), parts[part].c_str(), part);
    bool ok = false;
    if (_parent == NULL) {
	// Root node
	ok = true;
	part--; // Prevent increment of part later
    } else if (_parent->is_tag()) {
	// We're the varname after a tag
	if (parts[part] == "*") {
	    ok = true;
	} else {
	    // printf("--varname but not wildcarded\n");
	}
    } else {
	if (parts[part] != _segname) {
	    // printf("--mismatch\n");
	    return false;
	}
	ok = true;
    }
    if (ok) {
	if (part == parts.size() - 1) {
	    // Everything that we were looking for matched
	    // printf("**match successful**\n");
	    return true;
	}
	if (_children.empty()) {
	    // printf("--no children\n");
	    // No more children, but the search string still has components
	    return false;
	}
	list<TemplateTreeNode*>::const_iterator tti;
	for (tti = _children.begin(); tti != _children.end(); ++tti) {
	    if ((*tti)->check_variable_name(parts, part + 1)) {
		return true;
	    }
	}
	// printf("--no successful children, %d\n", part);
    }
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

    if (! type_match(initializer)) {
	string err = "Bad Text type value: " + initializer;
	xorp_throw(ParseError, err);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool
TextTemplate::type_match(const string &) const
{
    //
    // If the lexical analyser passed it to us, we can assume its a
    // value text value.
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
    if (! type_match(s)) {
	string err = "Bad UInt type value: " + initializer;
	xorp_throw(ParseError, err);
    }
    _default = atoi(s.c_str());
    set_has_default();
}

bool
UIntTemplate::type_match(const string& orig) const
{
    string s = strip_quotes(orig);

    for (size_t i = 0; i < s.length(); i++)
	if (s[i] < '0' || s[i] > '9')
	    return false;
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
    if (! type_match(s)) {
	string err = "Bad Int type value: " + initializer;
	xorp_throw(ParseError, err);
    }
    _default = atoi(s.c_str());
    set_has_default();
}

bool
IntTemplate::type_match(const string& orig) const
{
    string s = strip_quotes(orig);
    size_t start = 0;
    if (s[0] == '-') {
	if (s.length() == 1)
	    return false;
	start = 1;
    }
    for (size_t i = start; i < s.length(); i++)
	if (s[i] < '0' || s[i] > '9')
	    return false;
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

    if (! type_match(initializer)) {
	string err = "Bad Bool type value: " + initializer;
	xorp_throw(ParseError, err);
    }
    if (initializer == string("false"))
	_default = false;
    else
	_default = true;
    set_has_default();
}

bool
BoolTemplate::type_match(const string& s) const
{
    if (s == "true")
	return true;
    else if (s == "false")
	return true;
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

bool
IPv4Template::type_match(const string& s) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty())
	return false;

    try {
	IPv4* ipv4 = new IPv4(tmp.c_str());
	delete ipv4;
    } catch (InvalidString) {
	return false;
    }
    return true;
}


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

bool
IPv6Template::type_match(const string& s) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty())
	return false;

    try {
	IPv6* ipv6 = new IPv6(tmp.c_str());
	delete ipv6;
    } catch (InvalidString) {
	return false;
    }
    return true;
}


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
	}
	set_has_default();
    }
}

IPv4NetTemplate::~IPv4NetTemplate()
{
    if (_default != NULL)
	delete _default;
}

bool
IPv4NetTemplate::type_match(const string& s) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty())
	return false;

    try {
	IPv4Net* ipv4net = new IPv4Net(tmp.c_str());
	delete ipv4net;
    } catch (InvalidString) {
	return false;
    }
    return true;
}


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
	}
	set_has_default();
    }
}

IPv6NetTemplate::~IPv6NetTemplate()
{
    if (_default != NULL)
	delete _default;
}

bool
IPv6NetTemplate::type_match(const string& s) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty())
	return false;

    try {
	IPv6Net* ipv6net = new IPv6Net(tmp.c_str());
	delete ipv6net;
    } catch (InvalidString) {
	return false;
    }
    return true;
}


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
	} catch (BadMac) {
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

bool
MacaddrTemplate::type_match(const string& s) const
{
    string tmp = strip_quotes(s);

    if (tmp.empty())
	return false;

    try {
	EtherMac* mac = new EtherMac(tmp.c_str());
	delete mac;
    } catch (BadMac) {
	return false;
    }
    return true;
}
