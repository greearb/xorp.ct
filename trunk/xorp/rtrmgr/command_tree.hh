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

// $XORP: xorp/rtrmgr/command_tree.hh,v 1.1.1.1 2002/12/11 23:56:15 hodson Exp $

#ifndef __RTRMGR_COMMAND_TREE_HH__
#define __RTRMGR_COMMAND_TREE_HH__

#include <list>
#include "libxorp/xorp.h"
#include "config.h"
class ConfigTreeNode;
class TemplateTreeNode;

class CommandTreeNode {
public:
    CommandTreeNode(const string& name,
		    const ConfigTreeNode *ctn,
		    const TemplateTreeNode *ttn);
    ~CommandTreeNode();
    void add_child(CommandTreeNode* child);
    void set_parent(CommandTreeNode* parent) {_parent = parent;}
    CommandTreeNode* parent() const {return _parent;}
    const list <CommandTreeNode*>& children() const {return _children;}
    string str() const;
    const string& name() const {return _name;}
    void print() const;
    void set_has_command() {_has_command = true;}
    bool has_command() const {return _has_command;}
    const ConfigTreeNode *configtreenode() {return _configtreenode;}
    const TemplateTreeNode *templatetreenode() {return _templatetreenode;}
    const CommandTreeNode* subroot(list <string> path) const;
private:
    string _name;
    bool _has_command;
    CommandTreeNode* _parent;
    const ConfigTreeNode* _configtreenode;
    const TemplateTreeNode *_templatetreenode;
    list <CommandTreeNode*> _children;
};

class CommandTree {
public:
    CommandTree();
    ~CommandTree();
    void push(const string& str);
    void pop();
    void instantiate(const ConfigTreeNode *ctn,
		     const TemplateTreeNode *ttn);
    void activate_current();
    void print() const;
    const CommandTreeNode* root() const {return &_root;}
#ifdef NOTDEF
    const CommandTreeNode* subroot(const list <string>& path) const;
#endif
private:
    list <string> _temp_path;
    CommandTreeNode _root;
    CommandTreeNode *_current_node;
};

#endif // __RTRMGR_COMMAND_TREE_HH__
