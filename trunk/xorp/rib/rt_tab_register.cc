// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/rib/rt_tab_register.cc,v 1.2 2002/12/13 20:01:05 mjh Exp $"

//#define DEBUG_LOGGING
#include "rt_tab_register.hh"
#include "register_server.hh"

template<class A>
bool
RouteRegister<A>::delete_registrant(const ModuleData *module)
{
    debug_msg("delete_registrant: this: %p, Module: %s\n", this,
	   module->str().c_str());
    set<const ModuleData*, ModuleCmp>::iterator mod_iter;
    mod_iter = _modules.find(module);

    if (mod_iter == _modules.end()) {
	return false;
    }
    _modules.erase(mod_iter);
    debug_msg("new Value:\n%s\n", str().c_str());
    return true;
}

template<class A> string
RouteRegister<A>::str() const
{
    string s;
    s += "RR***********************\nRR RouteRegister: " + _valid_subnet.str() + "\n";
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
bool
RegisterTable<A>::find_matches(const IPRouteEntry<A> &route)
{
    bool matches = false;

    // note that the _ipregistry trie contains no overlapping routes,
    // so if we find an exact match or a less specific match then we're
    // done.

    debug_msg("FM: %s\n", route.net().str().c_str());
    // find any exact matches
    typename Trie<A, RouteRegister<A>*>::iterator iter;
    iter = _ipregistry.lookup_node(route.net());
    if (iter != _ipregistry.end()) {
	debug_msg("FM: exact match\n");
	iter.payload()->mark_modules();
	return true;
    }
    debug_msg("FM: no exact match\n");

    // find the parent
    // this is the case when a new more specific route appears
    iter = _ipregistry.find_less_specific(route.net());
    if (iter != _ipregistry.end()) {
	debug_msg("FM: less specific match\n");
	iter.payload()->mark_modules();
	return true;
    }
    debug_msg("FM: no less specific match\n");

    // find any children.
    iter = _ipregistry.search_subtree(route.net());
    while (iter != _ipregistry.end()) {
	debug_msg("FM: found child\n");
	iter.payload()->mark_modules();
	matches = true;
	iter++;
    }
    if (matches == false)
	debug_msg("FM:  no children found\n");
    return matches;
}

template<class A>
bool
RegisterTable<A>::notify_relevant_modules(bool add,
					  const IPRouteEntry<A> &changed_route)
{
    bool matches = false;
    IPNet<A> changed_net = changed_route.net();

    // note that the _ipregistry trie contains no overlapping routes,
    // so if we find an exact match or a less specific match then we're
    // done.

    debug_msg("NRM: %s\n", changed_net.str().c_str());
    // find any exact matches
    typename Trie<A, RouteRegister<A>*>::iterator iter, nextiter;
    iter = _ipregistry.lookup_node(changed_net);
    if (iter != _ipregistry.end()) {
	debug_msg("NRM: exact match\n");
	if (add)
	    notify_route_changed(iter, changed_route);
	else {
	    // delete
	    notify_invalidated(iter);
	}
	return true;
    }
    debug_msg("NRM: no exact match\n");

    // find the parent
    // this is the case when a new more specific route appears
    iter = _ipregistry.find_less_specific(changed_net);
    if (iter != _ipregistry.end()) {
	debug_msg("NRM: less specific match: %s\n", iter.payload()->str().c_str());
	if (add)
	    notify_invalidated(iter);
	else {
	    /*this can't happen because registrations can't be
              overlapped by more specific routes*/
	    abort();
	}
	return true;
    }
    debug_msg("NRM: no less specific match\n");

    // find any children.
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
    iter = _ipregistry.search_subtree(changed_net);
    while (iter != _ipregistry.end()) {
	debug_msg("NRM: found child\n");
	// move the iterator on, because otherwise a deletion may invalidate it
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
	    if (iter.payload()->route()->net() == changed_net)
		notify_invalidated(iter);
		matches = true;
	}
	iter = nextiter;
    }
    if (matches == false)
	debug_msg("NRM:  no children found\n");
    return matches;
}

