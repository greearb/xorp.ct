// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/policy/visitor_printer.cc,v 1.14 2008/10/02 21:58:01 bms Exp $"

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

    _out << "\tterm " << term.name() << " {" << endl;

    _out << "\t\tfrom {" << endl;
    // do source block
    for (i = source.begin(); i != source.end(); ++i) {
	_out << "\t\t\t";
        (i->second)->accept(*this);
	_out << ";" << endl;
    }
    _out << "\t\t}" << endl;

    _out << "\t\tto {" << endl;
    // do dest block
    for (i = dest.begin(); i != dest.end(); ++i) {
	_out << "\t\t\t";
        (i->second)->accept(*this);
	_out << ";" << endl;
    }
    _out << "\t\t}" << endl;

    _out << "\t\tthen {" << endl;
    // do action block
    for (i = actions.begin(); i != actions.end(); ++i) {
	_out << "\t\t\t";
        (i->second)->accept(*this);
	_out << ";" << endl;
    }
    _out << "\t\t}" << endl;

    _out << "\t}" << endl;

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

    case NodeNext::TERM:
	_out << "term ";
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
