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

#ident "$XORP: xorp/policy/code.cc,v 1.13 2008/10/02 21:57:57 bms Exp $"

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

    // add subroutines
    _subr.insert(rhs._subr.begin(), rhs._subr.end());

    return *this;
}

void
Code::add_subr(const string& policy, const string& code)
{
    _subr[policy] = code;
}

const SUBR&
Code::subr() const
{
    return _subr;
}
