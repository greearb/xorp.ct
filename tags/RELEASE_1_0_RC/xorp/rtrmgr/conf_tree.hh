// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/conf_tree.hh,v 1.13 2004/05/28 22:27:56 pavlin Exp $

#ifndef __RTRMGR_CONF_TREE_HH__
#define __RTRMGR_CONF_TREE_HH__


#include <map>
#include <list>
#include <set>

#include "conf_tree_node.hh"
#include "module_manager.hh"
#include "rtrmgr_error.hh"
#include "xorp_client.hh"


class TemplateTree;
class CommandTree;
class ConfTemplate;

class ConfigTree {
public:
    ConfigTree(TemplateTree *tt, bool verbose);
    ~ConfigTree();

    ConfigTree& operator=(const ConfigTree& orig_tree);
    bool parse(const string& configuration, const string& config_file,
	       string& errmsg);
    void push_path();
    void extend_path(const string& segment);
    void pop_path();
    void add_node(const string& nodename) throw (ParseError);
    void terminal_value(char* value, int type) throw (ParseError);
    list<string> path_as_segments() const;
    const TemplateTreeNode* 
        find_template(const list<string>& path_segments) const;
    ConfigTreeNode& root_node() { return _root_node; }
    const ConfigTreeNode& const_root_node() const { return _root_node; }
    ConfigTreeNode* find_node(const list<string>& path);
    const ConfigTreeNode* find_config_module(const string& module_name) const;
    string show_subtree(const list<string>& path_segments) const;
    string show_tree() const;
    string show_unannotated_tree() const;
    string tree_str() const;

    bool apply_deltas(uid_t user_id, const string& deltas, 
		      bool provisional_change, string& response);
    bool apply_deletions(uid_t user_id, const string& deletions, 
			 bool provisional_change, string& response);

    void expand_varname_to_matchlist(const string& varname, 
    				     list<string>& matches) const;
    void retain_different_nodes(const ConfigTree& them,
				bool retain_changed_values);
    void retain_common_nodes(const ConfigTree& them);
    void add_default_children();

protected:
    string path_as_string(const list<string>& path_segments) const;
    string current_path_as_string() const;
    const ConfigTreeNode* find_config_node(const list<string>& path_segments) const;

    string		_config_file;
    TemplateTree*	_template_tree;
    ConfigTreeNode	_root_node;
    ConfigTreeNode*	_current_node;
    list<string>	_path_segments;
    list<size_t>	_segment_lengths;
    bool		_verbose;
};

#endif // __RTRMGR_CONF_TREE_HH__
