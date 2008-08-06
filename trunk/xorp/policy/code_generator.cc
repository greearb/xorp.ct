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

#ident "$XORP: xorp/policy/code_generator.cc,v 1.15 2008/07/23 05:11:18 pavlin Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "code_generator.hh"    

CodeGenerator::CodeGenerator(const VarMap& varmap) : _varmap(varmap)
{
}

CodeGenerator::CodeGenerator(const string& proto, 
			     const filter::Filter& filter,
			     const VarMap& varmap) : _varmap(varmap)
{
    _protocol = proto;
    _code.set_target_protocol(proto);
    _code.set_target_filter(filter);
}

// constructor for import policies
CodeGenerator::CodeGenerator(const string& proto, const VarMap& varmap) : 
				_varmap(varmap)
{
    _protocol = proto;
    _code.set_target_protocol(proto);
    _code.set_target_filter(filter::IMPORT);
}

CodeGenerator::~CodeGenerator()
{
}

const Element* 
CodeGenerator::visit_policy(PolicyStatement& policy)
{
    PolicyStatement::TermContainer& terms = policy.terms();

    // go through all the terms
    for (PolicyStatement::TermContainer::iterator i = terms.begin(); 
	 i != terms.end(); ++i) {
	
	(i->second)->accept(*this);
    }	    

    ostringstream oss;

    oss << "POLICY_START " << policy.name() << endl;
    oss << _os.str();
    oss << "POLICY_END" << endl;

    _code.set_code(oss.str());

    return NULL;
}

const Element* 
CodeGenerator::visit_term(Term& term)
{
    Term::Nodes& source = term.source_nodes();
    Term::Nodes& dest = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();

    Term::Nodes::iterator i;

    _os << "TERM_START " << term.name() << endl ;

    // do the source block
    for(i = source.begin(); i != source.end(); ++i) {
	(i->second)->accept(*this);
        _os << "ONFALSE_EXIT" << endl;
    }

    // Import policies should not have a dest block
    if(!dest.empty()) {
	xorp_throw(CodeGeneratorErr, "Term " + term.name() + " has a dest part!");
    }

    //
    // Do the action block.
    // XXX: We generate last the code for the "accept" or "reject" statements.
    //
    for(i = actions.begin(); i != actions.end(); ++i) {
	if ((i->second)->is_accept_or_reject())
	    continue;
        (i->second)->accept(*this);
    }
    for(i = actions.begin(); i != actions.end(); ++i) {
	if ((i->second)->is_accept_or_reject())
	    (i->second)->accept(*this);
    }

    _os << "TERM_END\n";
    return NULL;

}
    
const Element* 
CodeGenerator::visit(NodeUn& node)
{
    node.node().accept(*this);

    _os << node.op().str() << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeBin& node)
{
    // reverse order, so they can be popped in correct order
    node.right().accept(*this);
    node.left().accept(*this);

    _os << node.op().str() << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeAssign& node)
{
    node.rvalue().accept(*this);

    VarRW::Id id = _varmap.var2id(protocol(), node.varid());

    _os << "STORE " << id << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeElem& node)
{
    _os << "PUSH " << node.val().type() << " " << 
	"\"" << node.val().str() << "\"" << endl;
    return NULL;	
}

const Element* 
CodeGenerator::visit(NodeVar& node)
{
    VarRW::Id id = _varmap.var2id(protocol(), node.val());

    _os << "LOAD " << id << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeSet& node)
{
    _os << "PUSH_SET " << node.setid() << endl;
    _code.add_referenced_set_name(node.setid());
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeAccept& /* node */)
{
    _os << "ACCEPT" << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit(NodeReject& /* node */)
{
    _os << "REJECT" << endl;
    return NULL;
}

const Element* 
CodeGenerator::visit_proto(NodeProto& node)
{
    ostringstream err;

    // import policies may not have protocol set.
    err << "INVALID protocol statement in line " << node.line() << endl;
    xorp_throw(CodeGeneratorErr, err.str());
}

const Code&
CodeGenerator::code()
{ 
    return _code; 
}

const Element*
CodeGenerator::visit(PolicyStatement& ps)
{
    return visit_policy(ps);
}

const Element*
CodeGenerator::visit(Term& term)
{
    return visit_term(term);
}

const Element*
CodeGenerator::visit(NodeProto& proto)
{
    return visit_proto(proto);
}

const string&
CodeGenerator::protocol()
{
    return _protocol;
}

const Element*
CodeGenerator::visit(NodeNext& next)
{
    _os << "NEXT ";

    switch (next.flow()) {
    case NodeNext::POLICY:
	_os << "POLICY";
	break;
    }

    _os << endl;

    return NULL;
}
