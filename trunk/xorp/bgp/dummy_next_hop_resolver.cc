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

#ident "$XORP: xorp/bgp/dummy_next_hop_resolver.cc,v 1.4 2003/04/02 19:44:43 mjh Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "dummy_next_hop_resolver.hh"

template <class A>
DummyNextHopResolver<A>::DummyNextHopResolver(EventLoop& event_loop)
    : NextHopResolver<A>(NULL, event_loop)
{
}

template <class A>
DummyNextHopResolver<A>::~DummyNextHopResolver()
{
}

template <class A>
bool 
DummyNextHopResolver<A>::lookup(const A nexthop, bool& resolvable, 
			   uint32_t& metric) const
{
    typename map <A, uint32_t>::const_iterator i;
    i = _metrics.find(nexthop);
    if (i == _metrics.end()) {
	resolvable = false;
	debug_msg("Lookup: %s, not resolvable\n", nexthop.str().c_str());
	return true;
    } 
    resolvable = true;
    metric = i->second;
    debug_msg("Lookup: %s, metric %d\n", nexthop.str().c_str(), metric);
    return true;
}

template <class A>
void
DummyNextHopResolver<A>::set_nexthop_metric(const A nexthop, 
					    uint32_t metric) {
    typename map <A, uint32_t>::const_iterator i;
    i = _metrics.find(nexthop);
    if (i != _metrics.end()) {
	abort();
    }
    _metrics[nexthop] = metric;
}

template <class A>
void
DummyNextHopResolver<A>::unset_nexthop_metric(const A nexthop) {
    typename map <A, uint32_t>::iterator i;
    i = _metrics.find(nexthop);
    if (i == _metrics.end()) {
	fprintf(stderr, "Can't unset nexthop %s\n", nexthop.str().c_str());
	abort();
    }
    _metrics.erase(i);
}


//force these templates to be built
template class DummyNextHopResolver<IPv4>;

