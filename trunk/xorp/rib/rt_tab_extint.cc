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

#ident "$XORP: xorp/rib/rt_tab_extint.cc,v 1.11 2003/09/30 03:08:00 pavlin Exp $"

#include "rib_module.h"
#include "libxorp/xlog.h"
#include "rt_tab_extint.hh"
//#define DEBUG_ROUTE_TABLE

#ifdef DEBUG_CODEPATH
#define cp(x) if (x < 10) printf("EITabCodePath:  %d\n", x); else printf("EITabCodePath: %d\n", x);
#else
#define cp(x) {}
#endif

template<class A>
ExtIntTable<A>::ExtIntTable<A>(const string&  tablename,
			       RouteTable<A>* ext_table,
			       RouteTable<A>* int_table)
    : RouteTable<A>(tablename)
{
    _ext_table = ext_table;
    _int_table = int_table;
    _ext_table->set_next_table(this);
    _int_table->set_next_table(this);
}

template<class A>
int
ExtIntTable<A>::add_route(const IPRouteEntry<A>& route, RouteTable<A> *caller) 
{
    debug_msg("EIT[%s]: Adding route %s\n", _tablename.c_str(),
	   route.str().c_str());
    if (caller == _int_table) {
	// the new route comes from the IGP table
	debug_msg("route comes from IGP\n");
	cp(1);
	const IPRouteEntry<A> *found;
	found = lookup_in_resolved_table(route.net());
	if (found != NULL) {
	    if (found->admin_distance() > route.admin_distance()) {
		// the admin distance of the existing route is worse
		cp(2);
		delete_route(found, _ext_table);
	    } else {
		cp(3);
		return XORP_ERROR;
	    }
	}

	if (route.nexthop()->type() == EXTERNAL_NEXTHOP) {
	    // An IGP route must have a local nexthop.
	    XLOG_ERROR(c_format("Received route from IGP that contains a non-local nexthop: %s\n",
				route.str().c_str()).c_str());
	    return XORP_ERROR;
	}

	if (_next_table != NULL)
	    _next_table->add_route(route, this);

	// Does this cause any previously resolved nexthops to resolve
	// differently?
	recalculate_nexthops(route);

	// Does this new route cause any unresolved nexthops to be resolved?
	resolve_unresolved_nexthops(route);

	return XORP_OK;
    } else if (caller == _ext_table) {
	// the new route comes from the EGP table
	debug_msg("route comes from EGP\n");
	cp(4);
	const IPRouteEntry<A> *found;
	found = lookup_route_in_igp_parent(route.net());
	if (found != NULL) {
	    if (found->admin_distance() > route.admin_distance()) {
		// the admin distance of the existing route is worse
		if (_next_table != NULL)
		    _next_table->delete_route(found, this);
		cp(5);
	    } else {
		cp(6);
		return XORP_ERROR;
	    }
	}
	IPNextHop<A>* rt_nexthop = (IPNextHop<A>*)(route.nexthop());
	A nh = rt_nexthop->addr();
	const IPRouteEntry<A> *nhroute = lookup_route_in_igp_parent(nh);
	if (nhroute == NULL) {
	    // Store the fact that this was unresolved for later.
	    debug_msg("nexthop %s was unresolved\n", nh.str().c_str());
	    _ip_unresolved_nexthops.insert(pair<A, const IPRouteEntry<A> *>
					   (rt_nexthop->addr(), &route));
	    cp(7);
	    return XORP_ERROR;
	} else {
	    Vif *vif = nhroute->vif();
	    if ((vif != NULL)
		&& (vif->is_same_subnet(IPvXNet(nhroute->net()))
		    || vif->is_same_p2p(IPvX(nh)))) {
		// despite it coming from the Ext table, the nexthop is
		// directly connected.  Just propagate it.
		debug_msg("nexthop %s was directly connected\n", nh.str().c_str());
		if (_next_table != NULL)
		    _next_table->add_route(route, this);
		cp(8);
		return XORP_OK;
	    } else {
		debug_msg("nexthop resolved to \n   %s\n",
		       nhroute->str().c_str());
		// resolve the nexthop for non-directly connected nexthops

		const ResolvedIPRouteEntry<A> *resolved_route;
		resolved_route = resolve_and_store_route(route, nhroute);

		if (_next_table != NULL)
		    _next_table->add_route(*resolved_route, this);
		cp(9);
		return XORP_OK;
	    }
	}
    } else {
	XLOG_FATAL("ExtIntTable::add_route called from a class that "
		   "isn't a component of this override table\n");
    }
    return XORP_OK;
}

