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

// $XORP: xorp/policy/code_generator.hh,v 1.11 2008/08/06 08:22:17 abittau Exp $

#ifndef __POLICY_CODE_GENERATOR_HH__
#define __POLICY_CODE_GENERATOR_HH__

#include <sstream>
#include <string>

#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"
#include "var_map.hh"
#include "visitor.hh"
#include "code.hh"
#include "policy_statement.hh"
#include "node.hh"

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
        CodeGeneratorErr(const char* file, size_t line, 
			 const string& init_why = "")
            : PolicyException("CodeGeneratorErr", file, line, init_why) {}   
    };

    // used by source match code generator.
    CodeGenerator(const VarMap& varmap, PolicyMap& pmap);

    /**
     * Generate code for a specific protocol and filter [target]
     *
     * This construct is mainly used by derived classes to set the code target.
     *
     * @param proto target protocol.
     * @param filter target filter type.
     * @param varmap varmap.
     */
    CodeGenerator(const string& proto, const filter::Filter& filter,
		  const VarMap& varmap, PolicyMap& pmap);

    /**
     * Initialize code generation for an import of a specific protocol.
     *
     * @param proto target protocol.
     * @param varmap varmap.
     */
    CodeGenerator(const string& proto, const VarMap& varmap, PolicyMap& pmap);

    virtual ~CodeGenerator();

    const Element* visit(NodeUn& node);
    const Element* visit(NodeBin& node);
    const Element* visit(NodeAssign& node);
    const Element* visit(NodeElem& node);
    const Element* visit(NodeVar& node);
    const Element* visit(NodeSet& node);
    const Element* visit(NodeAccept& node);
    const Element* visit(NodeReject& node);
    const Element* visit(PolicyStatement& policy); 
    const Element* visit(Term& policy); 
    const Element* visit(NodeProto& policy); 
    const Element* visit(NodeNext& node);
    const Element* visit(NodeSubr& node);

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
    virtual const string&  protocol();

    Code	    _code;
    ostringstream   _os;
    const VarMap&   _varmap;
    PolicyMap&	    _pmap;
    bool	    _subr;

private:
    string	    _protocol;
};

#endif // __POLICY_CODE_GENERATOR_HH__
