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

#ident "$XORP: xorp/rtrmgr/slave_conf_tree_node.cc,v 1.11 2004/05/28 22:27:58 pavlin Exp $"


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


SlaveConfigTreeNode::SlaveConfigTreeNode(const string& nodename,
					 const string &path, 
					 const TemplateTreeNode *ttn,
					 SlaveConfigTreeNode *parent,
					 uid_t user_id,
					 bool verbose)
    : ConfigTreeNode(nodename, path, ttn, parent, user_id, verbose)
{

}

void
SlaveConfigTreeNode::create_command_tree(CommandTree& cmd_tree,
					 const list<string>& cmd_names,
					 bool include_intermediates,
					 bool include_templates) const
{
    build_command_tree(cmd_tree, cmd_names, 0, include_intermediates,
		       include_templates);
}

bool
SlaveConfigTreeNode::build_command_tree(CommandTree& cmd_tree, 
					const list<string>& cmd_names, 
					int depth,
					bool include_intermediates,
					bool include_templates) const
{
    bool instantiated = false;

    debug_msg("build_command_tree depth=%d\n", depth);

    if (_deleted) {
	debug_msg("Node %s is deleted\n", _path.c_str());
	return false;
    }

    if (depth > 0) {	
	XLOG_ASSERT(_template_tree_node != NULL);
	cmd_tree.push(_segname);
	if (include_intermediates 
	    && (_template_tree_node->is_tag() == false)
	    && (_template_tree_node->children().empty() == false)) {
	    //
	    // Include_intermediates indicates that we want to include all
	    // true subtree interior nodes.  This is needed for show and
	    // edit commands, which can show or edit any point in the
	    // hierarchy.
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
	    cmd_tree.instantiate(this, _template_tree_node,
				 true /* has_command */);
	    instantiated = true;
	}
    }

    bool done = false;
    set<const TemplateTreeNode*> templates_done;
    list<ConfigTreeNode*>::const_iterator iter;
    for (iter = _children.begin(); iter != _children.end(); ++iter) {
	SlaveConfigTreeNode *sctn = (SlaveConfigTreeNode*)*iter;
	done = sctn->build_command_tree(cmd_tree, cmd_names, depth + 1,
					include_intermediates,
					include_templates);
	if (done) {
	    templates_done.insert(sctn->template_tree_node());
	    instantiated = true;
	}
    }

    XLOG_TRACE(_verbose, "-------\n***back at %s\n", _path.c_str());
    XLOG_TRACE(_verbose, "***templates_done.size()==%u\n",
	       static_cast<uint32_t>(templates_done.size()));
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
    if ((_template_tree_node != NULL)  && include_templates) {
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
						    include_intermediates, 
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
	    ConfigTreeNode* new_node;
	    const ConfigTreeNode* my_child = *iter;

	    new_node = new ConfigTreeNode(*my_child);
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
	_parent->remove_child(this);
	delete this;
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
	    ConfigTreeNode* new_node;
	    const ConfigTreeNode* my_child = *iter;
	    new_node = new ConfigTreeNode(*my_child);
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
	_parent->remove_child(this);
	delete this;
    }
    return deletions;
}

bool 
SlaveConfigTreeNode::check_allowed_value(string& errmsg) const
{
    if (_template_tree_node == NULL)
	return true;

    const Command* c = _template_tree_node->const_command("%allow");
    if (c == NULL) {
	c = _template_tree_node->const_command("%allow-range");
    }

    const AllowCommand* cmd = dynamic_cast<const AllowCommand *>(c);
    if (cmd != NULL) {
	string tmpmsg;
	if (cmd->verify_variable_value(*this, tmpmsg) != true) {
	    string errpath;
	    if (_parent != NULL && _parent->is_tag())
		errpath = _parent->path();
	    else
		errpath = path();
	    errmsg = c_format("Bad value for \"%s\": %s; ",
			      errpath.c_str(), errmsg.c_str());
	    return false;
	}
    }
    return true;
}

