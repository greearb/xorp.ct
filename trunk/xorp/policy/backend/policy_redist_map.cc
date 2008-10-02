// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/backend/policy_redist_map.cc,v 1.9 2008/10/01 23:44:26 pavlin Exp $"

#include "libxorp/xorp.h"

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
