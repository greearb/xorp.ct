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

#ident "$XORP: xorp/rtrmgr/template_tree.cc,v 1.19 2004/05/22 06:09:07 atanu Exp $"

#include <glob.h>
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "conf_tree.hh"
#include "conf_tree_node.hh"
#include "util.hh"


extern int init_template_parser(const char* filename, TemplateTree* c);
extern void parse_template() throw (ParseError);

TemplateTree::TemplateTree(const string& xorp_root_dir,
			   const string& config_template_dir,
			   const string& xrl_targets_dir,
			   bool verbose) throw (InitError)
    : _xrldb(xrl_targets_dir, verbose),
      _xorp_root_dir(xorp_root_dir),
      _verbose(verbose)
{
    string errmsg;
    list<string> files;

    _root_node = new TemplateTreeNode(*this, NULL, "", "");
    _current_node = _root_node;

    struct stat dirdata;
    if (stat(config_template_dir.c_str(), &dirdata) < 0) {
	errmsg = c_format("Error reading config directory %s: %s",
			  config_template_dir.c_str(), strerror(errno));
	xorp_throw(InitError, errmsg);
    }

    if ((dirdata.st_mode & S_IFDIR) == 0) {
	errmsg = c_format("Error reading config directory %s: not a directory",
			  config_template_dir.c_str());
	xorp_throw(InitError, errmsg);
    }

    // TODO: file suffix is hardcoded here!
    string globname = config_template_dir + "/*.tp";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find config files in %s",
			  config_template_dir.c_str());
	xorp_throw(InitError, errmsg);
    }

    if (pglob.gl_pathc == 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find any template files in %s",
			  config_template_dir.c_str());
	xorp_throw(InitError, errmsg);
    }

    for (size_t i = 0; i < (size_t)pglob.gl_pathc; i++) {
	debug_msg("Loading template file %s\n", pglob.gl_pathv[i]);
	if (init_template_parser(pglob.gl_pathv[i], this) < 0) {
	    globfree(&pglob);
	    errmsg = c_format("Failed to open template file: %s",
			      config_template_dir.c_str());
	    xorp_throw(InitError, errmsg);
	}
	try {
	    parse_template();
	} catch (const ParseError& pe) {
	    globfree(&pglob);
	    xorp_throw(InitError, pe.why());
	}
	if (_path_segments.size() != 0) {
	    globfree(&pglob);
	    errmsg = c_format("File %s is not terminated properly",
			      pglob.gl_pathv[i]);
	    xorp_throw(InitError, errmsg);
	}
    }

    globfree(&pglob);

    // Verify the template tree
    if (_root_node->check_template_tree(errmsg) != true) {
	xorp_throw(InitError, errmsg.c_str());
    }
}

TemplateTree::~TemplateTree()
{
    XLOG_ASSERT(_root_node != NULL);

    delete _root_node;
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
    case NODE_IPV6:
	ttn = new IPv6Template(*this, parent, path, varname, initializer);
	break;
    case NODE_IPV6NET:
	ttn = new IPv6NetTemplate(*this, parent, path, varname, initializer);
	break;
    case NODE_MACADDR:
	ttn = new MacaddrTemplate(*this, parent, path, varname, initializer);
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
	list<TemplateTreeNode*> matches;
	list<TemplateTreeNode*>::const_iterator ti;

	// First look for an exact name match
	for (ti = ttn->children().begin(); ti != ttn->children().end(); ++ti) {
	    if ((*ti)->segname() == *iter) {
		matches.push_back(*ti);
	    }
	}
	if (matches.size() == 1) {
	    ttn = matches.front();
	    continue;
	}
	if (matches.size() > 1) {
	    // This shouldn't be possible
	    XLOG_ERROR("Multiple match at node %s\n", (*iter).c_str());
	    XLOG_UNREACHABLE();
	}

	// There's no exact name match, so we're probably looking for a
	// match of a value against a typed variable.
	for (ti = ttn->children().begin(); ti != ttn->children().end(); ++ti) {
	    if ((*ti)->type() != NODE_VOID) {
		if ((*ti)->type_match(*iter))
		    matches.push_back(*ti);
	    }
	}
	if (matches.size() == 0)
	    return NULL;
	if (matches.size() > 1) {
	    string err = "Ambiguous value for node " + (*iter) +
		" - I can't tell which type this is.\n";
	    debug_msg("%s", err.c_str());
	    return NULL;
	}
	ttn = matches.front();
    }
    return ttn;
}

void
TemplateTree::add_cmd(char* cmd)
{
    _current_node->add_cmd(string(cmd), *this);
}

void
TemplateTree::add_cmd_action(const string& cmd, const list<string>& action)
{
    _current_node->add_action(cmd, action, _xrldb);
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
