// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/policy/visitor_setdep.cc,v 1.5 2005/08/04 15:26:56 bms Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "policy_module.h"
#include "visitor_setdep.hh"
#include <vector>
#include <string>

VisitorSetDep::VisitorSetDep(SetMap& setmap) : _setmap(setmap) 
{
}

const Element* 
VisitorSetDep::visit(PolicyStatement& policy)
{
    PolicyStatement::TermContainer& terms = policy.terms();

    PolicyStatement::TermContainer::iterator i;

    // go throgh all terms
    for(i = terms.begin(); i != terms.end(); ++i) {
        (i->second)->accept(*this);
    }	    
    return NULL;
}

const Element* 
VisitorSetDep::visit(Term& term)
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
VisitorSetDep::visit(NodeUn& node)
{
    // check arg
    node.node().accept(*this);
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeBin& node)
{
    // check args
    node.left().accept(*this);
    node.right().accept(*this);
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeAssign& node)
{
    // check arg
    node.rvalue().accept(*this);
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeVar& /* node */)
{
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeSet& node)
{
    // Only useful part of this visitor...
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
    
        throw sem_error(error.str());
    }
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeElem& /* node */)
{
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeAccept& /* node */)
{
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeReject& /* node */)
{
    return NULL;
}

const Element* 
VisitorSetDep::visit(NodeProto& /* node */)
{
    return NULL;
}
