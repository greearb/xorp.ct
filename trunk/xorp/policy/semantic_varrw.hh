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

// $XORP: xorp/policy/semantic_varrw.hh,v 1.11 2008/10/02 21:58:00 bms Exp $

#ifndef __POLICY_SEMANTIC_VARRW_HH__
#define __POLICY_SEMANTIC_VARRW_HH__

#include <string>

#include <boost/noncopyable.hpp>

#include "policy/common/varrw.hh"
#include "policy/common/element_base.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/policy_exception.hh"

#include "var_map.hh"

/**
 * @short A VarRW used for semantic checking.
 *
 * This VarRW checks if elements may be read/written to and does typechecking
 * according to the VarMap.
 *
 * The user may set which protocols should be "simulated" by the VarRW.
 *
 * The SemanticVarRW will create dummy elements which are initialized to a
 * default value. This may not be optimal for semantic checking.
 */
class SemanticVarRW :
    public boost::noncopyable,
    public VarRW
{
public:
    /**
     * @short Exception thrown on illegal variable use.
     */
    class var_error : public PolicyException {
    public:
        var_error(const char* file, size_t line, const string& init_why = "")   
            : PolicyException("var_error", file, line, init_why) {}  
    };
   
    /**
     * @param vars the VarMap to use.
     */
    SemanticVarRW(VarMap& vars);
    ~SemanticVarRW();

    /**
     * VarRW read interface.
     *
     * Checks if a variable may be read.
     *
     * Throws exception on error.
     *
     * @return dummy element initialized to a default value.
     * @param id name of variable.
     */
    const Element& read(const Id& id);

    /**
     * VarRW write interface.
     *
     * Checks if a variable may be written to, and if the type is correct.
     *
     * Throws exception on error.
     *
     * @param id name of variable.
     * @param elem value of variable.
     */
    void write(const Id& id, const Element& elem);

    /**
     * VarRW sync interface.
     *
     * Does garbage collection.
     */
    void sync();

    /**
     * Change the protocol being simulated.
     *
     * @param proto protocol to simulate.
     */
    void set_protocol(const string& proto);

private:
    string _protocol;
    
    VarMap& _vars;
    ElementFactory _ef;

    set<Element*> _trash;
};

#endif // __POLICY_SEMANTIC_VARRW_HH__
