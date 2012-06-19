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

template <typename A>
inline static string
make_extint_name(const RouteTable<A>* e, const RouteTable<A>* i)
{
    return string("Ext:(" + (e ? e->tablename() : "NULL") + ")Int:(" + (i ? i->tablename() : "NULL") + ")");
}

template<class A>
ExtIntTable<A>::ExtIntTable(RouteTable<A>* ext_table, RouteTable<A>* int_table)
    : RouteTable<A>(make_extint_name(ext_table, int_table)),
      _ext_table(ext_table), _int_table(int_table)
{
    if (_ext_table)
	_ext_table->set_next_table(this);
    if (_int_table)
	_int_table->set_next_table(this);

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
}

template<class A>
int
ExtIntTable<A>::add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller)
{
    debug_msg("EIT[%s]: Adding route %s\n", this->tablename().c_str(),
	   route.str().c_str());
    if (_int_table && caller == _int_table) {
	XLOG_ASSERT(route.nexthop()->type() != EXTERNAL_NEXTHOP);
	// The new route comes from the IGP table
	debug_msg("route comes from IGP\n");
	// If it came here, it must be the wining IGP route.
	// Insert it in wining IGP trie
	_wining_igp_routes.insert(route.net(), &route);

	if (_ext_table) {
	    // Try to find existing EGP routes, that are installed
	    // We're looking for EGP route in all wining routes.
	    // We can look here, because no IGP route that matches this subnet
	    // can't be in wining routes, because we're currently adding it.
	    // If it would be there, then some error has occurred.
	    // It should have been deleted. We check for that in the XLOG_ASSERT
	    const IPRouteEntry<A>* found = lookup_route(route.net());
	    if (found != NULL) {
		if (found->admin_distance() < route.admin_distance()) {
		    // The admin distance of the existing IGP route is better
		    return XORP_ERROR;
		} else {
		    XLOG_ASSERT(found->admin_distance() != route.admin_distance());
		    bool delete_is_propagated(false);
		    this->delete_ext_route(found, delete_is_propagated);
		}
	    }
	}

	_wining_routes.insert(route.net(), &route);

	if (this->next_table() != NULL)
	    this->next_table()->add_route(route, this);

	if (_ext_table) {
	    // Does this cause any previously resolved nexthops to resolve
	    // differently?
	    recalculate_nexthops(route);

	    // Does this new route cause any unresolved nexthops to be resolved?
	    resolve_unresolved_nexthops(route);
	}

	return XORP_OK;

    } else if (_ext_table && caller == _ext_table) {
	// The new route comes from the EGP table
	debug_msg("route comes from EGP\n");
	// If it came here, it must be the wining EGP route.
	// Insert it in wining EGP trie
	_wining_egp_routes.insert(route.net(), &route);

	const IPRouteEntry<A>* found = lookup_route_in_igp_parent(route.net());

	if (found != NULL) {
	    if (found->admin_distance() < route.admin_distance()) {
		// The admin distance of the existing IGP route is better
		return XORP_ERROR;
	    }
	}

	if (route.nexthop()->type() == PEER_NEXTHOP) {
	    //
	    // Despite it coming from the Ext table, the nexthop is
	    // directly connected.  Just propagate it.
	    //

	    debug_msg("nexthop %s was directly connected\n", route.nexthop()->addr().str().c_str());

	    if (found != NULL) {
		// Delete the IGP route that has worse admin distance
		_wining_routes.erase(found->net());

		if (this->next_table() != NULL)
		    this->next_table()->delete_route(found, this);
	    }

	    _wining_routes.insert(route.net(), &route);

	    if (this->next_table() != NULL)
		this->next_table()->add_route(route, this);
	    return XORP_OK;
	} else {
	    IPNextHop<A>* rt_nexthop = route.nexthop();

	    const IPRouteEntry<A>* nexthop_route =
		    lookup_route_in_igp_parent(rt_nexthop->addr());
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
		// The EGP route is resolvable
		if (found != NULL) {
		    // Delete the IGP route that has worse admin distance
		    _wining_routes.erase(found->net());

		    if (this->next_table() != NULL)
			this->next_table()->delete_route(found, this);
		}

		debug_msg("nexthop resolved to \n   %s\n", nexthop_route->str().c_str());

		// Resolve the nexthop for non-directly connected nexthops

		const ResolvedIPRouteEntry<A>* resolved_route =
			resolve_and_store_route(route, nexthop_route);

		_wining_routes.insert(resolved_route->net(), resolved_route);

		if (this->next_table() != NULL)
		    this->next_table()->add_route(*resolved_route, this);

		return XORP_OK;
	    }
	}
    } else {
	XLOG_FATAL("ExtIntTable::add_route called from a class that "
		   "isn't a component of this override table");
    }
    return XORP_OK;
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::resolve_and_store_route(const IPRouteEntry<A>& route,
					const IPRouteEntry<A>* nexthop_route)
{
    ResolvedIPRouteEntry<A>* resolved_route;
    resolved_route = new ResolvedIPRouteEntry<A>(route.net(),
						 nexthop_route->vif(),
						 // IPRouteEntry will free memory for the nexthop.
						 // That's why we pass copy of NextHop pointer
						 // instead of pointer!
						 nexthop_route->nexthop()->get_copy(),
						 route.protocol(),
						 route.metric(),
						 nexthop_route,
						 &route);
    resolved_route->set_admin_distance(route.admin_distance());
    _ip_resolved_table.insert(resolved_route->net(), resolved_route);
    if (_resolving_routes.lookup_node(nexthop_route->net())
	== _resolving_routes.end()) {
	_resolving_routes.insert(nexthop_route->net(), nexthop_route);
    }


    typename ResolvedRouteBackLink::iterator backlink
	= _ip_igp_parents.insert(make_pair(nexthop_route->net(),
						resolved_route));
    resolved_route->set_backlink(backlink);

    return resolved_route;
}

