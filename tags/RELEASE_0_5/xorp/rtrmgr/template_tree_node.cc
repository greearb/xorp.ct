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

#ident "$XORP: xorp/rtrmgr/template_tree_node.cc,v 1.6 2003/05/10 23:29:22 mjh Exp $"

#include <glob.h>
#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "module_command.hh"
#include "template_tree.hh"
#include "conf_tree.hh"
#include "command_tree.hh"

#define DEBUG_TEMPLATE_PARSER

extern int init_template_parser(const char *, TemplateTree* c);
extern int parse_template();
extern void tplterror(const char* s);

TemplateTreeNode::TemplateTreeNode(TemplateTreeNode* parent,
				   const string& path,
				   const string& varname)
    : _segname(path), _varname(varname)
{
    _parent = parent;
    if (parent != NULL)
	parent->add_child(this);
    _has_default = false;
    _is_tag = false;
}

TemplateTreeNode::~TemplateTreeNode() {
    while (!_children.empty()) {
	delete _children.front();
	_children.pop_front();
    }

    map <string, Command *>::iterator i1, i2;
    i1 = _cmd_map.begin();
    while (i1 != _cmd_map.end()) {
	i2 = i1;
	i1++;
	delete i2->second;
	_cmd_map.erase(i2);
    }
}

void
TemplateTreeNode::add_child(TemplateTreeNode* child) {
    _children.push_back(child);
}

void
TemplateTreeNode::add_cmd(const string& cmd, TemplateTree& tt) {
    Command* command;
    typedef map<string, Command*>::iterator I;
    I iter;
    if (cmd == "%modinfo") {
	iter = _cmd_map.find("%modinfo");
	if (iter == _cmd_map.end()) {
	    command = new ModuleCommand(cmd, tt);
	    _cmd_map[cmd] = command;
	} else {
	    command = iter->second;
	}
    } else if (cmd == "%allow") {
	//If the command already exists, no need to create it again.
	//The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new AllowCommand(cmd);
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
	//If the command already exists, no need to create it again.
	//The command action will simply be added to the existing command.
	if (_cmd_map.find(cmd) == _cmd_map.end()) {
	    command = new Command(cmd);
	    _cmd_map[cmd] = command;
	}
    } else {
	string err = "Invalid command \"" + cmd + "\"\n";
	err += "Valid commands are %create, %delete, %set, %unset, %get, ";
	err += "%default, %modinfo, %activate, %allow\n";
	xorp_throw(ParseError, err);
    }
}

set <string>
TemplateTreeNode::commands() const {
    set <string> cmds;
    map<string, Command *>::const_iterator i;
    for (i=_cmd_map.begin(); i!= _cmd_map.end(); i++) {
	cmds.insert(i->first);
    }
    return cmds;
}

void
TemplateTreeNode::add_action(const string& cmd,
			     const list<string>& action_list,
			     const XRLdb& xrldb) {
    Command* command;
    typedef map<string, Command*>::iterator I;
    I iter;
    if (cmd == "%modinfo") {
	iter = _cmd_map.find("%modinfo");
	command = iter->second;
	((ModuleCommand*)command)->add_action(action_list, xrldb);
    } else if (cmd == "%allow") {
	iter = _cmd_map.find("%allow");
	command = iter->second;
	((AllowCommand*)command)->add_action(action_list);
    } else {
	iter = _cmd_map.find(cmd);
	assert (iter != _cmd_map.end());
	command = iter->second;
	command->add_action(action_list, xrldb);
    }
}

map <string,string>
TemplateTreeNode::create_variable_map(const list <string>& segments) const
{
    map <string,string> varmap;
    const TemplateTreeNode* ttn = this;
    list <string>::const_reverse_iterator i;
    for (i = segments.rbegin(); i != segments.rend(); i++) {
	if (ttn->name_is_variable())
	    varmap[ttn->segname()] = *i;
	ttn = ttn->parent();
    }
    return varmap;
}

