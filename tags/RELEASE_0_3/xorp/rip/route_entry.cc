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

#ident "$XORP: xorp/rip/route_entry.cc,v 1.1 2003/04/10 00:27:43 hodson Exp $"

#include "rip_module.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "route_entry.hh"

template <typename A>
inline void
RouteEntry<A>::dissociate()
{
    RouteEntryOrigin<A>* o = _origin;
    _origin = 0;
    if (o) {
	o->dissociate(this);
    }
}

template <typename A>
inline void
RouteEntry<A>::associate(Origin* o)
{
    if (o)
	o->associate(this);
    _origin = o;
}

template <typename A>
RouteEntry<A>::RouteEntry(const Net&	n,
			  const Addr&	nh,
			  uint32_t	cost,
			  Origin*&	o,
			  uint32_t	tag)
    : _net(n), _nh(nh), _cost(cost), _tag(tag)
{
    associate(o);
}

template <typename A>
RouteEntry<A>::~RouteEntry()
{
    Origin* o = _origin;
    _origin = 0;
    if (o) {
	o->dissociate(this);
    }
}

template <typename A>
bool
RouteEntry<A>::set_nexthop(const A& nh)
{
    if (nh != _nh) {
	_nh = nh;
	return true;
    }
    return false;
}

template <typename A>
bool
RouteEntry<A>::set_cost(uint32_t cost)
{
    if (cost != _cost) {
	_cost = cost;
	return true;
    }
    return false;
}

template <typename A>
bool
RouteEntry<A>::set_origin(Origin* o)
{
    if (o != _origin) {
	dissociate();
	associate(o);
	return true;
    }
    return false;
}

template <typename A>
bool
RouteEntry<A>::set_tag(uint16_t tag)
{
    if (tag != _tag) {
	_tag = tag;
	return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// RouteEntryOrigin<A>::RouteEntryStore and related
//
// RouteEntryStore is a private class that is used by Route Origin's to store
// their associated routes.
//

#include <set>

template <typename A>
struct RouteEntryOrigin<A>::RouteEntryStore {
public:
    typedef set<const RouteEntry<A>*, RtPtrComparitor<A> > Container;
    Container routes;
};

// ----------------------------------------------------------------------------
// RouteEntryOrigin

template <typename A>
RouteEntryOrigin<A>::RouteEntryOrigin()
{
    _rtstore = new RouteEntryStore();
}

template <typename A>
RouteEntryOrigin<A>::~RouteEntryOrigin()
{
    // XXX store should be empty
    XLOG_ASSERT(_rtstore->routes.empty());
    delete _rtstore;
}

template <typename A>
bool
RouteEntryOrigin<A>::associate(const Route* r)
{
    XLOG_ASSERT(r != 0);
    if (_rtstore->routes.find(r) != _rtstore->routes.end()) {
	XLOG_FATAL("entry already exists");
	return false;
    }
    _rtstore->routes.insert(r);
    return true;
}

template <typename A>
bool
RouteEntryOrigin<A>::dissociate(const Route* r)
{
    typename RouteEntryStore::Container::iterator i = _rtstore->routes.find(r);
    if (i == _rtstore->routes.end()) {
	XLOG_FATAL("entry does not exist");
	return false;
    }
    _rtstore->routes.erase(i);
    return true;
}

template <typename A>
size_t
RouteEntryOrigin<A>::route_count() const
{
    return _rtstore->routes.size();
}

template <typename A>
void
RouteEntryOrigin<A>::dump_routes(vector<const Route*>& routes) const
{
    typename RouteEntryStore::Container::const_iterator
	i = _rtstore->routes.begin();
    typename RouteEntryStore::Container::const_iterator
	end = _rtstore->routes.end();

    while (i != end) {
	routes.push_back(*i);
	++i;
    }
}

template class RouteEntryOrigin<IPv4>;
template class RouteEntry<IPv4>;

template class RouteEntryOrigin<IPv6>;
template class RouteEntry<IPv6>;
