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

#ident "$XORP: xorp/rtrmgr/conf_tree.cc,v 1.21 2004/06/10 22:41:51 hodson Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "conf_tree.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"


extern int init_bootfile_parser(const char* configuration,
				const char* filename, ConfigTree* ct);
extern void parse_bootfile() throw (ParseError);
extern int booterror(const char* s) throw (ParseError);

/*************************************************************************
 * Config File class
 *************************************************************************/

ConfigTree::ConfigTree(TemplateTree* tt, bool verbose)
    : _template_tree(tt),
      _root_node(verbose),
      _current_node(&_root_node),
      _verbose(verbose)
{

}

ConfigTree::~ConfigTree()
{
    // XXX: _root_node will handle the deletion of all the tree nodes
}

ConfigTree&
ConfigTree::operator=(const ConfigTree& orig_tree)
{
    _root_node.clone_subtree(orig_tree.const_root_node());
    return *this;
}

bool
ConfigTree::parse(const string& configuration, const string& config_file,
		  string& errmsg)
{
    try {
	init_bootfile_parser(configuration.c_str(), config_file.c_str(), this);
	parse_bootfile();
	return true;
    } catch (const ParseError& pe) {
	errmsg = pe.why();
    }

    return false;
}

void ConfigTree::add_default_children()
{
    _root_node.recursive_add_default_children();
}

const TemplateTreeNode*
ConfigTree::find_template(const list<string>& path_segments) const
{
    const TemplateTreeNode *ttn;

    debug_msg("----------------------------------------------------------\n");
    debug_msg("looking for template for \"%s\"\n",
	      path_as_string(path_segments).c_str());

    ttn = _template_tree->find_node(path_segments);
    return ttn;

}

list<string>
ConfigTree::path_as_segments() const
{
    list<string> path_segments;
    ConfigTreeNode* ctn = _current_node;

    while (ctn->parent() != NULL) {
	path_segments.push_front(ctn->segname());
	ctn = ctn->parent();
    }
    return path_segments;
}

