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

#ident "$XORP: xorp/rib/rt_tab_extint.cc,v 1.19 2004/06/10 22:41:40 hodson Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"

#include "rt_tab_extint.hh"

template <typename A>
inline static string
make_extint_name(const RouteTable<A>* e, const RouteTable<A>* i)
{
    return string("Ext:(" + e->tablename() + ")Int:(" + i->tablename() + ")");
}

template<class A>
ExtIntTable<A>::ExtIntTable(RouteTable<A>* ext_table, RouteTable<A>* int_table)
    : RouteTable<A>(make_extint_name(ext_table, int_table)),
      _ext_table(ext_table), _int_table(int_table)
{
    _ext_table->set_next_table(this);
    _int_table->set_next_table(this);

    debug_msg("New ExtInt: %s\n", this->tablename().c_str());
}

template<class A>
int
ExtIntTable<A>::add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller)
{
    debug_msg("EIT[%s]: Adding route %s\n", this->tablename().c_str(),
	   route.str().c_str());
    if (caller == _int_table) {
	// The new route comes from the IGP table
	debug_msg("route comes from IGP\n");
	const IPRouteEntry<A>* found;

	if (route.nexthop()->type() == EXTERNAL_NEXTHOP) {
	    // An IGP route must have a local nexthop.
	    XLOG_ERROR("Received route from IGP that contains "
		       "a non-local nexthop: %s",
		       route.str().c_str());
	    return XORP_ERROR;
	}

	found = lookup_route_in_egp_parent(route.net());
	if (found != NULL) {
	    if (found->admin_distance() < route.admin_distance()) {
		// The admin distance of the existing EGP route is better
		return XORP_ERROR;
	    }
	}

	found = lookup_in_resolved_table(route.net());
	if (found != NULL) {
	    if (found->admin_distance() < route.admin_distance()) {
		// The admin distance of the existing route is better
		return XORP_ERROR;
	    } else {
		bool is_delete_propagated = false;
		this->delete_ext_route(found, is_delete_propagated);
	    }
	}

	if (this->next_table() != NULL)
	    this->next_table()->add_route(route, this);

	// Does this cause any previously resolved nexthops to resolve
	// differently?
	recalculate_nexthops(route);

	// Does this new route cause any unresolved nexthops to be resolved?
	resolve_unresolved_nexthops(route);

	return XORP_OK;

    } else if (caller == _ext_table) {
	// The new route comes from the EGP table
	debug_msg("route comes from EGP\n");
	const IPRouteEntry<A>* found;
	found = lookup_route_in_igp_parent(route.net());
	if (found != NULL) {
	    if (found->admin_distance() < route.admin_distance()) {
		// The admin distance of the existing IGP route is better
		return XORP_ERROR;
	    } else {
		if (this->next_table() != NULL)
		    this->next_table()->delete_route(found, this);
	    }
	}
	IPNextHop<A>* rt_nexthop;
	rt_nexthop = reinterpret_cast<IPNextHop<A>* >(route.nexthop());
	A nexthop_addr = rt_nexthop->addr();
	const IPRouteEntry<A>* nexthop_route;
	nexthop_route = lookup_route_in_igp_parent(nexthop_addr);
	if (nexthop_route == NULL) {
	    // Store the fact that this was unresolved for later
	    debug_msg("nexthop %s was unresolved\n",
		      nexthop_addr.str().c_str());
	    _ip_unresolved_nexthops.insert(
		pair<A, const IPRouteEntry<A>* >(rt_nexthop->addr(), &route));
	    return XORP_ERROR;
	} else {
	    Vif* vif = nexthop_route->vif();
	    if ((vif != NULL)
		&& (vif->is_same_subnet(IPvXNet(nexthop_route->net()))
		    || vif->is_same_p2p(IPvX(nexthop_addr)))) {
		//
		// Despite it coming from the Ext table, the nexthop is
		// directly connected.  Just propagate it.
		//
		debug_msg("nexthop %s was directly connected\n",
			  nexthop_addr.str().c_str());
		if (this->next_table() != NULL)
		    this->next_table()->add_route(route, this);
		return XORP_OK;
	    } else {
		debug_msg("nexthop resolved to \n   %s\n",
			  nexthop_route->str().c_str());
		// Resolve the nexthop for non-directly connected nexthops

		const ResolvedIPRouteEntry<A>* resolved_route;
		resolved_route = resolve_and_store_route(route, nexthop_route);

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
						 nexthop_route->nexthop(),
						 route.protocol(),
						 route.metric(),
						 nexthop_route,
						 &route);
    resolved_route->set_admin_distance(route.admin_distance());
    _ip_route_table.insert(resolved_route->net(), resolved_route);
    if (_resolving_routes.lookup_node(nexthop_route->net())
	== _resolving_routes.end()) {
	_resolving_routes.insert(nexthop_route->net(), nexthop_route);
    }

    typename RouteBackLink::iterator backlink;
    backlink = _ip_igp_parents.insert(
	pair<const IPRouteEntry<A>* , ResolvedIPRouteEntry<A>* >
	(nexthop_route, resolved_route));
    resolved_route->set_backlink(backlink);

    return resolved_route;
}

template<class A>
int
ExtIntTable<A>::delete_route(const IPRouteEntry<A>* route,
			     RouteTable<A>* caller)
{
    debug_msg("ExtIntTable::delete_route %s\n", route->str().c_str());

    if (caller == _int_table) {
	debug_msg("  called from _int_table\n");
	const IPRouteEntry<A>* egp_parent;

	if (route->nexthop()->type() == EXTERNAL_NEXTHOP) {
	    // We didn't propagate the add_route, so also ignore the deletion
	    return XORP_ERROR;
	}

	const IPRouteEntry<A>* found_egp_route;
	found_egp_route = lookup_route_in_egp_parent(route->net());
	if (found_egp_route != NULL) {
	    if (found_egp_route->admin_distance() < route->admin_distance()) {
		// The admin distance of the existing EGP route is better
		return XORP_ERROR;
	    }
	}

	const ResolvedIPRouteEntry<A>* found;
	found = lookup_by_igp_parent(route);

	if (found != NULL) {
	    _resolving_routes.erase(route->net());
	}

	while (found != NULL) {
	    debug_msg("found route using this nexthop:\n    %s\n",
		   found->str().c_str());
	    // Erase from table first to prevent lookups on this entry
	    _ip_route_table.erase(found->net());
	    _ip_igp_parents.erase(found->backlink());

	    // Propagate the delete next
	    if (this->next_table() != NULL)
		this->next_table()->delete_route(found, this);

	    // Now delete the local resolved copy, and reinstantiate it
	    egp_parent = found->egp_parent();
	    delete found;
	    add_route(*egp_parent, _ext_table);
	    found = lookup_by_igp_parent(route);
	}

	// Propagate the original delete
	this->next_table()->delete_route(route, this);
	// It is possible the internal route had masked an external one.
	const IPRouteEntry<A>* masked_route;
	masked_route = _ext_table->lookup_route(route->net());
	if (masked_route != NULL)
	    add_route(*masked_route, _ext_table);

    } else if (caller == _ext_table) {
	debug_msg("  called from _ext_table\n");

	const IPRouteEntry<A>* found_igp_route;
	found_igp_route = lookup_route_in_igp_parent(route->net());
	if (found_igp_route != NULL) {
	    if (found_igp_route->admin_distance() < route->admin_distance()) {
		// The admin distance of the existing IGP route is better
		return XORP_ERROR;
	    }
	}

	bool is_delete_propagated = false;
	this->delete_ext_route(route, is_delete_propagated);
	if (is_delete_propagated) {
	    // It is possible the external route had masked an internal one.
	    const IPRouteEntry<A>* masked_route;
	    masked_route = _int_table->lookup_route(route->net());
	    if (masked_route != NULL)
		add_route(*masked_route, _int_table);
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
	_ip_route_table.erase(found->net());
	_ip_igp_parents.erase(found->backlink());

	// Delete the route's IGP parent from _resolving_routes if
	// no-one is using it anymore
	if (lookup_by_igp_parent(found->igp_parent()) == NULL) {
	    _resolving_routes.erase(found->igp_parent()->net());
	}

	// Propagate the delete next
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
    iter = _ip_route_table.lookup_node(net);
    if (iter == _ip_route_table.end())
	return NULL;
    else
	return iter.payload();
}

template<class A>
void
ExtIntTable<A>::resolve_unresolved_nexthops(const IPRouteEntry<A>& nexthop_route)
{
    typename map<A, const IPRouteEntry<A>* >::iterator rpair, nextpair;

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
	    debug_msg("resolve_unresolved_nexthops: resolving %s\n",
		      (*rpair->second).str().c_str());

	    // We're going to erase rpair, so preserve the state.
	    const IPRouteEntry<A>* unresolved_route = rpair->second;
	    nextpair = rpair;
	    ++nextpair;

	    // Remove it from the unresolved table
	    _ip_unresolved_nexthops.erase(rpair);

	    // Instantiate a route for it in the resolved_route table
	    const ResolvedIPRouteEntry<A>* resolved_route;
	    resolved_route = resolve_and_store_route(*unresolved_route,
						     &nexthop_route);

	    // Propagate to downsteam tables
	    if (this->next_table() != NULL)
		this->next_table()->add_route(*resolved_route, this);

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

    typename map<A, const IPRouteEntry<A>* >::iterator rpair;
    IPNextHop<A>* rt_nexthop;
    rt_nexthop = reinterpret_cast<IPNextHop<A>* >(route->nexthop());
    rpair = _ip_unresolved_nexthops.find(rt_nexthop->addr());
    while ((rpair != _ip_unresolved_nexthops.end()) &&
	   (rpair->first == rt_nexthop->addr())) {
	if (rpair->second == route) {
	    _ip_unresolved_nexthops.erase(rpair);
	    return true;
	}
	++rpair;
    }
    return false;
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::lookup_by_igp_parent(const IPRouteEntry<A>* route)
{
    debug_msg("lookup_by_igp_parent %x -> %s\n",
	      (u_int)route, route->net().str().c_str());

    typename RouteBackLink::iterator iter;
    iter = _ip_igp_parents.find(route);
    if (iter == _ip_igp_parents.end()) {
	debug_msg("Found no routes with this IGP parent\n");
	return NULL;
    } else {
	debug_msg("Found route with IGP parent %x:\n    %s\n",
	       (u_int)(route), (iter->second)->str().c_str());
	return iter->second;
    }
}

template<class A>
const ResolvedIPRouteEntry<A>*
ExtIntTable<A>::lookup_next_by_igp_parent(const IPRouteEntry<A>* route,
				  const ResolvedIPRouteEntry<A>* previous)
{
    debug_msg("lookup_next_by_igp_parent %x -> %s\n",
	   (u_int)route, route->net().str().c_str());

    //
    // TODO: if we have a large number of routes with the same IGP parent,
    // this can be very inefficient.
    //

    typename RouteBackLink::iterator iter;
    iter = _ip_igp_parents.find(route);
    while (iter != _ip_igp_parents.end()
	   && iter->first == route
	   && iter->second != previous) {
	++iter;
    }

    if (iter == _ip_igp_parents.end() || iter->first != route) {
	debug_msg("Found no more routes with this IGP parent\n");
	return NULL;
    }

    ++iter;
    if (iter == _ip_igp_parents.end() || iter->first != route) {
	debug_msg("Found no more routes with this IGP parent\n");
	return NULL;
    }

    debug_msg("Found next route with IGP parent %x:\n    %s\n",
	   (u_int)(route), (iter->second)->str().c_str());
    return iter->second;
}

template<class A>
void
ExtIntTable<A>::recalculate_nexthops(const IPRouteEntry<A>& new_route)
{
    debug_msg("recalculate_nexthops: %s\n", new_route.str().c_str());

    const IPRouteEntry<A>* old_route;
    typename Trie<A, const IPRouteEntry<A>* >::iterator iter;

    iter = _resolving_routes.find_less_specific(new_route.net());
    if (iter == _resolving_routes.end()) {
	debug_msg("no old route\n");
	return;
    }
    old_route = iter.payload();
    debug_msg("old route was: %s\n", old_route->str().c_str());

    const ResolvedIPRouteEntry<A>* found;
    const ResolvedIPRouteEntry<A>* last_not_deleted = NULL;
    const IPRouteEntry<A>* egp_parent;
    found = lookup_by_igp_parent(old_route);
    while (found != NULL) {
	egp_parent = found->egp_parent();
	XLOG_ASSERT(egp_parent->nexthop()->type() != DISCARD_NEXTHOP);
	A nexthop = (reinterpret_cast<IPNextHop<A>* >(egp_parent->nexthop()))->addr();

	if (new_route.net().contains(nexthop)) {
	    debug_msg("found route using this nexthop:\n    %s\n",
		      found->str().c_str());
	    // Erase from table first to prevent lookups on this entry
	    _ip_route_table.erase(found->net());
	    _ip_igp_parents.erase(found->backlink());

	    // Delete the route's IGP parent from _resolving_routes if
	    // no-one's using it anymore
	    if (lookup_by_igp_parent(found->igp_parent()) == NULL) {
		_resolving_routes.erase(found->igp_parent()->net());
	    }

	    // Propagate the delete next
	    if (this->next_table() != NULL)
		this->next_table()->delete_route(found, this);

	    // Now delete the local resolved copy, and reinstantiate it
	    delete found;
	    add_route(*egp_parent, _ext_table);
	} else {
	    debug_msg("route matched but nexthop didn't: nexthop: %s\n    %s\n",
		   nexthop.str().c_str(),
		   found->str().c_str());
	    last_not_deleted = found;
	}

	if (last_not_deleted == NULL) {
	    found = lookup_by_igp_parent(old_route);
	} else {
	    found = lookup_next_by_igp_parent(old_route, last_not_deleted);
	}
    }
    debug_msg("done recalculating nexthops\n------------------------------------------------\n");
}

template<class A>
const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route(const IPNet<A>& ipv4net) const
{
    const IPRouteEntry<A>* int_found;
    const IPRouteEntry<A>* ext_found;

    // First try our local version
    debug_msg("------------------\nlookup_route in resolved table %s\n",
	      this->tablename().c_str());
    typename Trie<A, const ResolvedIPRouteEntry<A>* >::iterator iter;

    iter = _ip_route_table.lookup_node(ipv4net);
    if (iter != _ip_route_table.end()) {
	return iter.payload();
    }
    debug_msg("Not found in resolved table\n");
#ifdef DEBUG_LOGGING
    _ip_route_table.print();
#endif

    // Local version failed, so try the parent tables.
    int_found = lookup_route_in_igp_parent(ipv4net);
    ext_found = _ext_table->lookup_route(ipv4net);

    if (ext_found == NULL) {
	return int_found;
    }
    if (int_found == NULL) {
	return ext_found;
    }

    if (int_found->admin_distance() <= ext_found->admin_distance()) {
	return int_found;
    } else {
	return ext_found;
    }
}

template<class A>
const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route(const A& addr) const
{
    const IPRouteEntry<A>* ext_found = NULL;
    const IPRouteEntry<A>* int_found;
    list<const IPRouteEntry<A>* > found;

    debug_msg("ExtIntTable::lookup_route\n");

    // Lookup locally, and in both internal and external tables
    typename Trie<A, const ResolvedIPRouteEntry<A>* >::iterator trie_iter;
    trie_iter = _ip_route_table.find(addr);
    if (trie_iter != _ip_route_table.end()) {
	found.push_back(trie_iter.payload());
    }

    int_found = lookup_route_in_igp_parent(addr);
    if (int_found != NULL)
	found.push_back(int_found);

    ext_found = _ext_table->lookup_route(addr);
    // Check that the external route has a local nexthop (if it doesn't
    // we expect the version in local_found to have a local nexthop)
    if (ext_found != NULL
	&& ext_found->nexthop()->type() == EXTERNAL_NEXTHOP) {
	ext_found = NULL;
    }
    if (ext_found != NULL)
	found.push_back(ext_found);

    if (found.empty())
	return NULL;

    // Retain only the routes with the longest prefix length
    uint32_t longest_prefix_len = 0;
    typename list<const IPRouteEntry<A>* >::iterator iter, iter2;
    for (iter = found.begin(); iter != found.end(); ++iter) {
	if ((*iter)->net().prefix_len() > longest_prefix_len) {
	    longest_prefix_len = (*iter)->net().prefix_len();
	}
    }
    for (iter = found.begin(); iter != found.end(); ) {
	iter2 = iter;
	++iter2;
	if ((*iter)->net().prefix_len() < longest_prefix_len) {
	    found.erase(iter);
	}
	iter = iter2;
    }

    if (found.size() == 1) {
	return found.front();
    }

    // Retain only the routes with the lowest admin_distance
    int lowest_ad = 0xffff;
    for (iter = found.begin(); iter != found.end(); ++iter) {
	if ((*iter)->admin_distance() < lowest_ad) {
	    lowest_ad = (*iter)->admin_distance();
	}
    }
    for (iter = found.begin(); iter != found.end(); ) {
	iter2 = iter;
	++iter2;
	if ((*iter)->admin_distance() > lowest_ad) {
	    found.erase(iter);
	}
	iter = iter2;
    }

    if (found.size() == 1) {
	return found.front();
    }

    // This shouldn't happen.
    XLOG_ERROR("ExtIntTable has multiple routes with same prefix_len "
	       "and same admin_distance");
    return found.front();
}

template<class A>
const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_igp_parent(const IPNet<A>& ipnet) const
{
    const IPRouteEntry<A>* found;

    found = _int_table->lookup_route(ipnet);

    // Sanity check that the answer is good
    if (found != NULL) {
	if (found->nexthop()->type() == EXTERNAL_NEXTHOP)
	    found = NULL;
    }
    return found;
}

template<class A>
const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_igp_parent(const A& addr) const
{
    const IPRouteEntry<A>* found;

    found = _int_table->lookup_route(addr);

    // Sanity check that the answer is good
    if (found != NULL) {
	if (found->nexthop()->type() == EXTERNAL_NEXTHOP)
	    found = NULL;
    }
    return found;
}

template<class A>
const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_egp_parent(const IPNet<A>& ipnet) const
{
    const IPRouteEntry<A>* found;

    found = _ext_table->lookup_route(ipnet);

    return found;
}

template<class A>
const IPRouteEntry<A>*
ExtIntTable<A>::lookup_route_in_egp_parent(const A& addr) const
{
    const IPRouteEntry<A>* found;

    found = _ext_table->lookup_route(addr);

    return found;
}

template<class A>
void
ExtIntTable<A>::replumb(RouteTable<A>* old_parent,
			RouteTable<A>* new_parent)
{
    debug_msg("ExtIntTable::replumb replacing %s with %s\n",
	      old_parent->tablename().c_str(),
	      new_parent->tablename().c_str());

    if (_ext_table == old_parent) {
	_ext_table = new_parent;
    } else if (_int_table == old_parent) {
	_int_table = new_parent;
    } else {
	// Shouldn't be possible
	XLOG_UNREACHABLE();
    }
    set_tablename(make_extint_name(_ext_table, _int_table));
    debug_msg("ExtIntTable: now called \"%s\"\n", this->tablename().c_str());
}

template<class A>
RouteRange<A>*
ExtIntTable<A>::lookup_route_range(const A& addr) const
{
    // What do the parents think the answer is?
    RouteRange<A>* int_rr = _int_table->lookup_route_range(addr);
    RouteRange<A>* ext_rr = _ext_table->lookup_route_range(addr);

    // What do we think the answer is?
    const IPRouteEntry<A>* route;
    typename Trie<A, const ResolvedIPRouteEntry<A>* >::iterator iter;
    iter = _ip_route_table.find(addr);
    if (iter == _ip_route_table.end())
	route = NULL;
    else
	route = iter.payload();

    A bottom_addr, top_addr;
    _ip_route_table.find_bounds(addr, bottom_addr, top_addr);
    RouteRange<A>* rr = new RouteRange<A>(addr, route, top_addr, bottom_addr);

    //
    // If there's a matching routing in _ip_route_table, there'll also
    // be one in _ext_table.  But our version has the correct nexthop.
    // We still need to merge in the result of lookup_route_range on
    // _ext_table though, because the upper and lower bounds there may
    // be tighter than the ones we'd find ourselves.
    //

    rr->merge(int_rr);
    delete(int_rr);

    rr->merge(ext_rr);
    delete(ext_rr);

    return rr;
}

template<class A>
string
ExtIntTable<A>::str() const
{
    string s;

    s = "-------\nExtIntTable: " + this->tablename() + "\n";
    s += "_ext_table = " + _ext_table->tablename() + "\n";
    s += "_int_table = " + _int_table->tablename() + "\n";
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class ExtIntTable<IPv4>;
template class ExtIntTable<IPv6>;
