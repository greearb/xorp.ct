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

#ident "$XORP: xorp/rib/rt_tab_register.cc,v 1.16 2004/02/06 22:44:12 pavlin Exp $"

#include "rib_module.h"

#include "rt_tab_register.hh"
#include "register_server.hh"


template<class A>
int
RouteRegister<A>::delete_registrant(const ModuleData* module)
{
    debug_msg("delete_registrant: this: %p, Module: %s\n", this,
	   module->str().c_str());
    set<const ModuleData* , ModuleCmp>::iterator mod_iter;
    mod_iter = _modules.find(module);

    if (mod_iter == _modules.end()) {
	return XORP_ERROR;
    }
    _modules.erase(mod_iter);
    debug_msg("new Value:\n%s\n", str().c_str());
    return XORP_OK;
}

template<class A>
string
RouteRegister<A>::str() const
{
    string s;

    s += "RR***********************\nRR RouteRegister: ";
    s += _valid_subnet.str() + "\n";

    if (_route != NULL)
	s += "RR Route: " + _route->str() + "\n";
    else
	s += "RR Route: NONE \n";

    set<const ModuleData*, ModuleCmp>::iterator mod_iter;
    mod_iter = _modules.begin();
    while (mod_iter != _modules.end()) {
	s += "RR Module: " + (*mod_iter)->str() + "\n";
	++mod_iter;
    }
    s += "RR***********************\n";
    return s;
}

template<class A>
RegisterTable<A>::RegisterTable(const string& tablename, 
			     RegisterServer* rs, 
			     bool multicast)
    : RouteTable<A>(tablename),
      _parent(NULL),
      _register_server(rs),
      _multicast(multicast)
{
}

template<class A>
void 
RegisterTable<A>::replumb(RouteTable<A>* old_parent, 
			  RouteTable<A>* new_parent)
{
    XLOG_ASSERT(_parent == old_parent);
    _parent = new_parent;
}

template<class A>
int
RegisterTable<A>::find_matches(const IPRouteEntry<A>& route)
{
    bool matches = false;

    //
    // Note that the _ipregistry trie contains no overlapping routes,
    // so if we find an exact match or a less specific match then we're
    // done.
    //

    debug_msg("FM: %s\n", route.net().str().c_str());
    // Find any exact matches
    typename Trie<A, RouteRegister<A>* >::iterator iter;
    iter = _ipregistry.lookup_node(route.net());
    if (iter != _ipregistry.end()) {
	debug_msg("FM: exact match\n");
	iter.payload()->mark_modules();
	return XORP_OK;
    }
    debug_msg("FM: no exact match\n");

    //
    // Find the parent.
    // This is the case when a new more specific route appears.
    //
    iter = _ipregistry.find_less_specific(route.net());
    if (iter != _ipregistry.end()) {
	debug_msg("FM: less specific match\n");
	iter.payload()->mark_modules();
	return XORP_OK;
    }
    debug_msg("FM: no less specific match\n");

    // Find any children.
    iter = _ipregistry.search_subtree(route.net());
    while (iter != _ipregistry.end()) {
	debug_msg("FM: found child\n");
	iter.payload()->mark_modules();
	matches = true;
	iter++;
    }
    if (matches == false)
	debug_msg("FM:  no children found\n");

    if (matches)
	return XORP_OK;
    else
	return XORP_ERROR;
}

