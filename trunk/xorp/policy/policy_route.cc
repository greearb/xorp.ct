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

#ident "$XORP: xorp/policy/policy_route.cc,v 1.3 2003/10/23 05:24:35 atanu Exp $"

#include "policy_module.h"
#include "policy_route.hh"

template <typename A>
PolicyRoute<A>::PolicyRoute(const IPNet<A> &net, 
			    const A& nexthop, 
			    const string& origin_protocol)
    : _net(net), _nexthop(nexthop), _origin_protocol(origin_protocol)
{
}


template <typename A>
BGPPolicyRoute<A>::BGPPolicyRoute(const IPNet<A> &net, 
				  const A& nexthop, 
				  const AsPath& as_path, 
				  OriginType origin,
				  bool ibgp)
    : PolicyRoute<A>(net, nexthop, "bgp"),
      _as_path(as_path), _origin(origin), _ibgp(ibgp),
      _has_med(false), _has_localpref(false)
{
}
