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

#ident "$XORP: xorp/rtrmgr/slave_conf_tree_node.cc,v 1.35 2008/10/02 21:58:24 bms Exp $"


#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "cli.hh"
#include "command_tree.hh"
#include "slave_conf_tree_node.hh"
#include "template_commands.hh"
#include "template_tree_node.hh"
#include "util.hh"


extern int booterror(const char *s);

SlaveConfigTreeNode::SlaveConfigTreeNode(bool verbose)
    : ConfigTreeNode(verbose)
{
}

SlaveConfigTreeNode::SlaveConfigTreeNode(const string& nodename,
					 const string &path, 
					 const TemplateTreeNode *ttn,
					 SlaveConfigTreeNode *parent,
					 const ConfigNodeId& node_id,
					 uid_t user_id,
					 uint32_t clientid,
					 bool verbose)
    : ConfigTreeNode(nodename, path, ttn, parent, node_id, user_id, 
		     clientid, verbose)
{

}

ConfigTreeNode*
SlaveConfigTreeNode::create_node(const string& segment, const string& path,
				 const TemplateTreeNode* ttn,
				 ConfigTreeNode* parent_node,
				 const ConfigNodeId& node_id,
				 uid_t user_id,
				 uint32_t clientid,
				 bool verbose)
{
    SlaveConfigTreeNode *new_node, *parent;
    parent = dynamic_cast<SlaveConfigTreeNode *>(parent_node);

    // sanity check - all nodes in this tree should be Slave nodes
    if (parent_node != NULL)
	XLOG_ASSERT(parent != NULL);

    new_node = new SlaveConfigTreeNode(segment, path, ttn, parent, 
				       node_id, user_id, clientid, verbose);
    return reinterpret_cast<ConfigTreeNode*>(new_node);
}

ConfigTreeNode*
SlaveConfigTreeNode::create_node(const ConfigTreeNode& ctn)
{
    SlaveConfigTreeNode *new_node;
    const SlaveConfigTreeNode *orig;

    debug_msg("SlaveConfigTreeNode::create_node\n");

    // sanity check - all nodes in this tree should be Slave nodes
    orig = dynamic_cast<const SlaveConfigTreeNode *>(&ctn);
    XLOG_ASSERT(orig != NULL);

    new_node = new SlaveConfigTreeNode(*orig);
    return new_node;
}

void
SlaveConfigTreeNode::create_command_tree(CommandTree& cmd_tree,
					 const list<string>& cmd_names,
					 bool include_intermediate_nodes,
					 bool include_children_templates,
					 bool include_leaf_value_nodes,
					 bool include_read_only_nodes,
					 bool include_permanent_nodes,
					 bool include_user_hidden_nodes) const
{
    build_command_tree(cmd_tree, cmd_names, 0, include_intermediate_nodes,
		       include_children_templates, include_leaf_value_nodes,
		       include_read_only_nodes, include_permanent_nodes,
		       include_user_hidden_nodes);
}