template<class A>
int
RegisterTable<A>::add_route(const IPRouteEntry<A> &route,
			    RouteTable<A> *caller)
{
    debug_msg("RegisterTable<A>::add_route %s\n", route.str().c_str());
    print();
    if (caller != _parent)
	abort();
    if (_next_table != NULL)
	_next_table->add_route(route, this);

    notify_relevant_modules(true/*it's an add*/, route);


    debug_msg("Add route called on register table %s\n", _tablename.c_str());
    return 0;
}

template<class A>
int
RegisterTable<A>::delete_route(const IPRouteEntry<A> *route,
			       RouteTable<A> *caller)
{
    debug_msg("[REGT]: delete_route: %p\n%s\n", route, route->str().c_str());
    debug_msg("Before:\n");
    print();
    if (caller != _parent)
	abort();
    if (_next_table != NULL)
	_next_table->delete_route(route, this);

    notify_relevant_modules(false/*it's a delete*/, *route);
    debug_msg("Delete route called on register table\n");
    debug_msg("After:\n");
    print();
    return 0;
}

template<class A>
RouteRegister<A> *
RegisterTable<A>::add_registration(const IPNet<A> &net,
				   const IPRouteEntry<A> *route,
				   const string &module)
{
    /*add the registered module name to the list of module names if
      it's not there already*/
    const ModuleData *newmod = new ModuleData(module);
    set<const ModuleData*, ModuleCmp>::iterator mod_iter;
    mod_iter = _module_names.find(newmod);
    if (mod_iter == _module_names.end()) {
	_module_names.insert(newmod);
    } else {
	/* it already existed*/
	delete newmod;
	newmod = *mod_iter;
    }

    /*do we have an existing registry for this subnet?*/
    typename Trie<A, RouteRegister<A>*>::iterator iter, next_iter;
    iter = _ipregistry.lookup_node(net);
    RouteRegister<A> *rr;
    if (iter == _ipregistry.end()) {
	// no existing registry for this subnet
	print();
	if (route != NULL)
	    debug_msg("[REGT] Add registration for net %s, \nroute %s\nmodule %s\n",
		   net.str().c_str(), route->str().c_str(), module.c_str());
	else
	    debug_msg("[REGT] Add registration for net %s, NULL route, module %s\n",
		   net.str().c_str(), module.c_str());

	/* We might have some old registrations for more specific
           subsets of this subnet.  If we do, we must invalidate them
           now.  This might happen if the following occurs:
             We have two routes:  1.0.0.0/16 and 1.0.0.0/24.
             We have two addresses that register interest: 1.0.0.1 and 1.0.1.0
           This causes us to create registrations:
                 a)  1.0.0.0/24 on route 1.0.0.0/24
                 b)  1.0.1.0/24 on route 1.0.0.0/16
           Now the route for 1.0.0.0/24 is deleted, and (a) is invalidated.
           1.0.0.1 re-registers, and this entry will be created:
                 c) 1.0.0.0/16 on 1.0.0.0/16.
           But (b) is still registered.  So we need to detect this here,
	   and invalidate it, so it can pick up the new entry. */
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
bool
RegisterTable<A>::delete_registration(const IPNet<A> &net,
				      const string &module)
{
    const ModuleData *tmpmod = new ModuleData(module);
    set<const ModuleData*, ModuleCmp>::iterator mod_iter;
    mod_iter = _module_names.find(tmpmod);
    if (mod_iter == _module_names.end()) {
	fprintf(stderr,
		"delete_registration called for unregistered module: %s\n",
		module.c_str());
	return false;
    }
    debug_msg("tmpmod: %p\n", tmpmod);
    delete tmpmod;
    tmpmod = *mod_iter;
    debug_msg("tmpmod2: %p\n", tmpmod);

    typename Trie<A, RouteRegister<A>*>::iterator iter;
    iter = _ipregistry.lookup_node(net);
    if (iter == _ipregistry.end()) {
	fprintf(stderr, "delete_registration called for unregisted net: %s\n",
		net.str().c_str());
	return false;
    }
    RouteRegister<A> *rr = iter.payload();
    debug_msg("found registration %p\n", rr);
    if (rr->delete_registrant(tmpmod) == false) {
	fprintf(stderr, "delete_registration failed: %s\n",
		net.str().c_str());
	return false;
    }
    if (rr->size() > 0) {
	debug_msg("retaining RouteRegister %p\n", rr);
#ifdef DEBUG_LOGGING
	_ipregistry.print();
#endif
	return true;;
    }

    _ipregistry.erase(net);
    debug_msg("deleting RouteRegister %p\n", rr);
    delete rr;
#ifdef DEBUG_LOGGING
    _ipregistry.print();
#endif
    return true;
}

//this is the method to be called to register a route
template<class A>
RouteRegister<A>*
RegisterTable<A>::register_route_range(const A &addr,
				       const string &module)
{
    debug_msg("*****\nRRR: register_route_range: %s\n", addr.str().c_str());
    RouteRange<A> *rrange;
    rrange = lookup_route_range(addr);
    IPNet<A> subnet = rrange->minimal_subnet();
    debug_msg("RRR: minimal subnet = %s\n", subnet.str().c_str());
    if (rrange->route() == NULL)
	debug_msg("RRR: no matching route\n");
    else
	debug_msg("RRR: route = %s\n", rrange->route()->str().c_str());

    RouteRegister<A> * rreg;
    rreg = add_registration(subnet, rrange->route(), module);
    return rreg;
}

//this is the method to be called to deregister a route
template<class A>
bool
RegisterTable<A>::deregister_route_range(const IPNet<A> subnet,
					 const string &module)
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
    printf("%s\n", str().c_str());
    Trie<A, RouteRegister<A>*>::iterator i;
    for (i = _ipregistry.begin(); i != _ipregistry.end(); i++) {
	printf("----\n");
	printf("%s\n", i.payload()->str().c_str());
    }
#endif
}

template<class A>
void
RegisterTable<A>::notify_route_changed(
    typename Trie<A, RouteRegister<A>*>::iterator iter,
    const IPRouteEntry<A> &changed_route)
{
    list <string> module_names = iter.payload()->module_names();
    list <string>::const_iterator i;
    NextHop *nh = changed_route.nexthop();
    A nexthop;
    bool resolves;
    switch (nh->type()) {
    case GENERIC_NEXTHOP:
	// this shouldn't be possible
	abort();
    case PEER_NEXTHOP:
    case ENCAPS_NEXTHOP:
	resolves = true;
	nexthop = ((IPNextHop<A>*)nh)->addr();
	break;
    case EXTERNAL_NEXTHOP:
    case DISCARD_NEXTHOP:
	resolves = false;
	break;
    }
    if (resolves == false) {
	notify_invalidated(iter);
    } else {
	uint32_t metric = changed_route.global_metric();
	for (i = module_names.begin(); i != module_names.end(); i++) {
	    _register_server
		->send_route_changed(*i,
				     iter.payload()->valid_subnet(),
				     nexthop, metric, _mcast);
	}
    }
}

template<class A>
void
RegisterTable<A>::notify_invalidated(typename Trie<A, RouteRegister<A>*>::iterator iter)
{
    list <string> module_names = iter.payload()->module_names();
    list <string>::const_iterator i;
    IPNet<A> valid_subnet = iter.payload()->valid_subnet();
    debug_msg("notify_invalidated: %s\n", valid_subnet.str().c_str());
    for (i = module_names.begin(); i != module_names.end(); i++) {
	debug_msg("we will send an invalidate to %s\n", (*i).c_str());
	_register_server->send_invalidate(*i, valid_subnet, _mcast);
    }
    delete iter.payload();
    _ipregistry.erase(iter);
}

template class RouteRegister<IPv4>;
template class RouteRegister<IPv6>;
template class RegisterTable<IPv4>;
template class RegisterTable<IPv6>;
