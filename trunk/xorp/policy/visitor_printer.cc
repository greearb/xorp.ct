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

#ident "$XORP: xorp/policy/visitor_printer.cc,v 1.10 2008/08/06 08:18:31 abittau Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "visitor_printer.hh"

VisitorPrinter::VisitorPrinter(ostream& out) : _out(out)
{
}

const Element*
VisitorPrinter::visit(PolicyStatement& ps)
{
    PolicyStatement::TermContainer& terms = ps.terms();
    PolicyStatement::TermContainer::iterator i;

    _out << "policy-statement " << ps.name() << " {" << endl;
    // go throgh all terms
    for(i = terms.begin(); i != terms.end(); ++i) {
        (i->second)->accept(*this);
    }
    _out << "}" << endl;

    return NULL;
}

const Element*
VisitorPrinter::visit(Term& term)
{
    Term::Nodes& source = term.source_nodes();
    Term::Nodes& dest = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();
    Term::Nodes::iterator i;

    _out << "term " << term.name() << " {" << endl;

    _out << "source {" << endl;
    // do source block
    for(i = source.begin(); i != source.end(); ++i) {
        (i->second)->accept(*this);
	_out << ";" << endl;
    }
    _out << "}" << endl;

    _out << "dest {" << endl;
    // do dest block
    for(i = dest.begin(); i != dest.end(); ++i) {
        (i->second)->accept(*this);
	_out << ";" << endl;
    }
    _out << "}" << endl;

    _out << "action {" << endl;
    // do action block
    for(i = actions.begin(); i != actions.end(); ++i) {
        (i->second)->accept(*this);
	_out << ";" << endl;
    }
    _out << "}" << endl;

    return NULL;
}

const Element*
VisitorPrinter::visit(NodeUn& node) 
{
    _out << node.op().str() << " ";
    node.node().accept(*this);
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeBin& node) 
{
    node.left().accept(*this);
    _out << " " << node.op().str() << " ";
    node.right().accept(*this);
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeAssign& node) 
{
    _out << node.varid() << " ";

    if (node.mod()) 
	_out << node.mod()->str();

    _out << "= ";

    node.rvalue().accept(*this);

    return NULL;
}

const Element*
VisitorPrinter::visit(NodeVar& node) 
{
    _out << node.val();
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeSet& node) 
{
    _out << node.setid();
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeElem& node) 
{
    _out << node.val().str();
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeAccept& /* node */) 
{
    _out << "accept";
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeReject& /*node */)
{
    _out << "reject";
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeProto& node) 
{
    _out << "protocol " << node.proto();
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeNext& node)
{
    _out << "next ";

    switch (node.flow()) {
    case NodeNext::POLICY:
	_out << "policy ";
	break;
    }

    return NULL;
}

const Element*
VisitorPrinter::visit(NodeSubr& node)
{
    _out << "policy " << node.policy();

    return NULL;
}