template<class A>
const ResolvedIPRouteEntry<A> *
ExtIntTable<A>::resolve_and_store_route(const IPRouteEntry<A>& route,
					const IPRouteEntry<A> *nhroute) 
{
    cp(10);
    ResolvedIPRouteEntry<A> *resolved_route;
    resolved_route =
	new ResolvedIPRouteEntry<A>(route.net(), nhroute->vif(),
				    nhroute->nexthop(),
				    route.protocol(), route.metric(),
				    nhroute, &route);
    resolved_route->set_admin_distance(route.admin_distance());
    _ip_route_table.insert(resolved_route->net(),
			   resolved_route);
    if (_resolving_routes.lookup_node(nhroute->net())
	== _resolving_routes.end()) {
	cp(11);
	_resolving_routes.insert(nhroute->net(), nhroute);
    }

    typename RouteBackLink::iterator backlink =
	_ip_igp_parents.insert(pair<const IPRouteEntry<A> *,
			       ResolvedIPRouteEntry<A> *>
			       (nhroute, resolved_route));
    resolved_route->set_backlink(backlink);

    return resolved_route;
}

template<class A>
int
ExtIntTable<A>::delete_route(const IPRouteEntry<A> *route,
			     RouteTable<A> *caller) 
{
    debug_msg("ExtIntTable::delete_route %s\n", route->str().c_str());
    if (caller == _int_table) {
	cp(12);
	debug_msg("  called from _int_table\n");
	const ResolvedIPRouteEntry<A> *found;
	const IPRouteEntry<A> *egp_parent;

	if (route->nexthop()->type() == EXTERNAL_NEXTHOP) {
	    // we didn't propagate the add_route, so also ignore the deletion
	    return XORP_ERROR;
	}

	found = lookup_by_igp_parent(route);

	if (found != NULL) {
	    cp(13);
	    _resolving_routes.erase(route->net());
	}

	while (found != NULL) {
	    cp(14);
	    debug_msg("found route using this nh:\n    %s\n",
		   found->str().c_str());
	    // erase from table first to prevent lookups on this entry
	    _ip_route_table.erase(found->net());
	    _ip_igp_parents.erase(found->backlink());

	    // propagate the delete next
	    if (_next_table != NULL)
		_next_table->delete_route(found, this);

	    // now delete the local resolved copy, and reinstantiate it
	    egp_parent = found->egp_parent();
	    delete found;
	    add_route(*egp_parent, _ext_table);
	    found = lookup_by_igp_parent(route);
	}

	// propagate the original delete
	_next_table->delete_route(route, this);
	// it's possible the internal route had masked an external one.
	const IPRouteEntry<A> *masked_route;
	masked_route = _ext_table->lookup_route(route->net());
	if (masked_route != NULL)
	    add_route(*masked_route, _ext_table);

    } else if (caller == _ext_table) {
	cp(15);
	debug_msg("  called from _ext_table\n");
	const ResolvedIPRouteEntry<A> *found =
	    lookup_in_resolved_table(route->net());
	if (found != NULL) {
	    cp(16);
	    // erase from table first to prevent lookups on this entry
	    _ip_route_table.erase(found->net());
	    _ip_igp_parents.erase(found->backlink());

	    // delete the route's IGP parent from _resolving_routes if
	    // no-one's using it anymore
	    if (lookup_by_igp_parent(found->igp_parent()) == NULL) {
		cp(17);
		_resolving_routes.erase(found->igp_parent()->net());
	    }

	    // propagate the delete next
	    if (_next_table != NULL)
		_next_table->delete_route(found, this);

	    // now delete the locally modified copy
	    delete found;
	} else {
	    // propagate the delete only if the route wasn't found in
	    // the unresolved nexthops table
	    cp(18);
	    if (delete_unresolved_nexthop(route) == false)
		if (_next_table != NULL)
		    _next_table->delete_route(route, this);

	}


    } else {
	XLOG_FATAL("ExtIntTable::delete_route called from a class that "
		   "isn't a component of this override table\n");
    }
    return XORP_OK;
}

