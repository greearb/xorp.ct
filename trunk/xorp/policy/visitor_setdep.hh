// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_VISITOR_SETDEP_HH__
#define __POLICY_VISITOR_SETDEP_HH__

#include "policy/common/policy_exception.hh"

#include "visitor.hh"
#include "set_map.hh"
#include "policy_statement.hh"

#include "node.hh"

#include <string>

/**
 * @short This visitor is used to check what sets a policy uses.
 *
 * This is useful for set dependancy tracking.
 */
class VisitorSetDep : public Visitor {
public:
    /**
     * @short Semantic error thrown if set is not found.
     */
    class sem_error : public PolicyException {
    public:
	sem_error(const string& err) : PolicyException(err) {}

    };

    /**
     * @param setmap The setmap used.
     */
    VisitorSetDep(SetMap& setmap);


    
    const Element* visit(PolicyStatement& policy);

    const Element* visit(Term& term);

    const Element* visit(NodeUn& node);
    const Element* visit(NodeBin& node);
    
    const Element* visit(NodeAssign& node);
    
    const Element* visit(NodeVar& node);
    
    const Element* visit(NodeSet& node);

    const Element* visit(NodeElem& node);

    const Element* visit(NodeRegex& node);

    const Element* visit(NodeAccept& node);
    
    const Element* visit(NodeReject& node);
    
    const Element* visit(NodeProto& node);

    /**
     * @return the sets used by the policy.
     */
    const set<string>& sets() const {
	return _sets;
    }

private:
    SetMap& _setmap;
    
    set<string> _sets;
};

#endif // __POLICY_VISITOR_SETDEP_HH__
