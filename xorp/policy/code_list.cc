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



#include "policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "code_list.hh"
#include "policy/common/policy_utils.hh"

CodeList::CodeList(const string& policy)
    : _policy(policy)
{
}

CodeList::~CodeList()
{
    policy_utils::clear_container(_codes);
}

void
CodeList::push_back(Code* c)
{
    _codes.push_back(c);
}

string
CodeList::str() const
{
    string ret = "Policy: " + _policy + "\n";

    for (ListCode::const_iterator i = _codes.begin(); i != _codes.end(); ++i) {
        ret += (*i)->str();
    }

    return ret;
}

void
CodeList::link_code(Code& c) const
{
    // go through all the code we have, and link it to c.
    for (ListCode::const_iterator i = _codes.begin(); i != _codes.end(); ++i) {
	const Code* tmp = *i;

	// the += operator will check for target correctness.
	c += *tmp;
    }
}

void
CodeList::get_targets(Code::TargetSet& targets) const
{
    // go through all our code and see what targets the code is for
    for (ListCode::const_iterator i = _codes.begin(); i != _codes.end(); ++i) {
	const Code* c = *i;
	targets.insert(c->target());
    }
}

void
CodeList::get_targets(Code::TargetSet& targets,
		      const filter::Filter& filter) const
{
    //
    // Go through all our code and see what targets the code.
    // Insert only targets that match the given filter.
    //
    for (ListCode::const_iterator i = _codes.begin(); i != _codes.end(); ++i) {
	const Code* c = *i;
	if (c->target().filter() == filter)
	    targets.insert(c->target());
    }
}

void
CodeList::get_redist_tags(const string& protocol, Code::TagSet& tagset) const
{
    // go through all the code we have.
    for (ListCode::const_iterator i = _codes.begin(); i != _codes.end(); ++i) {
	const Code* c = *i;

	// we only want tags for specific protocols.
	if (c->target().protocol() != protocol)
	    continue;

	const Code::TagSet& ts = c->redist_tags();

	// insert the tags for this protocol.
	for (Code::TagSet::const_iterator j = ts.begin(); j != ts.end(); ++j) {
	    tagset.insert(*j);
	}
    }
}
