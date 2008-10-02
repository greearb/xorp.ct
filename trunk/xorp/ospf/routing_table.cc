// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/ospf/routing_table.cc,v 1.69 2008/08/20 01:34:03 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <map>
#include <list>
#include <set>
#include <deque>

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
	debug_msg("ire %s\n", cstring(ire));

	// If this entry contains a route from this area delete it.
	bool winner_changed;
	ire.delete_entry(area, winner_changed);
	
	// If there are no other routes don't put a copy in current.
	if (ire.empty()) {
	    debug_msg("empty ire %s only this area was present\n",
		      cstring(ire));
	    continue;
	}

	debug_msg("kept ire %s as other areas are present\n", cstring(ire));
	_current->insert(tip.key(), ire);
    }
}

template <typename A>
bool
RoutingTable<A>::add_entry(OspfTypes::AreaID area, IPNet<A> net,
			   const RouteEntry<A>& rt)
{
    debug_msg("area %s %s %s\n", pr_id(area).c_str(), cstring(net),
	      const_cast<RouteEntry<A>&>(rt).str().c_str());
    XLOG_ASSERT(_in_transaction);
    XLOG_ASSERT(area == rt.get_area());
    XLOG_ASSERT(rt.get_directly_connected() || rt.get_nexthop() != A::ZERO());
    bool status = true;

    // This router entry needs to be placed in the table.
    // In the OSPFv2 IPv4 case the {router ID}/32 is also placed in
    // the table as it has to be unique and makes for easy lookup.
    // In the OSPFv3 case lookups of a router will be done by router
    // ID only, so don't make an entry by net, plus there is no way of
    // guaranteeing a unique net as the net will be a link-local address.
    if (rt.get_destination_type() == OspfTypes::Router) {
	status = _adv.add_entry(area, rt.get_router_id(), rt);
	switch(_ospf.get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
// 	    XLOG_ASSERT(!net.is_valid());
//  	    if (net.is_valid()) {
//  		XLOG_WARNING("Net should be zero %s", cstring(net));
//  		status = false;
//  	    }
// 	    return status;
 	    return true;
	    break;
	}
    }

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i) {
	InternalRouteEntry<A> ire;
	i = _current->insert(net, ire);
// 	i = _current->lookup_node(net);
    }

    InternalRouteEntry<A>& irentry = i.payload();
    irentry.add_entry(area, rt);

    return status;
}

template <typename A>
bool
RoutingTable<A>::replace_entry(OspfTypes::AreaID area, IPNet<A> net,
			       const RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n", pr_id(area).c_str(), cstring(net));
    XLOG_ASSERT(_in_transaction);
    bool status = true;

    if (rt.get_destination_type() == OspfTypes::Router) {
	status = _adv.replace_entry(area, rt.get_router_id(), rt);
// 	switch(_ospf.get_version()) {
// 	case OspfTypes::V2:
// 	    break;
// 	case OspfTypes::V3:
// 	    XLOG_ASSERT(!net.is_valid());
// 	    if (net.is_valid()) {
// 		XLOG_WARNING("Net should be zero %s", cstring(net));
// 		status = false;
// 	    }
// 	    return status;
// 	    break;
// 	}
    }

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i) {
	return add_entry(area, net, rt);
    }

    InternalRouteEntry<A>& irentry = i.payload();
    irentry.replace_entry(area, rt);

    return status;
}

template <typename A>
bool
RoutingTable<A>::lookup_entry(A router, RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(router));

    if (0 == _current)
	return false;

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

    if (0 == _current)
	return false;

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

    if (0 == _current)
	return false;

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
RoutingTable<A>::lookup_entry(OspfTypes::AreaID area, IPNet<A> net,
			      RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(net));

    if (0 == _current)
	return false;

    typename Trie<A, InternalRouteEntry<A> >::iterator i;
    i = _current->lookup_node(net);
    if (_current->end() == i)
	return false;

    InternalRouteEntry<A>& irentry = i.payload();

    return irentry.get_entry(area, rt);
}

template <typename A>
bool
RoutingTable<A>::lookup_entry_by_advertising_router(OspfTypes::AreaID area,
						    uint32_t adv,
						    RouteEntry<A>& rt)
{
    debug_msg("area %s %s\n",  pr_id(area).c_str(), pr_id(adv).c_str());

    if (0 == _current)
	return false;

    return _adv.lookup_entry(area, adv, rt);
}

