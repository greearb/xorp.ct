// vim:set sts=4 ts=8:

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

#ident "$XORP$"

#include "policy_module.h"
#include "config.h"

#include "term.hh"
#include "policy/common/policy_utils.hh"

using namespace policy_utils;

Term::Term(const string& name) : _name(name),
				 _source_nodes(NULL),
				 _dest_nodes(NULL), 
				 _action_nodes(NULL) {}


Term::~Term() {
    delete_vector(_source_nodes);
    delete_vector(_dest_nodes);
    delete_vector(_action_nodes);
}

void 
Term::set_source(const string& src) {

    // parse and check syntax error
    Nodes* nodes = _parser.parse(src);
    if(!nodes) {
	string err = _parser.last_error();
	
	throw term_syntax_error("Syntax error in term " + _name + 
				" source: " + err);
    }

    // replace configuration [successful parse]
    delete_vector(_source_nodes);

    _source = src;
    _source_nodes = nodes;
    
}

void 
Term::set_dest(const string& dst) {
    
    // parse and check syntax errors
    Nodes* nodes = _parser.parse(dst);
    if(!nodes) {
	string err = _parser.last_error();
        throw term_syntax_error("Syntax error in term " + _name + 
				" dest: " + err);
    }

    // replace conf
    delete_vector(_dest_nodes);

    _dest = dst;
    _dest_nodes = nodes;
}

void 
Term::set_action(const string& act) {

    // parse and error check
    Nodes* nodes = _parser.parse(act);
    if(!nodes) {
	throw term_syntax_error("Syntax error in term " + _name + 
				" action: " + _parser.last_error());
    }
    
    // replace conf
    _action = act;
    _action_nodes = nodes;
}


string 
Term::str() {
    ostringstream oss;

    oss << "term " << _name << " {" << endl;

    oss << "source {" << endl;
    oss << _source << "}" << endl;

    oss << "dest {" << endl;
    oss << _dest << "}" << endl;

    oss << "action {" << endl;
    oss << _action << "}" << endl;

    oss << "}" << endl;

    return oss.str();
}
