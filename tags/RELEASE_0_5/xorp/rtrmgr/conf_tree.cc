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

#ident "$XORP: xorp/rtrmgr/conf_tree.cc,v 1.6 2003/09/24 16:16:07 hodson Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "conf_tree.hh"
#include "util.hh"

extern int init_bootfile_parser(const char *configuration,
				const char *filename, ConfigTree *c);
extern int parse_bootfile();
extern int booterror(const char *s);

/*************************************************************************
 * Config File class
 *************************************************************************/

ConfigTree::ConfigTree(TemplateTree *tt) {
    _template_tree = tt;
    _current_node = &_root_node;
}

ConfigTree::~ConfigTree() {
    //_root_node will handle the deletion of all the tree nodes
}

ConfigTree& ConfigTree::operator=(const ConfigTree& orig_tree) {
    _root_node.clone_subtree(orig_tree.const_root());
    return *this;
}

int ConfigTree::parse(const string& configuration, const string& conffile) {
    init_bootfile_parser(configuration.c_str(), conffile.c_str(), this);
    parse_bootfile();
    return 0;
}

void ConfigTree::add_default_children() {
    _root_node.recursive_add_default_children();
}

TemplateTreeNode *ConfigTree::find_template(const list <string>& pathsegs)
    throw (ParseError)
{
    debug_msg("----------------------------------------------------------\n");
    debug_msg("looking for template for \"%s\"\n",
	   path_as_string(pathsegs).c_str());
    TemplateTreeNode *ttn;

    ttn = _template_tree->find_node(pathsegs);
    return ttn;

}

list<string>
ConfigTree::path_as_segs() const {
    list <string> path_segs;
    ConfigTreeNode* ctn = _current_node;
    while (ctn->parent() != NULL) {
	path_segs.push_front(ctn->segname());
	ctn = ctn->parent();
    }
    return path_segs;
}

string
ConfigTree::current_path_as_string() const {
    string path;
    ConfigTreeNode* ctn = _current_node;
    while (ctn->parent() != NULL) {
	if (ctn == _current_node)
	    path = ctn->segname();
	else
	    path = ctn->segname() + " " + path;
	ctn = ctn->parent();
    }
    return path;
}

string
ConfigTree::path_as_string(const list <string>& pathsegs) const {
    string path;
    list <string>::const_iterator i;
    for (i = pathsegs.begin(); i!= pathsegs.end(); ++i) {
	if (path.empty())
	    path = *i;
	else
	    path += " " + *i;
    }
    return path;
}

void
ConfigTree::extend_path(const string &segment) {
    _path_segs.push_back(segment);
}

void
ConfigTree::pop_path() {
    int segs_to_pop = _seg_lengths.front();
    _seg_lengths.pop_front();
    for(int i =0; i<segs_to_pop; i++) {
	_current_node = _current_node->parent();
    }
}

void
ConfigTree::push_path() {
    string path = current_path_as_string();
    string nodename = _path_segs.back();

    //keep track of how many segments comprise this frame so we can
    //pop the right number later
    int len = _path_segs.size();
    _seg_lengths.push_front(len);

    list <string>::const_iterator i;
    for (i = _path_segs.begin(); i != _path_segs.end(); i++) {
	add_node(*i);
    }

    while (_path_segs.size()>0)
	_path_segs.pop_front();
}

