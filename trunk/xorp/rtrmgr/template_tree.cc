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



#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include "conf_tree.hh"
#include "conf_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"

extern int init_template_parser(const char* filename, TemplateTree* c);
extern void complete_template_parser();
extern void parse_template() throw (ParseError);

TemplateTree::TemplateTree(const string& xorp_root_dir,
			   bool verbose) throw (InitError)
    : _xorp_root_dir(xorp_root_dir),
      _verbose(verbose)
{
}

TemplateTree::~TemplateTree()
{
    XLOG_ASSERT(_root_node != NULL);

    delete _root_node;
}

bool 
TemplateTree::load_template_tree(const string& config_template_dir,
				 string& error_msg)
{
    list<string> files;

    _root_node = new TemplateTreeNode(*this, NULL, "", "");
    _current_node = _root_node;

    struct stat dirdata;
    if (stat(config_template_dir.c_str(), &dirdata) < 0) {
	error_msg = c_format("Error reading config directory %s: %s",
			     config_template_dir.c_str(), strerror(errno));
	return false;
    }
    if ((dirdata.st_mode & S_IFDIR) == 0) {
	error_msg = c_format("Error reading config directory %s: "
			     "not a directory",
			     config_template_dir.c_str());
	return false;
    }

    // TODO: file suffix is hardcoded here!
    string globname = config_template_dir + "/*.tp";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	globfree(&pglob);
	error_msg = c_format("Failed to find config files in %s",
			     config_template_dir.c_str());
	return false;
    }

    if (pglob.gl_pathc == 0) {
	globfree(&pglob);
	error_msg = c_format("Failed to find any template files in %s",
			     config_template_dir.c_str());
	return false;
    }

    for (size_t i = 0; i < (size_t)pglob.gl_pathc; i++) {
	debug_msg("Loading template file %s\n", pglob.gl_pathv[i]);
	if (! parse_file(string(pglob.gl_pathv[i]),
			 config_template_dir, error_msg)) {
	    globfree(&pglob);
	    return false;
	}
    }

    globfree(&pglob);

    // Expand and verify the template tree
    if (expand_template_tree(error_msg) != true)
	return (false);
    if (check_template_tree(error_msg) != true)
	return (false);

    return true;
}

bool 
TemplateTree::parse_file(const string& filename,
			 const string& config_template_dir, string& error_msg) 
{
    if (init_template_parser(filename.c_str(), this) < 0) {
	complete_template_parser();
	error_msg = c_format("Failed to open template file: %s",
			     config_template_dir.c_str());
	return false;
    }
    try {
	parse_template();
    } catch (const ParseError& pe) {
	complete_template_parser();
	error_msg = pe.why();
	return false;
    }
    if (_path_segments.size() != 0) {
	complete_template_parser();
	error_msg = c_format("File %s is not terminated properly", 
			     filename.c_str());
	return false;
    }
    complete_template_parser();
    return true;
}

bool
TemplateTree::expand_template_tree(string& error_msg)
{
    // Expand the template tree
    return root_node()->expand_template_tree(error_msg);
}

bool
TemplateTree::check_template_tree(string& error_msg)
{
    // Verify the template tree
    return root_node()->check_template_tree(error_msg);
}

string
TemplateTree::tree_str() const
{
    return _root_node->subtree_str();
}

void
TemplateTree::extend_path(const string& segment, bool is_tag)
{
    _path_segments.push_back(PathSegment(segment, is_tag));
}

void
TemplateTree::pop_path() throw (ParseError)
{
    if (_segment_lengths.empty()) {
	xorp_throw(ParseError, "Mismatched braces");
    }

    size_t segments_to_pop = _segment_lengths.front();

    _segment_lengths.pop_front();
    for (size_t i = 0; i < segments_to_pop; i++) {
	_current_node = _current_node->parent();
    }
}

string
TemplateTree::path_as_string()
{
    string path;

    list<PathSegment>::iterator iter;
    for (iter = _path_segments.begin(); iter != _path_segments.end(); ++iter) {
	PathSegment& path_segment = *iter;
	if (path == "") {
	    path = path_segment.segname();
	} else {
	    path += " " + path_segment.segname();
	}
    }
    return path;
}