template<class A>
int
ExtIntTable<A>::delete_route(const IPRouteEntry<A>* route,
			     RouteTable<A>* caller)
{
    debug_msg("ExtIntTable::delete_route %s\n", route->str().c_str());

    if (_int_table && caller == _int_table) {
	debug_msg("  called from _int_table\n");
	// If it came here, than we're certainly deleting wining IGP route
	_wining_igp_routes.erase(route->net());
	const IPRouteEntry<A>* found_egp_route = NULL;

	if (_ext_table) {
	    found_egp_route = lookup_route_in_egp_parent(route->net());

	    if (found_egp_route != NULL) {
		if (found_egp_route->admin_distance() < route->admin_distance()) {
		    // The admin distance of the existing EGP route is better
		    return XORP_ERROR;
		}
	    }

	    const ResolvedIPRouteEntry<A>* found
		= lookup_by_igp_parent(route->net());

	    if (found != NULL) {
		_resolving_routes.erase(route->net());
	    }

	    while (found != NULL) {
		debug_msg("found route using this nexthop:\n    %s\n", found->str().c_str());
		// Erase from table first to prevent lookups on this entry
		_ip_resolved_table.erase(found->net());
		_ip_igp_parents.erase(found->backlink());

		// Propagate the delete next
		_wining_routes.erase(found->net());

		if (this->next_table() != NULL)
		    this->next_table()->delete_route(found, this);

		// Now delete the local resolved copy, and reinstantiate it
		const IPRouteEntry<A>* egp_parent = found->egp_parent();
		delete found;

		// egp_parent route is one of the wining EGP routes.
		// Re-adding will overwrite it the in trie
		// That's no problem because we're overwriting
		// old pointer with the existing pointer.
		// That way we don't have any memory leaking.

		add_route(*egp_parent, _ext_table);
		found = lookup_by_igp_parent(route->net());
	    }
	}

	// Propagate the original delete
	_wining_routes.erase(route->net());

	this->next_table()->delete_route(route, this);

	if (_ext_table && found_egp_route) {
	    // It is possible the internal route had masked an external one.

	    // found_egp_route route is one of the wining EGP routes.
	    // Re-adding will overwrite it the in trie
	    // That's no problem because we're overwriting
	    // old pointer with the existing pointer.
	    // That way we don't have any memory leaking.

	    add_route(*found_egp_route, _ext_table);
	}

    } else if (_ext_table && caller == _ext_table) {
	debug_msg("  called from _ext_table\n");
	// If it came here, than we're certainly deleting wining EGP route
	_wining_egp_routes.erase(route->net());
	const IPRouteEntry<A>* found_igp_route =
		lookup_route_in_igp_parent(route->net());

	if (found_igp_route != NULL) {
	    if (found_igp_route->admin_distance() < route->admin_distance()) {
		// The admin distance of the existing IGP route is better
		return XORP_ERROR;
	    }
	}

	bool is_delete_propagated = false;
	this->delete_ext_route(route, is_delete_propagated);
	if (is_delete_propagated && found_igp_route) {
	    // It is possible the external route had masked an internal one.

	    // found_igp_route route is one of the wining IGP routes.
	    // Re-adding will overwrite it the in trie
	    // That's no problem because we're overwriting
	    // old pointer with the existing pointer.
	    // That way we don't have any memory leaking.

	    add_route(*found_igp_route, _int_table);
	}
    } else {
	XLOG_FATAL("ExtIntTable::delete_route called from a class that "
		   "isn't a component of this override table\n");
    }
    return XORP_OK;
}

