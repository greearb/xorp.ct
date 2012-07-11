// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rib.hh"
#include "rt_tab_extint.hh"

template<class A>
inline const string&
ExtIntTable<A>::ext_int_name()
{
    static string EXT_INT_NAME(c_format("ExtInt Table IPv%d", A::ip_version()));
    return EXT_INT_NAME;
}

template<class A>
ExtIntTable<A>::ExtIntTable()
    : RouteTable<A>(ext_int_name())
{
    debug_msg("New ExtInt: %s\n", this->tablename().c_str());
}

template<class A>
ExtIntTable<A>::~ExtIntTable()
{
    while (! _ip_unresolved_table.empty()) {
	delete _ip_unresolved_table.begin()->second;
	_ip_unresolved_table.erase(_ip_unresolved_table.begin());
    }

    while (! _ip_resolved_table.empty()) {
	delete *_ip_resolved_table.begin();
	_ip_resolved_table.erase(_ip_resolved_table.begin());
    }

    _igp_ad_set.clear();
    _egp_ad_set.clear();

    _all_tables.clear();
}

template<class A>
int
ExtIntTable<A>::change_admin_distance(OriginTable<A>* ot, uint32_t ad)
{
    XLOG_ASSERT(ot && ot->route_count() == 0);
    switch (ot->protocol_type()) {
    case IGP:
	if (_igp_ad_set.find(ot->admin_distance()) == _igp_ad_set.end()) {
	    return XORP_ERROR;
	}
	_igp_ad_set.erase(ot->admin_distance());
	_igp_ad_set.insert(ad);
	break;
    case EGP:
	if (_egp_ad_set.find(ot->admin_distance()) == _egp_ad_set.end()) {
	    return XORP_ERROR;
	}
	_egp_ad_set.erase(ot->admin_distance());
	_egp_ad_set.insert(ad);
	break;
    default:
	XLOG_ERROR("OriginTable for unrecognized protocol received!\n");
	return XORP_ERROR;
    }
    XLOG_ASSERT(_all_tables.find(ot->admin_distance()) != _all_tables.end() &&
		    _all_tables.find(ot->admin_distance())->second == ot);
    _all_tables.erase(ot->admin_distance());
    _all_tables[ad] = ot;

    /* Change the AD of OriginTable */
    ot->change_admin_distance(ad);
    return XORP_OK;
}

template<class A>
int
ExtIntTable<A>::add_protocol_table(OriginTable<A>* new_table)
{
    switch (new_table->protocol_type()) {
    case IGP:
	XLOG_ASSERT(_igp_ad_set.find(new_table->admin_distance()) == _igp_ad_set.end());
	_igp_ad_set.insert(new_table->admin_distance());
	break;
    case EGP:
	XLOG_ASSERT(_egp_ad_set.find(new_table->admin_distance()) == _egp_ad_set.end());
	_egp_ad_set.insert(new_table->admin_distance());
	break;
    default:
	XLOG_ERROR("OriginTable for unrecognized protocol received!\n");
	return XORP_ERROR;
    }
    XLOG_ASSERT(_all_tables.find(new_table->admin_distance()) == _all_tables.end());
    _all_tables[new_table->admin_distance()] = new_table;
    new_table->set_next_table(this);
    return XORP_OK;
}

template<class A>
bool
ExtIntTable<A>::best_igp_route(const IPRouteEntry<A>& route)
{
    typename RouteTrie::iterator iter = _wining_igp_routes.lookup_node(route.net());
    if (iter == _wining_igp_routes.end()) {
	_wining_igp_routes.insert(route.net(), &route);
	return true;
    } else if ((*iter)->admin_distance() > route.admin_distance()) {
	this->delete_igp_route(*iter);
	_wining_igp_routes.insert(route.net(), &route);
	return true;
    } else if ((*iter)->admin_distance() == route.admin_distance()) {
	// It's best IGP Route, but it wasn't overall best route when we added it first time
	return true;
    }
    return false;
}

