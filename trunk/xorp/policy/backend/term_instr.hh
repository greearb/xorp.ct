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

#ifndef __POLICY_BACKEND_TERM_INSTR_HH__
#define __POLICY_BACKEND_TERM_INSTR_HH__

#include <vector>
#include <string>
#include "instruction.hh"
#include "policy/common/policy_utils.hh"


/**
 * @short Container of instructions.
 *
 * A term is an atomic policy unit which may be executed.
 */
class TermInstr {
public:
    /**
     * @param name term name.
     * @param instr list of instructions of this term. Caller must not delete.
     */
    TermInstr(const string& name, vector<Instruction*>* instr) :
	    _name(name), _instructions(instr) { }
    
    ~TermInstr() {
	policy_utils::delete_vector(_instructions);
    }

    /**
     * @return the instructions of this term. Caller must not delete.
     */
    vector<Instruction*>& instructions() { return *_instructions; }

    /**
     * @return name of the term
     */
    const string& name() { return _name; }     

private:
    string _name;
    vector<Instruction*>* _instructions;

    // not impl
    TermInstr(const TermInstr&);
    TermInstr& operator=(const TermInstr&);
};

#endif // __POLICY_BACKEND_TERM_INSTR_HH__
