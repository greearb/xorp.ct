// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/dummy_next_hop_resolver.cc,v 1.19 2008/10/02 21:56:15 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "dummy_next_hop_resolver.hh"


template <class A>
DummyNextHopResolver<A>::DummyNextHopResolver(EventLoop& eventloop,
					      BGPMain& bgp)
    : NextHopResolver<A>(NULL, eventloop, bgp)
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
	XLOG_FATAL("Can't find nexthop's metric\n");
    }
    _metrics[nexthop] = metric;
}

template <class A>
void
DummyNextHopResolver<A>::unset_nexthop_metric(const A nexthop) {
    typename map <A, uint32_t>::iterator i;
    i = _metrics.find(nexthop);
    if (i == _metrics.end()) {
	XLOG_FATAL("Can't unset nexthop %s\n", nexthop.str().c_str());
    }
    _metrics.erase(i);
}


//force these templates to be built
template class DummyNextHopResolver<IPv4>;