template<class A>
bool
ExtIntTable<A>::best_egp_route(const IPRouteEntry<A>& route)
{
    typename RouteTrie::iterator iter = _wining_egp_routes.lookup_node(route.net());
    if (iter == _wining_egp_routes.end()) {
	_wining_egp_routes.insert(route.net(), &route);
	return true;
    } else if ((*iter)->admin_distance() > route.admin_distance()) {
	this->delete_egp_route(*iter);
	_wining_egp_routes.insert(route.net(), &route);
	return true;
    } else if ((*iter)->admin_distance() == route.admin_distance()) {
	// It's best EGP Route, but it wasn't overall best route when we added it first time
	return true;
    }
    return false;
}

template<class A>
int
ExtIntTable<A>::add_igp_route(const IPRouteEntry<A>& route)
{
    XLOG_ASSERT(_igp_ad_set.find(route.admin_distance()) != _igp_ad_set.end());
    XLOG_ASSERT(route.nexthop()->type() != EXTERNAL_NEXTHOP);
    // The new route comes from the IGP table
    debug_msg("route comes from IGP %s\n", route.str().c_str());

    // Is it the best IGP route?
    if (!best_igp_route(route))
	return XORP_ERROR;

    if (!_egp_ad_set.empty()) {
	// Try to find existing EGP routes, that are installed
	// We're looking for EGP route in all wining routes.
	// We can look here, because no IGP route that matches this subnet
	// can't be in wining routes, because we're currently adding it.
	// If it would be there, then some error has occurred.
	// It should have been deleted. We check for that in the XLOG_ASSERT
	const IPRouteEntry<A>* found = lookup_route(route.net());
	if (found != NULL) {
	    if (found->admin_distance() < route.admin_distance())
		// The admin distance of the existing EGP route is better
		return XORP_ERROR;

	    XLOG_ASSERT(found->admin_distance() != route.admin_distance());

	    this->delete_ext_route(found);
	}
    }

    _wining_routes.insert(route.net(), &route);

    this->next_table()->add_igp_route(route);

    if (!_egp_ad_set.empty()) {
	// Does this cause any previously resolved nexthops to resolve
	// differently?
	recalculate_nexthops(route);

	// Does this new route cause any unresolved nexthops to be resolved?
	resolve_unresolved_nexthops(route);
    }

    return XORP_OK;
}

template <class A>
int
ExtIntTable<A>::add_direct_egp_route(const IPRouteEntry<A>& route)
{
    const IPRouteEntry<A>* found = lookup_route(route.net());

    if (found && (found->admin_distance() < route.admin_distance()))
	return XORP_ERROR;

    XLOG_ASSERT(found ? (found->admin_distance() != route.admin_distance()) : true);

    //
    // Despite it coming from the Ext table, the nexthop is
    // directly connected.  Just propagate it.
    //
    debug_msg("nexthop %s was directly connected\n", route.nexthop()->addr().str().c_str());

    if (found != NULL) {
	// Delete the IGP route that has worse admin distance
	_wining_routes.erase(found->net());

	this->next_table()->delete_igp_route(found);
    }

    _wining_routes.insert(route.net(), &route);

    this->next_table()->add_egp_route(route);
    return XORP_OK;
}