template<class A>
int
RegisterTable<A>::notify_relevant_modules(bool add,
					  const IPRouteEntry<A>& changed_route)
{
    bool matches = false;
    IPNet<A> changed_net = changed_route.net();

    //
    // Note that the _ipregistry trie contains no overlapping routes,
    // so if we find an exact match or a less specific match then we're
    // done.
    //

    debug_msg("NRM: %s\n", changed_net.str().c_str());
    // Find any exact matches
    typename Trie<A, RouteRegister<A>* >::iterator iter, nextiter;
    iter = _ipregistry.lookup_node(changed_net);
    if (iter != _ipregistry.end()) {
	debug_msg("NRM: exact match\n");
	if (add) {
	    notify_route_changed(iter, changed_route);
	} else {
	    // Delete
	    notify_invalidated(iter);
	}
	return XORP_OK;
    }
    debug_msg("NRM: no exact match\n");

    //
    // Find the parent.
    // This is the case when a new more specific route appears.
    //
    iter = _ipregistry.find_less_specific(changed_net);
    if (iter != _ipregistry.end()) {
	debug_msg("NRM: less specific match: %s\n",
		  iter.payload()->str().c_str());
	if (add) {
	    notify_invalidated(iter);
	} else {
	    // This can't happen because registrations can't be
	    // overlapped by more specific routes.
	    XLOG_UNREACHABLE();
	}
	return XORP_OK;
    }
    debug_msg("NRM: no less specific match\n");

    //
    // Find any children.
    // Example:
    // we have routes 1.0.0.0/20,  1.0.0.0/24, and 1.0.2/24
    // use registers 1.0.1.1.
    // this maps to 1.0.1.0/24 on route 1.0.0.0/20.
    // now either of the following can happen:
    // - route 1.0.0.0/22 arrives, which needs to update the info on
    //   the registered route.
    // - route 1.0.0.0/20 is deleted.
    // in either case the registration should be invalidated and the
    // client should re-register
    //
    iter = _ipregistry.search_subtree(changed_net);
    while (iter != _ipregistry.end()) {
	debug_msg("NRM: found child\n");
	// Move the iterator on, because otherwise a deletion may invalidate it
	nextiter = iter;
	nextiter++;
	if (add) {
	    debug_msg("NRM:   add\n");
	    if (changed_net.contains(iter.payload()->valid_subnet())
		&& iter.payload()->route()->net().contains(changed_net)) {
		debug_msg("NRM:   child_matches\n");
		notify_invalidated(iter);
		matches = true;
	    }
	} else {
	    if (iter.payload()->route() != NULL
		&& iter.payload()->route()->net() == changed_net) {
		notify_invalidated(iter);
		matches = true;
	    }
	}
	iter = nextiter;
    }
    if (matches == false)
	debug_msg("NRM:  no children found\n");

    if (matches)
	return XORP_OK;
    else
	return XORP_ERROR;
}

template<class A>
int
RegisterTable<A>::add_route(const IPRouteEntry<A>& route,
			    RouteTable<A>* caller)
{
    debug_msg("RegisterTable<A>::add_route %s\n", route.str().c_str());
    debug_msg("Before:\n");
    print();
    XLOG_ASSERT(caller == _parent);

    if (_next_table != NULL)
	_next_table->add_route(route, this);

    notify_relevant_modules(true /* it's an add */, route);

    debug_msg("Add route called on register table %s\n", _tablename.c_str());
    return XORP_OK;
}

template<class A>
int
RegisterTable<A>::delete_route(const IPRouteEntry<A>* route,
			       RouteTable<A>* caller)
{
    debug_msg("[REGT]: delete_route: %p\n%s\n", route, route->str().c_str());
    debug_msg("Before:\n");
    print();
    XLOG_ASSERT(caller == _parent);

    if (_next_table != NULL)
	_next_table->delete_route(route, this);

    notify_relevant_modules(false /* it's a delete */, *route);
    debug_msg("Delete route called on register table\n");
    debug_msg("After:\n");
    print();

    return XORP_OK;
}

