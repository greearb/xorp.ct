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
#include "policy_redist_map.hh"
#include "policy/common/policy_utils.hh"

PolicyRedistMap::PolicyRedistMap() {
}

PolicyRedistMap::~PolicyRedistMap() {
    reset();
}

void
PolicyRedistMap::insert(const string& protocol, const PolicyTags& tags) {
    PolicyTags* ptags;
    
    Map::iterator i = _map.find(protocol);

    // create new policytags [first time we insert]
    if(i == _map.end()) {
	ptags = new PolicyTags(tags);
	_map[protocol] = ptags;
	return;
    }
    
    ptags = (*i).second;

    // just append the tags
    ptags->insert(tags);
}

void
PolicyRedistMap::reset() {
    // clear it ALL
    policy_utils::clear_map(_map);
}

void
PolicyRedistMap::get_protocols(set<string>& out, const PolicyTags& tags) {

    // XXX: maybe caller would like to control this
    out.clear();
    
    // go through all our tags.
    for(Map::iterator i = _map.begin(); i != _map.end(); ++i) {
	PolicyTags* ptags = (*i).second;

	// if atleast one tag in the taglist for this protocol is present in the
	// tags supplied by the user, then the protocol applies.
	if(ptags->contains_atleast_one(tags))
	    out.insert((*i).first);
    }
}
