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
#include "visitor_printer.hh"

VisitorPrinter::VisitorPrinter(ostream& out) : _out(out)
{
}

const Element*
VisitorPrinter::visit(PolicyStatement& ps)
{
    PolicyStatement::TermContainer& terms = ps.terms();
    PolicyStatement::TermContainer::iterator i;
    // Work around bugs in uSTL
    const char* pss = "policy-statement ";
    const char* op = " {";
    const char* cp = "}";
    _out << pss << ps.name() << op << endl;
    // go throgh all terms
    for(i = terms.begin(); i != terms.end(); ++i) {
        (i->second)->accept(*this);
    }
    _out << cp << endl;

    return NULL;
}

const Element*
VisitorPrinter::visit(Term& term)
{
    Term::Nodes& source = term.source_nodes();
    Term::Nodes& dest = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();
    Term::Nodes::iterator i;

    // NOTE:  const char* casts are to work around bugs in uSTL.
    _out << (const char*)("\tterm ") << term.name() << (const char*)(" {") << endl;

    _out << (const char*)("\t\tfrom {") << endl;
    // do source block
    for (i = source.begin(); i != source.end(); ++i) {
	_out << (const char*)("\t\t\t");
        (i->second)->accept(*this);
	_out << (const char*)(";") << endl;
    }
    _out << (const char*)("\t\t}") << endl;

    _out << (const char*)("\t\tto {") << endl;
    // do dest block
    for (i = dest.begin(); i != dest.end(); ++i) {
	_out << (const char*)("\t\t\t");
        (i->second)->accept(*this);
	_out << (const char*)(";") << endl;
    }
    _out << (const char*)("\t\t}") << endl;

    _out << (const char*)("\t\tthen {") << endl;
    // do action block
    for (i = actions.begin(); i != actions.end(); ++i) {
	_out << (const char*)("\t\t\t");
        (i->second)->accept(*this);
	_out << (const char*)(";") << endl;
    }
    _out << (const char*)("\t\t}") << endl;

    _out << (const char*)("\t}") << endl;

    return NULL;
}

const Element*
VisitorPrinter::visit(NodeUn& node) 
{
    // const char* cast works around uSTL bug.
    _out << node.op().str() << (const char*)(" ");
    node.node().accept(*this);
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeBin& node) 
{
    node.left().accept(*this);
    // const char* cast works around uSTL bug.
    _out << (const char*)(" ") << node.op().str() << (const char*)(" ");
    node.right().accept(*this);
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeAssign& node) 
{
    _out << node.varid() << (const char*)(" ");

    if (node.mod()) 
	_out << node.mod()->str();

    _out << (const char*)("= ");

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
    _out << (const char*)("accept");
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeReject& /*node */)
{
    _out << (const char*)("reject");
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeProto& node) 
{
    _out << (const char*)("protocol ") << node.proto();
    return NULL;
}

const Element*
VisitorPrinter::visit(NodeNext& node)
{
    _out << (const char*)("next ");

    switch (node.flow()) {
    case NodeNext::POLICY:
	_out << (const char*)("policy ");
	break;

    case NodeNext::TERM:
	_out << (const char*)("term ");
	break;
    }

    return NULL;
}

const Element*
VisitorPrinter::visit(NodeSubr& node)
{
    _out << (const char*)("policy ") << node.policy();

    return NULL;
}