template<class A>
RouteRegister<A>*
RegisterTable<A>::add_registration(const IPNet<A>& net,
				   const IPRouteEntry<A>* route,
				   const string& module)
{
    const ModuleData* newmod = new ModuleData(module);
    set<const ModuleData* , ModuleCmp>::iterator mod_iter;

    //
    // Add the registered module name to the list of module names if
    // it's not there already.
    //
    mod_iter = _module_names.find(newmod);
    if (mod_iter == _module_names.end()) {
	_module_names.insert(newmod);
    } else {
	// It already existed
	delete newmod;
	newmod = *mod_iter;
    }

    // Do we have an existing registry for this subnet?
    typename Trie<A, RouteRegister<A>* >::iterator iter, next_iter;
    iter = _ipregistry.lookup_node(net);
    RouteRegister<A>* rr;
    if (iter == _ipregistry.end()) {
	// No existing registry for this subnet
	print();
	if (route != NULL) {
	    debug_msg("[REGT] Add registration for net %s "
		      "route %s module %s\n",
		      net.str().c_str(), route->str().c_str(), module.c_str());
	} else {
	    debug_msg("[REGT] Add registration for net %s, "
		      "NULL route, module %s\n",
		      net.str().c_str(), module.c_str());
	}

	//
	// We might have some old registrations for more specific
	// subsets of this subnet.  If we do, we must invalidate them
	//   now.  This might happen if the following occurs:
	//   We have two routes:  1.0.0.0/16 and 1.0.0.0/24.
	//   We have two addresses that register interest: 1.0.0.1 and 1.0.1.0
	// This causes us to create registrations:
	//       a)  1.0.0.0/24 on route 1.0.0.0/24
	//       b)  1.0.1.0/24 on route 1.0.0.0/16
	// Now the route for 1.0.0.0/24 is deleted, and (a) is invalidated.
	// 1.0.0.1 re-registers, and this entry will be created:
	//       c) 1.0.0.0/16 on 1.0.0.0/16.
	// But (b) is still registered.  So we need to detect this here,
	// and invalidate it, so it can pick up the new entry.
	//
	iter = _ipregistry.search_subtree(net);
	while (iter != _ipregistry.end()) {
	    next_iter = iter;
	    next_iter++;
	    notify_invalidated(iter);
	    iter = next_iter;
	}

	rr = new RouteRegister<A>(net, route, newmod);
	_ipregistry.insert(net, rr);
	print();
    } else {
	rr = iter.payload();
	rr->add_registrant(newmod);
    }
    debug_msg("added registration: to %p\n%s", rr, rr->str().c_str());
#ifdef DEBUG_LOGGING
    _ipregistry.print();
#endif
    debug_msg("\n");
    return rr;
}

template<class A>
int
RegisterTable<A>::delete_registration(const IPNet<A>& net,
				      const string& module)
{
    const ModuleData* tmpmod = new ModuleData(module);
    set<const ModuleData* , ModuleCmp>::iterator mod_iter;

    mod_iter = _module_names.find(tmpmod);
    if (mod_iter == _module_names.end()) {
	XLOG_ERROR("delete_registration called for unregistered module: %s",
		   module.c_str());
	return XORP_ERROR;
    }

    debug_msg("tmpmod: %p\n", tmpmod);
    delete tmpmod;
    tmpmod = *mod_iter;
    debug_msg("tmpmod2: %p\n", tmpmod);

    typename Trie<A, RouteRegister<A>* >::iterator iter;
    iter = _ipregistry.lookup_node(net);
    if (iter == _ipregistry.end()) {
	XLOG_ERROR("delete_registration called for unregisted net: %s",
		   net.str().c_str());
	return XORP_ERROR;
    }

    RouteRegister<A>* rr = iter.payload();
    debug_msg("found registration %p\n", rr);
    if (rr->delete_registrant(tmpmod) != XORP_OK) {
	fprintf(stderr, "delete_registration failed: %s\n",
		net.str().c_str());
	return XORP_ERROR;
    }
    if (rr->size() > 0) {
	debug_msg("retaining RouteRegister %p\n", rr);
#ifdef DEBUG_LOGGING
	_ipregistry.print();
#endif
	return XORP_OK;
    }

    _ipregistry.erase(net);
    debug_msg("deleting RouteRegister %p\n", rr);
    delete rr;
#ifdef DEBUG_LOGGING
    _ipregistry.print();
#endif
    return XORP_OK;
}