string
ConfigTree::current_path_as_string() const
{
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
ConfigTree::path_as_string(const list<string>& path_segments) const
{
    string path;
    list<string>::const_iterator iter;

    for (iter = path_segments.begin(); iter != path_segments.end(); ++iter) {
	if (path.empty())
	    path = *iter;
	else
	    path += " " + *iter;
    }
    return path;
}

void
ConfigTree::extend_path(const string& segment)
{
    _path_segments.push_back(segment);
}

void
ConfigTree::pop_path()
{
    size_t segments_to_pop = _segment_lengths.front();

    _segment_lengths.pop_front();
    for (size_t i = 0; i < segments_to_pop; i++) {
	_current_node = _current_node->parent();
    }
}

void
ConfigTree::push_path()
{
    string path = current_path_as_string();
    string nodename = _path_segments.back();

    //
    // Keep track of how many segments comprise this frame so we can
    // pop the right number later.
    //
    size_t len = _path_segments.size();
    _segment_lengths.push_front(len);

    list<string>::const_iterator iter;
    for (iter = _path_segments.begin(); iter != _path_segments.end(); ++iter) {
	add_node(*iter);
    }

    _path_segments.clear();
}

void
ConfigTree::add_node(const string& segment) throw (ParseError)
{
    list<ConfigTreeNode*>::const_iterator iter;
    ConfigTreeNode *found = NULL;

    iter = _current_node->children().begin();
    while (iter != _current_node->children().end()) {
	if ((*iter)->segname() == segment) {
	    if (found != NULL) {
		//
		// If there are two nodes with the same segment name,
		// we can only distinguish between them by type.
		// extend_path doesn't have the type information
		// available because it wasn't at the relevant point
		// in the template file, so this is an error.  The
		// correct way to step past such a node would be
		// through a call to add_node().
		//
		string err = "Need to qualify type of " + segment + "\n";
		xorp_throw(ParseError, err);
	    }
	    found = *iter;
	}
	++iter;
    }
    if (found != NULL) {
	_current_node = found;
    } else {
	list<string> path_segments = path_as_segments();
	path_segments.push_back(segment);
	const TemplateTreeNode* ttn = find_template(path_segments);
	if (ttn == NULL) {
	    booterror("No template found in template map");
	}

	string path = current_path_as_string();
	if (path.empty())
	    path = segment;
	else
	    path += " " + segment;
	found = new ConfigTreeNode(segment, path, ttn, _current_node,
				   /* user_id */ 0, _verbose);
	_current_node = found;
    }
}

void
ConfigTree::terminal_value(char* value, int type) throw (ParseError)
{
    string path(current_path_as_string());
    string svalue(value);
    ConfigTreeNode *ctn = _current_node;

    XLOG_ASSERT(ctn != NULL);

    // Special case for bool types to avoid needing to type "true"
    if (svalue == "" && (type == NODE_VOID)) {
	if (ctn->type() == NODE_BOOL) {
	    type = NODE_BOOL;
	    svalue = "true";
	}
    }
    if ((ctn->type() == NODE_TEXT) && (type == NODE_TEXT)) {
	svalue = unquote(svalue);
    } else if ((ctn->type() == NODE_TEXT) && (type != NODE_TEXT)) {
	// We'll accept anything as text
    } else if ((ctn->type() != NODE_TEXT) && (type == NODE_TEXT)) {
	//
	// The value was quoted in the bootfile.  We can't tell if
	// there's a mismatch without doing some secondary parsing.
	//
	svalue = unquote(svalue);

	switch (ctn->type()) {
	case NODE_VOID:
	    // Not clear what to do here
	    break;
	case NODE_UINT:
	    for (size_t i = 0; i < svalue.size(); i++) {
		if ((svalue[i] < '0') || (svalue[i] > '9')) {
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
		IPv4(svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_IPV4NET:
	    try {
		IPv4Net(svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_IPV6:
	    try {
		IPv6(svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_IPV6NET:
	    try {
		IPv6Net(svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	case NODE_MACADDR:
	    try {
		Mac(svalue.c_str());
	    } catch (InvalidString) {
		goto parse_error;
	    }
	    break;
	default:
	    // Did we forget to add a new type?
	    XLOG_FATAL("Unexpected type %d received", ctn->type());
	}
    } else if (ctn->type() != type) {
	string err = "\"" + path + "\" has type " + ctn->typestr() +
	    ", and value " + svalue + " is not a valid " + ctn->typestr();
	booterror(err.c_str());
    }
    ctn->set_value(svalue, /* userid */ 0);
    return;

 parse_error:
    string err = "\"" + path + "\" has type " + ctn->typestr() +
	", and value " + svalue + " is not a valid " + ctn->typestr();
    booterror(err.c_str());
}

const ConfigTreeNode*
ConfigTree::find_config_node(const list<string>& path_segments) const
{
    const ConfigTreeNode *found = &_root_node;
    const ConfigTreeNode *found2 = found;
    list<string>::const_iterator pi;
    list<ConfigTreeNode *>::const_iterator ci;

    for (pi = path_segments.begin(); pi != path_segments.end(); ++pi) {
	for (ci = found->const_children().begin();
	     ci != found->const_children().end();
	     ++ci) {
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
ConfigTree::show_subtree(const list<string>& path_segments) const
{
    const ConfigTreeNode *found = find_config_node(path_segments);

    if (found == NULL)
	return "ERROR";

    string s= found->show_subtree(/* depth */ 0, /* indent */ 0,
				  /* do_indent */ true, /* annotate */ true);
    return s;
}

string
ConfigTree::show_tree() const
{
    return _root_node.show_subtree(/* depth */ 0, /* indent */ 0,
				   /* do_indent */ true, /* annotate */ true);
}

string
ConfigTree::show_unannotated_tree() const
{
    return _root_node.show_subtree(/* depth */ 0, /* indent */ 0,
				   /* do_indent */ true, /* annotate */ false);
}

ConfigTreeNode*
ConfigTree::find_node(const list<string>& path)
{
    // Copy the path so we can modify it while searching...
    list<string> path_copy;
    list<string>::const_iterator iter;

    for (iter = path.begin(); iter != path.end(); ++iter) {
	path_copy.push_back(*iter);
    }
    return _root_node.find_node(path_copy);
}

ConfigTreeNode*
ConfigTree::find_config_module(const string& module_name)
{
    return _root_node.find_config_module(module_name);
}

string
ConfigTree::tree_str() const
{
    return _root_node.subtree_str();
}

bool
ConfigTree::apply_deltas(uid_t user_id, const string& deltas,
			 bool provisional_change, string& response)
{
    XLOG_TRACE(_verbose, "CT apply_deltas %d %s\n", user_id, deltas.c_str());

    ConfigTree delta_tree(_template_tree, _verbose);
    if (delta_tree.parse(deltas, "", response) == false)
	return false;

    debug_msg("Delta tree:\n");
    debug_msg("%s", delta_tree.tree_str().c_str());
    debug_msg("end delta tree.\n");

    response = "";
    return _root_node.merge_deltas(user_id, delta_tree.const_root_node(),
				   provisional_change, response);
}

bool
ConfigTree::apply_deletions(uid_t user_id, const string& deletions,
			    bool provisional_change, string& response)
{
    XLOG_TRACE(_verbose, "CT apply_deletions %d %s\n",
	       user_id, deletions.c_str());

    ConfigTree deletion_tree(_template_tree, _verbose);
    if (deletion_tree.parse(deletions, "", response) == false)
	return false;

    debug_msg("Deletion tree:\n");
    debug_msg("%s", deletion_tree.tree_str().c_str());
    debug_msg("end deletion tree.\n");

    response = "";
    return _root_node.merge_deletions(user_id,
				      deletion_tree.const_root_node(),
				      provisional_change, response);
}

void
ConfigTree::expand_varname_to_matchlist(const string& varname,
				      list<string>& matches) const
{
    // Trim $( and )
    string trimmed = varname.substr(2, varname.size()-3);

    // Split on dots
    list<string> sl = split(trimmed, '.');

    // Copy into a vector
    size_t len = sl.size();
    vector<string> v(len);
    for (size_t i = 0; i < len; i++) {
	v[i] = sl.front();
	sl.pop_front();
    }

    _root_node.expand_varname_to_matchlist(v, 0, matches);
}

void
ConfigTree::retain_different_nodes(const ConfigTree& them,
				   bool retain_changed_values)
{
    _root_node.retain_different_nodes(them.const_root_node(),
				      retain_changed_values);
}

void
ConfigTree::retain_common_nodes(const ConfigTree& them)
{
    _root_node.retain_common_nodes(them.const_root_node());
}