bool
SlaveConfigTreeNode::build_command_tree(CommandTree& cmd_tree, 
					const list<string>& cmd_names, 
					int depth,
					bool include_intermediate_nodes,
					bool include_children_templates,
					bool include_leaf_value_nodes,
					bool include_read_only_nodes,
					bool include_permanent_nodes,
					bool include_user_hidden_nodes) const
{
    bool instantiated = false;

    debug_msg("build_command_tree depth=%d\n", depth);

    if (_template_tree_node != NULL) {
	// XXX: ignore deprecated subtrees
	if (_template_tree_node->is_deprecated())
	    return false;
	// XXX: ignore user-hidden subtrees
	if (_template_tree_node->is_user_hidden())
	    return false;
    }

    if (_deleted) {
	debug_msg("Node %s is deleted\n", _path.c_str());
	return false;
    }

    if (is_leaf_value() && (! include_leaf_value_nodes)) {
	// XXX: exclude leaf nodes when not needed
	return false;
    }

    if (depth > 0) {
	XLOG_ASSERT(_template_tree_node != NULL);
	cmd_tree.push(_segname);

	//
	// XXX: ignore read-only nodes and permanent nodes
	//
	bool include_node = true;
	if (is_read_only() && (! include_read_only_nodes))
	    include_node = false;
	if (is_permanent() && (! include_permanent_nodes))
	    include_node = false;
	if (is_user_hidden() && (! include_user_hidden_nodes))
	    include_node = false;

	if (include_node) {
	    if (include_intermediate_nodes
		&& (_template_tree_node->is_tag() == false)
		&& (_template_tree_node->children().empty() == false)) {
		//
		// Include_intermediate_nodes indicates that we want to
		// include all true subtree interior nodes.  This is needed
		// for show and edit commands, which can show or edit any
		// point in the hierarchy.
		//
		debug_msg("ACTIVATE NODE: %s\n", _path.c_str());
		cmd_tree.instantiate(this, _template_tree_node,
				     true /* has_command */);
		instantiated = true;
	    } else {
		// Check to see if this node has a command that matches
		// what we're looking for.
		list<string>::const_iterator iter;
		bool has_command = false;
		for (iter = cmd_names.begin(); iter != cmd_names.end(); ++iter) {
		    if (_template_tree_node->const_command(*iter) != NULL) {
			has_command = true;
			break;
		    }
		}
		if (is_leaf_value() && include_leaf_value_nodes)
		    has_command = true;
		cmd_tree.instantiate(this, _template_tree_node, has_command);
		instantiated = true;
	    }
	}
    }

    bool done = false;
    set<const TemplateTreeNode*> templates_done;
    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	SlaveConfigTreeNode *sctn = (SlaveConfigTreeNode*)*iter;
	done = sctn->build_command_tree(cmd_tree, cmd_names, depth + 1,
					include_intermediate_nodes,
					include_children_templates,
					include_leaf_value_nodes,
					include_read_only_nodes,
					include_permanent_nodes,
					include_user_hidden_nodes);
	if (done) {
	    templates_done.insert(sctn->template_tree_node());
	    instantiated = true;
	}
    }

    XLOG_TRACE(_verbose, "-------\n***back at %s\n", _path.c_str());
    XLOG_TRACE(_verbose, "***templates_done.size()==%u\n",
	       XORP_UINT_CAST(templates_done.size()));
    XLOG_TRACE(_verbose, "***templates_done = ");
    if (_verbose) {
	string debug_output;
	set<const TemplateTreeNode*>::const_iterator done_iter;
	for (done_iter = templates_done.begin();
	     done_iter != templates_done.end();
	     ++done_iter) {
	    debug_output += c_format("%p ", *done_iter);
	}
	debug_output += "\n";
	XLOG_TRACE(_verbose, "%s", debug_output.c_str());
    }

    //
    // If we haven't already added the children of the template node,
    // we need to consider whether they can add to the command tree too.
    //
    if ((_template_tree_node != NULL) && include_children_templates) {
	list<TemplateTreeNode*>::const_iterator ttn_iter;
	for (ttn_iter = _template_tree_node->children().begin();
	     ttn_iter != _template_tree_node->children().end();
	     ++ttn_iter) {
	    if (templates_done.find(*ttn_iter) == templates_done.end()) {

		XLOG_TRACE(_verbose, "***We might add TTN %s [%p] for ",
			   (*ttn_iter)->segname().c_str(), *ttn_iter);
		if (_verbose) {
		    string debug_output;
		    list<string>::const_iterator cmd_iter;
		    for (cmd_iter = cmd_names.begin();
			 cmd_iter != cmd_names.end(); ++cmd_iter) {
			debug_output += c_format("%s ", cmd_iter->c_str());
		    }
		    debug_output += c_format("\n*** at path %s\n",
					     _path.c_str());
		    XLOG_TRACE(_verbose, "%s", debug_output.c_str());
		}

		if ((*ttn_iter)->check_command_tree(cmd_names,
						    include_intermediate_nodes,
						    include_read_only_nodes,
						    include_permanent_nodes,
						    include_user_hidden_nodes,
						    /* depth */ 0)) {

		    XLOG_TRACE(_verbose, "***done == true\n");

		    cmd_tree.push((*ttn_iter)->segname());
		    cmd_tree.instantiate(NULL, (*ttn_iter),
					 true /* has_command */);
		    cmd_tree.pop();
		    instantiated = true;
		} else {
		    XLOG_TRACE(_verbose, "***done == false\n");
		}
	    }
	}
    }
    
    if (depth > 0)
	cmd_tree.pop();

    XLOG_TRACE(_verbose, "leaving build_command_tree\n");

    return instantiated;
}