template<class A>
const ResolvedIPRouteEntry<A> *
ExtIntTable<A>::lookup_in_resolved_table(const IPNet<A>& net) 
{
    debug_msg("------------------\nlookup_route in resolved table %s\n", tablename().c_str());
    typename Trie<A, const ResolvedIPRouteEntry<A>*>::iterator iter;
    iter = _ip_route_table.lookup_node(net);
    if (iter == _ip_route_table.end())
	return NULL;
    else
	return iter.payload();
}

template<class A>
void
ExtIntTable<A>::resolve_unresolved_nexthops(const IPRouteEntry<A>& nhroute) 
{
    typedef typename map<A, const IPRouteEntry<A> *>::iterator CI;

    A unh, new_subnet;
    new_subnet = nhroute.net().masked_addr();

    // _ipv4_unresolved_nexthops is ordered by address.  Consequently,
    // lower_bound on the subnet base address will efficiently give us
    // the first matching address
    CI rpair = _ip_unresolved_nexthops.lower_bound(new_subnet);
    cp(19);
    while (rpair != _ip_unresolved_nexthops.end()) {
	cp(20);
	unh = rpair->first;
	if (new_subnet == unh.mask_by_prefix_len(nhroute.net().prefix_len())) {
	    cp(21);
	    // the unresolved nexthop matches our subnet
	    debug_msg("resolve_unresolved_nexthops: resolving %s\n",
		      (*rpair->second).str().c_str());

	    // we're going to erase rpair, so preserve the state.
	    const IPRouteEntry<A> *unresolved_route = rpair->second;
	    CI nextpair = rpair;
	    ++nextpair;

	    // remove it from the unresolved table
	    _ip_unresolved_nexthops.erase(rpair);

	    // instantiate a route for it in the resolved_route table
	    const ResolvedIPRouteEntry<A> *resolved_route;
	    resolved_route =
		resolve_and_store_route(*unresolved_route, &nhroute);

	    // propagate to downsteam tables
	    if (_next_table != NULL)
		_next_table->add_route(*resolved_route, this);

	    rpair = nextpair;
	} else {
	    if (new_subnet < unh.mask_by_prefix_len(nhroute.net().prefix_len())) {
		cp(22);
		// we've gone past any routes that we might possibly resolve
		return;
	    }
	    cp(23);
	    ++rpair;
	}
    }
}

template<class A>
bool
ExtIntTable<A>::delete_unresolved_nexthop(const IPRouteEntry<A> *route) 
{
    debug_msg("delete_unresolved_nexthop %s\n", route->str().c_str());
    typedef typename map<A, const IPRouteEntry<A> *>::iterator CI;
    IPNextHop<A>* rt_nexthop = (IPNextHop<A>*)route->nexthop();
    CI rpair = _ip_unresolved_nexthops.find(rt_nexthop->addr());
    cp(24);
    while ((rpair != _ip_unresolved_nexthops.end()) &&
	   (rpair->first == rt_nexthop->addr())) {
	cp(25);
	if (rpair->second == route) {
	    cp(26);
	    _ip_unresolved_nexthops.erase(rpair);
	    return true;
	}
	++rpair;
    }
    return false;
}

template<class A>
const ResolvedIPRouteEntry<A> *
ExtIntTable<A>::lookup_by_igp_parent(const IPRouteEntry<A> *route) 
{
    debug_msg("lookup_by_igp_parent %x -> %s\n", (u_int)route, route->net().str().c_str());

    cp(27);
    typename RouteBackLink::iterator found;
    found = _ip_igp_parents.find(route);
    if (found == _ip_igp_parents.end()) {
	cp(28);
	debug_msg("Found no routes with this IGP parent\n");
	return NULL;
    } else {
	cp(29);
	debug_msg("Found route with IGP parent %x:\n    %s\n",
	       (u_int)(route), (found->second)->str().c_str());
	return found->second;
    }
}


template<class A>
const ResolvedIPRouteEntry<A> *
ExtIntTable<A>::lookup_next_by_igp_parent(const IPRouteEntry<A> *route,
				  const ResolvedIPRouteEntry<A> *previous) 
{
    debug_msg("lookup_next_by_igp_parent %x -> %s\n",
	   (u_int)route, route->net().str().c_str());

    // if we have a large number of routes with the same IGP parent,
    // this can be very inefficient.

    typename RouteBackLink::iterator found;
    found = _ip_igp_parents.find(route);
    cp(30);
    while (found != _ip_igp_parents.end()
	   && found->first == route
	   && found->second != previous) {
	cp(31);
	++found;
    }

    if (found == _ip_igp_parents.end() || found->first != route) {
	cp(32);
	debug_msg("Found no more routes with this IGP parent\n");
	return NULL;
    }

    found++;
    if (found == _ip_igp_parents.end() || found->first != route) {
	cp(33);
	debug_msg("Found no more routes with this IGP parent\n");
	return NULL;
    }

    cp(34);
    debug_msg("Found next route with IGP parent %x:\n    %s\n",
	   (u_int)(route), (found->second)->str().c_str());
    return found->second;
}


