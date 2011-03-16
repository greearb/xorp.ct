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

// $XORP: xorp/policy/backend/term_instr.hh,v 1.10 2008/10/02 21:58:05 bms Exp $

#ifndef __POLICY_BACKEND_TERM_INSTR_HH__
#define __POLICY_BACKEND_TERM_INSTR_HH__






#include "instruction.hh"
#include "policy/common/policy_utils.hh"

/**
 * @short Container of instructions.
 *
 * A term is an atomic policy unit which may be executed.
 */
class TermInstr :
    public NONCOPYABLE
{
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
};

#endif // __POLICY_BACKEND_TERM_INSTR_HH__
