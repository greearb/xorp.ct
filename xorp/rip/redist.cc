// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
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


// ----------------------------------------------------------------------------
// RouteRedistributor

template <typename A>
RouteRedistributor<A>::RouteRedistributor(RouteDB<A>&	rdb)
    : _route_db(rdb), _wdrawer(0)
{
    _rt_origin = new RedistRouteOrigin<A>();
}

template <typename A>
RouteRedistributor<A>::~RouteRedistributor()
{
    delete _rt_origin;
    delete _wdrawer;
}

template <typename A>
bool
RouteRedistributor<A>::add_route(const Net&  	net,
				 const Addr& 	nexthop,
				 const string&	ifname,
				 const string&	vifname,
				 uint16_t 	cost,
				 uint16_t 	tag,
				 const PolicyTags& policytags)
{
    _route_db.add_rib_route(net, nexthop, ifname, vifname, cost, tag,
			    _rt_origin, policytags);
    return _route_db.update_route(net, nexthop, ifname, vifname, cost, tag,
				  _rt_origin, policytags, false);
}

template <typename A>
bool
RouteRedistributor<A>::expire_route(const Net& net)
{
    string ifname, vifname;		// XXX: not set, because not needed

    _route_db.delete_rib_route(net);
    return _route_db.update_route(net, A::ZERO(), ifname, vifname,
				  RIP_INFINITY, 0, _rt_origin, PolicyTags(),
				  false);
}

template <typename A>
uint32_t
RouteRedistributor<A>::route_count() const
{
    return _rt_origin->route_count();
}

template <typename A>
void
RouteRedistributor<A>::withdraw_routes()
{
    if (_wtimer.scheduled() == false) {
	EventLoop& e = _route_db.eventloop();
	_wtimer = e.new_periodic_ms(5,
				    callback(this, &RouteRedistributor::withdraw_batch));
    }
}

template <typename A>
bool
RouteRedistributor<A>::withdrawing_routes() const
{
    return _wtimer.scheduled();
}

template <typename A>
bool
RouteRedistributor<A>::withdraw_batch()
{
    if (_wdrawer == 0) {
	_wdrawer = new RouteWalker<A>(_route_db);
	_wdrawer->reset();
    }

    XLOG_ASSERT(_wdrawer->state() == RouteWalker<A>::STATE_RUNNING);

    uint32_t visited = 0;
    const RouteEntry<A>* r = _wdrawer->current_route();
    while (r != 0) {
	if (r->origin() == _rt_origin) {
	    _route_db.update_route(r->net(), r->nexthop(), r->ifname(),
				   r->vifname(), RIP_INFINITY, r->tag(),
				   _rt_origin, r->policytags(), false);
	}
	r = _wdrawer->next_route();

	if (++visited == 5) {
	    return true;	// we're not finished - reschedule timer
	}
    }
    delete _wdrawer;
    _wdrawer = 0;
    return false; // we're finished - cancel timer
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class RedistRouteOrigin<IPv4>;
template class RouteRedistributor<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class RouteRedistributor<IPv6>;
template class RedistRouteOrigin<IPv6>;
#endif
