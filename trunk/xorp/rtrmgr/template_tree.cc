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

#ident "$XORP: xorp/rtrmgr/template_tree.cc,v 1.2 2003/03/10 23:21:02 hodson Exp $"

#include <glob.h>
#include "rtrmgr_module.h"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "conf_tree.hh"
#include "conf_tree_node.hh"
#include "split.hh"

//#define DEBUG_TEMPLATE_PARSER

extern int init_template_parser(const char* , TemplateTree* c);
extern int parse_template();
extern void tplterror(const char* s);

TemplateTree::TemplateTree(const string& templatedirname, 
			   const string& xrldirname) 
    : _xrldb(xrldirname)
{
    _root = new TemplateTreeNode(NULL, "", "");
    _current_node = _root;

    list <string> files;

    struct stat dirdata;
    if (stat(templatedirname.c_str(), &dirdata) < 0) {
	string errstr = "rtrmgr: error reading config directory "
	    + templatedirname + "\n";
	errstr += strerror(errno); 
	errstr += "\n";
	fprintf(stderr, "%s", errstr.c_str());
	exit(1);
    }

    if ((dirdata.st_mode & S_IFDIR) == 0) {
	string errstr = "rtrmgr: error reading config directory "
	    + templatedirname + "\n" + templatedirname + " is not a directory\n";
	fprintf(stderr, "%s", errstr.c_str());
	exit(1);
    }
    
    string globname = templatedirname + "/*.tp";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	fprintf(stderr, "rtrmgr failed to find config files in %s\n",
		templatedirname.c_str());
	globfree(&pglob);
	exit(1);
    }

    if (pglob.gl_pathc == 0) {
	fprintf(stderr, "rtrmgr failed to find any template files in %s\n", 
		templatedirname.c_str());
	globfree(&pglob);
	exit(1);
    }
    
    for (size_t i=0; i< (size_t)pglob.gl_pathc; i++) {
	printf("Loading template file %s\n", pglob.gl_pathv[i]);
	if (init_template_parser(pglob.gl_pathv[i], this) < 0) {
	    fprintf(stderr, "Failed to open template file: %s\n", 
		    templatedirname.c_str());
	    globfree(&pglob);
	    exit(-1);
	}
	try {
	    parse_template();
	} catch (ParseError &pe) {
	    tplterror(pe.err.c_str());
	    globfree(&pglob);
	    exit(1);
	}
	if (_path_segs.size() != 0) {
	    fprintf(stderr, "Error: file %s is not terminated properly\n", 
		    pglob.gl_pathv[i]);
	    globfree(&pglob);
	    exit(1);
	}
    }
    
    globfree(&pglob);
}

TemplateTree::~TemplateTree() {
    delete _root;
}

void 
TemplateTree::display_tree() {
    _root->print();
}

void TemplateTree::extend_path(string segment, bool is_tag) {
#ifdef DEBUG_TEMPLATE_PARSER
    printf("extend_path %s\n", segment.c_str());
#endif
    _path_segs.push_back(PathSegment(segment,is_tag));
}

void 
TemplateTree::pop_path() {
    if (_seg_lengths.size()==0) {
	throw ParseError("Mismatched braces");
    }
    int segs_to_pop = _seg_lengths.front();
#ifdef DEBUG_TEMPLATE_PARSER
    printf("pop_path: %d\n", segs_to_pop);
#endif
    _seg_lengths.pop_front();
    for(int i =0; i<segs_to_pop; i++) {
	_current_node = _current_node->parent();
    }
#ifdef DEBUG_TEMPLATE_PARSER
    printf("popped to %s\n", _current_node->path().c_str());
#endif
}

string
TemplateTree::path_as_string() {
    string path;
    typedef list<PathSegment>::iterator CI;
    CI ptr = _path_segs.begin();
    while (ptr != _path_segs.end()) {
	if (path == "") {
	    path = ptr->segname();
	} else {
	    path += " " + ptr->segname();
	}
	++ptr;
    }
    return path;
}

TemplateTreeNode* 
TemplateTree::new_node(TemplateTreeNode* parent,
		       const string& path, const string& varname,
		       int type, const string& initializer) {
    TemplateTreeNode* t;
    switch (type) {
    case NODE_VOID:
	t = new TemplateTreeNode(parent, path, varname);
	break;
    case NODE_TEXT:
	t = new TextTemplate(parent, path, varname, initializer);
	break;
    case NODE_UINT:
	t = new UIntTemplate(parent, path, varname, initializer);
	break;
    case NODE_INT:
	t = new IntTemplate(parent, path, varname, initializer);
	break;
    case NODE_BOOL:
	t = new BoolTemplate(parent, path, varname, initializer);
	break;
    case NODE_IPV4:
	t = new IPv4Template(parent, path, varname, initializer);
	break;
    case NODE_IPV4PREFIX:
	t = new IPv4NetTemplate(parent, path, varname, initializer);
	break;
    case NODE_IPV6:
	t = new IPv6Template(parent, path, varname, initializer);
	break;
    case NODE_IPV6PREFIX:
	t = new IPv6NetTemplate(parent, path, varname, initializer);
	break;
    case NODE_MACADDR:
	t = new MacaddrTemplate(parent, path, varname, initializer);
	break;
    default:
	abort();
    }
    return t;
}