template<class A>
void
ExtIntTable<A>::recalculate_nexthops(const IPRouteEntry<A>& new_route) 
{
    debug_msg("recalculate_nexthops: %s\n", new_route.str().c_str());
    const IPRouteEntry<A> *old_route;
    typename Trie<A, const IPRouteEntry<A>*>::iterator iter;
    iter = _resolving_routes.find_less_specific(new_route.net());
    cp(35);
    if (iter == _resolving_routes.end()) {
	cp(36);
	debug_msg("no old route\n");
	return;
    }
    old_route = iter.payload();
    debug_msg("old route was: %s\n", old_route->str().c_str());
    cp(37);

    const ResolvedIPRouteEntry<A> *found, *last_not_deleted = NULL;
    const IPRouteEntry<A> *egp_parent;
    found = lookup_by_igp_parent(old_route);
    while (found != NULL) {
	cp(38);
	egp_parent = found->egp_parent();
	XLOG_ASSERT(egp_parent->nexthop()->type() != DISCARD_NEXTHOP);
	A nexthop = ((IPNextHop<A>*)(egp_parent->nexthop()))->addr();

	if (new_route.net().contains(nexthop)) {
	    cp(39);
	    debug_msg("found route using this nh:\n    %s\n",
		      found->str().c_str());
	    // erase from table first to prevent lookups on this entry
	    _ip_route_table.erase(found->net());
	    _ip_igp_parents.erase(found->backlink());

	    // delete the route's IGP parent from _resolving_routes if
	    // no-one's using it anymore
	    if (lookup_by_igp_parent(found->igp_parent()) == NULL) {
		cp(40);
		_resolving_routes.erase(found->igp_parent()->net());
	    }


	    // propagate the delete next
	    if (_next_table != NULL)
		_next_table->delete_route(found, this);

	    // now delete the local resolved copy, and reinstantiate it
	    delete found;
	    add_route(*egp_parent, _ext_table);
	} else {
	    cp(41);
	    debug_msg("route matched but nh didn't: nh: %s\n    %s\n",
		   nexthop.str().c_str(),
		   found->str().c_str());
	    last_not_deleted = found;
	}

	if (last_not_deleted == NULL) {
	    cp(42);
	    found = lookup_by_igp_parent(old_route);
	} else {
	    cp(43);
	    found = lookup_next_by_igp_parent(old_route, last_not_deleted);
	}
    }
    debug_msg("done recalculating nexthops\n------------------------------------------------\n");
}

template<class A>
const IPRouteEntry<A> *
ExtIntTable<A>::lookup_route(const IPNet<A>& ipv4net) const 
{
    const IPRouteEntry<A> *int_found;
    const IPRouteEntry<A> *ext_found;
    // first try our local version
    debug_msg("------------------\nlookup_route in resolved table %s\n", tablename().c_str());
    typename Trie<A, const ResolvedIPRouteEntry<A>*>::iterator iter;
    iter = _ip_route_table.lookup_node(ipv4net);
    cp(44);
    if (iter != _ip_route_table.end()) {
	cp(45);
	return iter.payload();
    }
    debug_msg("Not found in resolved table\n");
#ifdef DEBUG_LOGGING
    _ip_route_table.print();
#endif

    // local version failed, so try the parent tables.
    int_found = lookup_route_in_igp_parent(ipv4net);

    ext_found = _ext_table->lookup_route(ipv4net);

    if (ext_found == NULL) {
	return int_found;
    }
    if (int_found == NULL) {
	return ext_found;
    }

    if (int_found->admin_distance() <= ext_found->admin_distance()) {
	cp(46);
	return int_found;
    } else {
	cp(47);
	return ext_found;
    }
}

