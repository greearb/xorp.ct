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

#ident "$XORP: xorp/rip/route_db.cc,v 1.2 2003/07/08 16:57:20 hodson Exp $"

#include "rip_module.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "constants.hh"
#include "route_db.hh"
#include "update_queue.hh"

template <typename A>
RouteDB<A>::RouteDB(EventLoop& e) : _eventloop(e)
{
    _uq = new UpdateQueue<A>();
}

template <typename A>
RouteDB<A>::~RouteDB()
{
    _routes.delete_all_nodes();
    XLOG_ASSERT(_routes.route_count() == 0);
    delete _uq;
}

template <typename A>
void
RouteDB<A>::delete_route(Route* r)
{
    typename RouteTrie::iterator i = _routes.lookup_node(r->net());
    if (i == _routes.end()) {
	// Libxorp is bjorkfest if this happens...
	XLOG_ERROR("Route for %s missing when deletion came.",
		   r->net().str().c_str());
	return;
    }
    i.payload() = 0;
    _routes.erase(i);
}

template <typename A>
void
RouteDB<A>::set_deletion_timer(Route* r)
{
    RouteOrigin* o = r->origin();
    uint32_t deletion_ms = o->deletion_secs() * 1000;

    XorpTimer t = _eventloop.new_oneoff_after_ms(deletion_ms,
				callback(this, &RouteDB<A>::delete_route, r));

    r->set_timer(t);
}

template <typename A>
void
RouteDB<A>::expire_route(Route* r)
{
    if (false == update_route(r->net(), r->nexthop(), RIP_INFINITY, r->tag(),
			      r->origin())) {
	XLOG_ERROR("Expire route failed.");
    }
}

template <typename A>
void
RouteDB<A>::set_expiry_timer(Route* r)
{
    RouteOrigin* o = r->origin();

    XorpTimer t;
    uint32_t expire_ms = o->expiry_secs() * 1000;

    if (expire_ms) {
	t = _eventloop.new_oneoff_after_ms(expire_ms,
			   callback(this, &RouteDB<A>::expire_route, r));
    }
    r->set_timer(t);
}

template <typename A>
bool
RouteDB<A>::update_route(const Net&	net,
			 const Addr&	nexthop,
			 uint32_t	cost,
			 uint32_t	tag,
			 RouteOrigin*	o)
{
    if (tag > 0xffff) {
	// Ingress sanity checks should take care of this
	XLOG_FATAL("Invalid tag (%d) when updating route.", tag);
	return false;
    }

    if (cost > RIP_INFINITY) {
	cost = RIP_INFINITY;
    }

    //
    // Update steps, based on RFC2453 pp. 26-28
    //
    typename RouteTrie::iterator i = _routes.lookup_node(net);
    if (_routes.end() == i) {
	if (cost == RIP_INFINITY) {
	    // Don't bother adding a route for unreachable net
	    return false;
	}
	DBRouteEntry dre(new Route(net, nexthop, cost, o, tag));
	set_expiry_timer(dre.get());
	_routes.insert(net, dre);
	_uq->push_back(dre);
	return true;
    } else {
	bool updated = false;
	if (i.payload()->origin() == o) {
	    updated |= i.payload()->set_nexthop(nexthop);
	    updated |= i.payload()->set_tag(tag);
	    bool updated_cost = i.payload()->set_cost(cost);
	    if (cost == RIP_INFINITY) {
		set_deletion_timer(i.payload().get());
	    } else {
		set_expiry_timer(i.payload().get());
	    }
	    updated |= updated_cost;
	} else if (i.payload()->cost() > cost) {
	    i.payload()->set_nexthop(nexthop);
	    i.payload()->set_tag(tag);
	    i.payload()->set_cost(cost);
	    i.payload()->set_origin(o);
	    set_expiry_timer(i.payload().get());
	    updated = true;
	}
	if (updated) {
	    _uq->push_back(i.payload());
	}
	return updated;
    }

    return false;
}

template <typename A>
void
RouteDB<A>::dump_routes(vector<ConstDBRouteEntry>& routes)
{
    typename RouteTrie::iterator i = _routes.begin();
    while (i != _routes.end()) {
	if (i.has_payload())
	    routes.push_back(i.payload());
	++i;
    }
}

template <typename A>
UpdateQueue<A>&
RouteDB<A>::update_queue()
{
    return *_uq;
}

template <typename A>
const UpdateQueue<A>&
RouteDB<A>::update_queue() const
{
    return *_uq;
}

template class RouteDB<IPv4>;
template class RouteDB<IPv6>;
