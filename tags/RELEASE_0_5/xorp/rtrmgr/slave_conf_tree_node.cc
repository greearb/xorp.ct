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

#ident "$XORP: xorp/rtrmgr/slave_conf_tree_node.cc,v 1.4 2003/05/10 23:29:22 mjh Exp $"

#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "slave_conf_tree_node.hh"
#include "command_tree.hh"
#include "cli.hh"
#include "util.hh"

extern int booterror(const char *s);

#ifdef NOTDEF
SlaveConfigTreeNode::SlaveConfigTreeNode() : ConfigTreeNode()
{
}
#endif


SlaveConfigTreeNode::SlaveConfigTreeNode(const string& nodename,
					 const string &path, 
					 const TemplateTreeNode *ttn,
					 SlaveConfigTreeNode *parent,
					 uid_t user_id)
    : ConfigTreeNode(nodename, path, ttn, parent, user_id)
{
}

void
SlaveConfigTreeNode::create_command_tree(CommandTree& cmd_tree,
					 const list <string>& cmd_names,
					 bool include_intermediates,
					 bool include_templates) const {
#ifdef NOTDEF
    printf("#############  ConfigTreeNode::get_command_tree called on node %s\n", _segname.c_str());
#endif
    build_command_tree(cmd_tree, cmd_names, 0, 
		       include_intermediates, include_templates);
#ifdef NOTDEF
    printf("#############  leaving ConfigTreeNode::get_command_tree\n");
#endif
}

bool
SlaveConfigTreeNode::build_command_tree(CommandTree& cmd_tree, 
					const list<string>& cmd_names, 
					int depth,
					bool include_intermediates,
					bool include_templates) const {
    bool instantiated = false;
#ifdef DEBUG
    printf("build_command_tree depth=%d\n", depth);
#endif
    if (_deleted) {
#ifdef DEBUG
	printf("Node %s is deleted\n", _path.c_str());
#endif
	return false;
    }

    if (depth > 0) {	
	assert(_template != NULL);
	cmd_tree.push(_segname);
	if (include_intermediates 
	    && _template->is_tag()==false
	    && _template->children().empty()==false) {
	    //include_intermediates indicates that we want to include all
	    //true subtree interior nodes.  This is needed for show and
	    //edit commands, which can show or edit any point in the
	    //hierarchy
#ifdef DEBUG
	    printf("ACTIVATE NODE: %s\n", _path.c_str());
#endif
	    cmd_tree.instantiate(this, _template);
	    instantiated = true;
	} else {
	    //check to see if this node has a command that matches
	    //what we're looking for
	    list <string>::const_iterator ci;
	    for (ci = cmd_names.begin(); ci != cmd_names.end(); ci++) {
		if(_template->const_command(*ci)!=NULL) {
#ifdef DEBUG
		    printf("CMDTREE: %s NODE: %s\n", ci->c_str(), 
			   _path.c_str());
#endif
		    cmd_tree.instantiate(this, _template);
		    instantiated = true;
		    break;
		}
	    }
	}
    }

    list <ConfigTreeNode*>::const_iterator cti;
    set <const TemplateTreeNode*> templates_done;
    bool done = false;
    for (cti=_children.begin(); cti!= _children.end(); ++cti) {
	SlaveConfigTreeNode *sctn = (SlaveConfigTreeNode*)*cti;
	done = sctn->build_command_tree(cmd_tree, cmd_names, depth+1,
					include_intermediates,
					include_templates);
	if (done) {
	    templates_done.insert(sctn->template_node());
	    instantiated = true;
	}
    }
#ifdef DEBUG
    printf("-------\n***back at %s\n", _path.c_str());
    printf("***templates_done.size()==%d\n", templates_done.size());
    printf("***templates_done = ");
    set <const TemplateTreeNode*>::const_iterator donei;
    for (donei = templates_done.begin(); donei != templates_done.end(); donei++) {
	printf("%p ", *donei);
    }
    printf("\n");
#endif
    //if we haven't already added the children of the template node,
    //we need to consider whether they can add to the command tree
    //too.
    if ((_template != NULL)  && include_templates) {
	list <TemplateTreeNode*>::const_iterator tti;
	for(tti = _template->children().begin();
	    tti != _template->children().end();
	    tti++) {
	    if (templates_done.find(*tti) == templates_done.end()) {
#ifdef DEBUG
		printf("***We might add TTN %s [%p] for ", (*tti)->segname().c_str(), *tti);
		list <string>::const_iterator ci;
		for (ci = cmd_names.begin(); ci != cmd_names.end(); ci++) {
		    printf("%s ", ci->c_str());
		}
		printf("\n*** at path %s\n", _path.c_str());
#endif
		if ((*tti)->check_command_tree(cmd_names, 
					       include_intermediates, 
					       /*depth*/0)) {
#ifdef DEBUG
		    printf("***done == true\n");
#endif
		    cmd_tree.push((*tti)->segname());
		    cmd_tree.instantiate(NULL, (*tti));
		    cmd_tree.pop();
		    instantiated = true;
		} else {
#ifdef DEBUG
		    printf("***done == false\n");
#endif
		}
	    }
	}
    }
    
    if (depth > 0)
	cmd_tree.pop();
#ifdef DEBUG
    printf("leaving build_command_tree\n");
#endif
    return instantiated;
}


