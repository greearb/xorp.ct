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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"

#include "constants.hh"
#include "route_db.hh"
#include "route_entry.hh"
#include "redist.hh"

// ----------------------------------------------------------------------------
// RedistRouteOrigin

template <typename A>
uint32_t
RedistRouteOrigin<A>::expiry_secs() const
{
    return 0;
}

template <typename A>
uint32_t
RedistRouteOrigin<A>::deletion_secs() const
{
    return DEFAULT_DELETION_SECS;
}

template class RedistRouteOrigin<IPv4>;
template class RedistRouteOrigin<IPv6>;


// ----------------------------------------------------------------------------
// RouteRedistributor

template <typename A>
RouteRedistributor<A>::RouteRedistributor(RouteDB<A>&	rdb,
					  const string&	protocol,
					  uint16_t	tag)
    : _route_db(rdb), _protocol(protocol), _tag(tag), _rt_origin(0)
{
    _rt_origin = new RedistRouteOrigin<A>();
}

template <typename A>
RouteRedistributor<A>::~RouteRedistributor()
{
    XLOG_ASSERT(_rt_origin->route_count() == 0);
    delete _rt_origin;
}

template <typename A>
bool
RouteRedistributor<A>::add_route(const Net&  net,
				 const Addr& nexthop,
				 uint32_t    cost)
{
    return _route_db.update_route(net, nexthop, cost, _tag, _rt_origin);
}

template <typename A>
bool
RouteRedistributor<A>::expire_route(const Net& net)
{
    return _route_db.update_route(net, A::ZERO(), RIP_INFINITY,
				  _tag, _rt_origin);
}

template <typename A>
const string&
RouteRedistributor<A>::protocol() const
{
    return _protocol;
}

template <typename A>
uint16_t
RouteRedistributor<A>::tag() const
{
    return _tag;
}

template class RouteRedistributor<IPv4>;
template class RouteRedistributor<IPv6>;
