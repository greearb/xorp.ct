// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/command_tree.cc,v 1.7 2002/12/09 18:29:36 hodson Exp $"

#include "rtrmgr_module.h"
#include "command_tree.hh"

CommandTreeNode::CommandTreeNode(const string& name,
				 const ConfigTreeNode *ctn,
				 const TemplateTreeNode *ttn)
    : _name(name) {
    _parent = NULL;
    _has_command = false;
    _configtreenode = ctn;
    _templatetreenode = ttn;
}

CommandTreeNode::~CommandTreeNode() {
    list<CommandTreeNode*>::iterator i;
    for (i=_children.begin(); i!=_children.end(); ++i) {
	delete *i;
    }
}

void CommandTreeNode::add_child(CommandTreeNode* child) {
    _children.push_back(child);
    child->set_parent(this);
}

string CommandTreeNode::str() const {
    if (_parent == NULL) 
	return "";
    else {
	string s = _parent->str();
	if (s != "") 
	    s += " ";
	s += _name;
	if (_has_command) s += "[*]";
	return s;
    }
}

#ifdef NOTDEF
const CommandTreeNode* 
CommandTreeNode::subroot(list <string> path) const {
    if (path.size()==0) {
	return this;
    }
    list <CommandTreeNode*>::const_iterator i;
    for (i=_children.begin(); i!= _children.end(); ++i) {
	if ((*i)->name() == path.front()) {
	    path.pop_front();
	    return (*i)->subroot(path);
	}
    }
    /* debugging only below this point */
    printf("ERROR doing subroot at node >%s<\n", _name.c_str());
    for (i=_children.begin(); i!= _children.end(); ++i) {
	printf("Child: %s\n", (*i)->name().c_str());
    }
    while (!path.empty()) {
	printf("Path front is >%s<\n", path.front().c_str());
	path.pop_front();
    }
    return NULL;
}

#endif

void CommandTreeNode::print() const {
    printf("Node: %s\n", str().c_str());
    list <CommandTreeNode*>::const_iterator i;
    for (i=_children.begin(); i!= _children.end(); ++i) {
	(*i)->print();
    }
}

CommandTree::CommandTree() 
    : _root("ROOT", NULL, NULL)
{
    _current_node = &_root;
}

CommandTree::~CommandTree() {
    
}

void 
CommandTree::push(const string& str) {
    _temp_path.push_back(str);
}

void 
CommandTree::pop() {
    if (_temp_path.size() == 0) {
	_current_node = _current_node->parent();
    } else {
	_temp_path.pop_back();
    }
}

void 
CommandTree::instantiate(const ConfigTreeNode *ctn, 
			 const TemplateTreeNode *ttn) {
    assert(_temp_path.size() > 0);

    list <string>::const_iterator i;
    for (i=_temp_path.begin(); i!= _temp_path.end(); ++i) {
#ifdef NOTDEF
	printf("Instantiating node >%s<\n", i->c_str());
#endif
	CommandTreeNode *commtn = new CommandTreeNode(*i, ctn, ttn);
	_current_node->add_child(commtn);
	_current_node = commtn;
    }
    _current_node->set_has_command();

    while (_temp_path.size() > 0)
	_temp_path.pop_front();
}

void 
CommandTree::activate_current() {
    _current_node->set_has_command();
}


void 
CommandTree::print() const {
    _root.print();
}
