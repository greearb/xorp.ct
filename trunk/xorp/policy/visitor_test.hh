// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 International Computer Science Institute
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

// $XORP: xorp/policy/visitor_test.hh,v 1.1 2008/08/06 08:27:11 abittau Exp $

#ifndef __POLICY_VISITOR_TEST_HH__
#define __POLICY_VISITOR_TEST_HH__

#include "configuration.hh"

class VisitorTest : public Visitor {
public:
    VisitorTest(SetMap& sm, PolicyMap& pm, VarMap& vm,
		const RATTR& attr, RATTR& mods);
    ~VisitorTest();

    bool accepted();

    const Element* visit(NodeUn&);
    const Element* visit(NodeBin&);
    const Element* visit(NodeVar&);
    const Element* visit(NodeAssign&);
    const Element* visit(NodeSet&);
    const Element* visit(NodeAccept&);
    const Element* visit(NodeReject&);
    const Element* visit(Term&);
    const Element* visit(PolicyStatement&);
    const Element* visit(NodeElem&);
    const Element* visit(NodeProto&);
    const Element* visit(NodeNext&);
    const Element* visit(NodeSubr& node);

private:
    typedef set<Element*>	TRASH;
    typedef NodeNext::Flow	Flow;
    typedef VarRW::Id		Id;
    typedef VarMap::Variable	Variable;

    enum Outcome {
	DEFAULT,
	ACCEPT,
	REJECT
    };

    void	    trash_add(Element*);
    const Element*  do_policy_statement(PolicyStatement& ps);
    const Element*  do_bin(const Element& left, const Element& right,
			   const BinOper& op);
    const Element&  read(const string& id);
    void	    write(const string& id, const Element& e);
    void	    change_protocol(const string& protocol);
    Id		    var2id(const string& var);
    const Variable& var2variable(const string& var);
    bool	    match(const Element* e);

    SetMap&		_sm;
    PolicyMap&		_pm;
    VarMap&		_vm;
    bool		_finished;
    VarRW*		_varrw;
    Dispatcher		_disp;
    TRASH		_trash;
    Outcome		_outcome;
    Flow		_flow;
    string		_protocol;
    string		_current_protocol;
    RATTR&		_mod;
    ElementFactory	_ef;
};

#endif // __POLICY_VISITOR_TEST_HH__
