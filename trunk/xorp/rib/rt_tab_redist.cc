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

#ident "$XORP: xorp/rib/rt_tab_redist.cc,v 1.8 2004/02/11 08:48:49 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"

#include "rt_tab_redist.hh"


template<class A>
RedistTable<A>::RedistTable<A>(const string&   tablename,
			       RouteTable<A>*  from_table,
			       OriginTable<A>* to_table)
    : RouteTable<A>(tablename),
      _from_table(from_table),
      _to_table(to_table)
{
    // Plumb ourselves into the table graph
    set_next_table(_from_table->next_table());
    _from_table->set_next_table(this);
    if (next_table() != NULL)
	next_table()->replumb(from_table, this);
}

template<class A>
RedistTable<A>::~RedistTable<A>()
{
    // Unplumb ourselves from the table graph
    _from_table->set_next_table(next_table());
}

template<class A>
int
RedistTable<A>::add_route(const IPRouteEntry<A>& route,
			  RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _from_table);
    debug_msg("RT[%s]: Adding route %s\n",
	      tablename().c_str(), route.str().c_str());

    next_table()->add_route(route, this);
    return XORP_OK;
}

template<class A>
int
RedistTable<A>::delete_route(const IPRouteEntry<A>* route,
			     RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _from_table);
    debug_msg("RT[%s]: Delete route %s\n",
	      tablename().c_str(), route->str().c_str());

    next_table()->delete_route(route, this);
    return XORP_OK;
}

template<class A>
const IPRouteEntry<A>*
RedistTable<A>::lookup_route(const IPNet<A>& net) const
{
    return _from_table->lookup_route(net);
}

template<class A>
const IPRouteEntry<A>*
RedistTable<A>::lookup_route(const A& addr) const
{
    return _from_table->lookup_route(addr);
}

template<class A>
void
RedistTable<A>::replumb(RouteTable<A>* old_parent,
			RouteTable<A>* new_parent)
{
    XLOG_ASSERT(_from_table == old_parent);
    _from_table = new_parent;
}

template<class A>
RouteRange<A>*
RedistTable<A>::lookup_route_range(const A& addr) const
{
    return _from_table->lookup_route_range(addr);
}

template<class A>
string
RedistTable<A>::str() const
{
    string s;

    s = "-------\nRedistTable: " + tablename() + "\n";
    s += "_from_table = " + _from_table->tablename() + "\n";
    s += "_to_table = " + _to_table->tablename() + "\n";
    if (next_table() == NULL)
	s+= "no next table\n";
    else
	s+= "next table = " + next_table()->tablename() + "\n";
    return s;
}

template class RedistTable<IPv4>;
template class RedistTable<IPv6>;