template <class A>
int
ExtIntTable<A>::add_indirect_egp_route(const IPRouteEntry<A>& route)
{
    IPNextHop<A>* rt_nexthop = route.nexthop();

    const IPRouteEntry<A>* nexthop_route = lookup_winning_igp_route(rt_nexthop->addr());

    if (nexthop_route == NULL) {
	// Store the fact that this was unresolved for later
	debug_msg("nexthop %s was unresolved\n", rt_nexthop->addr().str().c_str());
	UnresolvedIPRouteEntry<A>* unresolved_route =
		new UnresolvedIPRouteEntry<A>(&route);

	_ip_unresolved_table.insert(make_pair(route.net(), unresolved_route));
	typename UnresolvedRouteBackLink::iterator backlink =
		_ip_unresolved_nexthops.insert(make_pair(rt_nexthop->addr(), unresolved_route));

	unresolved_route->set_backlink(backlink);
	return XORP_ERROR;
    } else {
	const IPRouteEntry<A>* found = lookup_route(route.net());

	if (found && (found->admin_distance() < route.admin_distance()))
	    return XORP_ERROR;

	XLOG_ASSERT(found ? (found->admin_distance() != route.admin_distance()) : true);

	// The EGP route is resolvable
	if (found != NULL) {
	    // Delete the IGP route that has worse admin distance
	    _wining_routes.erase(found->net());

	    this->next_table()->delete_igp_route(found);
	}

	debug_msg("nexthop resolved to \n   %s\n", nexthop_route->str().c_str());

	// Resolve the nexthop for non-directly connected nexthops

	const ResolvedIPRouteEntry<A>* resolved_route = resolve_and_store_route(route, nexthop_route);

	_wining_routes.insert(resolved_route->net(), resolved_route);

	this->next_table()->add_egp_route(*resolved_route);

	return XORP_OK;
    }
}

