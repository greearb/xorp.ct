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

#ident "$XORP: xorp/policy/policy_map.cc,v 1.14 2008/08/06 08:28:01 abittau Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "visitor_printer.hh"
#include "policy_map.hh"

PolicyStatement& 
PolicyMap::find(const string& name) const
{
    return _deps.find(name);
}

bool 
PolicyMap::exists(const string& name)
{
    return _deps.exists(name);
}

void 
PolicyMap::create(const string& name,SetMap& smap)
{
    PolicyStatement* ps = new PolicyStatement(name, smap, *this);

    if (!_deps.create(name,ps)) {
	delete ps;
	xorp_throw(PolicyMapError,
		   "Can't create policy " + name + " : already exists");
    }
}

void 
PolicyMap::delete_policy(const string& name)
{
    _deps.remove(name);
}

void 
PolicyMap::add_dependency(const string& policyname, const string& protocol)
{
    _deps.add_dependency(policyname,protocol);
}

void 
PolicyMap::del_dependency(const string& policyname, const string& protocol)
{
    _deps.del_dependency(policyname,protocol);
}

string
PolicyMap::str()
{
    ostringstream out;
    VisitorPrinter printer(out);

    // go through all policies and print them
    Dep::Map::const_iterator i = _deps.get_iterator();

    while (_deps.has_next(i)) {
	Dep::ObjPair p = _deps.next(i);

	// XXX hack! lame! [anyway this is only for debug]
	string policyname = p.name;
	printer.visit(find(policyname));
    }

    return out.str();
}

void
PolicyMap::policy_deps(const string& policy, DEPS& deps)
{
    // XXX we mix protocol names and policy names =(
    DEPS tmp;

    _deps.get_deps(policy, tmp);

    for (DEPS::iterator i = tmp.begin(); i != tmp.end(); ++i) {
	const string& name = *i;

	if (exists(name))
	    deps.insert(name);
    }
}

void
PolicyMap::policies(KEYS& out)
{
    _deps.keys(out);
}
