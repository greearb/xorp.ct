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

#ident "$XORP: xorp/rib/rt_tab_export.cc,v 1.4 2003/03/15 02:28:38 pavlin Exp $"

#include "rib_module.h"
#include "fea_client.hh"
#include "rt_tab_export.hh"

#define DEBUG_ROUTE_TABLE

#define codepath(x) printf("ExTabCodePath: %d\n", x);

template<class A>
ExportTable<A>::ExportTable<A>(const string&  tablename,
			       RouteTable<A>* parent,
			       FeaClient*     fea)
    : RouteTable<A>(tablename)
{
    _parent = parent;
    _fea = fea;

    // plumb ourselves into the table graph
    if (_parent != NULL) {
	if (_parent->next_table() != NULL)
	    abort();

	_parent->set_next_table(this);
    }
}

template<class A>
ExportTable<A>::~ExportTable<A>() {
}

template<class A>
int
ExportTable<A>::add_route(const IPRouteEntry<A>& route,
			  RouteTable<A> *caller) 
{
    if (caller != _parent)
	abort();

    if (route.protocol().name() == "connected") {
	printf("Add route called for connected route\n");
	return 0;
    }
    _fea->add_route(route);

    debug_msg(("Add route called on export table " + _tablename +
	       "\n").c_str());
    return 0;
}

template<class A>
int
ExportTable<A>::delete_route(const IPRouteEntry<A> *route,
			     RouteTable<A> *caller) 
{
    if (caller != _parent)
	abort();
    if (route->protocol().name() == "connected") {
	printf("Delete route called for connected route\n");
	return 0;
    }
    _fea->delete_route(*route);

    debug_msg("Delete route called on export table\n");
    return 0;
}

template<class A>
void
ExportTable<A>::flush() 
{
    // XXXX TBD
    debug_msg("Flush called on ExportTable\n");
}

template<class A>
const IPRouteEntry<A> *
ExportTable<A>::lookup_route(const IPNet<A>& net) const 
{
    return _parent->lookup_route(net);
}

template<class A>
const IPRouteEntry<A> *
ExportTable<A>::lookup_route(const A& addr) const 
{
    return _parent->lookup_route(addr);
}

template<class A>
void
ExportTable<A>::replumb(RouteTable<A> *old_parent,
			RouteTable<A> *new_parent) 
{
    debug_msg("ExportTable::replumb\n");
    if (_parent == old_parent) {
	_parent = new_parent;
    } else {
	// shouldn't be possible
	abort();
    }
}

template<class A>
RouteRange<A>*
ExportTable<A>::lookup_route_range(const A& addr) const 
{
    return _parent->lookup_route_range(addr);
}

template<class A> string
ExportTable<A>::str() const 
{
    string s;
    s = "-------\nExportTable: " + _tablename + "\n";
    s += "parent = " + _parent -> tablename() + "\n";
    if (_next_table == NULL)
	s += "no next table\n";
    else
	s += "next table = " + _next_table->tablename() + "\n";
    return s;
}

template class ExportTable<IPv4>;
template class ExportTable<IPv6>;
