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

#ident "$XORP: xorp/policy/term.cc,v 1.1 2003/02/13 00:51:03 mjh Exp $"

#include "policy_module.h"
#include "term.hh"

template <class A>
PolicyTerm<A>::PolicyTerm<A>(const PolicyFrom<A>& from, 
			     const PolicyTo<A>& to, 
			     const PolicyThen<A>& then)
    : _from(from), _to(to), _then(then)
{
}


template <class A>
void 
PolicyTerm<A>::apply_policy(PolicyRoute<A>& route, 
			    bool& changed, 
			    bool& reject, 
			    bool& last_term) const
{
    UNUSED(route);
    UNUSED(changed);
    UNUSED(reject);
    UNUSED(last_term);
}
