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

// $XORP: xorp/rtrmgr/conf_tree_node.hh,v 1.13 2003/12/02 09:38:55 pavlin Exp $

#ifndef __RTRMGR_CONF_TREE_NODE_HH__
#define __RTRMGR_CONF_TREE_NODE_HH__

#include <map>
#include <list>
#include <set>
#include <vector>
#include <sys/time.h>
#include "libxorp/xorp.h"
#include "task.hh"


class Command;
class CommandTree;
class RouterCLI;
class TaskManager;
class TemplateTreeNode;

class ConfigTreeNode {
public:
    ConfigTreeNode();
    ConfigTreeNode(const ConfigTreeNode& ctn);
    ConfigTreeNode(const string &node_name, const string &path, 
		   const TemplateTreeNode* ttn, ConfigTreeNode* parent,
		   uid_t user_id);
    ~ConfigTreeNode();

    bool operator==(const ConfigTreeNode& them) const;
    void add_child(ConfigTreeNode* child);
    void remove_child(ConfigTreeNode* child);
    void add_default_children();
    void recursive_add_default_children();
    void set_value(const string& value, uid_t user_id);
    void mark_subtree_as_committed();
    void command_status_callback(Command* cmd, bool success);

    bool merge_deltas(uid_t user_id,
		      const ConfigTreeNode& delta_node, 
		      bool provisional_change,
		      string& response);

    bool merge_deletions(uid_t user_id, const ConfigTreeNode& deletion_node, 
			 bool provisional_change, string& response);
    void find_changed_modules(set<string>& changed_modules) const;
    void find_active_modules(set<string>& active_modules) const;
    void find_all_modules(set<string>& all_modules) const;
    const ConfigTreeNode* find_config_module(const string& module_name) const;

    bool verify_configuration(string& result) const;

    void initialize_commit();
    bool commit_changes(TaskManager& task_manager, bool do_commit,
			int depth, int last_depth, string& result);
    bool check_commit_status(string &response) const;
    void finalize_commit();
    string discard_changes(int depth, int last_depth);
    int type() const;
    bool is_root_node() const { return (_parent == NULL); }
    bool is_tag() const;
    bool is_leaf() const;
    const TemplateTreeNode* template_node() const { return _template; }
    string str() const;
    string node_str() const;
    string typestr() const;
    const string& segname() const { return _segname; }
    const string& value() const;
    const string& named_value(const string& varname) const;
    void set_named_value(const string& varname, const string& value);
    uid_t user_id() const { return _user_id; }
    void set_existence_committed(bool v) { _existence_committed = v; }
    bool existence_committed() const { return _existence_committed; }
    bool deleted() const { return _deleted; }
    void undelete() { _deleted = false; }
    const TimeVal& modification_time() const { return _modification_time; }
    const string& path() const { return _path; }
    void set_parent(ConfigTreeNode* parent) { _parent = parent; }
    ConfigTreeNode* parent() { return _parent; }
    const ConfigTreeNode* const_parent() const { return _parent; }
    list<ConfigTreeNode*>& children() { return _children; }
    const list<ConfigTreeNode*>& const_children() const { return _children; }
    string show_subtree(int depth, int indent, bool do_indent,
			bool annotate) const;
    void mark_subtree_for_deletion(uid_t user_id);
    void delete_subtree_silently();
    void clone_subtree(const ConfigTreeNode& orig_node);
    bool retain_different_nodes(const ConfigTreeNode& them, 
				bool retain_changed_values);
    void retain_common_nodes(const ConfigTreeNode& them);
    ConfigTreeNode* find_node(list<string>& path);
    void print_tree() const;
    string get_module_name_by_variable(const string& varname) const;
    bool expand_expression(const string& varname, string& value) const;
    bool expand_variable(const string& varname, string& value) const;
    void expand_varname_to_matchlist(const vector<string>& v, size_t depth,
				     list<string>& matches) const;
    bool set_variable(const string& varname, string& value);

protected:
    void split_up_varname(const string& varname,
			  list<string>& var_parts) const;
    string join_up_varname(const list<string>& var_parts) const;
    enum VarType { NONE, NODE_VALUE, NAMED, TEMPLATE_DEFAULT };
    ConfigTreeNode* find_varname_node(const string& varname, VarType& type);
    const ConfigTreeNode* find_const_varname_node(const string& varname, 
						  VarType& type) const;
    ConfigTreeNode* find_parent_varname_node(const list<string>& var_parts,
					     VarType& type);
    ConfigTreeNode* find_child_varname_node(const list<string>& var_parts,
					    VarType& type);

    const TemplateTreeNode* _template;
    bool _deleted;	// Indicates node has been deleted, but commit has not 
			// yet happened
    bool _has_value;
    string _value;
    string _committed_value;
    string _segname;
    string _path;
    ConfigTreeNode* _parent;
    list<ConfigTreeNode *> _children;
    uid_t _user_id;	// the user ID of the user who last changed this node
    uid_t _committed_user_id;	// The user ID of the user who last changed
				// this node before the last commit
    TimeVal _modification_time;	// When the node was last changed
    TimeVal _committed_modification_time; // When the node was last changed
					  // before the last commit

    // Flags to keep track of what changes we've made since the last commit
    bool _existence_committed;	// Do we need to run %create commands
    bool _value_committed;	// Do we need to run %setting or %set commands

    int _actions_pending;	// Needed to track how many response callbacks
				// callbacks we expect during a commit
    bool _actions_succeeded;	// Did any action fail during the commit?
    Command *_cmd_that_failed;

    // Variables contains the explicit variables set on this node
    map<string, string> _variables;

    // on_parent_path is used during variable expansion to keep track of where
    // we came from
    bool _on_parent_path;
};

#endif // __RTRMGR_CONF_TREE_NODE_HH__
