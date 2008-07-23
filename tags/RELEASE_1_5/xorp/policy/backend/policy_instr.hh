// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/backend/policy_instr.hh,v 1.7 2008/05/03 00:37:30 bms Exp $

#ifndef __POLICY_BACKEND_POLICY_INSTR_HH__
#define __POLICY_BACKEND_POLICY_INSTR_HH__

#include "term_instr.hh"
#include <vector>
#include <string>


/**
 * @short Container for terms instructions.
 *
 * A policy instruction is a list of term instructions.
 */
class PolicyInstr {
public:
    /**
     * @param name name of the policy.
     * @param terms terms of the policy. Caller must not delete terms.
     */
    PolicyInstr(const string& name, vector<TermInstr*>* terms) :
        _name(name) { 
   
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

private:
    string _name;
    TermInstr** _terms;
    int _termc;

    // not impl
    PolicyInstr(const PolicyInstr&);
    PolicyInstr& operator=(const PolicyInstr&);
};

#endif // __POLICY_BACKEND_POLICY_INSTR_HH__
