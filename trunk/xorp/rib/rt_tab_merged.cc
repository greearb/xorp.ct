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

#ident "$XORP: xorp/rib/rt_tab_merged.cc,v 1.17 2002/12/10 06:56:09 mjh Exp $"

#include "urib_module.h"
#include "rt_tab_merged.hh"

//A = Address Type

#ifdef DEBUG_CODEPATH
#define cp(x) if (x < 10) printf("MTabCodePath:  %d\n", x); else printf("MTabCodePath: %d\n", x);
#else
#define cp(x) {}
#endif

template <class A>
MergedTable<A>::MergedTable<A>(const string&  tablename,
			       RouteTable<A>* table_a,
			       RouteTable<A>* table_b)
    : RouteTable<A>(tablename)
{
    cp(1);
    _table_a = table_a;
    _table_b = table_b;
    _table_a->set_next_table(this);
    _table_b->set_next_table(this);
}


template <class A>
int
MergedTable<A>::
add_route(const IPRouteEntry<A> &route, RouteTable<A> *caller)
{
    debug_msg("MT[%s]: Adding route %s\n", _tablename.c_str(),
	   route.str().c_str());

    cp(2);
    if (_next_table == NULL) return -1;
    cp(3);
    cp(4);
    RouteTable<A> *other_table;
    if (caller == _table_b) {
	cp(5);
	other_table = _table_a;
    } else if (caller == _table_a) {
	cp(6);
	other_table = _table_b;
    } else {
	abort();
    }
    const IPRouteEntry<A> *found;
    found = other_table->lookup_route(route.net());
    if (found != NULL) {
	if (found->admin_distance() > route.admin_distance()) {
	    // the admin distance of the existing route is worse
	    cp(7);
	    _next_table->delete_route(found, this);
	} else {
	    cp(8);
	    return -1;
	}
    }
    cp(9);
    _next_table->add_route(route, this);

    return 0;
}


template <class A>
int
MergedTable<A>::delete_route(const IPRouteEntry<A> *route,
			       RouteTable<A> *caller) 
{
    if (_next_table == NULL) return -1;
    RouteTable<A> *other_table;
    cp(10);
    if (caller == _table_b) {
	cp(11);
	other_table = _table_a;
    } else if (caller == _table_a) {
	cp(12);
	other_table = _table_b;
    } else {
	abort();
    }
    const IPRouteEntry<A> *found;
    found = other_table->lookup_route(route->net());
    if (found != NULL) {
	if (found->admin_distance() > route->admin_distance()) {
	    // the admin distance of the existing route is worse
	    cp(13);
	    _next_table->delete_route(route, this);
	    _next_table->add_route(*found, this);
	    return 0;
	} else {
	    // the admin distance of the existing route is better, so
	    // this route would not previously have been propagated
	    // downstream
	    cp(14);
	    return -1;
	}
    }
    cp(15);
    _next_table->delete_route(route, this);
    return 0;
}


template <class A>
const IPRouteEntry<A> *
MergedTable<A>::lookup_route(const IPNet<A> &net) const 
{
    const IPRouteEntry<A> *found_a, *found_b;
    found_a = _table_a->lookup_route(net);
    found_b = _table_b->lookup_route(net);

    if (found_b == NULL) {
	cp(16);
	return found_a;
    }
    if (found_a == NULL) {
	cp(17);
	return found_b;
    }

    if (found_a->admin_distance() <= found_b->admin_distance()) {
	cp(18);
	return found_a;
    } else {
	cp(19);
	return found_b;
    }
}


template <class A>
const IPRouteEntry<A> *
MergedTable<A>::lookup_route(const A &addr) const 
{
    const IPRouteEntry<A> *found_b, *found_a;

    debug_msg("MergedTable::lookup_route\n");
    found_b = _table_b->lookup_route(addr);
    found_a = _table_a->lookup_route(addr);
    if (found_b == NULL) {
	cp(20);
	return found_a;
    }
    if (found_a == NULL) {
	cp(21);
	return found_b;
    }

    // The route was in both tables.  We need to do longest match
    // unless the prefixes are the same, when we take the route with the
    // lowest admin distance
    int hi_prefix, lo_prefix;
    hi_prefix = found_b->net().prefix_len();
    lo_prefix = found_a->net().prefix_len();

    if (hi_prefix > lo_prefix) {
	cp(22);
	return found_b;
    } else if (hi_prefix < lo_prefix) {
	cp(23);
	return found_a;
    } else if (found_b->admin_distance() <= found_a->admin_distance()) {
	cp(24);
	return found_b;
    } else {
	cp(25);
	return found_a;
    }

}


template <class A>
void
MergedTable<A>::replumb(RouteTable<A> *old_parent,
			RouteTable<A> *new_parent)
{
    debug_msg(("MergedTable::replumb replacing " + old_parent->tablename()
	       + " with " + new_parent->tablename() + "\n").c_str());
    if (_table_a == old_parent) {
	cp(26);
	_table_a = new_parent;
    } else if (_table_b == old_parent) {
	cp(27);
	_table_b = new_parent;
    } else {
	/*shouldn't be possible*/
	abort();
    }
}

template<class A>
RouteRange<A>*
MergedTable<A>::lookup_route_range(const A &addr) const 
{
    RouteRange<A>* lo_rr = _table_a->lookup_route_range(addr);
    RouteRange<A>* hi_rr = _table_b->lookup_route_range(addr);
    hi_rr->merge(lo_rr);
    delete lo_rr;
    return hi_rr;
}

template <class A> string
MergedTable<A>::str() const 
{
    string s;
    s = "-------\nMergedTable: " + _tablename + "\n";
    s += "_table_a = " + _table_a -> tablename() + "\n";
    s += "_table_b = " + _table_b -> tablename() + "\n";
    if (_next_table == NULL)
	s += "no next table\n";
    else
	s += "next table = " + _next_table->tablename() + "\n";
    return s;
}

template class MergedTable<IPv4>;
template class MergedTable<IPv6>;
