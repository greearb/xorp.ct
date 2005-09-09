// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/routing_table.cc,v 1.3 2005/09/09 13:00:05 atanu Exp $"
// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>
#include <set>
#include <deque>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"

template <typename A>
void
RoutingTable<A>::begin()
{
    delete _previous;
    _previous = _current;
    _current = new Trie<A, RouteEntry<A> >;
}

template <typename A>
bool
RoutingTable<A>::add_entry(IPNet<A> net, RouteEntry<A>& rt)
{
    _current->insert(net, rt);

    return true;
}

template <typename A>
void
RoutingTable<A>::end()
{
    typename Trie<A, RouteEntry<A> >::iterator tip;
    typename Trie<A, RouteEntry<A> >::iterator tic;

    // If there is no previous routing table just install the current
    // table and return.

    if (0 == _previous) {
	for (tic = _current->begin(); tic != _current->begin(); tic++) {
	    tip = _previous->lookup_node(tic.key());
	    RouteEntry<A>& rt = tic.payload();
	    if (!_ospf.add_route(tip.key(), rt._nexthop, rt._cost,
				 false /* equal */, false /* discard */)) {
		XLOG_WARNING("Add of %s failed", cstring(tip.key()));
	    }
	}
	return;
    }

    // Sweep through the previous table looking up routes in the
    // current table. If no route is found then: delete route.

    // Sweep through the current table looking up entries in the
    // previous table.
    // - No route found: add route.
    // - Route Found
    //		- If the routes match do nothing.
    //		- If the routes are different: replace route.

    for (tip = _previous->begin(); tip != _previous->end(); tip++) {
	if (_current->end() == _current->lookup_node(tip.key())) {
	    if (!_ospf.delete_route(tip.key())) {
		XLOG_WARNING("Delete of %s failed", cstring(tip.key()));
	    }
	}
    }

    for (tic = _current->begin(); tic != _current->begin(); tic++) {
	tip = _previous->lookup_node(tic.key());
 	RouteEntry<A>& rt = tic.payload();
	if (_previous->end() == tip) {
	    if (!_ospf.add_route(tip.key(), rt._nexthop, rt._cost,
				 false /* equal */, false /* discard */)) {
		XLOG_WARNING("Add of %s failed", cstring(tip.key()));
	    }
	} else {
	    RouteEntry<A>& rt_previous = tip.payload();
	    if (rt._nexthop != rt_previous._nexthop) {
		if (!_ospf.replace_route(tip.key(), rt._nexthop, rt._cost,
					 false /* equal */,
					 false /* discard */)) {
		    XLOG_WARNING("Replace of %s failed", cstring(tip.key()));
		}
	    }
	}
    }
}

template class RoutingTable<IPv4>;
template class RoutingTable<IPv6>;