TemplateTreeNode*
TemplateTree::new_node(TemplateTreeNode* parent,
		       const string& path, const string& varname,
		       int type, const string& initializer)
{
    TemplateTreeNode* ttn;

    switch (type) {
    case NODE_VOID:
	ttn = new TemplateTreeNode(*this, parent, path, varname);
	break;
    case NODE_TEXT:
	ttn = new TextTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_UINT:
	ttn = new UIntTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_UINTRANGE:
	ttn = new UIntRangeTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_INT:
	ttn = new IntTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_BOOL:
	ttn = new BoolTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV4:
	ttn = new IPv4Template(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV4NET:
	ttn = new IPv4NetTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV4RANGE:
	ttn = new IPv4RangeTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV6:
	ttn = new IPv6Template(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV6NET:
	ttn = new IPv6NetTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV6RANGE:
	ttn = new IPv6RangeTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_MACADDR:
	ttn = new MacaddrTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_URL_FILE:
	ttn = new UrlFileTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_URL_FTP:
	ttn = new UrlFtpTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_URL_HTTP:
	ttn = new UrlHttpTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_URL_TFTP:
	ttn = new UrlTftpTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_ARITH:
	ttn = new ArithTemplate(*this, parent, path, varname, initializer);
	break;
    default:
	XLOG_UNREACHABLE();
    }
    return ttn;
}

void
TemplateTree::push_path(int type, char* cinit)
{
    list<PathSegment>::const_iterator iter;
    iter = _path_segments.begin();
    size_t len = _path_segments.size();

    if (len > 0) {
	for (size_t i = 0; i < len - 1; i++) {
	    // Add all except the last segment
	    add_untyped_node(iter->segname(), iter->is_tag());
	    ++iter;
	}
    }
    add_node(iter->segname(), type, cinit);

    _segment_lengths.push_front(len);

    while (_path_segments.size() > 0)
	_path_segments.pop_front();
}

void
TemplateTree::add_untyped_node(const string& segment, bool is_tag)
    throw (ParseError)
{
    TemplateTreeNode* found = NULL;

    list<TemplateTreeNode*>::const_iterator iter;
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
		// through a call to add_node .
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
	found = new TemplateTreeNode(*this, _current_node, segment, "");
	if (is_tag)
	    found->set_tag();
	_current_node = found;
    }
}

void
TemplateTree::add_node(const string& segment, int type, char* cinit)
{
    string varname = _path_segments.back().segname();

    if (varname == "@") {
	if (_current_node->is_tag()) {
	    // Parent node is a tag
	    varname = _current_node->segname() + "." + "@";
	} else {
	    // What happened here?
	    XLOG_UNREACHABLE();
	}
    }
    if (varname.empty() || varname[0] != '$') {
	// This can't be a variable.
	varname = "";
    }

    // Keep track of how many segments comprise this frame so we can
    // pop the right number later.

    string initializer;
    if (cinit != NULL)
	initializer = cinit;

    TemplateTreeNode* found = NULL;
    list<TemplateTreeNode*>::const_iterator iter;
    for (iter = _current_node->children().begin();
	 iter != _current_node->children().end();
	 ++iter) {
	TemplateTreeNode* ttn = *iter;
	if (ttn->segname() == segment) {
	    if ((ttn->type() == type)
		|| (type == NODE_VOID) || (ttn->type() == NODE_VOID)) {
		if (found != NULL) {
		    // I don't think this can happen
		    XLOG_UNREACHABLE();
		}
		found = ttn;
	    }
	}
    }
    if (found != NULL) {
	_current_node = found;
    } else {
	found = new_node(_current_node, segment, varname, type, initializer);
	_current_node = found;
    }
}

const TemplateTreeNode*
TemplateTree::find_node(const list<string>& path_segments) const
{
    TemplateTreeNode* ttn = _root_node;

    list<string>::const_iterator iter;
    for (iter = path_segments.begin(); iter != path_segments.end(); ++iter) {
	const string& segname = *iter;
	list<TemplateTreeNode*> matches;
	list<TemplateTreeNode*>::const_iterator ti;

	// First look for an exact name match
	for (ti = ttn->children().begin(); ti != ttn->children().end(); ++ti) {
	    if ((*ti)->segname() == segname) {
		matches.push_back(*ti);
	    }
	}
	if (matches.size() == 1) {
	    ttn = matches.front();
	    continue;
	}
	if (matches.size() > 1) {
	    // This shouldn't be possible
	    XLOG_ERROR("Multiple match at node %s\n", segname.c_str());
	    XLOG_UNREACHABLE();
	}

	// There's no exact name match, so we're probably looking for a
	// match of an encoded typestr or a value against a typed variable.
	for (ti = ttn->children().begin(); ti != ttn->children().end(); ++ti) {
	    TemplateTreeNode* t = *ti;
	    if (t->type() == NODE_VOID)
		continue;
	    if ((t->parent() == NULL) || (! t->parent()->is_tag()))
		continue;
	    if (t->encoded_typestr() == segname) {
		matches.push_back(t);
		continue;
	    }
	    string s;
	    if (t->type_match(segname, s))
		matches.push_back(t);
	}
	if (matches.size() == 0)
	    return NULL;
	if (matches.size() > 1) {
	    string err = "Ambiguous value for node " + segname +
		" - I can't tell which type this is.\n";
	    debug_msg("%s", err.c_str());
	    return NULL;
	}
	ttn = matches.front();
    }
    return ttn;
}

const TemplateTreeNode*
TemplateTree::find_node_by_type(const list<ConfPathSegment>& path_segments) 
    const
{
    TemplateTreeNode* ttn = _root_node;

    list<ConfPathSegment>::const_iterator iter;
    for (iter = path_segments.begin(); iter != path_segments.end(); ++iter) {
	const string& segname = iter->segname();
	int type = iter->type();
	list<TemplateTreeNode*> matches;
	list<TemplateTreeNode*>::const_iterator ti;

	// First look for an exact name match
	for (ti = ttn->children().begin(); ti != ttn->children().end(); ++ti) {
	    if ((*ti)->segname() == segname) {
		matches.push_back(*ti);
	    }
	}
	if (matches.size() == 1) {
	    ttn = matches.front();
	    continue;
	}
	if (matches.size() > 1) {
	    // This shouldn't be possible
	    XLOG_ERROR("Multiple match at node %s\n", segname.c_str());
	    XLOG_UNREACHABLE();
	}

	//
	// There's no exact name match, so we're probably looking for a
	// match of a value against a typed variable.
	//
	list<TemplateTreeNode*> matches_text_type;
	for (ti = ttn->children().begin(); ti != ttn->children().end(); ++ti) {
	    TemplateTreeNode* t = *ti;
	    if (t->type() == NODE_VOID)
		continue;
	    if ((t->parent() == NULL) || (! t->parent()->is_tag()))
		continue;
	    if (t->type() == type) {
		matches.push_back(t);
		continue;
	    }
	    //
	    // XXX: the type check failed.
	    // If there is a matching template node type of type NODE_TEXT,
	    // then we accept this node.
	    // 
	    // The upside of this is that we can use a single template
	    // node like "foo @: txt" that can be used with, say,
	    // IPv4 or IPv6 addresses, a host name, or any other text string.
	    // The downside is that we lose the strict type checking
	    // when our template tree contains such nodes.
	    //
	    if (t->type() == NODE_TEXT)
		matches_text_type.push_back(t);
	}
	if (matches.size() == 0)
	    matches = matches_text_type;
	if (matches.size() == 0)
	    return NULL;
	if (matches.size() > 1) {
	    string err = "Ambiguous value for node " + segname +
		" - I can't tell which type this is.\n";
	    debug_msg("%s", err.c_str());
	    return NULL;
	}
	ttn = matches.front();
    }
    return ttn;
}

void
TemplateTree::add_cmd(char* cmd) throw (ParseError)
{
    _current_node->add_cmd(string(cmd));
}

void
TemplateTree::add_cmd_action(const string& cmd, const list<string>& action)
    throw (ParseError)
{
    _current_node->add_action(cmd, action);
}

void
TemplateTree::register_module(const string& name, ModuleCommand* mc)
{
    _registered_modules[name] = mc;
}

ModuleCommand*
TemplateTree::find_module(const string& name)
{
    map<string, ModuleCommand*>::const_iterator rpair;
    rpair = _registered_modules.find(name);
    if (rpair == _registered_modules.end())
	return NULL;
    else
	return rpair->second;
}

bool
TemplateTree::check_variable_name(const string& s) const
{
    // Trim $( and )
    string trimmed = s.substr(2, s.size() - 3);

    // Split on dots
    list<string> sl = split(trimmed, '.');

    // Copy into a vector
    size_t len = sl.size();
    vector<string> v(len);
    for (size_t i = 0; i < len; i++) {
	v[i] = sl.front();
	sl.pop_front();
    }

    return _root_node->check_variable_name(v, 0);
}
