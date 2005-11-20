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

#ident "$XORP: xorp/ospf/routing_table.cc,v 1.36 2005/11/20 22:58:02 atanu Exp $"

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
RoutingTable<A>::begin(OspfTypes::AreaID area)
{
    debug_msg("area %s\n", pr_id(area).c_str());
    XLOG_ASSERT(!_in_transaction);
    _in_transaction = true;

    _adv.clear_area(area);

    delete _previous;
    _previous = _current;
    _current = new Trie<A, InternalRouteEntry<A> >;

    // It is possible that multiple areas have added route to the
    // routing table. This area is about to add or replace all its
    // routes again. All routes from other areas must be preserved.

    if (0 == _previous)	// First time
	return;

    typename Trie<A, InternalRouteEntry<A> >::iterator tip;
    for (tip = _previous->begin(); tip != _previous->end(); tip++) {
	// This should be a copy not a reference.
 	InternalRouteEntry<A> ire = tip.payload();
	debug_msg("ire %s", cstring(ire));

	// If this entry contains a route from this area delete it.
	bool winner_changed;
	ire.delete_entry(area, winner_changed);
	
	// If there are no other routes don't put a copy in current.
	if (ire.empty()) {
	    debug_msg(" empty ire %s only this area was present\n",
		      cstring(ire));
	    continue;
	}

	debug_msg(" kept as other areas are present\n");
	_current->insert(tip.key(), ire);
    }
}

