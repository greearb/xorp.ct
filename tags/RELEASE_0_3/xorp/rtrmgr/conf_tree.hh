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

// $XORP: xorp/rtrmgr/conf_tree.hh,v 1.3 2003/04/23 04:24:34 mjh Exp $

#ifndef __RTRMGR_CONF_TREE_HH__
#define __RTRMGR_CONF_TREE_HH__

#include <map>
#include <list>
#include <set>
#include "config.h"
#include "libxorp/xorp.h"
#include "conf_tree_node.hh"
#include "module_manager.hh"
#include "xorp_client.hh"
#include "parse_error.hh"

class TemplateTree;
class CommandTree;
class ConfTemplate;

class ConfigTree {
public:
    ConfigTree(TemplateTree *tt);
    ~ConfigTree();
    ConfigTree& operator=(const ConfigTree& orig_tree);
    int parse(const string& configuration, const string& conffile);
    void push_path();
    void extend_path(const string &segment);
    void pop_path();
    void add_node(const string& nodename);
    void terminal_value(char *value, int type);
    list <string> path_as_segs() const;
    TemplateTreeNode *find_template(const list<string>& pathsegs) 
	throw (ParseError);
    ConfigTreeNode& root() {return _root_node;}
    const ConfigTreeNode& const_root() const {return _root_node;}
    ConfigTreeNode *find_node(const list <string>& path);
    string show_subtree(const list <string>& pathsegs) const;
    string show_tree() const;
    string show_unannotated_tree() const;
    void print() const;

    bool apply_deltas(uid_t user_id, const string& deltas, 
		      bool provisional_change, string& response);
    bool apply_deletions(uid_t user_id, const string& deletions, 
			 bool provisional_change, string& response);

    void expand_varname_to_matchlist(const string& varname, 
    				     list <string>& matches) const;
    void retain_different_nodes(const ConfigTree& them,
				bool retain_changed_values);
    void retain_common_nodes(const ConfigTree& them);
    void add_default_children();
protected:
    string path_as_string(const list <string>& pathsegs) const;
    string current_path_as_string() const;
    const ConfigTreeNode* 
        find_config_node(const list <string>& pathsegs) const;


    string _conffile;
    TemplateTree *_template_tree;
    ConfigTreeNode _root_node, *_current_node;
    list <string> _path_segs;
    list <int> _seg_lengths;
};

#endif // __RTRMGR_CONF_TREE_HH__
