// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/policy/action.cc,v 1.1 2003/02/13 04:27:48 mjh Exp $"

#include "policy_module.h"
#include "action.hh"

template <class A>
PolicyAction<A>::PolicyAction<A>()
{
}

template <class A>
AcceptAction<A>::AcceptAction<A>()
{
}

template <class A>
bool 
AcceptAction<A>::apply_actions(const PolicyRoute<A>& in_route, 
			       PolicyRoute<A>*& out_route) const
{
    //XXX
    out_route = &in_route;
    return true;
}

template <class A>
RejectAction<A>::RejectAction<A>()
{
}

template <class A>
bool 
RejectAction<A>::apply_actions(const PolicyRoute<A>& in_route, 
			       PolicyRoute<A>*& out_route) const
{
    //XXX
    out_route = &in_route;
    return true;
}
