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

// $XORP: xorp/rtrmgr/command_tree.hh,v 1.5 2003/11/20 06:37:38 pavlin Exp $

#ifndef __RTRMGR_COMMAND_TREE_HH__
#define __RTRMGR_COMMAND_TREE_HH__

#include "libxorp/xorp.h"
#include <list>

class ConfigTreeNode;
class TemplateTreeNode;

class CommandTreeNode {
public:
    CommandTreeNode(const string& name,
		    const ConfigTreeNode* ctn,
		    const TemplateTreeNode* ttn);
    ~CommandTreeNode();

    void add_child(CommandTreeNode* child);
    void set_parent(CommandTreeNode* parent) { _parent = parent; }
    CommandTreeNode* parent() const { return _parent; }
    const list<CommandTreeNode*>& children() const { return _children; }
    string str() const;
    const string& name() const { return _name; }
    void print() const;
    void set_has_command() { _has_command = true; }
    bool has_command() const { return _has_command; }
    const ConfigTreeNode* config_tree_node() { return _config_tree_node; }
    const TemplateTreeNode* template_tree_node() { return _template_tree_node;}
    const CommandTreeNode* subroot(list<string> path) const;
    const string& help() const;

private:
    string			_name;
    bool			_has_command;
    CommandTreeNode*		_parent;
    const ConfigTreeNode*	_config_tree_node;
    const TemplateTreeNode*	_template_tree_node;
    list<CommandTreeNode*>	_children;
};

class CommandTree {
public:
    CommandTree();
    ~CommandTree();

    void push(const string& str);
    void pop();
    void instantiate(const ConfigTreeNode *ctn, const TemplateTreeNode *ttn);
    void activate_current();
    void print() const;
    const CommandTreeNode& root_node() const { return _root_node; }

private:
    list<string>	_temp_path;
    CommandTreeNode	_root_node;
    CommandTreeNode*	_current_node;
};

#endif // __RTRMGR_COMMAND_TREE_HH__