void 
TemplateTree::push_path(int type, char* cinit) {
#ifdef DEBUG_TEMPLATE_PARSER
    printf("push_path\n");
#endif
    list <PathSegment>::const_iterator iter;
    iter = _path_segs.begin();
    int len = _path_segs.size();
    for (int i = 0; i< len-1; i++) {
	//add all except the last segment
	add_untyped_node(iter->segname(), iter->is_tag());
	iter++;
    }
    add_node(iter->segname(), type, cinit);

    _seg_lengths.push_front(len);

    while (_path_segs.size()>0)
	_path_segs.pop_front();
}

void 
TemplateTree::add_untyped_node(string segment, bool is_tag) {
#ifdef DEBUG_TEMPLATE_PARSER
    printf("add_untyped_node: segment=%s\n", segment.c_str());
#endif
    list <TemplateTreeNode*>::const_iterator i;
    i = _current_node->children().begin();
    TemplateTreeNode* found = NULL;
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
		throw ParseError(err);
	    }
	    found = *i;
	}
	i++;
    }
    if (found != NULL) {
	_current_node = found;
    } else {
	found = new TemplateTreeNode(_current_node, segment, "");
	if (is_tag)
	    found->set_tag();
	_current_node = found;
    }
}

void 
TemplateTree::add_node(const string& segment, int type, char* cinit) {
#ifdef DEBUG_TEMPLATE_PARSER
    printf("add_node: segment=%s type: %d\n", segment.c_str(), type);
    printf("cn=%p\n", _current_node);
#endif
    string varname = _path_segs.back().segname();
    if (varname == "@") {
	if (_current_node->is_tag()) {
	    //parent node is a tag
	    varname = _current_node->segname() + "." + "@";
	} else {
	    //what happened here?
	    abort();
	}
    }
    if (varname.empty() || varname[0]!='$') {
	//this can't be a variable.
	varname = "";
    }

    //keep track of how many segments comprise this frame so we can
    //pop the right number later

    string initializer;
    if (cinit != NULL)
	initializer = cinit;

    list <TemplateTreeNode*>::const_iterator i;
    i = _current_node->children().begin();
    TemplateTreeNode* found = NULL;
    while (i!= _current_node->children().end()) {
	if ((*i)->segname() == segment) {
	    if (((*i)->type() == type)
		|| (type == NODE_VOID) || ((*i)->type() == NODE_VOID)) {
		if (found != NULL) {
		    //I don't think this can happen
		    abort();
		}
		found = *i;
	    }
	}
	i++;
    }
    if (found != NULL) {
	_current_node = found;
#ifdef DEBUG_TEMPLATE_PARSER
	printf("add_node: found %s type: %d\n", found->path().c_str(), type);
#endif
    } else {
	found = new_node(_current_node, segment, varname, type, initializer);
#ifdef DEBUG_TEMPLATE_PARSER
	printf("add_node: %s type: %d\n", found->path().c_str(), type);
#endif
	_current_node = found;
    }
}

TemplateTreeNode* 
TemplateTree::find_node(const list<string>& segs) {
    TemplateTreeNode* ttn = _root;
    list <string>::const_iterator i;
    for (i=segs.begin(); i!= segs.end(); ++i) {
	list <TemplateTreeNode*> matches;
	list <TemplateTreeNode*>::const_iterator ti;

	//first look for an exact name match
	ti = ttn->children().begin();
	while (ti != ttn->children().end()) {
	    if ((*ti)->segname() == *i) {
		matches.push_back(*ti);
#ifdef DEBUG_TEMPLATE_PARSER
		printf("matched: %s type %d\n", (*ti)->path().c_str(), 
		       (*ti)->type());
#endif
	    }
	    ++ti;
	}
	if (matches.size()==1) {
	    ttn = matches.front();
	    continue;
	}
	if (matches.size()>1) {
	    //this shouldn't be possible
	    fprintf(stderr, "Multiple match at node %s\n", (*i).c_str());
	    abort();
	}
	
	//there's no exact name match, so we're probably looking for a
	//match of a value against a typed variable
	ti = ttn->children().begin();
	while (ti != ttn->children().end()) {
	    if ((*ti)->type() != NODE_VOID) {
		if ((*ti)->type_match(*i))
		    matches.push_back(*ti);
	    }
	    ++ti;
	}
	if (matches.size()==0)
	    return NULL;
	if (matches.size()>1) {
	    string err = "Ambiguous value for node " + (*i) + 
		" - I can't tell which type this is.\n";
	    throw ParseError(err);
	}
	ttn = matches.front();
#ifdef DEBUG_TEMPLATE_PARSER
	printf("matched: %s type: %d\n", ttn->path().c_str(), ttn->type());
#endif
    }
    return ttn;
}


void
TemplateTree::add_cmd(char* cmd) {
    _current_node->add_cmd(string(cmd), *this);
}

void 
TemplateTree::add_cmd_action(string cmd, const list<string>& action) {
    _current_node->add_action(cmd, action, _xrldb);
}

void 
TemplateTree::register_module(const string& name, ModuleCommand* mc) {
    _registered_modules[name] = mc;
}
 
ModuleCommand* 
TemplateTree::find_module(const string& name) {
    typedef map<string, ModuleCommand*>::const_iterator CI;
    CI rpair = _registered_modules.find(name);
    if (rpair == _registered_modules.end())
	return NULL;
    else 
	return rpair->second;
}

bool 
TemplateTree::check_variable_name(const string& s) const {
    //trim $( and )
    string trimmed = s.substr(2, s.size()-3);

    //split on dots
    list <string> sl = split(trimmed, '.');

    //copy into a vector
    int len = sl.size();
    vector<string> v(len);
    for (int i = 0; i < len; i++) {
	v[i] = sl.front();
	sl.pop_front();
    }
    
    return _root->check_variable_name(v, 0);
}
