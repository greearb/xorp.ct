// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rtrmgr/command_tree.hh,v 1.16 2008/07/23 05:11:40 pavlin Exp $

#ifndef __RTRMGR_COMMAND_TREE_HH__
#define __RTRMGR_COMMAND_TREE_HH__


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
    string subtree_str() const;
    void set_has_command() { _has_command = true; }
    bool has_command() const { return _has_command; }
    const ConfigTreeNode* config_tree_node() const { return _config_tree_node; }
    const TemplateTreeNode* template_tree_node() const { return _template_tree_node;}
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
    void instantiate(const ConfigTreeNode *ctn, const TemplateTreeNode *ttn,
		     bool has_command);
    void activate_current();
    string tree_str() const;
    const CommandTreeNode& root_node() const { return _root_node; }

private:
    list<string>	_temp_path;
    CommandTreeNode	_root_node;
    CommandTreeNode*	_current_node;
};

#endif // __RTRMGR_COMMAND_TREE_HH__