string TemplateTreeNode::path() const {
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

string TemplateTreeNode::s() const {
    string tmp;
    tmp = path() + " Type:" + typestr();
    if (has_default()) {
	tmp += " = " + default_str();
    }
    if (!_cmd_map.empty()) {
	typedef map<string, Command*>::const_iterator CI;
	CI rpair = _cmd_map.begin();
	while (rpair != _cmd_map.end()) {
	    tmp += "\n    " + rpair->second->str();
	    ++rpair;
	}
    }
    return tmp;
}

void TemplateTreeNode::print() const {
    printf("%s\n", s().c_str());
    list <TemplateTreeNode*>::const_iterator i;
    for (i=_children.begin(); i!= _children.end(); ++i) {
	(*i)->print();
    }
}

bool
TemplateTreeNode::type_match(const string& ) const {
    return true;
}

string TemplateTreeNode::strip_quotes(const string& s) const {
    int len = s.length();
    if (len < 2) return s;
    if ((s[0]=='"') && (s[len-1]=='"'))
	return s.substr(1,len-2);
    else
	return s;
}

bool TemplateTreeNode::name_is_variable() const {
    if (_segname.size() < 4) return false;
    if (_segname[0]!='$' || _segname[1]!='(')
	return false;
    return true;
}



bool
TemplateTreeNode::check_command_tree(const list<string>& cmd_names,
				     bool include_intermediates,
				     int depth) const {
    bool instantiated = false;
#ifdef DEBUG
    printf("TTN:check_command_tree %s type %s depth %d\n", _segname.c_str(), typestr().c_str(), depth);
#endif

    if (_parent != NULL && _parent->is_tag() && (depth == 0)) {
	/*if the parent is a tag, then this node must be a pure
          variable. we don't want to instantiate any pure variable
          nodes in the command tree */
#ifdef DEBUG
	printf("pure variable\n");
#endif
	return false;
    }

    list <string>::const_iterator ci;
    for (ci = cmd_names.begin(); ci != cmd_names.end(); ci++) {
	if(const_command(*ci)!=NULL) {
#ifdef DEBUG
	    printf("node has command %s\n", (*ci).c_str());
#endif
	    instantiated = true;
	    break;
	} else {
#ifdef DEBUG
	    printf("node doesn't have command %s\n", (*ci).c_str());
#endif
	}
    }

    //when we're building a command tree we only want to go one level
    //into the template tree beyond the part of the tree that is
    //instantiated by the config tree.  The exception is if the first
    //level node is a tag or a VOID node, then we need to check
    //further to find a real node.
    if (_is_tag
	|| ((type() == NODE_VOID) && include_intermediates)) {
	list <TemplateTreeNode*>::const_iterator tti;
	for (tti=_children.begin(); tti!= _children.end(); ++tti) {
	    if ((*tti)->check_command_tree(cmd_names, include_intermediates,
					   depth+1)) {
		instantiated = true;
		break;
	    }
	}
    }
    return instantiated;
}

bool
TemplateTreeNode::check_variable_name(const vector<string>& parts,
				      uint part) const {
    //printf("check_variable_name: us>%s< match>%s<, %d\n", _segname.c_str(),
    //   parts[part].c_str(), part);
    bool ok = false;
    if (_parent == NULL) {
	//root node
	ok = true;
	part--; //prevent increment of part later
    } else if (_parent->is_tag()) {
	//we're the varname after a tag
	if (parts[part] == "*") {
	    ok = true;
	} else {
	    //printf("--varname but not wildcarded\n");
	}
    } else {
	if (parts[part]!=_segname) {
	    //printf("--mismatch\n");
	    return false;
	}
	ok = true;
    }
    if (ok) {
	if (part == parts.size()-1) {
	    //everything that we were looking for matched
	    //printf("**match successful**\n");
	    return true;
	}
	if (_children.empty()) {
	    //printf("--no children\n");
	    //no more children, but the search string still has components
	    return false;
	}
	list <TemplateTreeNode*>::const_iterator tti;
	for (tti=_children.begin(); tti!= _children.end(); ++tti) {
	    if ((*tti)->check_variable_name(parts, part+1)) {
		return true;
	    }
	}
	//printf("--no successful children, %d\n", part);
    }
    return false;
}


/**************************************************************************
 * TextTemplate
 **************************************************************************/

TextTemplate::TextTemplate(TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname), _default("")
{

    if (initializer == "")
	return;

    if (!type_match(initializer)) {
	string err = "Bad Text type value: " + initializer;
	xorp_throw(ParseError, err);
    }

    string s = strip_quotes(initializer);
    _default = s;
    set_has_default();
}

bool TextTemplate::type_match(const string &) const {
    //if the lexical analyser passed it to us, we can assume its a
    //value text value
    return true;
}


/**************************************************************************
 * UIntTemplate
 **************************************************************************/

UIntTemplate::UIntTemplate(TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname)
{
    if (initializer == "")
	return;

    string s = strip_quotes(initializer);
    if (!type_match(s)) {
	string err = "Bad UInt type value: " + initializer;
	xorp_throw(ParseError, err);
    }
    _default = atoi(s.c_str());
    set_has_default();
}

bool UIntTemplate::type_match(const string& orig) const {
    string s = strip_quotes(orig);
    for(u_int i=0; i<s.length(); i++)
	if (s[i] < '0' || s[i] > '9')
	    return false;
    return true;
}

string
UIntTemplate::default_str() const {
    return c_format("%u", _default);
}


/**************************************************************************
 * IntTemplate
 **************************************************************************/

IntTemplate::IntTemplate(TemplateTreeNode* parent,
			 const string& path, const string& varname,
			 const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname)
{
    if (initializer == "")
	return;

    string s = strip_quotes(initializer);
    if (!type_match(s)) {
	string err = "Bad Int type value: " + initializer;
	xorp_throw(ParseError, err);
    }
    _default = atoi(s.c_str());
    set_has_default();
}

bool IntTemplate::type_match(const string& orig) const {
    string s = strip_quotes(orig);
    int start=0;
    if (s[0]=='-') {
	if (s.length()==1)
	    return false;
	start = 1;
    }
    for(u_int i=start; i<s.length(); i++)
	if (s[i] < '0' || s[i] > '9')
	    return false;
    return true;
}

string
IntTemplate::default_str() const {
    return c_format("%d", _default);
}


/**************************************************************************
 * BoolTemplate
 **************************************************************************/

BoolTemplate::BoolTemplate(TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname) {
    if (initializer == "")
	return;

    if (!type_match(initializer)) {
	string err = "Bad Bool type value: " + initializer;
	xorp_throw(ParseError, err);
    }
    if (initializer == string("false"))
	_default = false;
    else
	_default = true;
    set_has_default();
}

bool BoolTemplate::type_match(const string& s) const {
    if (s == "true")
	return true;
    else if (s == "false")
	return true;
    return false;
}

string BoolTemplate::default_str() const {
    if (_default)
	return "true";
    else
	return "false";
}

IPv4Template::IPv4Template(TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname), _default(0)
{
    if (!initializer.empty()) {
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
    delete _default;
}

bool IPv4Template::type_match(const string& s) const {
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


IPv6Template::IPv6Template(TemplateTreeNode* parent,
			   const string& path, const string& varname,
			   const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname), _default(0)
{
    if (!initializer.empty()) {
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
    delete _default;
}

bool IPv6Template::type_match(const string& s) const {
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


IPv4NetTemplate::IPv4NetTemplate(TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname), _default(0)
{
    if (!initializer.empty()) {
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
    delete _default;
}

bool IPv4NetTemplate::type_match(const string& s) const {
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


IPv6NetTemplate::IPv6NetTemplate(TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname), _default(0)
{
    if (!initializer.empty()) {
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
    delete _default;
}

bool IPv6NetTemplate::type_match(const string& s) const {
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


MacaddrTemplate::MacaddrTemplate(TemplateTreeNode* parent,
				 const string& path, const string& varname,
				 const string& initializer)
    throw (ParseError)
    : TemplateTreeNode(parent, path, varname), _default(0) {
    if (!initializer.empty()) {
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
    delete _default;
}

bool MacaddrTemplate::type_match(const string& s) const {
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

