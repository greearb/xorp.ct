// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "statement.hh"

template <typename A>
PolicyStatement<A>::PolicyStatement(const string& name)
    : _name(name)
{
}


template <typename A>
void 
PolicyStatement<A>::add_term(const PolicyTerm<A>& term) 
{
    _terms.push_back(term);
}


template <typename A>
const PolicyRoute<A>* 
PolicyStatement<A>::apply_policy(const PolicyRoute<A>* in_route) const
{
    bool route_changed = false;
    PolicyRoute<A> modified_route(*in_route);
    list<PolicyTerm<A> >::const_iterator i;
    for (i = _terms.begin(); i!= _terms.end(); i++) {
	bool changed, reject, last_term;
	(*i)->apply_policy(modified_route, changed, reject, last_term);
	if (changed)
	    route_changed = true;
	if (last_term) {
	    /* This term indicates to go no further */
	    if (reject)
		return NULL;
	    if (changed)
		return modified_route.clone_self();
	    else
		return in_route();
	}
    }

    //we shouldn't get this far - the last term should explicitly
    //accept or reject
    abort();
}
