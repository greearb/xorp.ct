// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rtrmgr/conf_tree_node.hh,v 1.68 2008/01/04 03:17:39 pavlin Exp $

#ifndef __RTRMGR_CONF_TREE_NODE_HH__
#define __RTRMGR_CONF_TREE_NODE_HH__

#include "libxorp/xorp.h"

#include <map>
#include <list>
#include <set>
#include <vector>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "libxorp/timeval.hh"
#include "libproto/config_node_id.hh"

#include "config_operators.hh"


class Command;
class CommandTree;
class RouterCLI;
class TaskManager;
class TemplateTreeNode;
class ConfigTreeNode;

class CTN_Compare {
public:
    bool operator() (ConfigTreeNode* a, ConfigTreeNode *b);
};

class ConfigTreeNode {
public:
    ConfigTreeNode(bool verbose);
    ConfigTreeNode(const ConfigTreeNode& ctn);
    ConfigTreeNode(const string& node_name, const string& path, 
		   const TemplateTreeNode* ttn, ConfigTreeNode* parent,
		   const ConfigNodeId& node_id,
		   uid_t user_id, uint32_t clientid, bool verbose);
    virtual ~ConfigTreeNode();

    bool operator==(const ConfigTreeNode& them) const;
    bool is_same(const ConfigTreeNode& them, bool ignore_node_id) const;
    
    virtual ConfigTreeNode* create_node(const string& segment, 
					const string& path,
					const TemplateTreeNode* ttn, 
					ConfigTreeNode* parent_node, 
					const ConfigNodeId& node_id,
					uid_t user_id, 
					uint32_t clientid, 
					bool verbose) = 0;
    virtual ConfigTreeNode* create_node(const ConfigTreeNode& ctn) = 0;

    void add_child(ConfigTreeNode* child);
    void remove_child(ConfigTreeNode* child);
    void add_default_children();
    void recursive_add_default_children();
    bool check_allowed_value(string& error_msg) const;
    bool check_allowed_value(const string& value, string& error_msg) const;
    bool check_allowed_operator(const string& value, string& error_msg) const;
    bool set_value(const string& value, uid_t user_id, string& error_msg);
    void set_value_without_verification(const string& value, uid_t user_id);
    bool set_operator(ConfigOperator op, uid_t user_id, string& error_msg);
    void set_operator_without_verification(ConfigOperator op, uid_t user_id);
    void mark_subtree_as_committed();
    void mark_subtree_as_uncommitted();

    bool merge_deltas(uid_t user_id,
		      const ConfigTreeNode& delta_node,
		      bool provisional_change,
		      bool preserve_node_id,
		      string& error_msg);

    bool merge_deletions(uid_t user_id, const ConfigTreeNode& deletion_node, 
			 bool provisional_change, string& error_msg);
    ConfigTreeNode* find_config_module(const string& module_name);

    bool check_config_tree(string& error_msg) const;

    string discard_changes(int depth, int last_depth);
    int type() const;
    bool is_root_node() const { return (_parent == NULL); }
    bool is_tag() const;
    bool is_multi_value_node() const;
    bool is_leaf_value() const;
    bool is_read_only() const;
    const string& read_only_reason() const;
    bool is_permanent() const;
    const string& permanent_reason() const;
    bool is_user_hidden() const;
    const string& user_hidden_reason() const;
    unsigned int depth() const;
    const TemplateTreeNode* template_tree_node() const { return _template_tree_node; }
    int child_number() const;
    string str() const;
    string node_str() const;
    string typestr() const;
    const string& segname() const { return _segname; }
    const string& value() const;
    const ConfigNodeId& node_id() const { return _node_id; }
    void set_node_id_position(const ConfigNodeId::Position& p) {
	_node_id.set_position(p);
    }
    ConfigNodeId& node_id_generator() { return _node_id_generator; }
    uint32_t clientid() const { return _clientid; }
    bool has_value() const { return _has_value; }
    ConfigOperator get_operator() const;
    uid_t user_id() const { return _user_id; }
    void set_existence_committed(bool v) { _existence_committed = v; }
    bool existence_committed() const { return _existence_committed; }
    bool deleted() const { return _deleted; }
    void undelete() { _deleted = false; }
    void undelete_subtree();
    void undelete_node_and_ancestors();
    const TimeVal& modification_time() const { return _modification_time; }
    const string& path() const { return _path; }
    void set_parent(ConfigTreeNode* parent) { _parent = parent; }
    ConfigTreeNode* parent() { return _parent; }
    const ConfigTreeNode* const_parent() const { return _parent; }
    list<ConfigTreeNode*>& children() { return _children; }
    const list<ConfigTreeNode*>& const_children() const { return _children; }
    string show_subtree(bool show_top, int depth, int indent, bool do_indent,
			bool numbered, bool annotate,
			bool suppress_default_values) const;
    void mark_subtree_for_deletion(uid_t user_id);
    void delete_subtree_silently();
    void clone_subtree(const ConfigTreeNode& orig_node);
    bool retain_different_nodes(const ConfigTreeNode& them, 
				bool retain_changed_values);
    bool retain_deletion_nodes(const ConfigTreeNode& them,
			       bool retain_value_changed);
    void retain_common_nodes(const ConfigTreeNode& them);
    ConfigTreeNode* find_node(const list<string>& path);
    const ConfigTreeNode* find_const_node(const list<string>& path) const;
    string subtree_str() const;