template <typename A>
bool
RoutingTable<A>::longest_match_entry(A nexthop, RouteEntry<A>& rt)
{
    debug_msg("%s\n", cstring(nexthop));

    if (0 == _current)
	return false;

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
			   rt.get_nexthop(), rt.get_cost(), rt, true)) {
		XLOG_WARNING("Add of %s failed", cstring(tic.key()));
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
	    if (!delete_route(rt.get_area(), tip.key(), rt, true)) {
		XLOG_WARNING("Delete of %s failed", cstring(tip.key()));
	    }
	}
    }

    for (tic = _current->begin(); tic != _current->end(); tic++) {
	tip = _previous->lookup_node(tic.key());
 	RouteEntry<A>& rt = tic.payload().get_entry();
	if (_previous->end() == tip) {
	    if (!add_route(rt.get_area(), tic.key(),
			   rt.get_nexthop(), rt.get_cost(), rt, true)) {
		XLOG_WARNING("Add of %s failed", cstring(tic.key()));
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
	    } else {
 		rt.set_filtered(rt_previous.get_filtered());
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
	    delete_route(area, tic.key(), rt, true);
	    
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
		      ire.get_entry(), true);
	}
    }
}

template <typename A>
bool
RoutingTable<A>::add_route(OspfTypes::AreaID area, IPNet<A> net, A nexthop,
			   uint32_t metric, RouteEntry<A>& rt, bool summaries)
{
    debug_msg("ADD ROUTE area %s net %s nexthop %s metric %u\n",
	      pr_id(area).c_str(), cstring(net), cstring(nexthop), metric);

    bool result = true;
    if (!rt.get_discard()) {
	PolicyTags policytags;
	bool accepted = do_filtering(net, nexthop, metric, rt, policytags);
	rt.set_filtered(!accepted);
	if (accepted)
	    result = _ospf.add_route(net, nexthop, rt.get_nexthop_id(), metric,
				     false /* equal */,
				     false /* discard */,
				     policytags);
    }  else {
	XLOG_WARNING("TBD - installing discard routes");
	result = false;
    }

    if (summaries)
	_ospf.get_peer_manager().summary_announce(area, net, rt);

    return result;
}

template <typename A>
bool
RoutingTable<A>::delete_route(OspfTypes::AreaID area, IPNet<A> net,
			      RouteEntry<A>& rt, bool summaries)
{
    debug_msg("DELETE ROUTE area %s %s filtered %s\n", pr_id(area).c_str(), 
	      cstring(net), bool_c_str(rt.get_filtered()));

    bool result;
    if (!rt.get_discard()) {
	if (!rt.get_filtered())
	    result = _ospf.delete_route(net);
	else
	    result = true;
    } else {
	XLOG_WARNING("TBD - removing discard routes");
	result = false;
    }

    if (summaries)
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
    debug_msg("REPLACE ROUTE area %s %s\n", pr_id(area).c_str(), cstring(net));

    bool result = delete_route(previous_area, net, previous_rt, false);
    if (!result)
	XLOG_WARNING("Failed to delete: %s", cstring(net));
    result = add_route(area, net, nexthop, metric, rt, false);

    _ospf.get_peer_manager().summary_replace(area, net, rt, previous_rt,
					     previous_area);

    return result;
}

template <typename A>
void
RoutingTable<A>::push_routes()
{
    typename Trie<A, InternalRouteEntry<A> >::iterator tic;

    if (0 == _current)
	return;

    for (tic = _current->begin(); tic != _current->end(); tic++) {
	RouteEntry<A>& rt = tic.payload().get_entry();
	if (rt.get_discard())
	    continue;
	PolicyTags policytags;
	IPNet<A> net = tic.key();
	A nexthop = rt.get_nexthop();
	uint32_t nexthop_id = rt.get_nexthop_id();
	uint32_t metric = rt.get_cost();
	bool accepted = do_filtering(net, nexthop, metric, rt, policytags);

	if (accepted) {
	    if (!rt.get_filtered()) {
		_ospf.replace_route(net, nexthop, nexthop_id, metric,
				    false /* equal */,
				    false /* discard */,
				    policytags);
				    
	    } else {
		_ospf.add_route(net, nexthop, nexthop_id, metric,
				false /* equal */,
				false /* discard */,
				policytags);
	    }
	} else {
	    if (!rt.get_filtered()) {
		_ospf.delete_route(net);
	    }
	}

	rt.set_filtered(!accepted);
    }
}