template<class A>
int
ExtIntTable<A>::delete_ext_route(const IPRouteEntry<A>* route,
				 bool& is_delete_propagated)
{
    const ResolvedIPRouteEntry<A>* found;

    is_delete_propagated = false;

    found = lookup_in_resolved_table(route->net());
    if (found != NULL) {
	// Erase from table first to prevent lookups on this entry
	_ip_resolved_table.erase(found->net());
	_ip_igp_parents.erase(found->backlink());

	// Delete the route's IGP parent from _resolving_routes if
	// no-one is using it anymore
	if (lookup_by_igp_parent(found->igp_parent()->net()) == NULL) {
	    _resolving_routes.erase(found->igp_parent()->net());
	}

	// Propagate the delete next
	_wining_routes.erase(found->net());

	if (this->next_table() != NULL) {
	    this->next_table()->delete_route(found, this);
	    is_delete_propagated = true;
	}

	// Now delete the locally modified copy
	delete found;
    } else {
	// Propagate the delete only if the route wasn't found in
	// the unresolved nexthops table.
	if (delete_unresolved_nexthop(route) == false) {
	    _wining_routes.erase(route->net());

	    if (this->next_table() != NULL) {
		this->next_table()->delete_route(route, this);
		is_delete_propagated = true;
	    }
	}
    }

    return XORP_OK;
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
    XLOG_ASSERT(_ext_table);
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
	    add_route(*unresolved_route, _ext_table);

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
    iter = _ip_igp_parents.find(route_net);
    if (iter == _ip_igp_parents.end()) {
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
			const typename IGPParentMultiMap::iterator& previous)
{
    debug_msg("lookup_next_by_igp_parent %s, starting from %p -> %s\n",
	      route_net.str().c_str(),
	      previous->second, previous->second->net().str().c_str());

    pair<typename IGPParentMultiMap::iterator, typename IGPParentMultiMap::iterator> route_range = _ip_igp_parents.equal_range(route_net);
    typename IGPParentMultiMap::iterator iter = previous;
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
    XLOG_ASSERT(_ext_table);
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
    typename IGPParentMultiMap::iterator last_not_deleted = _ip_igp_parents.end();
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
	    _ip_igp_parents.erase(found->backlink());

	    // Delete the route's IGP parent from _resolving_routes if
	    // no-one's using it anymore
	    if (lookup_by_igp_parent(found->igp_parent()->net()) == NULL) {
		_resolving_routes.erase(found->igp_parent()->net());
	    }

	    // Propagate the delete next
	    _wining_routes.erase(found->net());

	    if (this->next_table() != NULL)
		this->next_table()->delete_route(found, this);

	    // Now delete the local resolved copy, and reinstantiate it
	    delete found;

	    // egp_parent route is one of the wining EGP routes.
	    // Re-adding will overwrite it the in trie
	    // That's no problem because we're overwriting
	    // old pointer with the existing pointer.
	    // That way we don't have any memory leaking.

	    add_route(*egp_parent, _ext_table);
	} else {
	    debug_msg("route matched but nexthop didn't: nexthop: %s\n    %s\n",
		    (egp_parent->nexthop())->addr().str().c_str(),
		   found->str().c_str());
	    last_not_deleted = found->backlink();
	}

	if (last_not_deleted == _ip_igp_parents.end()) {
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
ExtIntTable<A>::lookup_route_in_igp_parent(const IPNet<A>& ipnet) const
{
    typename RouteTrie::iterator iter = _wining_igp_routes.lookup_node(ipnet);
    return ((iter == _wining_igp_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_igp_parent(const A& addr) const
{
    typename RouteTrie::iterator iter = _wining_igp_routes.find(addr);
    return ((iter == _wining_igp_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_egp_parent(const IPNet<A>& ipnet) const
{
    typename RouteTrie::iterator iter = _wining_egp_routes.lookup_node(ipnet);
    return ((iter == _wining_egp_routes.end()) ? NULL : *iter);
}

template<class A>
inline const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_egp_parent(const A& addr) const
{
    typename RouteTrie::iterator iter = _wining_egp_routes.find(addr);
    return ((iter == _wining_egp_routes.end()) ? NULL : *iter);
}

template<class A>
void
ExtIntTable<A>::replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent)
{
    XLOG_ASSERT(old_parent);
    if (old_parent == _int_table)
	replumb_internal(new_parent);
    else if (old_parent == _ext_table)
	replumb_external(new_parent);
}

template<class A>
inline void
ExtIntTable<A>::replumb_external(RouteTable<A>* new_ext_parent)
{
    debug_msg("ExtIntTable::replumb_external\n");
    _ext_table = new_ext_parent;
    _ext_table->set_next_table(this);
    this->set_tablename(make_extint_name(_ext_table, _int_table));
    debug_msg("ExtIntTable: now called \"%s\"\n", this->tablename().c_str());
}

template<class A>
inline void
ExtIntTable<A>::replumb_internal(RouteTable<A>* new_int_parent)
{
    debug_msg("ExtIntTable::replumb_internal\n");
    _int_table = new_int_parent;
    _int_table->set_next_table(this);
    this->set_tablename(make_extint_name(_ext_table, _int_table));
    debug_msg("ExtIntTable: now called \"%s\"\n", this->tablename().c_str());
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
    s += "_ext_table = " + (_ext_table ? _ext_table->tablename() : "NULL") + "\n";
    s += "_int_table = " + (_int_table ? _int_table->tablename() : "NULL") + "\n";
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class ExtIntTable<IPv4>;
template class ExtIntTable<IPv6>;
