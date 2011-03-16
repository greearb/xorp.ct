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

// $XORP: xorp/policy/visitor_dep.hh,v 1.3 2008/10/02 21:58:01 bms Exp $

#ifndef __POLICY_VISITOR_DEP_HH__
#define __POLICY_VISITOR_DEP_HH__



#include "policy/common/policy_exception.hh"
#include "visitor.hh"
#include "set_map.hh"
#include "policy_map.hh"
#include "policy_statement.hh"
#include "node.hh"

/**
 * @short This visitor is used to check what sets a policy uses.
 *
 * This is useful for set dependancy tracking.
 */
class VisitorDep : public Visitor {
public:
    /**
     * @short Semantic error thrown if set is not found.
     */
    class sem_error : public PolicyException {
    public:
        sem_error(const char* file, size_t line, const string& init_why = "")   
            : PolicyException("sem_error", file, line, init_why) {} 
    };

    /**
     * @param setmap The setmap used.
     */
    VisitorDep(SetMap& setmap, PolicyMap& pmap);

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
     * @return the sets used by the policy.
     */
    const DEPS& sets() const;

private:
    void commit_deps(PolicyStatement& policy);

    SetMap&		_setmap;
    PolicyMap&		_pmap;
    DEPS		_sets;
    DEPS		_policies;
};

#endif // __POLICY_VISITOR_DEP_HH__