template <typename A>
bool
RoutingTable<A>::do_filtering(IPNet<A>& net, A& nexthop,
			      uint32_t& metric, RouteEntry<A>& rt,
			      PolicyTags& policytags)
{
    // The OSPF routing table needs to contain directly connected
    // routes and routes to routers to satisfy requirements for
    // AS-External-LSAs and Summary-LSAs. Drop them here so they don't
    // make it the the RIB.
    if (/*net.contains(nexthop) ||*/
	OspfTypes::Router == rt.get_destination_type() ||
	rt.get_directly_connected())
 	return false;

    // The import policy filter.
    try {
	bool e_bit;
	uint32_t tag;
	bool tag_set;
	OspfVarRW<A> varrw(net, nexthop, metric, e_bit, tag, tag_set,
			   policytags);

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

	OspfVarRW<A> varrw2(net, nexthop, metric, e_bit, tag, tag_set,
			    policytags);

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
	reset_winner();
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
    XLOG_ASSERT(0 != _winner);

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
	if (comp.get_path_type() < _winner->get_path_type()) {
	    _winner = &comp;
	    continue;
	}
	if (comp.get_path_type() == _winner->get_path_type()) {
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
	output += "Area: " + pr_id(i->first) + " " + i->second.str() + " ";
	if (&(i->second) == _winner)
	    output += "winner ";
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
bool
Adv<A>::add_entry(OspfTypes::AreaID area, uint32_t adv,
		  const RouteEntry<A>& rt)
{
    debug_msg("Add entry area %s adv %s\n", pr_id(area).c_str(),
	   pr_id(adv).c_str());

    XLOG_ASSERT(dynamic_cast<RouterLsa *>(rt.get_lsa().get())||
		dynamic_cast<SummaryRouterLsa *>(rt.get_lsa().get()));

    if (0 == _adv.count(area)) {
	AREA a;
	a[adv] = rt;
	_adv[area] = a;
	return true;
    }

    typename ADV::iterator i = _adv.find(area);
    XLOG_ASSERT(_adv.end() != i);
    typename AREA::iterator j = i->second.find(adv);
    if (i->second.end() != j) {
	XLOG_WARNING("An entry with this advertising router already exists %s",
		     cstring(*rt.get_lsa()));
	return false;
    }

    AREA& aref = _adv[area];
    aref[adv] = rt;

    return true;
}

template <typename A>
bool
Adv<A>::replace_entry(OspfTypes::AreaID area, uint32_t adv,
		      const RouteEntry<A>& rt)
{
    debug_msg("Add entry area %s adv %s\n", pr_id(area).c_str(),
	   pr_id(adv).c_str());

    XLOG_ASSERT(dynamic_cast<RouterLsa *>(rt.get_lsa().get())||
		dynamic_cast<SummaryRouterLsa *>(rt.get_lsa().get()));

    if (0 == _adv.count(area)) {
	XLOG_WARNING("There should already be an entry for this area %s",
		     cstring(*rt.get_lsa()));
	AREA a;
	a[adv] = rt;
	_adv[area] = a;
	return false;
    }

    bool status = true;

    typename ADV::iterator i = _adv.find(area);
    XLOG_ASSERT(_adv.end() != i);
    typename AREA::iterator j = i->second.find(adv);
    if (i->second.end() == j) {
	XLOG_WARNING("There should already be an entry with this adv %s",
		     cstring(*rt.get_lsa()));
	status = false;
    }

    AREA& aref = _adv[area];
    aref[adv] = rt;

    return status;
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

    debug_msg("Found area %s adv %s\n", pr_id(area).c_str(),
	   pr_id(adv).c_str());

    return true;
}

template class RouteEntry<IPv4>;
template class RouteEntry<IPv6>;
template class InternalRouteEntry<IPv4>;
template class InternalRouteEntry<IPv6>;
template class Adv<IPv4>;
template class Adv<IPv6>;
template class RoutingTable<IPv4>;
template class RoutingTable<IPv6>;
