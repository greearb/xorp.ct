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

#ident "$XORP: xorp/rib/rt_tab_export.cc,v 1.14 2004/03/25 01:45:09 hodson Exp $"

#include "rib_module.h"
#include "libxorp/xlog.h"
#include "rib_client.hh"
#include "rt_tab_export.hh"


template<class A>
ExportTable<A>::ExportTable<A>(const string&		tablename,
			       RouteTable<A>*		parent,
			       list<RibClient* >*	rib_clients)
    : RouteTable<A>(tablename),
      _parent(parent),
      _rib_clients(rib_clients)
{
    // Plumb ourselves into the table graph
    if (_parent != NULL) {
	XLOG_ASSERT(_parent->next_table() == NULL);
	_parent->set_next_table(this);
    }
}

template<class A>
ExportTable<A>::~ExportTable()
{
}

template<class A>
int
ExportTable<A>::add_route(const IPRouteEntry<A>& route,
			  RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    // TODO: XXX: hard-coded protocol name!
    if (route.protocol().name() == "connected") {
	debug_msg("Add route called for connected route\n");
	return XORP_OK;
    }

    //
    // Add the route to all RIB clients
    //
    list<RibClient* >::iterator iter;
    for (iter = _rib_clients->begin(); iter != _rib_clients->end(); ++iter) {
	RibClient* rib_client = *iter;
	rib_client->add_route(route);
    }

    debug_msg("Add route called on export table %s\n", 
	      this->tablename().c_str());
    return XORP_OK;
}

template<class A>
int
ExportTable<A>::delete_route(const IPRouteEntry<A>* route,
			     RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    // TODO: XXX: hard-coded protocol name!
    if (route->protocol().name() == "connected") {
	debug_msg("Delete route called for connected route\n");
	return XORP_OK;
    }

    //
    // Delete the route from all RIB clients
    //
    list<RibClient* >::iterator iter;
    for (iter = _rib_clients->begin(); iter != _rib_clients->end(); ++iter) {
	RibClient* rib_client = *iter;
	rib_client->delete_route(*route);
    }

    debug_msg("Delete route called on export table\n");
    return XORP_OK;
}

template<class A>
void
ExportTable<A>::flush()
{
    // TODO: XXX: NOT IMPLEMENTED!!!
    debug_msg("Flush called on ExportTable\n");
}

template<class A>
const IPRouteEntry<A>*
ExportTable<A>::lookup_route(const IPNet<A>& net) const
{
    return _parent->lookup_route(net);
}

template<class A>
const IPRouteEntry<A>*
ExportTable<A>::lookup_route(const A& addr) const
{
    return _parent->lookup_route(addr);
}

template<class A>
void
ExportTable<A>::replumb(RouteTable<A>* old_parent,
			RouteTable<A>* new_parent)
{
    debug_msg("ExportTable::replumb\n");
    if (_parent == old_parent) {
	_parent = new_parent;
    } else {
	// Shouldn't be possible
	XLOG_UNREACHABLE();
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
    s = "-------\nExportTable: " + this->tablename() + "\n";
    s += "parent = " + _parent->tablename() + "\n";
    if (this->next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + this->next_table()->tablename() + "\n";
    return s;
}

template class ExportTable<IPv4>;
template class ExportTable<IPv6>;
