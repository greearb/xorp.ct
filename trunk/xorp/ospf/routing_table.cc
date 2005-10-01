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

#ident "$XORP: xorp/ospf/routing_table.cc,v 1.10 2005/10/01 05:28:31 atanu Exp $"
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
    debug_msg("\n");

    delete _previous;
    _previous = _current;
    _current = new Trie<A, InternalRouteEntry<A> >;
}

template <typename A>
bool
RoutingTable<A>::add_entry(OspfTypes::AreaID area, IPNet<A> net,
			   RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(net));

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i) {
	InternalRouteEntry<A> ire;
	_current->insert(net, ire);
	i = _current->lookup_node(net);
    }

    InternalRouteEntry<A>& irentry = i.payload();
    irentry.add_entry(area, rt);

    return true;
}

template <typename A>
bool
RoutingTable<A>::replace_entry(OspfTypes::AreaID area, IPNet<A> net,
			       RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(net));

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i) {
	return add_entry(area, net, rt);
    }

    InternalRouteEntry<A>& irentry = i.payload();
    irentry.replace_entry(area, rt);

    return true;
}

template <typename A>
bool
RoutingTable<A>::lookup_entry(A router, RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(router));

    IPNet<A> net(router, A::ADDR_BITLEN);

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i)
	return false;

    InternalRouteEntry<A>& irentry = i.payload();

    rt = irentry.get_entry();

    return true;
}

template <typename A>
bool
RoutingTable<A>::lookup_entry(IPNet<A> net, RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(net));

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i)
	return false;

    InternalRouteEntry<A>& irentry = i.payload();

    rt = irentry.get_entry();

    return true;
}

template <typename A>
void
RoutingTable<A>::end()
{
    debug_msg("\n");

    typename Trie<A, InternalRouteEntry<A> >::iterator tip;
    typename Trie<A, InternalRouteEntry<A> >::iterator tic;

    // If there is no previous routing table just install the current
    // table and return.

    if (0 == _previous) {
	for (tic = _current->begin(); tic != _current->end(); tic++) {
	    RouteEntry<A>& rt = tic.payload().get_entry();
	    if (!add_route(rt._area, tic.key(), rt._nexthop, rt._cost, rt)) {
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
	    RouteEntry<A>& rt = tip.payload().get_entry();
	    if (!delete_route(rt._area, tip.key())) {
		XLOG_WARNING("Delete of %s failed", cstring(tip.key()));
	    }
	}
    }

    for (tic = _current->begin(); tic != _current->begin(); tic++) {
	tip = _previous->lookup_node(tic.key());
 	RouteEntry<A>& rt = tic.payload().get_entry();
	if (_previous->end() == tip) {
	    if (!add_route(rt._area, tip.key(), rt._nexthop, rt._cost, rt)) {
		XLOG_WARNING("Add of %s failed", cstring(tip.key()));
	    }
	} else {
	    RouteEntry<A>& rt_previous = tip.payload().get_entry();
	    if (rt._nexthop != rt_previous._nexthop ||
		rt._cost != rt_previous._cost) {
		if (!replace_route(rt._area, tip.key(), rt._nexthop, rt._cost,
				   rt)) {
		    XLOG_WARNING("Replace of %s failed", cstring(tip.key()));
		}
	    }
	}
    }
}

template <typename A>
bool
RoutingTable<A>::add_route(OspfTypes::AreaID area, IPNet<A> net, A nexthop,
			   uint32_t metric, RouteEntry<A>& rt)
{
    bool result = _ospf.add_route(net, nexthop, metric,
				  false /* equal */, false /* discard */);

    _ospf.get_peer_manager().summary_announce(area, net, rt);

    return result;
}

template <typename A>
bool
RoutingTable<A>::delete_route(OspfTypes::AreaID area, IPNet<A> net)
{
    bool result = _ospf.delete_route(net);

    _ospf.get_peer_manager().summary_withdraw(area, net);

    return result;
}

template <typename A>
bool
RoutingTable<A>::replace_route(OspfTypes::AreaID area, IPNet<A> net, A nexthop,
			       uint32_t metric, RouteEntry<A>& rt)
{
    bool result = _ospf.replace_route(net, nexthop, metric,
				      false /* equal */, false /* discard */);

    _ospf.get_peer_manager().summary_withdraw(area, net);
    _ospf.get_peer_manager().summary_announce(area, net, rt);

    return result;
}

template <typename A>
bool
InternalRouteEntry<A>::add_entry(OspfTypes::AreaID area, RouteEntry<A>& rt)
{
    // An entry for this *area* should not already exist.
    XLOG_ASSERT(0 == _entries.count(area));
    if (0 == _entries.size()) {
	_entries[area] = rt;
	_winner = &_entries[area];
	return true;
    }
    // Add the entry and compute the winner.
    _entries[area] = rt;
    reset_winner();

    return true;
}

template <typename A>
bool
InternalRouteEntry<A>::replace_entry(OspfTypes::AreaID area, RouteEntry<A>& rt)
{
    bool winner_changed;
    delete_entry(area, winner_changed);
    return add_entry(area, rt);
}

template <typename A>
bool
InternalRouteEntry<A>::delete_entry(OspfTypes::AreaID area,
				    bool& winner_changed)
{
    if (0 == _entries.count(area))
	return false;

    _entries.erase(_entries.find(area));
    
    winner_changed = reset_winner();

    return true;
}

template <typename A>
RouteEntry<A>&
InternalRouteEntry<A>::get_entry() const
{
    return *_winner;
}

template <typename A>
bool
InternalRouteEntry<A>::reset_winner()
{
    bool winner_changed = false;

    typename map<OspfTypes::AreaID, RouteEntry<A> >::iterator i;
    for (i = _entries.begin(); i != _entries.end(); i++) {
	RouteEntry<A>& comp = i->second;
	if (comp._path_type <= _winner->_path_type)
	    if (comp._cost < _winner->_cost) {
		_winner = &comp;
		winner_changed = true;
	    }
    }

    return winner_changed;
}

template class RoutingTable<IPv4>;
template class RoutingTable<IPv6>;
