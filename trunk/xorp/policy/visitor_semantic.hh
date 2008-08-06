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

// $XORP: xorp/policy/visitor_semantic.hh,v 1.15 2008/08/06 08:22:19 abittau Exp $

#ifndef __POLICY_VISITOR_SEMANTIC_HH__
#define __POLICY_VISITOR_SEMANTIC_HH__

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
class VisitorSemantic : public Visitor {
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

    // not impl
    VisitorSemantic(const VisitorSemantic&);
    VisitorSemantic& operator=(const VisitorSemantic&);
};

#endif // __POLICY_VISITOR_SEMANTIC_HH__
