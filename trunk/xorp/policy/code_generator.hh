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

#ifndef __POLICY_CODE_GENERATOR_HH__
#define __POLICY_CODE_GENERATOR_HH__

#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"

#include "visitor.hh"
#include "code.hh"
#include "policy_statement.hh"

#include "node.hh"

#include <sstream>
#include <string>


/**
 * @short Generic code generator. It is suitable for import filters.
 *
 * This class visits a structure of Nodes and generates appropriate code.
 */
class CodeGenerator : public Visitor {
public:
    /**
     * @short Exception thrown if code generation fails.
     *
     * This may occur for example, if an import policy has a dest part. The
     * semantic check should however get rid of all errors.
     */
    class CodeGeneratorErr : public PolicyException {
    public:
	CodeGeneratorErr(const string& err) : PolicyException(err) {}
    };

    
    CodeGenerator();

    /**
     * Generate code for a specific protocol and filter [target]/
     *
     * This construct is mainly used by derived classes to set the code target.a
     *
     * @param proto target protocol.
     * @param filter target filter type.
     */
    CodeGenerator(const string& proto, const filter::Filter& filter);
    
    /**
     * Initialize code generation for an import of a specific protocol.
     *
     * @param proto target protocol.
     */
    CodeGenerator(const string& proto);

    virtual ~CodeGenerator();

    

    const Element* visit(NodeUn& node);
    const Element* visit(NodeBin& node);
    const Element* visit(NodeAssign& node);
    const Element* visit(NodeRegex& node);
    const Element* visit(NodeElem& node);
    const Element* visit(NodeVar& node);
    const Element* visit(NodeSet& node);
    const Element* visit(NodeAccept& node);
    const Element* visit(NodeReject& node);
    
    const Element* visit(PolicyStatement& policy); 
    const Element* visit(Term& policy); 
    const Element* visit(NodeProto& policy); 

    /**
     * @return code generated.
     */
    const Code& code();

protected:
    // may not overload virtual functions =(
    // yes it is a triple dispatch... but we will get there eventually =D
    virtual const Element* visit_policy(PolicyStatement& policy);
    virtual const Element* visit_term(Term& term);
    virtual const Element* visit_proto(NodeProto& node);


    Code _code;
    ostringstream _os;
};

#endif // __POLICY_CODE_GENERATOR_HH__
