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

#include "policy_map.hh"

PolicyStatement& 
PolicyMap::find(const string& name) const {
    return _deps.find(name);
}

bool 
PolicyMap::exists(const string& name) {
    return _deps.exists(name);
}

void 
PolicyMap::create(const string& name,SetMap& smap) {
    PolicyStatement* ps = new PolicyStatement(name,smap);

    if(!_deps.create(name,ps)) {
	delete ps;
	throw PolicyMapError("Can't create policy " + name + 
			     " : already exists");
    }
}

void 
PolicyMap::delete_policy(const string& name) {
    _deps.remove(name);
}

void 
PolicyMap::add_dependancy(const string& policyname, const string& protocol) {
    _deps.add_dependancy(policyname,protocol);
}

void 
PolicyMap::del_dependancy(const string& policyname, const string& protocol) {
    _deps.add_dependancy(policyname,protocol);
}
