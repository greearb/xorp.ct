// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/visitor_test.hh,v 1.3 2008/08/06 08:30:57 abittau Exp $

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
