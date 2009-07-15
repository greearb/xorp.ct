// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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




#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "command_tree.hh"
#include "template_tree_node.hh"


CommandTreeNode::CommandTreeNode(const string& name,
				 const ConfigTreeNode* ctn,
				 const TemplateTreeNode* ttn)
    : _name(name),
      _config_tree_node(ctn),
      _template_tree_node(ttn)
{
    _parent = NULL;
    _has_command = false;
}

CommandTreeNode::~CommandTreeNode()
{
    list<CommandTreeNode*>::iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	delete *iter;
    }
}

void
CommandTreeNode::add_child(CommandTreeNode* child)
{
    _children.push_back(child);
    child->set_parent(this);
}

string
CommandTreeNode::str() const
{
    if (_parent == NULL) {
	return "";
    } else {
	string s = _parent->str();
	if (s != "") 
	    s += " ";
	s += _name;
	if (_has_command)
	    s += "[*]";
	return s;
    }
}

#if 0
const CommandTreeNode* 
CommandTreeNode::subroot(list<string> path) const
{
    if (path.empty()) {
	return this;
    }
    list<CommandTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter !=  _children.end(); ++iter) {
	if ((*iter)->name() == path.front()) {
	    path.pop_front();
	    return (*iter)->subroot(path);
	}
    }
    // Debugging only below this point
    XLOG_TRACE(_verbose, "ERROR doing subroot at node >%s<\n", _name.c_str());
    for (iter =_children.begin(); iter !=  _children.end(); ++iter) {
	XLOG_TRACE(_verbose, "Child: %s\n", (*iter)->name().c_str());
    }
    while (!path.empty()) {
	XLOG_TRACE(_verbose, "Path front is >%s<\n", path.front().c_str());
	path.pop_front();
    }
    return NULL;
}
#endif // 0

string
CommandTreeNode::subtree_str() const
{
    string s;

    s = c_format("Node: %s\n", str().c_str());
    list<CommandTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter !=  _children.end(); ++iter) {
	s += (*iter)->subtree_str();
    }

    return s;
}

const string& 
CommandTreeNode::help() const 
{
    return _template_tree_node->help();
}


CommandTree::CommandTree() 
    : _root_node("ROOT", NULL, NULL)
{
    _current_node = &_root_node;
}

CommandTree::~CommandTree()
{
    
}

void 
CommandTree::push(const string& str)
{
    _temp_path.push_back(str);
}

void 
CommandTree::pop()
{
    if (_temp_path.empty()) {
	_current_node = _current_node->parent();
    } else {
	_temp_path.pop_back();
    }
}

void 
CommandTree::instantiate(const ConfigTreeNode* ctn, 
			 const TemplateTreeNode* ttn,
			 bool has_command)
{
    XLOG_ASSERT(! _temp_path.empty());

    list<string>::const_iterator iter;

    for (iter = _temp_path.begin(); iter != _temp_path.end(); ++iter) {
	debug_msg("Instantiating node >%s<\n", iter->c_str());
	CommandTreeNode* new_ctn = new CommandTreeNode(*iter, ctn, ttn);
	_current_node->add_child(new_ctn);
	_current_node = new_ctn;
    }
    if (has_command)
	_current_node->set_has_command();

    _temp_path.clear();
}

void 
CommandTree::activate_current()
{
    _current_node->set_has_command();
}



string
CommandTree::tree_str() const
{
    return _root_node.subtree_str();
}
