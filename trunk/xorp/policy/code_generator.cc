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

#include "code_generator.hh"    
    
CodeGenerator::CodeGenerator() {
}

CodeGenerator::CodeGenerator(const string& proto, 
			     const filter::Filter& filter) {
    _code._target.protocol = proto;
    _code._target.filter = filter;
}


// constructor for import policies
CodeGenerator::CodeGenerator(const string& proto) {
    _code._target.protocol = proto;
    _code._target.filter = filter::IMPORT;
}


CodeGenerator::~CodeGenerator() {
}

const Element* 
CodeGenerator::visit_policy(PolicyStatement& policy) {
    _os << "POLICY_START " << policy.name() << endl;

    PolicyStatement::TermContainer& terms = policy.terms();

    // go through all the terms
    for(PolicyStatement::TermContainer::iterator i = terms.begin(); 
	i != terms.end(); ++i) {
	
	(*i)->accept(*this);
    }	    

    _os << "POLICY_END\n";

    _code._code = _os.str();
    return NULL;
}

const Element* 
CodeGenerator::visit_term(Term& term) {
    vector<Node*>& source = term.source_nodes();
    vector<Node*>& dest = term.dest_nodes();
    vector<Node*>& actions = term.action_nodes();

    vector<Node*>::iterator i;

    _os << "TERM_START " << term.name() << endl ;

    // do the source block
    for(i = source.begin(); i != source.end(); ++i) {
	(*i)->accept(*this);
        _os << "ONFALSE_EXIT" << endl;
    }

    // Import policies should not have a dest block
    if(!dest.empty()) {
	throw CodeGeneratorErr("Term " + term.name() + " has a dest part!");
    }

    // do the action block
    for(i = actions.begin(); i != actions.end(); ++i) {
        (*i)->accept(*this);
    }

    _os << "TERM_END\n";
    return NULL;

}
    


const Element* 
CodeGenerator::visit(NodeUn& node) {
    node.node().accept(*this);

    _os << node.op().str() << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeBin& node) {
    // reverse order, so they can be popped in correct order
    node.right().accept(*this);
    node.left().accept(*this);

    _os << node.op().str() << endl;
    return NULL;
}


const Element* 
CodeGenerator::visit(NodeAssign& node) {
    node.rvalue().accept(*this);

    _os << "STORE " << node.varid() << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeRegex& node) {
    node.arg().accept(*this);

    _os << "REGEX " << node.reg() << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeElem& node) {
    _os << "PUSH " << node.val().type() << " " << 
	"\"" << node.val().str() << "\"" << endl;
    return NULL;	
}


const Element* 
CodeGenerator::visit(NodeVar& node) {
    _os << "LOAD " << node.val() << endl;
    return NULL;
}


const Element* 
CodeGenerator::visit(NodeSet& node) {
    _os << "PUSH_SET " << node.setid() << endl;
    _code._sets.insert(node.setid());
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeAccept& /* node */) {
    _os << "ACCEPT" << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeReject& /* node */) {
    _os << "REJECT" << endl;
    return NULL;
}


const Element* 
CodeGenerator::visit_proto(NodeProto& node) {
    ostringstream err;

    // import policies may not have protocol set.
    err << "INVALID protocol statement in line " << node.line() << endl;
    throw CodeGeneratorErr(err.str());
}

const Code&
CodeGenerator::code() { 
    return _code; 
}

const Element*
CodeGenerator::visit(PolicyStatement& ps) {
    return visit_policy(ps);
}

const Element*
CodeGenerator::visit(Term& term) {
    return visit_term(term);
}

const Element*
CodeGenerator::visit(NodeProto& proto) {
    return visit_proto(proto);
}