void
ConfigTree::add_node(const string& segment) {
    list <ConfigTreeNode*>::const_iterator i;
    i = _current_node->children().begin();
    ConfigTreeNode *found = NULL;
    while (i!= _current_node->children().end()) {
	if ((*i)->segname() == segment) {
	    if (found != NULL) {
		/*
		 * If there are two nodes with the same segment name,
		 * we can only distinguish between them by type.
		 * extend_path doesn't have the type information
		 * available because it wasn't at the relevant point
		 * in the template file, so this is an error.  The
		 * correct way to step past such a node would be
		 * through a call to add_node .
		 */
		string err = "Need to qualify type of " + segment + "\n";
		xorp_throw(ParseError, err);
	    }
	    found = *i;
	}
	i++;
    }
    if (found != NULL) {
	_current_node = found;
    } else {
	list<string> pathsegs = path_as_segs();
	pathsegs.push_back(segment);
	TemplateTreeNode *ttn = find_template(pathsegs);
	if (ttn == NULL) {
	    booterror("No template found in template map");
	    exit(1);
	}

	string path = current_path_as_string();
	if (path.empty())
	    path = segment;
	else
	    path += " " + segment;
	found = new ConfigTreeNode(segment, path, ttn, _current_node, /*user_id*/0);
	_current_node = found;
    }
}