template<class A>
int
ExtIntTable<A>::add_egp_route(const IPRouteEntry<A>& route)
{
    XLOG_ASSERT(_egp_ad_set.find(route.admin_distance()) != _egp_ad_set.end());
    debug_msg("EIT[%s]: Adding route %s\n", this->tablename().c_str(), route.str().c_str());

    // The new route comes from the EGP table
    debug_msg("route comes from EGP %s\n", route.str().c_str());

    // Is it the best EGP route?
    if (!best_egp_route(route))
	return XORP_ERROR;

    if (route.nexthop()->type() == PEER_NEXTHOP)
	return add_direct_egp_route(route);
    else
	return add_indirect_egp_route(route);
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::resolve_and_store_route(const IPRouteEntry<A>& route,
					const IPRouteEntry<A>* nexthop_route)
{
    ResolvedIPRouteEntry<A>* resolved_route;
    resolved_route = new ResolvedIPRouteEntry<A>(nexthop_route,
						 &route);
    resolved_route->set_admin_distance(route.admin_distance());
    _ip_resolved_table.insert(resolved_route->net(), resolved_route);
    if (_resolving_routes.lookup_node(nexthop_route->net())
	== _resolving_routes.end()) {
	_resolving_routes.insert(nexthop_route->net(), nexthop_route);
    }


    typename ResolvedRouteBackLink::iterator backlink
	= _ip_resolving_parents.insert(make_pair(nexthop_route->net(),
						resolved_route));
    resolved_route->set_backlink(backlink);

    return resolved_route;
}

template <class A>
bool
ExtIntTable<A>::deleting_best_igp_route(const IPRouteEntry<A>* route)
{
    typename RouteTrie::iterator iter = _wining_igp_routes.lookup_node(route->net());
    if (iter != _wining_igp_routes.end()) {
	if ((*iter)->admin_distance() != route->admin_distance())
	    return false;
	else {
	    _wining_igp_routes.erase(route->net());
	    return true;
	}
    }
    return false;
}

template <class A>
bool
ExtIntTable<A>::deleting_best_egp_route(const IPRouteEntry<A>* route)
{
    typename RouteTrie::iterator iter = _wining_egp_routes.lookup_node(route->net());
    if (iter != _wining_egp_routes.end()) {
	if ((*iter)->admin_distance() != route->admin_distance())
	    return false;
	else {
	    _wining_egp_routes.erase(route->net());
	    return true;
	}
    }
    return false;
}

template <class A>
const IPRouteEntry<A>*
ExtIntTable<A>::masked_route(const IPRouteEntry<A>* route)
{
    typename RouteTableMap::iterator border = _all_tables.find(route->admin_distance());
    const IPRouteEntry<A>* found = NULL;
    XLOG_ASSERT(border != _all_tables.end());
    for (typename RouteTableMap::iterator iter = ++border; iter != _all_tables.end(); ++iter) {
	found = iter->second->lookup_ip_route(route->net());
	if (found)
	    return found;
    }
    return found;
}

template <class A>
void
ExtIntTable<A>::delete_resolved_routes(const IPRouteEntry<A>* route)
{
    const ResolvedIPRouteEntry<A>* found_resolved
	= lookup_by_igp_parent(route->net());

    if (found_resolved)
	_resolving_routes.erase(route->net());

    while (found_resolved) {
	debug_msg("found route using this nexthop:\n    %s\n", found_resolved->str().c_str());
	// Erase from table first to prevent lookups on this entry
	_ip_resolved_table.erase(found_resolved->net());
	_ip_resolving_parents.erase(found_resolved->backlink());

	// Propagate the delete next
	_wining_routes.erase(found_resolved->net());

	this->next_table()->delete_egp_route(found_resolved);

	// Now delete the local resolved copy, and reinstantiate it
	const IPRouteEntry<A>* egp_parent = found_resolved->egp_parent();
	delete found_resolved;

	// egp_parent route is one of the wining EGP routes.
	// Re-adding will overwrite it the in trie
	// That's no problem because we're overwriting
	// old pointer with the existing pointer.
	// That way we don't have any memory leaking.

	add_egp_route(*egp_parent);
	found_resolved = lookup_by_igp_parent(route->net());
    }
}

template<class A>
int
ExtIntTable<A>::delete_igp_route(const IPRouteEntry<A>* route)
{
    XLOG_ASSERT(_igp_ad_set.find(route->admin_distance()) != _igp_ad_set.end());
    debug_msg("  called from _int_table\n");
    debug_msg("route comes from IGP %s\n", route->str().c_str());
    // If it came here, than we're certainly deleting wining IGP route

    if (!deleting_best_igp_route(route))
	return XORP_ERROR;

    const IPRouteEntry<A>* found_route = NULL;

    if (!_egp_ad_set.empty()) {
	found_route = lookup_route(route->net());

	if (found_route != NULL) {
	    if (found_route->admin_distance() == route->admin_distance())
		// This route was the best route overall
		found_route = NULL;
	    else {
		// Our route wasn't the best route overall
		XLOG_ASSERT(found_route->admin_distance() < route->admin_distance());
		return XORP_ERROR;
	    }
	}

	delete_resolved_routes(route);
    }

    // Propagate the original delete
    _wining_routes.erase(route->net());

    this->next_table()->delete_igp_route(route);

    found_route = masked_route(route);

    if (found_route) {
	if (_igp_ad_set.find(found_route->admin_distance()) != _igp_ad_set.end())
	    add_igp_route(*found_route);
	else if (_egp_ad_set.find(found_route->admin_distance()) != _egp_ad_set.end())
	    add_egp_route(*found_route);
	else
	    XLOG_UNREACHABLE();
    }
    return XORP_OK;
}

template<class A>
int
ExtIntTable<A>::delete_egp_route(const IPRouteEntry<A>* route)
{
    XLOG_ASSERT(_egp_ad_set.find(route->admin_distance()) != _egp_ad_set.end());
    debug_msg("ExtIntTable::delete_route %s\n", route->str().c_str());
    XLOG_ASSERT(this->next_table());

    debug_msg("route comes from EGP %s\n", route->str().c_str());

    if (!deleting_best_egp_route(route))
	return XORP_ERROR;

    const IPRouteEntry<A>* found_route = lookup_route(route->net());

    if (found_route != NULL) {
	if (found_route->admin_distance() == route->admin_distance())
	    found_route = NULL;
	else if (found_route->admin_distance() < route->admin_distance()) {
	    //Our route wasn't the best overall
	    delete_ext_route(found_route, false);
	    return XORP_OK;
	}
    }

    found_route = masked_route(route);

    if (this->delete_ext_route(route) && found_route) {
	if (_igp_ad_set.find(found_route->admin_distance()) != _igp_ad_set.end())
	    add_igp_route(*found_route);
	else if (_egp_ad_set.find(found_route->admin_distance()) != _egp_ad_set.end())
	    add_egp_route(*found_route);
	else
	    XLOG_UNREACHABLE();
    }

    return XORP_OK;
}

template<class A>
bool
ExtIntTable<A>::delete_ext_route(const IPRouteEntry<A>* route, bool winning_route)
{
    const ResolvedIPRouteEntry<A>* found;

    bool is_delete_propagated = false;

    found = lookup_in_resolved_table(route->net());
    if (found != NULL) {
	// Erase from table first to prevent lookups on this entry
	_ip_resolved_table.erase(found->net());
	_ip_resolving_parents.erase(found->backlink());

	// Delete the route's IGP parent from _resolving_routes if
	// no-one is using it anymore
	if (lookup_by_igp_parent(found->resolving_parent()->net()) == NULL)
	    _resolving_routes.erase(found->resolving_parent()->net());

	if (winning_route == true) {
	    // Propagate the delete next
	    _wining_routes.erase(found->net());

	    this->next_table()->delete_egp_route(found);
	    is_delete_propagated = true;
	}

	// Now delete the locally modified copy
	delete found;
    } else if (!delete_unresolved_nexthop(route) && winning_route) {
	// Propagate the delete only if the route wasn't found in
	// the unresolved nexthops table and if it was the winning route.
	// Propagate the delete next
	_wining_routes.erase(route->net());

	if (_egp_ad_set.find(route->admin_distance()) != _egp_ad_set.end())
	    this->next_table()->delete_egp_route(route);
	else if (_igp_ad_set.find(route->admin_distance()) != _igp_ad_set.end())
	    this->next_table()->delete_igp_route(route);
	is_delete_propagated = true;
    }

    return is_delete_propagated;
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::lookup_in_resolved_table(const IPNet<A>& net)
{
    debug_msg("------------------\nlookup_route in resolved table %s\n",
	      this->tablename().c_str());

    typename Trie<A, const ResolvedIPRouteEntry<A>* >::iterator iter;
    iter = _ip_resolved_table.lookup_node(net);
    return ((iter == _ip_resolved_table.end()) ? NULL : *iter);
}

template<class A>
void
ExtIntTable<A>::resolve_unresolved_nexthops(const IPRouteEntry<A>& nexthop_route)
{
    typename multimap<A, UnresolvedIPRouteEntry<A>* >::iterator rpair, nextpair;

    A unresolved_nexthop, new_subnet;
    size_t prefix_len = nexthop_route.net().prefix_len();

    new_subnet = nexthop_route.net().masked_addr();

    // _ipv4_unresolved_nexthops is ordered by address.  Consequently,
    // lower_bound on the subnet base address will efficiently give us
    // the first matching address
    rpair = _ip_unresolved_nexthops.lower_bound(new_subnet);
    while (rpair != _ip_unresolved_nexthops.end()) {
	unresolved_nexthop = rpair->first;
	if (new_subnet == unresolved_nexthop.mask_by_prefix_len(prefix_len)) {
	    // The unresolved nexthop matches our subnet
	    UnresolvedIPRouteEntry<A>* unresolved_entry = rpair->second;
	    const IPRouteEntry<A>* unresolved_route = unresolved_entry->route();

	    debug_msg("resolve_unresolved_nexthops: resolving %s\n",
		      unresolved_route->str().c_str());

	    // We're going to erase rpair, so preserve the state.
	    nextpair = rpair;
	    ++nextpair;

	    // Remove it from the unresolved table
	    _ip_unresolved_nexthops.erase(rpair);
	    _ip_unresolved_table.erase(unresolved_route->net());
	    delete unresolved_entry;

	    // Unresolved routes are also wining EGP routes
	    // Re-adding them will overwrite them in trie
	    // That's no problem because we're overwriting
	    // old pointer with the existing pointer.
	    // That way we don't have any memory leaking.

	    // Reinstantiate the resolved route
	    add_egp_route(*unresolved_route);

	    rpair = nextpair;
	} else {
	    if (new_subnet
		< unresolved_nexthop.mask_by_prefix_len(prefix_len)) {
		// We've gone past any routes that we might possibly resolve
		return;
	    }
	    ++rpair;
	}
    }
}

template<class A>
bool
ExtIntTable<A>::delete_unresolved_nexthop(const IPRouteEntry<A>* route)
{
    debug_msg("delete_unresolved_nexthop %s\n", route->str().c_str());

    typename map<IPNet<A>, UnresolvedIPRouteEntry<A>* >::iterator iter;
    iter = _ip_unresolved_table.find(route->net());
    if (iter == _ip_unresolved_table.end())
	return false;

    UnresolvedIPRouteEntry<A>* unresolved_entry = iter->second;
    _ip_unresolved_table.erase(iter);
    _ip_unresolved_nexthops.erase(unresolved_entry->backlink());
    delete unresolved_entry;

    return true;
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::lookup_by_igp_parent(const IPNet<A>& route_net)
{
    debug_msg("lookup_by_igp_parent %s\n",
	      route_net.str().c_str());

    typename ResolvedRouteBackLink::iterator iter;
    iter = _ip_resolving_parents.find(route_net);
    if (iter == _ip_resolving_parents.end()) {
	debug_msg("Found no routes with this IGP parent\n");
	return NULL;
    } else {
	debug_msg("Found route with IGP parent %s:\n    %s\n",
		  route_net.str().c_str(), (iter->second)->str().c_str());
	return iter->second;
    }
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::lookup_next_by_igp_parent(const IPNet<A>& route_net,
			const typename ResolvingParentMultiMap::iterator& previous)
{
    debug_msg("lookup_next_by_igp_parent %s, starting from %p -> %s\n",
	      route_net.str().c_str(),
	      previous->second, previous->second->net().str().c_str());

    pair<typename ResolvingParentMultiMap::iterator, typename ResolvingParentMultiMap::iterator> route_range = _ip_resolving_parents.equal_range(route_net);
    typename ResolvingParentMultiMap::iterator iter = previous;
    if (iter++ == route_range.second ||
        iter == route_range.second) {
	debug_msg("Found no more routes with this IGP parent\n");
	return NULL;
    }

    debug_msg("Found next route with IGP parent %s:\n    %s\n",
	      route_net.str().c_str(), (iter->second)->str().c_str());
    return iter->second;
}

template<class A>
void
ExtIntTable<A>::recalculate_nexthops(const IPRouteEntry<A>& new_route)
{
    debug_msg("recalculate_nexthops: %s\n", new_route.str().c_str());

    const IPRouteEntry<A>* old_route;
    typename RouteTrie::iterator iter;

    iter = _resolving_routes.find_less_specific(new_route.net());
    if (iter == _resolving_routes.end()) {
	debug_msg("no old route\n");
	return;
    }
    old_route = *iter;
    debug_msg("old route was: %s\n", old_route->str().c_str());

    const ResolvedIPRouteEntry<A>* found;
    typename ResolvingParentMultiMap::iterator last_not_deleted = _ip_resolving_parents.end();
    const IPRouteEntry<A>* egp_parent;
    found = lookup_by_igp_parent(old_route->net());
    while (found != NULL) {
	egp_parent = found->egp_parent();
	XLOG_ASSERT(egp_parent->nexthop()->type() != DISCARD_NEXTHOP);
	XLOG_ASSERT(egp_parent->nexthop()->type() != UNREACHABLE_NEXTHOP);

	if (new_route.net().contains((egp_parent->nexthop())->addr())) {
	    debug_msg("found route using this nexthop:\n    %s\n",
		      found->str().c_str());
	    // Erase from table first to prevent lookups on this entry
	    _ip_resolved_table.erase(found->net());
	    _ip_resolving_parents.erase(found->backlink());

	    // Delete the route's IGP parent from _resolving_routes if
	    // no-one's using it anymore
	    if (lookup_by_igp_parent(found->resolving_parent()->net()) == NULL) {
		_resolving_routes.erase(found->resolving_parent()->net());
	    }

	    // Propagate the delete next
	    _wining_routes.erase(found->net());

	    this->next_table()->delete_egp_route(found);

	    // Now delete the local resolved copy, and reinstantiate it
	    delete found;

	    // egp_parent route is one of the wining EGP routes.
	    // Re-adding will overwrite it the in trie
	    // That's no problem because we're overwriting
	    // old pointer with the existing pointer.
	    // That way we don't have any memory leaking.

	    add_egp_route(*egp_parent);
	} else {
	    debug_msg("route matched but nexthop didn't: nexthop: %s\n    %s\n",
		    (egp_parent->nexthop())->addr().str().c_str(),
		   found->str().c_str());
	    last_not_deleted = found->backlink();
	}

	if (last_not_deleted == _ip_resolving_parents.end()) {
	    found = lookup_by_igp_parent(old_route->net());
	} else {
	    found = lookup_next_by_igp_parent(old_route->net(), last_not_deleted);
	}
    }
    debug_msg("done recalculating nexthops\n------------------------------------------------\n");
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route(const IPNet<A>& ipv4net) const
{
    typename RouteTrie::iterator iter = _wining_routes.lookup_node(ipv4net);
    return ((iter == _wining_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route(const A& addr) const
{
    typename RouteTrie::iterator iter = _wining_routes.find(addr);
    return ((iter == _wining_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_winning_igp_route(const IPNet<A>& ipnet) const
{
    typename RouteTrie::iterator iter = _wining_igp_routes.lookup_node(ipnet);
    return ((iter == _wining_igp_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_winning_igp_route(const A& addr) const
{
    typename RouteTrie::iterator iter = _wining_igp_routes.find(addr);
    return ((iter == _wining_igp_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_winning_egp_route(const IPNet<A>& ipnet) const
{
    typename RouteTrie::iterator iter = _wining_egp_routes.lookup_node(ipnet);
    return ((iter == _wining_egp_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_winning_egp_route(const A& addr) const
{
    typename RouteTrie::iterator iter = _wining_egp_routes.find(addr);
    return ((iter == _wining_egp_routes.end()) ? NULL : *iter);
}

template<class A>
RouteRange<A>*
ExtIntTable<A>::lookup_route_range(const A& addr) const
{
    typename RouteTrie::iterator iter;
    iter = _wining_routes.find(addr);

    const IPRouteEntry<A>* route =
	    (iter == _wining_routes.end()) ? NULL : *iter;

    A bottom_addr, top_addr;
    _wining_routes.find_bounds(addr, bottom_addr, top_addr);
    return (new RouteRange<A>(addr, route, top_addr, bottom_addr));
}

template<class A>
string
ExtIntTable<A>::str() const
{
    string s;

    s = "-------\nExtIntTable: " + this->tablename() + "\n";
    s += "_int_tables:\n";
    for (typename AdminDistanceSet::const_iterator iter = _igp_ad_set.begin(); iter != _igp_ad_set.end(); ++iter) {
	s += c_format("AD: %d \n", *iter);
	typename RouteTableMap::const_iterator rt_iter = _all_tables.find(*iter);
	s += rt_iter->second->str() + "\n";
    }
    s += "_ext_tables:\n";
    for (typename AdminDistanceSet::const_iterator iter = _egp_ad_set.begin(); iter != _egp_ad_set.end(); ++iter) {
	s += c_format("AD: %d \n", *iter);
	typename RouteTableMap::const_iterator rt_iter = _all_tables.find(*iter);
	s += rt_iter->second->str() + "\n";
    }
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class ExtIntTable<IPv4>;
template class ExtIntTable<IPv6>;
