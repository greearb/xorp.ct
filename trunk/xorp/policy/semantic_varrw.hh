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

#ifndef __POLICY_SEMANTIC_VARRW_HH__
#define __POLICY_SEMANTIC_VARRW_HH__

#include "policy/common/varrw.hh"
#include "policy/common/element_base.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/policy_exception.hh"
#include "var_map.hh"
#include <string>


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
class SemanticVarRW : public VarRW {
public:
    /**
     * @short Exception thrown on illegal variable use.
     */
    class var_error : public PolicyException {
    public:
	var_error(const string& err) : PolicyException(err) {}
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
    const Element& read(const string& id);

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
    void write(const string& id, const Element& elem);

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

    // not implemented.
    SemanticVarRW(const SemanticVarRW&);
    SemanticVarRW& operator=(const SemanticVarRW&);
};

#endif // __POLICY_SEMANTIC_VARRW_HH__
