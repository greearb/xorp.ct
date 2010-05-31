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



#include "policy_module.h"
#include "libxorp/xorp.h"
#include "visitor_test.hh"
#include "test_varrw.hh"

// XXX duplication with semantic visitor.  Factor out common functionality and
// inherit.

VisitorTest::VisitorTest(SetMap& sm, PolicyMap& pm, VarMap& vm,
			 const RATTR& attr, RATTR& mod) 
	 : _sm(sm), _pm(pm), _vm(vm), _finished(false), _varrw(NULL), _mod(mod)
{
    TestVarRW* varrw = new TestVarRW();
    _varrw = varrw;

    RATTR::const_iterator i = attr.find("protocol");
    if (i != attr.end())
	_protocol = i->second;

    change_protocol(_protocol);

    // init varrw with route attributes
    for (i = attr.begin(); i != attr.end(); ++i) {
	string name = i->first;

	if (name.compare("protocol") == 0)
	    continue;

	const VarMap::Variable& v = var2variable(name);

	Element* e = _ef.create(v.type, (i->second).c_str());
	trash_add(e);
	varrw->write(v.id, *e);
    }
}

VisitorTest::~VisitorTest()
{
    delete _varrw;

    for (TRASH::iterator i = _trash.begin(); i != _trash.end(); ++i)
	delete (*i);

    _trash.clear();
}

const Element*
VisitorTest::visit(PolicyStatement& ps)
{
    do_policy_statement(ps);

    return NULL;
}

const Element*
VisitorTest::do_policy_statement(PolicyStatement& ps)
{
    PolicyStatement::TermContainer& terms = ps.terms();

    _outcome = DEFAULT;

    // go throgh all terms
    for (PolicyStatement::TermContainer::iterator i = terms.begin();
         i != terms.end(); ++i) {
	(i->second)->accept(*this);

	if (_outcome != DEFAULT)
	    break;

	if (_finished) {
	    switch (_flow) {
	    case NodeNext::POLICY:
		return NULL;

	    case NodeNext::TERM:
		continue;
	    }
	}
    }

    return NULL;
}

const Element*
VisitorTest::visit(Term& term)
{
    Term::Nodes& source  = term.source_nodes();
    Term::Nodes& dest    = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();
    Term::Nodes::iterator i;

    _finished = false;
    _flow     = NodeNext::TERM;

    change_protocol(_protocol);

    // do source block
    for (i = source.begin(); i != source.end(); ++i) {
        const Element* e = (i->second)->accept(*this);

	if (_finished)
	    return NULL;

	if (!match(e))
	    return NULL;
    }

    change_protocol(_protocol);

    // do dest block
    for (i = dest.begin(); i != dest.end(); ++i) {
        const Element* e = (i->second)->accept(*this);

	if (_finished)
	    return NULL;

	if (!match(e))
	    return NULL;
    }

    // do action block
    for (i = actions.begin(); i != actions.end(); ++i) {
        (i->second)->accept(*this);

	if (_finished)
	    return NULL;
    }

    return NULL;
}

const Element*
VisitorTest::visit(NodeUn& node) 
{
    const Element* arg = node.node().accept(*this);

    Element* res = _disp.run(node.op(), *arg);
    trash_add(res);

    return res;
}

const Element*
VisitorTest::visit(NodeBin& node) 
{
    const Element* left  = node.left().accept(*this);
    const Element* right = node.right().accept(*this);

    return do_bin(*left, *right, node.op());
}

const Element*
VisitorTest::do_bin(const Element& left, const Element& right,
		    const BinOper& op)
{
    Element* res = _disp.run(op, left, right);
    trash_add(res);

    return res;
}

const Element*
VisitorTest::visit(NodeAssign& node) 
{
    const Element* rvalue = node.rvalue().accept(*this);

    if (node.mod()) {
	const Element& left = read(node.varid());

	rvalue = do_bin(left, *rvalue, *node.mod());
    }

    write(node.varid(), *rvalue);

    return NULL;
}

const Element*
VisitorTest::visit(NodeVar& node) 
{
    const Element& e = read(node.val());

    return &e;
}

const Element*
VisitorTest::visit(NodeSet& node) 
{
    const Element& e = _sm.getSet(node.setid());

    return &e;
}

const Element*
VisitorTest::visit(NodeElem& node) 
{
    const Element& e = node.val();

    return &e;
}

const Element*
VisitorTest::visit(NodeAccept& /* node */) 
{
    _outcome = ACCEPT;
    _finished = true;

    return NULL;
}

const Element*
VisitorTest::visit(NodeReject& /*node */)
{
    _outcome = REJECT;
    _finished = true;

    return NULL;
}

const Element*
VisitorTest::visit(NodeProto& node) 
{
    change_protocol(node.proto());

    return NULL;
}

const Element*
VisitorTest::visit(NodeNext& node)
{
    _flow = node.flow();
    _finished = true;

    return NULL;
}

const Element*
VisitorTest::visit(NodeSubr& node)
{
    PolicyStatement& policy = _pm.find(node.policy());

    bool finished   = _finished;
    Outcome outcome = _outcome;
    Flow flow       = _flow;

    do_policy_statement(policy);

    Element* e = new ElemBool(_outcome == REJECT ? false : true);

    _finished = finished;
    _outcome  = outcome;
    _flow     = flow;

    return e;
}

void
VisitorTest::trash_add(Element* e)
{
    if (e->refcount() == 1)
	_trash.insert(e);
}

bool
VisitorTest::accepted()
{
    return _outcome != REJECT;
}

void
VisitorTest::change_protocol(const string& protocol)
{
    _current_protocol = protocol;
}

const Element&
VisitorTest::read(const string& id)
{
    try {
	Id i = var2id(id);

	const Element& e = _varrw->read(i);

	return e;
    } catch (const PolicyException& e) {
	ostringstream oss;

	oss << "Can't read uninitialized attribute " << id;

	xorp_throw(PolicyException, oss.str());
    }
}

void
VisitorTest::write(const string& id, const Element& e)
{
    const Variable& v = var2variable(id);

    // XXX perhaps we should do a semantic check before a test run...
    if (!v.writable())
	xorp_throw(PolicyException, "writing a read-only variable");

    if (v.type != e.type())
	xorp_throw(PolicyException, "type mismatch on write");

    _varrw->write(v.id, e);

    _mod[id] = e.str();
}

VisitorTest::Id
VisitorTest::var2id(const string& var)
{
    const Variable& v = var2variable(var);

    return v.id;
}

const VisitorTest::Variable&
VisitorTest::var2variable(const string& var)
{
    string protocol = _current_protocol;

    // Always allow reading prefix.
    // XXX we could code this better...
    if (protocol.empty()) {
	if (var.compare("network4") == 0 || var.compare("network6") == 0)
	    protocol = "bgp";
    }

    if (protocol.empty())
	xorp_throw(PolicyException, "Provide a protocol name");

    Id id = _vm.var2id(protocol, var);

    return _vm.variable(protocol, id);
}

bool
VisitorTest::match(const Element* e)
{
    if (!e)
	return true;

    const ElemBool* b = dynamic_cast<const ElemBool*>(e);
    XLOG_ASSERT(b);

    return b->val();
}
