// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/backend/term_instr.hh,v 1.6 2008/01/04 03:17:16 pavlin Exp $

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
	    _name(name) { 

	_instrc	= instr->size();
	_instructions = new Instruction*[_instrc];

	vector<Instruction*>::iterator iter;
	int i = 0;

	for (iter = instr->begin(); iter != instr->end(); iter++) {
	    _instructions[i] = *iter;
	    i++;
	}
	
	delete instr;
    }
    
    ~TermInstr() {
	for (int i = 0; i < _instrc; i++)
	    delete _instructions[i];
	delete [] _instructions;
    }

    /**
     * @return the instructions of this term. Caller must not delete.
     */
    Instruction** instructions() { return _instructions; }

    /**
     * @return name of the term
     */
    const string& name() { return _name; }

    int instrc() { return _instrc; }

private:
    string _name;
    Instruction** _instructions;
    int		  _instrc;

    // not impl
    TermInstr(const TermInstr&);
    TermInstr& operator=(const TermInstr&);
};

#endif // __POLICY_BACKEND_TERM_INSTR_HH__
