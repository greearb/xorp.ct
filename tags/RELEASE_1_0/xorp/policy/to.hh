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

// $XORP: xorp/policy/to.hh,v 1.1 2003/01/30 19:21:11 mjh Exp $

#ifndef __POLICY_TO_HH__
#define __POLICY_TO_HH__

#include <list>
#include "libxorp/xorp.h"
#include "policy_route.hh"

template <class A>
class PolicyTo {
public:
    PolicyTo();
    bool matches(const PolicyRoute<A>& in_route);
private:
};

#endif // __POLICY_TO_HH__
