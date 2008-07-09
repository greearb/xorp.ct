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

#ident "$XORP: xorp/policy/code.cc,v 1.9 2007/02/16 22:46:52 pavlin Exp $"

#include "policy_module.h"

#include "libxorp/xorp.h"

#include "policy/common/policy_utils.hh"

#include "code.hh"

bool
Code::Target::operator<(const Target& rhs) const
{
    // XXX: I think XOR will do the trick [filter number / protocol]

    string left, right;

    left = _protocol + policy_utils::to_str(_filter);
    right = rhs._protocol + policy_utils::to_str(rhs._filter);

    return left < right;
}

bool
Code::Target::operator==(const Target& rhs) const
{
    if (_protocol != rhs._protocol)
	return false;

    if (_filter != rhs._filter)
	return false;

    return true;
}

bool
Code::Target::operator!=(const Target& rhs) const
{
    return !(*this == rhs);
}

string
Code::Target::str() const
{
    string ret = "Protocol: ";

    ret += _protocol;
    ret += ", Filter: " + filter2str(_filter);

    return ret;
}

void
Code::set_target_protocol(const string& protocol)
{
    _target.set_protocol(protocol);
}

void
Code::set_target_filter(const filter::Filter& filter)
{
    _target.set_filter(filter);
}

string
Code::str()
{
    string ret = "TARGET proto: " + _target.protocol();

    ret += " FILTER: ";
    ret += filter2str(_target.filter());
    ret += "\nCODE:\n";
    ret += _code;

    ret += "SETS:";

    for (set<string>::iterator i = _referenced_set_names.begin();
	 i != _referenced_set_names.end();
	 ++i) {
	ret += " " + *i;
    }

    ret += "\n";

    return ret;
}

Code&
Code::operator+=(const Code& rhs)
{
    // may only add for same target
    if (_target != rhs._target)
	return *this;	// do nothing

    // add the code [link it]
    _code += rhs._code;

    // add any new sets.
    for (set<string>::const_iterator i = rhs._referenced_set_names.begin();
	 i != rhs._referenced_set_names.end();
	 ++i) {
	_referenced_set_names.insert(*i);
    }

    // add tags
    for (TagSet::const_iterator i = rhs._all_tags.begin();
	 i != rhs._all_tags.end(); ++i) {
	_all_tags.insert(*i);
    }
    for (TagSet::const_iterator i = rhs._redist_tags.begin();
	 i != rhs._redist_tags.end(); ++i) {
	_redist_tags.insert(*i);
    }

    // add protos
    for (set<string>::const_iterator i = rhs._source_protocols.begin();
	 i != rhs._source_protocols.end(); ++i) {
	_source_protocols.insert(*i);
    }

    return *this;	
}