    bool expand_variable(const string& varname, string& value,
			 bool ignore_deleted_nodes) const;
    bool expand_variable_to_full_varname(const string& varname,
					 string& full_varname) const;
    bool expand_expression(const string& expression, string& value) const;
    void expand_varname_to_matchlist(const vector<string>& v, size_t depth,
				     list<string>& matches) const;
    bool set_variable(const string& varname, const string& value);
    const string& named_value(const string& varname) const;
    void set_named_value(const string& varname, const string& value);
    string show_operator() const;
    bool is_committed() const;
    bool is_uncommitted() const;
    bool is_default_value() const;
    bool is_default_value(const string& test_value) const;
    bool has_undeleted_children() const;
    virtual void update_node_id_position();

protected:
    bool split_up_varname(const string& varname,
			  list<string>& var_parts) const;
    string join_up_varname(const list<string>& var_parts) const;
    enum VarType { NONE, NODE_VALUE, NODE_OPERATOR, NODE_ID, 
		   NAMED, TEMPLATE_DEFAULT };
    ConfigTreeNode* find_varname_node(const string& varname, VarType& type);
    const ConfigTreeNode* find_const_varname_node(const string& varname, 
						  VarType& type) const;
    ConfigTreeNode* find_parent_varname_node(const list<string>& var_parts,
					     VarType& type);
    ConfigTreeNode* find_child_varname_node(const list<string>& var_parts,
					    VarType& type);
    void sort_by_template(list<ConfigTreeNode*>& children) const;
    string show_node_id(bool numbered, const ConfigNodeId& node_id) const;
    virtual void allocate_unique_node_id();
    string quoted_value(const string& value) const;


    const TemplateTreeNode* _template_tree_node;
    bool _has_value;
    string _value;
    string _committed_value;
    ConfigOperator _operator;
    ConfigOperator _committed_operator;
    string _segname;
    string _path;
    ConfigTreeNode* _parent;
    list<ConfigTreeNode *> _children;
    ConfigNodeId _node_id;
    ConfigNodeId _node_id_generator;
    uid_t _user_id;	// the user ID of the user who last changed this node
    uid_t _committed_user_id;	// The user ID of the user who last changed
				// this node before the last commit
    uint32_t _clientid;
    TimeVal _modification_time;	// When the node was last changed
    TimeVal _committed_modification_time; // When the node was last changed
					  // before the last commit

    // Flags to keep track of what changes we've made since the last commit
    bool _existence_committed;	// Do we need to run %create commands
    bool _value_committed;	// Do we need to run %set commands
    bool _deleted;	// The node is deleted, but commit has not yet happened

    // Variables contains the explicit variables set on this node
    map<string, string> _variables;

    // on_parent_path is used during variable expansion to keep track of where
    // we came from
    bool _on_parent_path;

    bool _verbose;


private:
};

#endif // __RTRMGR_CONF_TREE_NODE_HH__
