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

#include "source_match_code_generator.hh"

SourceMatchCodeGenerator::SourceMatchCodeGenerator(uint32_t tagstart) : 
				_currtag(tagstart) 
{
}


const Element* 
SourceMatchCodeGenerator::visit_policy(PolicyStatement& policy) {
    PolicyStatement::TermContainer& terms = policy.terms();

    PolicyStatement::TermContainer::iterator i;

    // go through all the terms
    for(i = terms.begin(); i != terms.end(); ++i) {
	// reset code and sets
	_os.str("");
	_code._sets.clear();

	(*i)->accept(*this);

	// term may be for a new target, so deal with that.
	addTerm(policy.name());
    }

    // mark the end for all policies
    for(CodeMap::iterator i = _codes.begin();
	i != _codes.end(); ++i) {

        Code* c = (*i).second;
        c->_code += "POLICY_END\n";

        _codes_vect.push_back(c);
    }

    return NULL;
}

void 
SourceMatchCodeGenerator::addTerm(const string& pname) {
    // copy the code for the term
    Code* term = new Code();

    term->_target.protocol = _protocol;
    term->_target.filter = filter::EXPORT_SOURCEMATCH;
    term->_sets = _code._sets;

    // see if we have code for this target already
    CodeMap::iterator i = _codes.find(_protocol);

    // if so link it
    if(i != _codes.end()) {
	Code* existing = (*i).second;

	// link "raw" code
	term->_code = _os.str();
	*existing += *term;

	delete term;
        return;
    }

    // code for a new target, need to create policy start header.
    term->_code = "POLICY_START " + pname + "\n" + _os.str();
    _codes[_protocol] = term;
}

const Element* 
SourceMatchCodeGenerator::visit_term(Term& term) {
    vector<Node*>& source = term.source_nodes();

    vector<Node*>::iterator i;

    _os << "TERM_START " << term.name() << endl ;

    _protocol = "";

    // generate code for source block
    for(i = source.begin(); i != source.end(); ++i) {
	(*i)->accept(*this);
    
        // if it was a protocol statement, no need for "ONFALSE_EXIT", if its
	// any other statement, then yes. The protocol is not read as a variable
	// by the backend filters... it is only used by the policy manager.
        if(_protocol == "")
	   _os << "ONFALSE_EXIT" << endl;
    }

    // XXX: we can assume _protocol = PROTOCOL IN EXPORT STATEMENT
    if(_protocol == "") 
        throw NoProtoSpec("No protocol specified in term " + term.name() + 
			  " in export policy source match");

    // ignore any destination block [that is dealt with in the export code
    // generator]


    // ignore actions too...


    // as an action, store policy tags...
    _os << "PUSH u32 " << _currtag << endl;
    _os << "LOAD policytags\n";
    _os << "+\n";
    _os << "STORE policytags\n";


    _os << "TERM_END\n";

    // FIXME: integer overflow
    _currtag++;

    return NULL;
}

const Element* 
SourceMatchCodeGenerator::visit_proto(NodeProto& node) {
    // check for protocol redifinition
    if(_protocol != "") {
	ostringstream err; 
        err << "PROTOCOL REDEFINED FROM " << _protocol << " TO " <<
	    node.proto() << " AT LINE " << node.line();
        throw ProtoRedefined(err.str());
    }

    // define protocol
    _protocol = node.proto();

    return NULL;

}

vector<Code*>& 
SourceMatchCodeGenerator::codes() { 
    return _codes_vect; 
}
