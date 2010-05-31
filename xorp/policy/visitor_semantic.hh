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

// $XORP: xorp/policy/visitor_semantic.hh,v 1.17 2008/10/02 21:58:02 bms Exp $

#ifndef __POLICY_VISITOR_SEMANTIC_HH__
#define __POLICY_VISITOR_SEMANTIC_HH__

#include <boost/noncopyable.hpp>

#include "libxorp/xorp.h"

#include "policy/common/varrw.hh"
#include "policy/common/dispatcher.hh"

#include "visitor.hh"
#include "semantic_varrw.hh"
#include "set_map.hh"
#include "policy_statement.hh"
#include "node.hh"

/**
 * @short A policy semantic checker.
 *
 * A policy is instantiated by a protocol and policytype. Thus, semantic
 * checking must be performed realtive to the instantiation. [Generic semantic
 * checking may be accomplished too, but it is not done.]
 */
class VisitorSemantic :
    public boost::noncopyable,
    public Visitor
{
public:
    enum PolicyType {
	IMPORT,
	EXPORT
    };

    /**
     * @short Exception thrown on a semantic error
     */
    class sem_error : public PolicyException {
    public:
        sem_error(const char* file, size_t line, const string& init_why = "")
            : PolicyException("sem_error", file, line, init_why) {}
    };

    /**
     * @param varrw semantic VarRW used to simulate a protocol.
     * @param varmap the varmap.
     * @param setmap the SetMap to check if sets exist.
     * @param pmap the policy map to check subroutines.
     * @param protocol the protocol which instantiates the policy.
     * @param ptype the type of policy [import/export].
     */
    VisitorSemantic(SemanticVarRW& varrw, VarMap& varmap, SetMap& setmap,
		    PolicyMap& pmap, const string& protocol, PolicyType ptype);

    const Element* visit(PolicyStatement& policy);
    const Element* visit(Term& term);
    const Element* visit(NodeUn& node);
    const Element* visit(NodeBin& node);
    const Element* visit(NodeAssign& node);
    const Element* visit(NodeVar& node);
    const Element* visit(NodeSet& node);
    const Element* visit(NodeElem& node);
    const Element* visit(NodeAccept& node);
    const Element* visit(NodeReject& node);
    const Element* visit(NodeProto& node);
    const Element* visit(NodeNext& node);
    const Element* visit(NodeSubr& node);

    /**
     * @return sets used by the policy.
     *
     */
    const set<string>& sets() const {
	return _sets;
    }

private:
    void	    change_protocol(const string& proto);
    const string&   semantic_protocol();
    const Element*  do_bin(const Element& left, const Element& right,
			   const BinOper& op, const Node& from);
    void	    do_policy_statement(PolicyStatement& ps);

    SemanticVarRW&  _varrw;
    VarMap&	    _varmap;
    SetMap&	    _setmap;
    PolicyMap&	    _pmap;
    Dispatcher	    _disp;
    set<string>	    _sets;
    string	    _protocol;
    string	    _current_protocol;
    string	    _semantic_protocol;
    PolicyType	    _ptype;
    set<Element*>   _trash;
    bool	    _reject;
};

#endif // __POLICY_VISITOR_SEMANTIC_HH__