int
SlaveConfigTreeNode::get_deltas(const SlaveConfigTreeNode& master_node)
{
    int deltas = 0;

    XLOG_TRACE(_verbose, "get_deltas >%s<\n", _path.c_str());

    if (!_deleted) {
	if ((!_existence_committed || !_value_committed)
	    && (_parent != NULL)) {
	    deltas++;
	    XLOG_TRACE(_verbose, ">%s< CHANGED\n", _path.c_str());
	}
	list<ConfigTreeNode*>::const_iterator iter;
	for (iter = master_node.const_children().begin();
	     iter != master_node.const_children().end();
	     ++iter) {
	    SlaveConfigTreeNode* new_node;
	    const SlaveConfigTreeNode* my_child
		= dynamic_cast<SlaveConfigTreeNode*>(*iter);
	    XLOG_ASSERT(my_child != NULL);

	    new_node = new SlaveConfigTreeNode(*my_child);
	    new_node->set_parent(this);
	    add_child(new_node);
	    deltas += ((SlaveConfigTreeNode*)new_node)->
		get_deltas(*(const SlaveConfigTreeNode*)my_child);
	}
    } else {
	XLOG_TRACE(_verbose, ">%s< was deleted\n", _path.c_str());
    }
    if ((deltas == 0) && (_parent != NULL)) {
	XLOG_TRACE(_verbose, "removing >%s<\n", _path.c_str());
	delete_subtree_silently();	// XXX: will delete 'this' as well
    }
    return deltas;
}

int
SlaveConfigTreeNode::get_deletions(const SlaveConfigTreeNode& master_node)
{
    int deletions = 0;

    XLOG_TRACE(_verbose, "get_deletions >%s<\n", _path.c_str());

    if ((! master_node.deleted())
	|| (_template_tree_node != NULL && is_tag())) {
	list<ConfigTreeNode*>::const_iterator iter;
	for (iter = master_node.const_children().begin();
	     iter != master_node.const_children().end();
	     ++iter) {
	    SlaveConfigTreeNode* new_node;
	    const SlaveConfigTreeNode* my_child  
		= dynamic_cast<SlaveConfigTreeNode*>(*iter);
	    XLOG_ASSERT(my_child != NULL);
	    new_node = new SlaveConfigTreeNode(*my_child);
	    new_node->set_parent(this);
	    new_node->undelete();
	    add_child(new_node);
	    deletions += ((SlaveConfigTreeNode*)new_node)->
		get_deletions(*(const SlaveConfigTreeNode*)my_child);
	}
    } else {
	if (_existence_committed) {
	    XLOG_TRACE(_verbose, "%s deleted\n", _path.c_str());
	    deletions = 1;
	} else {
	    XLOG_TRACE(_verbose, "%s deleted but not committed\n",
		       _path.c_str());
	}
    }
    if ((deletions == 0) && (_parent != NULL)) {
	XLOG_TRACE(_verbose, "removing >%s<\n", _path.c_str());
	delete_subtree_silently();	// XXX: will delete 'this' as well
    }
    return deletions;
}

void 
SlaveConfigTreeNode::finalize_commit()
{
    debug_msg("SlaveConfigTreeNode::finalize_commit %s\n",
	      _segname.c_str());

    if (_deleted) {
	debug_msg("node deleted\n");

	//
	// Delete the entire subtree. We expect a deleted node here
	// to have previously had its existence committed
	// (_existence_committed == true) but its non-existence not
	// previously committed (_value_committed == false)
	//
	XLOG_ASSERT(_existence_committed);
	XLOG_ASSERT(!_value_committed);  
	delete_subtree_silently();
	// No point in going further
	return;
    }
    if ((_existence_committed == false) || (_value_committed == false)) {
	debug_msg("node finalized\n");

	_existence_committed = true;
	_value_committed = true;
	_deleted = false;
	_committed_value = _value;
	_committed_operator = _operator;
	_committed_user_id = _user_id;
	_committed_modification_time = _modification_time;
    }

    //
    // Note: finalize_commit may delete the child, so we need to be
    // careful the iterator stays valid.
    //
    list<ConfigTreeNode *>::iterator iter, prev_iter;
    iter = _children.begin(); 
    while (iter != _children.end()) {
	prev_iter = iter;
	++iter;
	SlaveConfigTreeNode *child = (SlaveConfigTreeNode*)(*prev_iter);
	child->finalize_commit();
    }
}