void
ConfigTree::terminal_value(char *value, int type) {
    string path(current_path_as_string());
    string svalue(value);
    ConfigTreeNode *ctn = _current_node;
    XLOG_ASSERT(ctn != NULL);

    /*special case for bool types to avoid needing to type "true"*/
    if (svalue == "" && (type == NODE_VOID)) {
	if (ctn->type() == NODE_BOOL)
	    type = NODE_BOOL;
	    svalue = "true";
    }
    if ((ctn->type() == NODE_TEXT) && (type == NODE_TEXT)) {
	/*trim the quotes*/
	if (svalue[0]=='"' && svalue[svalue.size()-1]=='"')
	    svalue = svalue.substr(1,svalue.size()-2);
    } else if ((ctn->type() == NODE_TEXT) && (type != NODE_TEXT)) {
	/*we'll accept anything as text*/
    } else if ((ctn->type() != NODE_TEXT) && (type == NODE_TEXT)) {
	/*the value was quoted in the bootfile.  We can't tell if
          there's a mismatch without doing some secondary parsing*/
	if (svalue[0]=='"' && svalue[svalue.size()-1]=='"')
	    svalue = svalue.substr(1,svalue.size()-2);

	switch (ctn->type()) {
	case NODE_VOID:
	    //not clear what to do here
	    break;
	case NODE_UINT:
	    for(u_int i=0; i<svalue.size(); i++) {
		if ((svalue[i]< '0') || (svalue[i]> '9')) {
		    goto parse_error;
		}
	    }
	    break;
	case NODE_BOOL:
	    if (svalue == "true" || svalue == "false" || svalue == "")
		break;
	    goto parse_error;
	case NODE_IPV4:
	    try {
		IPv4 (svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_IPV4PREFIX:
	    try {
		IPv4Net (svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_IPV6:
	    try {
		IPv6 (svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_IPV6PREFIX:
	    try {
		IPv6Net (svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_MACADDR:
	    try {
		Mac (svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	default:
	    //Did we forget to add a new type?
	    XLOG_FATAL("Unexpected type %d received\n", ctn->type());
	}
    } else if (ctn->type() != type) {
	string err = "\"" + path + "\" has type " + ctn->typestr() +
	    ", and value " + svalue + " is not a valid " +
	    ctn->typestr();
	booterror(err.c_str());
	exit(1);
    }
    ctn->set_value(svalue, /*userid*/0);
    return;

    parse_error:
    string err = "\"" + path + "\" has type " + ctn->typestr() +
	", and value " + svalue + " is not a valid " +
	ctn->typestr();
    booterror(err.c_str());
    exit(1);
}

const ConfigTreeNode*
ConfigTree::find_config_node(const list <string>& pathsegs) const {
    const ConfigTreeNode *found = &_root_node;
    const ConfigTreeNode *found2 = found;
    list <string>::const_iterator pi;
    list <ConfigTreeNode *>::const_iterator ci;
    int i = 0;
    for (pi = pathsegs.begin(); pi != pathsegs.end(); pi++) {
	i++;
	for (ci = found->const_children().begin();
	     ci != found->const_children().end();
	     ci++) {
	    if (*pi == (*ci)->segname()) {
		found2 = *ci;
		break;
	    }
	}
	if (found2 == found)
	    return NULL;
	found = found2;
    }
    return found;
}


string
ConfigTree::show_subtree(const list <string>& pathsegs) const {
    const ConfigTreeNode *found = find_config_node(pathsegs);
    if (found == NULL)
	return "ERROR";

    string s= found->show_subtree(/*depth*/0, /*indent*/ 0, /*do_indent*/true,
				  /*annotate*/true);
    return s;
}

string
ConfigTree::show_tree() const {
    return _root_node.show_subtree(/*depth*/0, /*indent*/ 0,
				   /*do_indent*/true, /*annotate*/true);
}

string
ConfigTree::show_unannotated_tree() const {
    return _root_node.show_subtree(/*depth*/0, /*indent*/ 0,
				   /*do_indent*/true, /*annotate*/false);
}

ConfigTreeNode*
ConfigTree::find_node(const list <string>& path) {
    /* copy the path so we can modify it while searching... */
    list <string> path_copy;
    list <string>::const_iterator ci;
    for (ci=path.begin(); ci != path.end(); ci++) {
	path_copy.push_back(*ci);
    }
    return _root_node.find_node(path_copy);
}

void
ConfigTree::print() const {
    _root_node.print_tree();
}

bool
ConfigTree::apply_deltas(uid_t user_id, const string& deltas,
			 bool provisional_change, string& response) {
#ifdef DEBUG_CONFIG_CHANGE
    printf("CT apply_deltas %d %s\n",
	   user_id, deltas.c_str());
#endif
    ConfigTree delta_tree(_template_tree);
    try {
	 ((ConfigTree*)(&delta_tree))->parse(deltas, "");
    } catch (ParseError &pe) {
	response = pe.why();
#ifdef DEBUG_CONFIG_CHANGE
	printf("apply deltas failed, response = %s\n", response.c_str());
#endif
	return false;
    }
#ifdef DEBUG_CONFIG_CHANGE
    printf("Delta tree:\n");
    delta_tree.print();
    printf("end delta tree.\n");
#endif
    response = "";
    return root().merge_deltas(user_id, delta_tree.const_root(),
				provisional_change, response);
}

bool
ConfigTree::apply_deletions(uid_t user_id, const string& deletions,
			    bool provisional_change, string& response) {
#ifdef DEBUG_CONFIG_CHANGE
    printf("CT apply_deletions %d %s\n",
	   user_id, deletions.c_str());
#endif
    ConfigTree deletion_tree(_template_tree);
    try {
	((ConfigTree*)(&deletion_tree))->parse(deletions, "");
    } catch (ParseError &pe) {
	response = pe.why();
	return false;
    }
#ifdef DEBUG_CONFIG_CHANGE
    printf("Deletion tree:\n");
    deletion_tree.print();
    printf("end deletion tree.\n");
#endif
    response = "";
    return root().merge_deletions(user_id, deletion_tree.const_root(),
				   provisional_change, response);
}

void
ConfigTree::expand_varname_to_matchlist(const string& varname,
				      list <string>& matches) const {
    //trim $( and )
    string trimmed = varname.substr(2, varname.size()-3);

    //split on dots
    list <string> sl = split(trimmed, '.');

    //copy into a vector
    int len = sl.size();
    vector<string> v(len);
    for (int i = 0; i < len; i++) {
	v[i] = sl.front();
	sl.pop_front();
    }

    _root_node.expand_varname_to_matchlist(v, 0, matches);
}

void
ConfigTree::retain_different_nodes(const ConfigTree& them,
				   bool retain_changed_values) {
    _root_node.retain_different_nodes(them.const_root(),
				      retain_changed_values);
}

void
ConfigTree::retain_common_nodes(const ConfigTree& them) {
    _root_node.retain_common_nodes(them.const_root());
}

