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

// $XORP: xorp/rtrmgr/conf_tree.hh,v 1.36 2008/10/02 21:58:22 bms Exp $

#ifndef __RTRMGR_CONF_TREE_HH__
#define __RTRMGR_CONF_TREE_HH__






#include "libproto/config_node_id.hh"

#include "conf_tree_node.hh"
#include "rtrmgr_error.hh"
#include "xorp_client.hh"


class TemplateTree;
class CommandTree;
class ConfTemplate;

class ConfPathSegment {
public:
    ConfPathSegment(const string& segname, int type,
		    const ConfigNodeId& node_id)
	: _segname(segname), _type(type), _node_id(node_id) {}
    const string& segname() const { return _segname; }
    int type() const { return _type; }
    const ConfigNodeId& node_id() const { return _node_id; }

private:
    string		_segname;
    int			_type;
    ConfigNodeId	_node_id;
};

class ConfigTree {
public:
    ConfigTree(TemplateTree *tt, bool verbose);
    virtual ~ConfigTree();

#if 0
    ConfigTree& operator=(const ConfigTree& orig_tree);
#endif
    bool parse(const string& configuration, const string& config_file,
	       string& error_msg);
    void push_path();
    void extend_path(const string& segment, int type,
		     const ConfigNodeId& node_id);
    void pop_path();
    void add_node(const string& nodename, int type,
		  const ConfigNodeId& node_id) 
	throw (ParseError);
    virtual ConfigTreeNode* create_node(const string& segment, 
					const string& path,
					const TemplateTreeNode* ttn, 
					ConfigTreeNode* parent_node, 
					const ConfigNodeId& node_id,
					uid_t user_id, bool verbose) = 0;
    virtual ConfigTree* create_tree(TemplateTree *tt, bool verbose) = 0;
    void terminal_value(const string& value,
			int type, 
			ConfigOperator op) throw (ParseError);
    list<ConfPathSegment> path_as_segments() const;
    const TemplateTreeNode* 
        find_template(const list<string>& path_segments) const;
    const TemplateTreeNode* 
        find_template_by_type(const list<ConfPathSegment>& path_segments) 
	const;
    virtual ConfigTreeNode& root_node() = 0;
    virtual const ConfigTreeNode& const_root_node() const = 0;
    ConfigTreeNode* find_node(const list<string>& path);
    const ConfigTreeNode* find_const_node(const list<string>& path) const;
    ConfigTreeNode* find_config_module(const string& module_name);
    string show_subtree(bool show_top, const list<string>& path_segments, 
			bool numbered, bool suppress_default_values) const;
    string show_tree(bool numbered) const;
    string show_unannotated_tree(bool numbered) const;
    string tree_str() const;

    bool apply_deltas(uid_t user_id, const string& deltas, 
		      bool provisional_change, bool preserve_node_id,
		      string& response);
    bool apply_deletions(uid_t user_id, const string& deletions, 
			 bool provisional_change, string& response);

    void retain_different_nodes(const ConfigTree& them,
				bool retain_changed_values);
    void retain_deletion_nodes(const ConfigTree& them,
			       bool retain_value_changed);
    void retain_common_nodes(const ConfigTree& them);
    void add_default_children();

    void expand_varname_to_matchlist(const string& varname, 
    				     list<string>& matches) const;

protected:
    string path_as_string(const list<string>& path_segments) const;
    string current_path_as_string() const;
    const ConfigTreeNode* find_config_node(const list<string>& path_segments) const;

    string		_config_file;
    TemplateTree*	_template_tree;
    //ConfigTreeNode	_root_node;
    ConfigTreeNode*	_current_node;
    list<ConfPathSegment>	_path_segments;
    list<size_t>	_segment_lengths;
    bool		_verbose;
};

#endif // __RTRMGR_CONF_TREE_HH__
