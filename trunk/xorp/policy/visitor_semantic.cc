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

#include "visitor_semantic.hh"

#include <sstream>

VisitorSemantic::VisitorSemantic(SemanticVarRW& varrw, 
				 SetMap& setmap,
				 const string& protocol, 
				 PolicyType ptype) : 
		
		_varrw(varrw), _setmap(setmap), 
		_protocol(protocol), _ptype(ptype) 

{
}


    
const Element* 
VisitorSemantic::visit(PolicyStatement& policy) {
    PolicyStatement::TermContainer& terms = policy.terms();

    PolicyStatement::TermContainer::iterator i;

    // go through all terms
    for(i = terms.begin(); i != terms.end(); ++i) {
	(*i)->accept(*this);
    }

    // helps for garbage gollection in varrw
    _varrw.sync();
    return NULL;
}

const Element* 
VisitorSemantic::visit(Term& term) {
    vector<Node*>& source = term.source_nodes();
    vector<Node*>& dest = term.dest_nodes();
    vector<Node*>& actions = term.action_nodes();

    vector<Node*>::iterator i;

    _current_protocol = "";
    // assume import policy, so set protocol to whatever protocol instantiated
    // the policy
    _varrw.set_protocol(_protocol);

    // go through the source block
    for(i = source.begin(); i != source.end(); ++i) {
        (*i)->accept(*this);
    }

    // if it was an export policy maybe varrw was switched to some other
    // protocol during source match, so replace it with original protocol.
    _varrw.set_protocol(_protocol);


    // if it is an export policy, a source protocol must be specified [XXX: need
    // to fix this]
    if(_ptype == EXPORT && _current_protocol == "") {
	string err = "No protocol specified in source match of export policy";

	err += " in term: " + term.name();

	throw sem_error(err);
    }

    // import policies should not have dest blocks
    if(_ptype == IMPORT && !(dest.empty())) {
	throw sem_error("Invalid use of dest in import policy in term " + 
		       term.name());
    }

    // check dest block
    for(i = dest.begin(); i != dest.end(); ++i) {
        (*i)->accept(*this);

    }

    // check actions
    for(i = actions.begin(); i != actions.end(); ++i) {
        (*i)->accept(*this);
    }
    return NULL;
}
    


const Element* 
VisitorSemantic::visit(NodeUn& node) {
    // check argument
    const Element* arg = node.node().accept(*this);

    Element* res;
   
    // see if we may execute unary operation
    try {
	res = _disp.run(node.op(),*arg);
	_trash.insert(res);
	return res;
    } 
    // we can't
    catch(const PolicyException& e) {
	ostringstream error;

	error << "Invalid unop " << e.str() << " at line " << node.line();
	throw sem_error(error.str());
    }
}


const Element* 
VisitorSemantic::visit(NodeBin& node) {
    // check arguments
    const Element* left = node.left().accept(*this);
    const Element* right = node.right().accept(*this);

    Element* res;
    // see if we may execute bin operation.
    try {
	res = _disp.run(node.op(),*left,*right);
	_trash.insert(res);
	return res;
    } 
    // nope
    catch(const PolicyException& e) {
        ostringstream error;

        error << "Invalid binop " << e.str() << " at line " << node.line();
    
        throw sem_error(error.str());
    }
}

    
const Element* 
VisitorSemantic::visit(NodeAssign& node) {
    // check argument
    const Element* rvalue = node.rvalue().accept(*this);

    // try assignment
    try {
    _varrw.write(node.varid(),*rvalue);
    } catch(SemanticVarRW::var_error e) {
        ostringstream error;

        error << e.str() << " at line " << node.line();

        throw sem_error(error.str());
    }
    return NULL;
}


const Element* 
VisitorSemantic::visit(NodeVar& node) {
    // try reading a variable
    try {
    return &_varrw.read(node.val());
    } catch(SemanticVarRW::var_error e) {
        ostringstream error;

        error << e.str() << " at line " << node.line();
    
        throw sem_error(error.str());
    }
}
    
const Element* 
VisitorSemantic::visit(NodeSet& node) {
    // try getting a set [setdep should have caught there errors]
    try {
	const Element& e = _setmap.getSet(node.setid());
	_sets.insert(node.setid());
	return &e;
    } catch(const PolicyException& e) {
        ostringstream error;
        error << "Set not found: " << node.setid() << " at line " << node.line();
    
        throw sem_error(error.str());
    }
}

const Element* 
VisitorSemantic::visit(NodeElem& node) {
    return &node.val();
}

const Element* 
VisitorSemantic::visit(NodeRegex& node) {
    // check arg
    node.arg().accept(*this);

    regex_t reg;

    // try compiling the regex
    if(regcomp(&reg,node.reg().c_str(),REG_EXTENDED)) {
	regfree(&reg);
        ostringstream err;

        err << "INVALID REGEX " << node.reg() << " at line " << node.line();

        throw sem_error(err.str());
    }
    regfree(&reg);

    // just return a dummy value
    // XXX: no dispatch, as any element may be compared to a regex.
    Element* e = new ElemBool(true);
    _trash.insert(e);
    return e;
}

const Element* 
VisitorSemantic::visit(NodeAccept& /* node */) {
    return NULL;
}

const Element* 
VisitorSemantic::visit(NodeReject& /* node */) {
    return NULL;
}

const Element* 
VisitorSemantic::visit(NodeProto& node) {
    ostringstream err;

    // import policies may not use protocol directive
    if(_ptype == IMPORT) {
	err << "May not define protocol for import policy at line " <<
        node.line();
        
        throw sem_error(err.str());
    }

    string proto = node.proto();
    

    // check for redifinition in same term.
    if(_current_protocol != "") {
        err << "Redifinition of protocol from " << _current_protocol << 
	    " to " << proto << " at line " << node.line();
    
        throw sem_error(err.str());
    }

    // do the switch
    _current_protocol = proto;
    // make the varrw emulate the new protocol
    _varrw.set_protocol(_current_protocol);
    return NULL;
}