template<class A>
const IPRouteEntry<A> *
ExtIntTable<A>::lookup_route(const A& addr) const 
{
    const IPRouteEntry<A> *ext_found = NULL, *int_found;
    list<const IPRouteEntry<A>*> found;
    cp(48);
    debug_msg("ExtIntTable::lookup_route\n");

    // lookup locally, and in both internal and external tables
    typename Trie<A, const ResolvedIPRouteEntry<A>*>::iterator iter;
    iter = _ip_route_table.find(addr);
    if (iter != _ip_route_table.end()) {
	found.push_back(iter.payload());
    }

    int_found = lookup_route_in_igp_parent(addr);
    if (int_found != NULL)
	found.push_back(int_found);

    ext_found = _ext_table->lookup_route(addr);
    // check that the external route has a local nexthop (if it doesn't
    // we expect the version in local_found to have a local nexthop)
    if (ext_found != NULL 
	&& ext_found->nexthop()->type() == EXTERNAL_NEXTHOP) {
	ext_found = NULL;
    }
    if (ext_found != NULL)
	found.push_back(ext_found);

    if (found.empty())
	return NULL;

    // retain only the routes with the longest prefix length
    uint32_t longest_prefix_len = 0;
    typename list<const IPRouteEntry<A>*>::iterator i, i2;
    for (i = found.begin(); i != found.end(); i++) {
	if ((*i)->net().prefix_len() > longest_prefix_len) {
	    longest_prefix_len = (*i)->net().prefix_len();
	}
    }
    for (i = found.begin(); i != found.end();) {
	i2 = i; i2++;
	if ((*i)->net().prefix_len() < longest_prefix_len) {
	    found.erase(i);
	}
	i = i2;
    }
    
    if (found.size() == 1) {
	return found.front();
    }
    
    // retain only the routes with the lowest admin_distance
    int lowest_ad = 0xffff;
    for (i = found.begin(); i != found.end(); i++) {
	if ((*i)->admin_distance() < lowest_ad) {
	    lowest_ad = (*i)->admin_distance();
	}
    }
    for (i = found.begin(); i != found.end();) {
	i2 = i; i2++;
	if ((*i)->admin_distance() > lowest_ad) {
	    found.erase(i);
	}
	i = i2;
    }
    
    if (found.size() == 1) {
	return found.front();
    }

    // this shouldn't happen.
    XLOG_ERROR("ExtIntTable has multiple routes with same prefix_len and same admin_distance");
    return found.front();
}

template<class A>
const IPRouteEntry<A> *
ExtIntTable<A>::lookup_route_in_igp_parent(const IPNet<A>& ipnet) const 
{
    const IPRouteEntry<A> *found;
    found = _int_table->lookup_route(ipnet);
    // sanity check that the answer is good
    if (found != NULL) {
	if (found->nexthop()->type() == EXTERNAL_NEXTHOP)
	    found = NULL;
    }
    return found;
}

template<class A>
const IPRouteEntry<A> *
ExtIntTable<A>::lookup_route_in_igp_parent(const A& addr) const 
{
    const IPRouteEntry<A> *found;
    found = _int_table->lookup_route(addr);
    // sanity check that the answer is good
    if (found != NULL) {
	if (found->nexthop()->type() == EXTERNAL_NEXTHOP)
	    found = NULL;
    }
    return found;
}

template<class A>
void
ExtIntTable<A>::replumb(RouteTable<A> *old_parent,
			RouteTable<A> *new_parent) 
{
    cp(62);
    if (_ext_table == old_parent) {
	cp(63);
	_ext_table = new_parent;
    } else if (_int_table == old_parent) {
	cp(64);
	_int_table = new_parent;
    } else {
	// shouldn't be possible
	XLOG_UNREACHABLE();
    }
}

template<class A>
RouteRange<A>*
ExtIntTable<A>::lookup_route_range(const A& addr) const
{
    // what do the parents think the answer is?
    RouteRange<A>* int_rr = _int_table->lookup_route_range(addr);
    RouteRange<A>* ext_rr = _ext_table->lookup_route_range(addr);

    // what do we think the answer is?
    const IPRouteEntry<A>* route;
    typename Trie<A, const ResolvedIPRouteEntry<A>*>::iterator iter;
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

template<class A> string
ExtIntTable<A>::str() const 
{
    string s;
    s = "-------\nExtIntTable: " + _tablename + "\n";
    s += "_ext_table = " + _ext_table -> tablename() + "\n";
    s += "_int_table = " + _int_table -> tablename() + "\n";
    if (_next_table == NULL)
	s += "no next table\n";
    else
	s += "next table = " + _next_table->tablename() + "\n";
    return s;
}

template class ExtIntTable<IPv4>;
template class ExtIntTable<IPv6>;

