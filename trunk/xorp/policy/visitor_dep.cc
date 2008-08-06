// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/policy/visitor_setdep.cc,v 1.10 2008/01/04 03:17:13 pavlin Exp $"

#include <vector>

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "visitor_dep.hh"

VisitorDep::VisitorDep(SetMap& setmap, PolicyMap& pmap) 
			    : _setmap(setmap), _pmap(pmap)
{
}

const Element* 
VisitorDep::visit(PolicyStatement& policy)
{
    PolicyStatement::TermContainer& terms = policy.terms();
    PolicyStatement::TermContainer::iterator i;

    // go throgh all terms
    for (i = terms.begin(); i != terms.end(); ++i) {
	(i->second)->accept(*this);
    }

    commit_deps(policy);

    return NULL;
}

const Element* 
VisitorDep::visit(Term& term)
{
    Term::Nodes& source = term.source_nodes();
    Term::Nodes& dest = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();

    Term::Nodes::iterator i;

    // do source block
    for(i = source.begin(); i != source.end(); ++i) {
        (i->second)->accept(*this);
    }

    // do dest block
    for(i = dest.begin(); i != dest.end(); ++i) {
        (i->second)->accept(*this);

    }

    // do action block
    for(i = actions.begin(); i != actions.end(); ++i) {
        (i->second)->accept(*this);
    }
    return NULL;
}

const Element* 
VisitorDep::visit(NodeSet& node)
{
    // see if set exists
    try {
	_setmap.getSet(node.setid());

	// track sets this policy uses
	_sets.insert(node.setid());
    } 
    // it doesn't
    catch(const PolicyException& e) {	
        ostringstream error;
        error << "Set not found: " << node.setid() << " at line " << node.line();
    
        xorp_throw(sem_error, error.str());
    }
    return NULL;
}

const Element*
VisitorDep::visit(NodeSubr& node)
{
    string policy = node.policy();

    if (!_pmap.exists(policy)) {
	ostringstream err;

	err << "Policy not found: " << policy << " at line " << node.line();

	xorp_throw(sem_error, err.str());
    }

    _policies.insert(policy);

    return NULL;
}

void
VisitorDep::commit_deps(PolicyStatement& policy)
{
    policy.set_dependency(_sets, _policies);
}

const Element* 
VisitorDep::visit(NodeUn& node)
{
    // check arg
    node.node().accept(*this);
    return NULL;
}

const Element* 
VisitorDep::visit(NodeBin& node)
{
    // check args
    node.left().accept(*this);
    node.right().accept(*this);
    return NULL;
}

const Element* 
VisitorDep::visit(NodeAssign& node)
{
    // check arg
    node.rvalue().accept(*this);
    return NULL;
}

const Element* 
VisitorDep::visit(NodeVar& /* node */)
{
    return NULL;
}

const Element* 
VisitorDep::visit(NodeElem& /* node */)
{
    return NULL;
}

const Element* 
VisitorDep::visit(NodeAccept& /* node */)
{
    return NULL;
}

const Element* 
VisitorDep::visit(NodeReject& /* node */)
{
    return NULL;
}

const Element* 
VisitorDep::visit(NodeProto& /* node */)
{
    return NULL;
}

const Element*
VisitorDep::visit(NodeNext& /* node */)
{
    return NULL;
}

const set<string>&
VisitorDep::sets() const
{
    return _sets;
}
