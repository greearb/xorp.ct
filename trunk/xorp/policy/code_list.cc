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

#include "policy_module.h"
#include "config.h"

#include "code_list.hh"
#include "policy/common/policy_utils.hh"

CodeList::CodeList(const string& p) : _policy(p) {
}

CodeList::~CodeList() {
    policy_utils::clear_container(_codes);
}


void 
CodeList::push_back(Code* c) {
    _codes.push_back(c);
}

string 
CodeList::str() const {
    string ret = "Policy: " + _policy + "\n";

    for(ListCode::const_iterator i = _codes.begin();
	i != _codes.end(); ++i)

        ret += (*i)->str();

    return ret;    
}

void 
CodeList::link_code(Code& c) const {
    // go through all the code we have, and link it to c.
    for(ListCode::const_iterator i = _codes.begin();
        i != _codes.end(); ++i) {
	
	const Code* tmp = *i;

	// the += operator will check for target correctness.
	c += *tmp;
    }    
}

void 
CodeList::get_targets(Code::TargetSet& targets) const {

    // go through all our code and see what targets the code is for
    for(ListCode::const_iterator i = _codes.begin(); 
	i != _codes.end() ; ++i) {
	
	const Code* c = *i;

	targets.insert(c->_target);
    }
}

void 
CodeList::get_tags(const string& protocol, Code::TagSet& tagset) const {
    
    // go through all the code we have.
    for(ListCode::const_iterator i = _codes.begin(); 
	i != _codes.end(); ++i) {
	
	const Code* c = *i;

	// we only want tags for specific protocols.
	if(c->_target.protocol != protocol)
	    continue;

	const Code::TagSet& ts = c->_tags;

	// insert the tags for this protocol.
	for(Code::TagSet::iterator j = ts.begin(); j != ts.end(); ++j) 
	    tagset.insert(*j);
    }
}
