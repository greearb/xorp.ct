// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "policy_module.h"
#include "libxorp/xorp.h"

#include "source_match_code_generator.hh"

SourceMatchCodeGenerator::SourceMatchCodeGenerator(uint32_t tagstart,
						   const VarMap& varmap,
						   PolicyMap& pmap,
					map<string, set<uint32_t> > & ptags) 
	   : CodeGenerator(varmap, pmap), _currtag(tagstart),
	     _protocol_tags(ptags)
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

    // Maybe we got called from subr.  If so, we're not a protocol statement
    // (maybe one of our internal statements was though).  Reset it.
    _protocol_statement = false;

    if (_subr)
	return NULL;

    // mark the end for all policies
    for (CodeMap::iterator i = _codes.begin(); i != _codes.end(); ++i) {
        Code* c = (*i).second;
        c->add_code("POLICY_END\n");

	// for all subroutines too
	for (SUBR::const_iterator j = c->subr().begin();
	     j != c->subr().end();) {
	    string x = j->second;

	    x += "POLICY_END\n";

	    string p = j->first;
	    j++;
	    c->add_subr(p, x);
	}

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
	string s = _os.str();

	if (_subr) {
	    SUBR::const_iterator j = existing->subr().find(_policy);
	    XLOG_ASSERT(j != existing->subr().end());

	    term->add_subr(_policy, (j->second) + s);
	} else
	    term->set_code(s);

	*existing += *term;

	delete term;
        return;
    }

    XLOG_ASSERT(!_policy.empty());

    // code for a new target, need to create policy start header.
    string s = "POLICY_START " + _policy + "\n" + _os.str();

    if (_subr)
	term->add_subr(_policy, s);
    else
	term->set_code(s);

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

    // If subroutine, do actions.  Tags are set by caller.
    if (_subr) {
	for (Term::Nodes::iterator i = term.action_nodes().begin();
	     i != term.action_nodes().end(); ++i) {

	    Node* n = i->second;

	    n->accept(*this);
	}

	return;
    }

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
    _os << "LOAD " << (int)(VarRW::VAR_POLICYTAGS) << "\n";
    _os << "<=\n";
    _os << "ONFALSE_EXIT" << endl;

    // Add another tag to the route
    _os << "PUSH u32 " << _currtag << endl;
    _os << "LOAD " << (int)(VarRW::VAR_POLICYTAGS) << "\n";
    _os << "+\n";
    _os << "STORE " << (int)(VarRW::VAR_POLICYTAGS) << "\n";


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