int
SlaveConfigTreeNode::get_deltas(const SlaveConfigTreeNode& master_node) {
    int deltas = 0;
    printf("get_deltas >%s<\n", _path.c_str());
    if (!_deleted) {
	if ((!_existence_committed || !_value_committed) && (_parent!=NULL)) {
	    deltas++;
	    printf(">%s< CHANGED\n", _path.c_str());
	}
	list <ConfigTreeNode*>::const_iterator i;
	for (i=master_node.const_children().begin();
	     i!=master_node.const_children().end();
	     i++) {
	    ConfigTreeNode *new_node;
	    const ConfigTreeNode *my_child = *i;
	    new_node = new ConfigTreeNode(*my_child);
	    new_node->set_parent(this);
	    add_child(new_node);
	    deltas += ((SlaveConfigTreeNode*)new_node)->
		get_deltas(*(const SlaveConfigTreeNode*)my_child);
	}
    } else {
	printf(">%s< was deleted\n", _path.c_str());
    }
    if ((deltas == 0) && (_parent != NULL)) {
	printf("removing >%s<\n", _path.c_str());
	_parent->remove_child(this);
	delete this;
    }
    return deltas;
}

int
SlaveConfigTreeNode::get_deletions(const SlaveConfigTreeNode& master_node) {
    int deletions = 0;
    printf("get_deletions >%s<\n", _path.c_str());
    if ((!master_node.deleted()) || (_template!=NULL && is_tag())) {
	list <ConfigTreeNode*>::const_iterator i;
	for (i=master_node.const_children().begin();
	     i!=master_node.const_children().end();
	     i++) {
	    ConfigTreeNode *new_node;
	    const ConfigTreeNode *my_child = *i;
	    new_node = new ConfigTreeNode(*my_child);
	    new_node->set_parent(this);
	    new_node->undelete();
	    add_child(new_node);
	    deletions += ((SlaveConfigTreeNode*)new_node)->
		get_deletions(*(const SlaveConfigTreeNode*)my_child);
	}
    } else {
	if (_existence_committed) {
	    printf("%s deleted\n", _path.c_str());
	    deletions = 1;
	} else 
	    printf("%s deleted but not committed\n", _path.c_str());
    }
    if ((deletions == 0) && (_parent != NULL)) {
	printf("removing >%s<\n", _path.c_str());
	_parent->remove_child(this);
	delete this;
    }
    return deletions;
}

bool 
SlaveConfigTreeNode::check_allowed_value(string& errmsg) const {
    if (_template == NULL)
	return true;

    const AllowCommand *cmd;
    cmd = dynamic_cast<const AllowCommand *>
	(_template->const_command("%allow"));
    if (cmd != NULL) {
	try {
	    cmd->execute(*this);
	} catch (ParseError(&pe)) {
	    string errpath;
	    if (_parent != NULL && _parent->is_tag())
		errpath = _parent->path();
	    else
		errpath = path();
	    errmsg = "Bad value for \"" + errpath + "\"\n";
	    list <string> allowed_values = cmd->allowed_values();
	    if (allowed_values.size()==1) {
		errmsg += "The only value allowed is " +
		    allowed_values.front() + ".\n";
	    } else {
		errmsg += "Allowed values are ";
		errmsg += allowed_values.front();
		allowed_values.pop_front();
		while (!allowed_values.empty()) {
		    if (allowed_values.size()==1)
			errmsg += " and ";
		    else
			errmsg += ", ";
		    errmsg += allowed_values.front();
		    allowed_values.pop_front();
		}
		errmsg += ".\n";
	    }
	    return false;
	}
    }
    return true;
}

