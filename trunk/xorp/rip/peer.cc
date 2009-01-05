// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rip/peer.cc,v 1.11 2008/10/02 21:58:16 bms Exp $"

#include "peer.hh"
#include "port.hh"
#include "system.hh"

template <typename A>
uint32_t
PeerRoutes<A>::expiry_secs() const
{
    return _peer.expiry_secs();
}

template <typename A>
uint32_t
PeerRoutes<A>::deletion_secs() const
{
    return _peer.deletion_secs();
}

template <typename A>
Peer<A>::Peer(RipPort& p, const Addr& addr)
    : RouteEntryOrigin<A>(false), _port(p), _addr(addr),
      _peer_routes(*this)
{
    RouteDB<A>& rdb = _port.port_manager().system().route_db();
    rdb.insert_peer(this);
}

template <typename A>
Peer<A>::~Peer()
{
    RouteDB<A>& rdb = _port.port_manager().system().route_db();
    rdb.erase_peer(this);

    //
    // XXX: explicitly remove all peer routes, because we don't use them
    // as an indication whether the peer has expired.
    //
    _peer_routes.clear();
}

template <typename A>
uint32_t
Peer<A>::expiry_secs() const
{
    return port().constants().expiry_secs();
}

template <typename A>
uint32_t
Peer<A>::deletion_secs() const
{
    return port().constants().deletion_secs();
}

template <typename A>
bool
Peer<A>::update_route(const IPNet<A>&	net,
		      const A&		nexthop,
		      uint32_t		cost,
		      uint32_t		tag,
		      const PolicyTags& policytags)
{
    string ifname, vifname;

    // Get the interface and vif name
    if (port().io_handler() != NULL) {
	ifname = port().io_handler()->ifname();
	vifname = port().io_handler()->vifname();
    }

    Route* route = _peer_routes.find_route(net);
    if (route == NULL) {
	RouteEntryOrigin<A>* origin = &_peer_routes;
	route = new Route(net, nexthop, ifname, vifname,
			  cost, origin, tag, policytags);
    }
    set_expiry_timer(route);

    RouteDB<A>& rdb = _port.port_manager().system().route_db();
    return (rdb.update_route(net, nexthop, ifname, vifname, cost, tag, this,
			     policytags, false));
}

template <typename A>
void
Peer<A>::push_routes()
{
    RouteDB<A>& rdb = _port.port_manager().system().route_db();
    vector<const RouteEntry<A>*> routes;

    if (! port().enabled())
	return;			// XXX: push routes only for enabled ports

    _peer_routes.dump_routes(routes);
    typename vector<const RouteEntry<A>*>::const_iterator ri;
    for (ri = routes.begin(); ri != routes.end(); ++ri) {
	const RouteEntry<A>* r = *ri;
	rdb.update_route(r->net(), r->nexthop(), r->ifname(), r->vifname(),
			 r->cost(), r->tag(), this, r->policytags(), true);
    }
}

template <typename A>
void
Peer<A>::set_expiry_timer(Route* route)
{
    XorpTimer t;
    uint32_t secs = expiry_secs();
    EventLoop& eventloop = _port.port_manager().eventloop();

    if (secs) {
        t = eventloop.new_oneoff_after_ms(secs * 1000,
					  callback(this, &Peer<A>::expire_route, route));
    }
    route->set_timer(t);
}

template <typename A>
void
Peer<A>::expire_route(Route* route)
{
    delete route;
}



// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class Peer<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class Peer<IPv6>;
#endif
