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

#ident "$XORP$"

#include "config.h"
#include "policy_filter.hh"
#include "policy_backend_parser.hh"
#include "set_manager.hh"
#include "iv_exec.hh"
#include "policy/common/policy_utils.hh"

using namespace policy_utils;
using policy_backend_parser::policy_backend_parse;


PolicyFilter::PolicyFilter() : _policies(NULL) {}

void PolicyFilter::configure(const string& str) {
    
    vector<PolicyInstr*>* policies = new vector<PolicyInstr*>();
    map<string,Element*>* sets = new map<string,Element*>();
    string err;
    
    // do the actual parsing
    if(policy_backend_parse(*policies, *sets, str, err)) {
	// get rid of temporary parse junk.
	delete_vector(policies);
	clear_map(*sets);
	delete sets;
	throw ConfError(err);
    }

    // properly erase old conf
    reset();

    // replace with new conf
    _policies = policies;
    _sman.replace_sets(sets);
}

PolicyFilter::~PolicyFilter() {
    reset();
}

void PolicyFilter::reset() {
    if(_policies) {
	delete_vector(_policies);
	_policies = NULL;
    }
    _sman.clear();
}

bool PolicyFilter::acceptRoute(VarRW& varrw, ostream* os) {
    bool default_action = true;

    // no configuration done yet.
    if(!_policies)
	return default_action;

    // run policies
    IvExec ive(*_policies,_sman,varrw,os);

    IvExec::FlowAction fa = ive.run();

    // decide what to do
    switch(fa) {
        case IvExec::REJ:
	    return false;
	    
        case IvExec::DEFAULT:
	    return default_action;

        case IvExec::ACCEPT:
	    return true;
    }

    // unreash [hopefully]
    return default_action;
}