template <typename A>
bool
RoutingTable<A>::add_entry(OspfTypes::AreaID area, IPNet<A> net,
			   const RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n", pr_id(area).c_str(), cstring(net));
    XLOG_ASSERT(_in_transaction);
    XLOG_ASSERT(area == rt.get_area());

    _adv.add_entry(area, rt.get_advertising_router(), rt);

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
			       const RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n", pr_id(area).c_str(), cstring(net));
    XLOG_ASSERT(_in_transaction);

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
RoutingTable<A>::lookup_entry(OspfTypes::AreaID area, A router,
			      RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n",  pr_id(area).c_str(), cstring(router));

    IPNet<A> net(router, A::ADDR_BITLEN);

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i)
	return false;

    InternalRouteEntry<A>& irentry = i.payload();

    return irentry.get_entry(area, rt);
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
bool
RoutingTable<A>::lookup_entry_by_advertising_router(OspfTypes::AreaID area,
						    uint32_t adv,
						    RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n",  pr_id(area).c_str(), pr_id(adv).c_str());

    return _adv.lookup_entry(area, adv, rt);
}

template <typename A>
bool
RoutingTable<A>::longest_match_entry(A nexthop, RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(nexthop));

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->find(nexthop);
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
    XLOG_ASSERT(_in_transaction);
    _in_transaction = false;

    typename Trie<A, InternalRouteEntry<A> >::iterator tip;
    typename Trie<A, InternalRouteEntry<A> >::iterator tic;

    // If there is no previous routing table just install the current
    // table and return.

    if (0 == _previous) {
	for (tic = _current->begin(); tic != _current->end(); tic++) {
	    RouteEntry<A>& rt = tic.payload().get_entry();
	    if (!add_route(rt.get_area(), tic.key(),
			   rt.get_nexthop(), rt.get_cost(), rt)) {
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
	    if (!delete_route(rt.get_area(), tip.key(), rt)) {
		XLOG_WARNING("Delete of %s failed", cstring(tip.key()));
	    }
	}
    }

    for (tic = _current->begin(); tic != _current->end(); tic++) {
	tip = _previous->lookup_node(tic.key());
 	RouteEntry<A>& rt = tic.payload().get_entry();
	if (_previous->end() == tip) {
	    if (!add_route(rt.get_area(), tic.key(),
			   rt.get_nexthop(), rt.get_cost(), rt)) {
		XLOG_WARNING("Add of %s failed", cstring(tip.key()));
	    }
	} else {
	    RouteEntry<A>& rt_previous = tip.payload().get_entry();
	    if (rt.get_nexthop() != rt_previous.get_nexthop() ||
		rt.get_cost() != rt_previous.get_cost()) {
		if (!replace_route(rt.get_area(), tip.key(),
				   rt.get_nexthop(), rt.get_cost(),
				   rt, rt_previous, rt_previous.get_area())) {
		    XLOG_WARNING("Replace of %s failed", cstring(tip.key()));
		}
	    }
	}
    }
}

template <typename A>
void
RoutingTable<A>::remove_area(OspfTypes::AreaID area)
{
    XLOG_ASSERT(!_in_transaction);

    if (0 == _current)
	return;

    // Sweep through the current table and delete any routes that came
    // from this area.

    // XXX - Should consider this a candidate for running as a
    // background task.

    typename Trie<A, InternalRouteEntry<A> >::iterator tic;
    for (tic = _current->begin(); tic != _current->begin(); tic++) {
 	InternalRouteEntry<A>& ire = tic.payload();
	RouteEntry<A>& rt = ire.get_entry();
	// If the winning entry is for this area delete it from the
	// routing table.
	if (rt.get_area() == area)
	    delete_route(area, tic.key(), rt);
	    
	// Unconditionally remove the area, it may be a losing route.
	bool winner_changed;
	if (!ire.delete_entry(area, winner_changed))
	    continue;

	// No more route entries exist so remove this internal entry.
	if (ire.empty()) {
	    _current->erase(tic);
	    continue;
	}

	// If a new winner has emerged add it to the routing table.
	if (winner_changed) {
	    add_route(area, tic.key(), rt.get_nexthop(), rt.get_cost(),
		      ire.get_entry());
	}
    }
}

template <typename A>
bool
RoutingTable<A>::add_route(OspfTypes::AreaID area, IPNet<A> net, A nexthop,
			   uint32_t metric, RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n", pr_id(area).c_str(), cstring(net));

    bool result = true;
    if (!rt.get_discard()) {
	PolicyTags policytags;
	bool accepted = do_filtering(net, nexthop, metric, rt, policytags);
	rt.set_filtered(!accepted);
	if (accepted)
	    result = _ospf.add_route(net, nexthop, metric,
				     false /* equal */,
				     false /* discard */,
				     policytags);
    }  else {
	XLOG_WARNING("TBD - installing discard routes");
	result = false;
    }

    _ospf.get_peer_manager().summary_announce(area, net, rt);

    return result;
}

template <typename A>
bool
RoutingTable<A>::delete_route(OspfTypes::AreaID area, IPNet<A> net,
			      RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n", pr_id(area).c_str(), cstring(net));

    bool result;
    if (!rt.get_discard()) {
	if (!rt.get_filtered())
	    result = _ospf.delete_route(net);
	else
	    return true;
    } else {
	XLOG_WARNING("TBD - removing discard routes");
	result = false;
    }

    _ospf.get_peer_manager().summary_withdraw(area, net, rt);

    return result;
}

template <typename A>
bool
RoutingTable<A>::replace_route(OspfTypes::AreaID area, IPNet<A> net, A nexthop,
			       uint32_t metric, RouteEntry<A>& rt,
			       RouteEntry<A>& previous_rt,
			       OspfTypes::AreaID previous_area)
{
    debug_msg("area %s %s\n", pr_id(area).c_str(), cstring(net));

    bool result = delete_route(previous_area, net, previous_rt);
    if (!result)
	XLOG_WARNING("Failed to delete: %s", cstring(net));
    result = add_route(area, net, nexthop, metric, rt);

    return result;
}

template <typename A>
bool
RoutingTable<A>::do_filtering(IPNet<A>& net, A& nexthop,
			      uint32_t& metric, RouteEntry<A>& rt,
			      PolicyTags& policytags)
{
    // Routes to routers are required in the OSPF routing table to
    // satisfy requirements for AS-External-LSAs and
    // Summary-LSAs. Drop them here so they don't make it the the RIB.
    if (net.contains(nexthop) ||
	OspfTypes::Router == rt.get_destination_type())
 	return false;

    // The import policy filter.
    try {
	bool e_bit;
	uint32_t tag;
	OspfVarRW<A> varrw(net, nexthop, metric, e_bit, tag, policytags);

	// Import filtering
	bool accepted;

	debug_msg("[OSPF] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::IMPORT).c_str(), cstring(net));
	XLOG_TRACE(_ospf.trace()._import_policy,
		   "[OSPF] Running filter: %s on route: %s\n",
		   filter::filter2str(filter::IMPORT).c_str(), cstring(net));
		   
	accepted = _ospf.get_policy_filters().
	    run_filter(filter::IMPORT, varrw);

	// Route Rejected 
	if (!accepted) 
	    return accepted;

	OspfVarRW<A> varrw2(net, nexthop, metric, e_bit, tag, policytags);

	// Export source-match filtering
	debug_msg("[OSPF] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::EXPORT_SOURCEMATCH).c_str(),
		  cstring(net));
	XLOG_TRACE(_ospf.trace()._import_policy,
		   "[OSPF] Running filter: %s on route: %s\n",
		   filter::filter2str(filter::EXPORT_SOURCEMATCH).c_str(),
		   cstring(net));

	_ospf.get_policy_filters().
	    run_filter(filter::EXPORT_SOURCEMATCH, varrw2);

	return accepted;
    } catch(const PolicyException& e) {
	XLOG_WARNING("PolicyException: %s", e.str().c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
InternalRouteEntry<A>::add_entry(OspfTypes::AreaID area,
				 const RouteEntry<A>& rt)
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
InternalRouteEntry<A>::replace_entry(OspfTypes::AreaID area,
				     const RouteEntry<A>& rt)
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
InternalRouteEntry<A>::get_entry(OspfTypes::AreaID area,
				 RouteEntry<A>& rt) const
{
    typename map<OspfTypes::AreaID, RouteEntry<A> >::const_iterator i;

    if (_entries.end() == (i = _entries.find(area)))
	return false;

    rt = i->second;

    return true;
}

template <typename A>
bool
InternalRouteEntry<A>::reset_winner()
{
    RouteEntry<A> *old_winner = _winner;
    _winner = 0;
    typename map<OspfTypes::AreaID, RouteEntry<A> >::iterator i;
    for (i = _entries.begin(); i != _entries.end(); i++) {
	if (i == _entries.begin()) {
	    _winner = &(i->second);
	    continue;
	}
	RouteEntry<A>& comp = i->second;
	if (comp.get_path_type() <= _winner->get_path_type())
	    if (comp.get_cost() < _winner->get_cost()) {
		_winner = &comp;
		continue;
	    }
	    if (comp.get_cost() == _winner->get_cost()) {
		if (comp.get_area() > _winner->get_area())
		    _winner = &comp;
		continue;
	    }
    }

    return _winner != old_winner;
}

template <typename A>
string
InternalRouteEntry<A>::str()
{
    string output;

    typename map<OspfTypes::AreaID, RouteEntry<A> >::iterator i;
    for (i = _entries.begin(); i != _entries.end(); i++) {
	output += i->second.str();
    }

    return output;
}

template <typename A>
void
Adv<A>::clear_area(OspfTypes::AreaID area)
{
    debug_msg("Clearing area %s\n", pr_id(area).c_str());

    if (0 == _adv.count(area))
	return;

    typename ADV::iterator i = _adv.find(area);
    XLOG_ASSERT(_adv.end() != i);
    i->second.clear();
}

template <typename A>
void
Adv<A>::add_entry(OspfTypes::AreaID area, uint32_t adv,
		  const RouteEntry<A>& rt)
{
    debug_msg("Add entry area %s adv %s\n", pr_id(area).c_str(),
	   pr_id(adv).c_str());

    if (0 == _adv.count(area)) {
	AREA a;
	a[adv] = rt;
	_adv[area] = a;
	return;
    }

    typename ADV::iterator i = _adv.find(area);
    XLOG_ASSERT(_adv.end() != i);
    typename AREA::iterator j = i->second.find(adv);
    // A router can contribute to many routes
    if(i->second.end() != j)
	return;

    AREA& aref = _adv[area];
    aref[adv] = rt;
}

template <typename A>
bool
Adv<A>::lookup_entry(OspfTypes::AreaID area, uint32_t adv,
		     RouteEntry<A>& rt) const
{
    debug_msg("Lookup entry area %s adv %s\n", pr_id(area).c_str(),
	   pr_id(adv).c_str());

    if (0 == _adv.count(area)) {
	return false;
    }

    typename ADV::const_iterator i = _adv.find(area);
    XLOG_ASSERT(_adv.end() != i);
    typename AREA::const_iterator j = i->second.find(adv);
    if (i->second.end() == j)
	return false;
    rt = j->second;

    return true;
}

template class RoutingTable<IPv4>;
template class RoutingTable<IPv6>;
