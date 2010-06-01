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

// $XORP: xorp/policy/backend/policy_instr.hh,v 1.11 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_POLICY_INSTR_HH__
#define __POLICY_BACKEND_POLICY_INSTR_HH__

#include <vector>
#include <string>



#include "term_instr.hh"

/**
 * @short Container for terms instructions.
 *
 * A policy instruction is a list of term instructions.
 */
class PolicyInstr :
    public NONCOPYABLE
{
public:
    /**
     * @param name name of the policy.
     * @param terms terms of the policy. Caller must not delete terms.
     */
    PolicyInstr(const string& name, vector<TermInstr*>* terms) :
        _name(name), _trace(false) { 
   
	int i = 0;

	vector<TermInstr*>::iterator iter;

	_termc = terms->size();
	_terms = new TermInstr*[_termc];
	for (iter = terms->begin(); iter != terms->end(); ++iter) {
	    _terms[i] = *iter;
	    i++;
	}
	
	delete terms;
    }

    ~PolicyInstr() {
	for (int i = 0; i < _termc; i++)
	    delete _terms[i];
	
	delete [] _terms;
    }

    /**
     * @return terms of this policy. Caller must not delete terms.
     */
    TermInstr** terms() { return _terms; }

    /**
     * @return name of the policy.
     */
    const string& name() { return _name; }

    int termc() const { return _termc; }

    void set_trace(bool trace)	{ _trace = trace; }
    bool trace() const		{ return _trace; }

private:
    string	_name;
    TermInstr**	_terms;
    int		_termc;
    bool	_trace;
};

#endif // __POLICY_BACKEND_POLICY_INSTR_HH__
