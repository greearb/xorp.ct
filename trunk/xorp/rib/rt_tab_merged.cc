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

#ident "$XORP: xorp/rib/rt_tab_merged.cc,v 1.13 2004/04/01 19:54:12 mjh Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"

#include "rt_tab_merged.hh"

template <typename A>
inline static string
make_merged_name(const RouteTable<A>* a, const RouteTable<A>* b)
{
    return string("Merged:(" + a->tablename() + ")+( " + b->tablename() + ")");
}

//
// A = Address Type. E.g., IPv4 or IPv6
//
template <class A>
MergedTable<A>::MergedTable(RouteTable<A>* table_a, RouteTable<A>* table_b)
    : RouteTable<A>(make_merged_name(table_a, table_b)),
      _table_a(table_a),
      _table_b(table_b)
{
    _table_a->set_next_table(this);
    _table_b->set_next_table(this);
    debug_msg("New Merged: %s\n", this->tablename().c_str());
}

template <class A>
int
MergedTable<A>::add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller)
{
    debug_msg("MT[%s]: Adding route %s\n", this->tablename().c_str(),
	   route.str().c_str());

    if (this->next_table() == NULL)
	return XORP_ERROR;
    RouteTable<A>* other_table;
    if (caller == _table_b) {
	other_table = _table_a;
    } else if (caller == _table_a) {
	other_table = _table_b;
    } else {
	XLOG_UNREACHABLE();
    }
    const IPRouteEntry<A>* found;
    found = other_table->lookup_route(route.net());
    if (found != NULL) {
	if (found->admin_distance() > route.admin_distance()) {
	    // The admin distance of the existing route is worse
	    this->next_table()->delete_route(found, this);
	} else {
	    return XORP_ERROR;
	}
    }
    this->next_table()->add_route(route, this);

    return XORP_OK;
}

template <class A>
int
MergedTable<A>::delete_route(const IPRouteEntry<A>* route,
			     RouteTable<A>* caller)
{
    if (this->next_table() == NULL)
	return XORP_ERROR;
    RouteTable<A>* other_table;
    if (caller == _table_b) {
	other_table = _table_a;
    } else if (caller == _table_a) {
	other_table = _table_b;
    } else {
	XLOG_UNREACHABLE();
    }
    const IPRouteEntry<A>* found;
    found = other_table->lookup_route(route->net());
    if (found != NULL) {
	if (found->admin_distance() > route->admin_distance()) {
	    // The admin distance of the existing route is worse
	    this->next_table()->delete_route(route, this);
	    this->next_table()->add_route(*found, this);
	    return XORP_OK;
	} else {
	    // The admin distance of the existing route is better, so
	    // this route would not previously have been propagated
	    // downstream.
	    return XORP_ERROR;
	}
    }
    this->next_table()->delete_route(route, this);
    return XORP_OK;
}

template <class A>
const IPRouteEntry<A>*
MergedTable<A>::lookup_route(const IPNet<A>& net) const
{
    const IPRouteEntry<A>* found_a;
    const IPRouteEntry<A>* found_b;

    found_a = _table_a->lookup_route(net);
    found_b = _table_b->lookup_route(net);

    if (found_b == NULL) {
	return found_a;
    }
    if (found_a == NULL) {
	return found_b;
    }

    if (found_a->admin_distance() <= found_b->admin_distance()) {
	return found_a;
    } else {
	return found_b;
    }
}

template <class A>
const IPRouteEntry<A>*
MergedTable<A>::lookup_route(const A& addr) const
{
    const IPRouteEntry<A>* found_a;
    const IPRouteEntry<A>* found_b;

    found_b = _table_b->lookup_route(addr);
    found_a = _table_a->lookup_route(addr);
    debug_msg("MergedTable::lookup_route.  Table_a: %p\n", _table_a);
    debug_msg("MergedTable::lookup_route.  Table_b: %p\n", _table_b);

    if (found_b == NULL) {
	return found_a;
    }
    if (found_a == NULL) {
	return found_b;
    }

    //
    // The route was in both tables.  We need to do longest match
    // unless the prefixes are the same, when we take the route with the
    // lowest admin distance.
    //
    size_t hi_prefix_len, lo_prefix_len;
    hi_prefix_len = found_b->net().prefix_len();
    lo_prefix_len = found_a->net().prefix_len();

    if (hi_prefix_len > lo_prefix_len) {
	return found_b;
    } else if (hi_prefix_len < lo_prefix_len) {
	return found_a;
    } else if (found_b->admin_distance() <= found_a->admin_distance()) {
	return found_b;
    } else {
	return found_a;
    }
}

template <class A>
void
MergedTable<A>::replumb(RouteTable<A>* old_parent,
			RouteTable<A>* new_parent)
{
    debug_msg("MergedTable::replumb replacing %s with %s\n",
	      old_parent->tablename().c_str(),
	      new_parent->tablename().c_str());

    if (_table_a == old_parent) {
	_table_a = new_parent;
    } else if (_table_b == old_parent) {
	_table_b = new_parent;
    } else {
	XLOG_UNREACHABLE();
    }
    set_tablename(make_merged_name(_table_a, _table_b));
    debug_msg("MergedTable: now called \"%s\"\n", this->tablename().c_str());
}

template<class A>
RouteRange<A>*
MergedTable<A>::lookup_route_range(const A& addr) const
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

    s = "-------\nMergedTable: " + this->tablename() + "\n";
    s += "_table_a = " + _table_a->tablename() + "\n";
    s += "_table_b = " + _table_b->tablename() + "\n";
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class MergedTable<IPv4>;
template class MergedTable<IPv6>;
