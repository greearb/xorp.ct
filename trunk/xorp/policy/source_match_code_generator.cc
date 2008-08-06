// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/source_match_code_generator.cc,v 1.17 2008/07/23 05:11:21 pavlin Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"

#include "source_match_code_generator.hh"

SourceMatchCodeGenerator::SourceMatchCodeGenerator(uint32_t tagstart,
				const VarMap& varmap) : 
				 CodeGenerator(varmap),
				 _currtag(tagstart) 
{
}

const Element* 
SourceMatchCodeGenerator::visit_policy(PolicyStatement& policy)
{
    PolicyStatement::TermContainer& terms = policy.terms();

    _policy = policy.name();

    // go through all the terms
    for (PolicyStatement::TermContainer::iterator i = terms.begin();
         i != terms.end(); ++i) {
	Term* term = i->second;
	term->accept(*this);
    }

    // mark the end for all policies
    for (CodeMap::iterator i = _codes.begin(); i != _codes.end(); ++i) {
        Code* c = (*i).second;
        c->add_code("POLICY_END\n");

        _codes_vect.push_back(c);
    }

    return NULL;
}

void 
SourceMatchCodeGenerator::addTerm()
{
    // copy the code for the term
    Code* term = new Code();

    term->set_target_protocol(_protocol);
    term->set_target_filter(filter::EXPORT_SOURCEMATCH);
    term->set_referenced_set_names(_code.referenced_set_names());

    // see if we have code for this target already
    CodeMap::iterator i = _codes.find(_protocol);

    // if so link it
    if (i != _codes.end()) {
	Code* existing = (*i).second;

	// link "raw" code
	term->set_code(_os.str());
	*existing += *term;

	delete term;
        return;
    }

    XLOG_ASSERT(!_policy.empty());

    // code for a new target, need to create policy start header.
    term->set_code("POLICY_START " + _policy + "\n" + _os.str());
    _codes[_protocol] = term;
}

const Element*
SourceMatchCodeGenerator::visit_term(Term& term)
{
    // reset code and sets
    _os.str("");
    _code.clear_referenced_set_names();

    // make sure the source of the term has something [non empty source]
    if (term.source_nodes().size()) {
	do_term(term);

	// term may be for a new target, so deal with that.
	addTerm();
    } else
	_tags.push_back(Taginfo(false, _currtag));

    return NULL;
}

void
SourceMatchCodeGenerator::do_term(Term& term)
{
    Term::Nodes& source = term.source_nodes();

    Term::Nodes::iterator i;

    _os << "TERM_START " << term.name() << endl ;

    _protocol = "";

    //
    // XXX: Generate first the source block for the protocol statement,
    // because the protocol value is needed by the processing of other
    // statements.
    //
    for(i = source.begin(); i != source.end(); ++i) {
	if ((i->second)->is_protocol_statement()) {
	    (i->second)->accept(*this);
	    term.set_from_protocol(_protocol);
	}
    }

    //
    // Generate the remaining code for the source block
    //
    for(i = source.begin(); i != source.end(); ++i) {
	if ((i->second)->is_protocol_statement()) {
	    // XXX: the protocol statement was processes above
	    continue;
	}

	_protocol_statement = false;
	(i->second)->accept(*this);
    
        // if it was a protocol statement, no need for "ONFALSE_EXIT", if its
	// any other statement, then yes. The protocol is not read as a variable
	// by the backend filters... it is only used by the policy manager.
        if(!_protocol_statement)
	   _os << "ONFALSE_EXIT" << endl;
    }

    // XXX: we can assume _protocol = PROTOCOL IN EXPORT STATEMENT
    if(_protocol == "") 
        xorp_throw(NoProtoSpec, "No protocol specified in term " + term.name() + 
		                " in export policy source match");

    // ignore any destination block [that is dealt with in the export code
    // generator]


    // ignore actions too...


    //
    // As an action, store policy tags...
    // XXX: Note that we store the policy tags only if the route
    // doesn't carry some other protocol's tags.
    //
    _tags.push_back(Taginfo(true, _currtag));
    _protocol_tags[_protocol].insert(_currtag);

    // Create the set of the tags known (so far) to belong to this protocol
    ElemSetU32 element_set;
    const set<uint32_t>& protocol_tags = _protocol_tags[_protocol];
    for (set<uint32_t>::const_iterator iter = protocol_tags.begin();
	 iter != protocol_tags.end();
	 ++iter) {
	ElemU32 e(*iter);
	element_set.insert(e);
    }

    // Check that the route's tags are subset of this protocol's tags
    _os << "PUSH set_u32 " << element_set.str() << endl;
    _os << "LOAD " << VarRW::VAR_POLICYTAGS << "\n";
    _os << "<=\n";
    _os << "ONFALSE_EXIT" << endl;

    // Add another tag to the route
    _os << "PUSH u32 " << _currtag << endl;
    _os << "LOAD " << VarRW::VAR_POLICYTAGS << "\n";
    _os << "+\n";
    _os << "STORE " << VarRW::VAR_POLICYTAGS << "\n";


    _os << "TERM_END\n";

    // FIXME: integer overflow
    _currtag++;
}

const Element* 
SourceMatchCodeGenerator::visit_proto(NodeProto& node)
{
    // check for protocol redifinition
    if(_protocol != "") {
	ostringstream err; 
        err << "PROTOCOL REDEFINED FROM " << _protocol << " TO " <<
	    node.proto() << " AT LINE " << node.line();
        xorp_throw(ProtoRedefined, err.str());
    }

    // define protocol
    _protocol = node.proto();
    _protocol_statement = true;
    return NULL;
}

vector<Code*>& 
SourceMatchCodeGenerator::codes()
{ 
    return _codes_vect; 
}

const SourceMatchCodeGenerator::Tags&
SourceMatchCodeGenerator::tags() const
{
    return _tags;
}

uint32_t
SourceMatchCodeGenerator::next_tag() const
{
    return _currtag;
}

const string&
SourceMatchCodeGenerator::protocol()
{
    return _protocol;
}