//
// This is the method to be called to register a route.
//
template<class A>
RouteRegister<A>*
RegisterTable<A>::register_route_range(const A& addr,
				       const string& module)
{
    debug_msg("*****\nRRR: register_route_range: %s\n", addr.str().c_str());
    RouteRange<A>* rrange;

    rrange = lookup_route_range(addr);
    IPNet<A> subnet = rrange->minimal_subnet();
    debug_msg("RRR: minimal subnet = %s\n", subnet.str().c_str());
    if (rrange->route() == NULL)
	debug_msg("RRR: no matching route\n");
    else
	debug_msg("RRR: route = %s\n", rrange->route()->str().c_str());

    RouteRegister<A>* rreg;
    rreg = add_registration(subnet, rrange->route(), module);
    return rreg;
}

//
// This is the method to be called to deregister a route.
//
template<class A>
int
RegisterTable<A>::deregister_route_range(const IPNet<A>& subnet,
					 const string& module)
{
    return delete_registration(subnet, module);
}

template<class A>
string
RegisterTable<A>::str() const
{
    string s;

    s = "-------\nRegisterTable: " + _tablename + "\n";
    s += "parent = " + _parent -> tablename() + "\n";
    if (_next_table == NULL)
	s += "no next table\n";
    else
	s += "next table = " + _next_table->tablename() + "\n";
    return s;
}

template<class A>
void
RegisterTable<A>::print()
{
#ifdef DEBUG_LOGGING
    debug_msg("%s\n", str().c_str());
    typename Trie<A, RouteRegister<A>* >::iterator iter;
    for (iter = _ipregistry.begin(); iter != _ipregistry.end(); ++iter) {
	debug_msg("----\n");
	debug_msg("%s\n", iter.payload()->str().c_str());
    }
#endif
}

template<class A>
void
RegisterTable<A>::notify_route_changed(
    typename Trie<A, RouteRegister<A>* >::iterator trie_iter,
    const IPRouteEntry<A>& changed_route)
{
    list<string> module_names = trie_iter.payload()->module_names();
    NextHop* nexthop = changed_route.nexthop();
    A nexthop_addr;
    bool resolves;

    switch (nexthop->type()) {
    case GENERIC_NEXTHOP:
	// This shouldn't be possible
	XLOG_UNREACHABLE();
    case PEER_NEXTHOP:
    case ENCAPS_NEXTHOP:
	resolves = true;
	nexthop_addr = (reinterpret_cast<IPNextHop<A>* >(nexthop))->addr();
	break;
    case EXTERNAL_NEXTHOP:
    case DISCARD_NEXTHOP:
	resolves = false;
	break;
    }
    if (resolves == false) {
	notify_invalidated(trie_iter);
    } else {
	uint32_t metric = changed_route.metric();
	uint32_t admin_distance = changed_route.admin_distance();
	const string& protocol_origin = changed_route.protocol().name();
	list<string>::const_iterator iter;
	for (iter = module_names.begin(); iter != module_names.end(); ++iter) {
	    _register_server->send_route_changed(
		*iter,
		trie_iter.payload()->valid_subnet(),
		nexthop_addr, metric, admin_distance,
		protocol_origin, _multicast);
	}
    }
}

template<class A>
void
RegisterTable<A>::notify_invalidated(typename Trie<A, RouteRegister<A>* >::iterator trie_iter)
{
    list<string> module_names = trie_iter.payload()->module_names();
    IPNet<A> valid_subnet = trie_iter.payload()->valid_subnet();

    debug_msg("notify_invalidated: %s\n", valid_subnet.str().c_str());

    list<string>::const_iterator iter;
    for (iter = module_names.begin(); iter != module_names.end(); ++iter) {
	debug_msg("we will send an invalidate to %s\n", (*iter).c_str());
	_register_server->send_invalidate(*iter, valid_subnet, _multicast);
    }
    delete trie_iter.payload();
    _ipregistry.erase(trie_iter);
}

template class RouteRegister<IPv4>;
template class RouteRegister<IPv6>;
template class RegisterTable<IPv4>;
template class RegisterTable<IPv6>;
