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

#ident "$XORP: xorp/rip/route_db.cc,v 1.11 2004/02/12 17:58:11 hodson Exp $"

#include "config.h"
#include <map>

#include "rip_module.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "constants.hh"
#include "route_db.hh"
#include "update_queue.hh"

// ----------------------------------------------------------------------------
// NetCmp

template <typename A>
bool
NetCmp<A>::operator() (const IPNet<A>& l, const IPNet<A>& r) const
{
    if (l.prefix_len() < r.prefix_len())
	return true;
    if (l.prefix_len() > r.prefix_len())
	return false;
    return l.masked_addr() < r.masked_addr();
}


// ----------------------------------------------------------------------------
// RouteDB

template <typename A>
RouteDB<A>::RouteDB(EventLoop& e) : _eventloop(e)
{
    _uq = new UpdateQueue<A>();
}

template <typename A>
RouteDB<A>::~RouteDB()
{
    _routes.erase(_routes.begin(), _routes.end());
    delete _uq;
}

template <typename A>
void
RouteDB<A>::delete_route(Route* r)
{
    typename RouteContainer::iterator i = _routes.find(r->net());
    if (i == _routes.end()) {
	// Libxorp is bjorkfest if this happens...
	XLOG_ERROR("Route for %s missing when deletion came.",
		   r->net().str().c_str());
	return;
    }
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
    XorpTimer t;
    RouteOrigin* o = r->origin();
    uint32_t expiry_secs = o->expiry_secs();

    if (expiry_secs) {
	t = _eventloop.new_oneoff_after_ms(expiry_secs * 1000,
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
    bool updated = false;

    Route* r = 0;
    typename RouteContainer::iterator i = _routes.find(net);
    if (_routes.end() == i) {

	// Route does not appear in table so it needs to be
	// created if peer does not have an entry for it or
	// resurrected if it does.  But first this...
	if (cost == RIP_INFINITY) {
	    // Don't bother adding a route for unreachable net
	    return false;
	}

	// Create route if necessary
	r = o->find_route(net);
	if (r == 0) {
	    r = new Route(net, nexthop, cost, o, tag);
	    set_expiry_timer(r);
	    bool ok(_routes.insert(RouteContainer::value_type(net, r)).second);
	    XLOG_ASSERT(ok);
	    _uq->push_back(r);
	    return true;
	}

	// Resurrect route
	updated = true;
	bool ok(_routes.insert(RouteContainer::value_type(net, r)).second);
	XLOG_ASSERT(ok);
    } else {
	r = i->second.get();
    }

    if (r->origin() == o) {
	updated |= r->set_nexthop(nexthop);
	updated |= r->set_tag(tag);
	updated |= r->set_cost(cost);
	if (cost == RIP_INFINITY) {
	    set_deletion_timer(r);
	} else {
	    set_expiry_timer(r);
	}
    } else if (r->cost() > cost) {
	r->set_nexthop(nexthop);
	r->set_tag(tag);
	r->set_cost(cost);
	r->set_origin(o);
	set_expiry_timer(r);
	updated = true;
    }
    if (updated) {
	_uq->push_back(r);
    }
    return updated;
}

template <typename A>
void
RouteDB<A>::dump_routes(vector<ConstDBRouteEntry>& routes)
{
    typename RouteContainer::iterator i = _routes.begin();
    while (i != _routes.end()) {
	routes.push_back(i->second);
	++i;
    }
}

template <typename A>
void
RouteDB<A>::flush_routes()
{
    _uq->flush();
    _routes.erase(_routes.begin(), _routes.end());
}

template <typename A>
uint32_t
RouteDB<A>::route_count() const
{
    return _routes.size();
}

template <typename A>
const RouteEntry<A>*
RouteDB<A>::find_route(const IPNet<A>& net) const
{
    typename RouteContainer::const_iterator ri = _routes.find(net);
    if (ri == _routes.end())
	return 0;
    return ri->second.get();
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

template <typename A>
typename RouteDB<A>::RouteContainer&
RouteDB<A>::routes()
{
    return _routes;
}


// ----------------------------------------------------------------------------
// RouteWalker

template <typename A>
const typename RouteWalker<A>::Net RouteWalker<A>::NO_NET(~A::ZERO(), 0);

template <typename A>
RouteWalker<A>::RouteWalker(RouteDB<A>& rdb)
    : _route_db(rdb), _state(RUNNING),
      _last_visited(NO_NET),
      _pos(rdb.routes().begin())
{
}

template <typename A>
RouteWalker<A>::~RouteWalker()
{
}

template <typename A>
const typename RouteWalker<A>::Route*
RouteWalker<A>::next_route()
{
    if (state() != RUNNING) {
	XLOG_ERROR("Calling RouteWalker::next_route() whilst not in RUNNING "
		   "state.");
	return 0;
    }
    if (++_pos == _route_db.routes().end()) {
	return 0;
    }
    return _pos->second.get();
}

template <typename A>
const typename RouteWalker<A>::Route*
RouteWalker<A>::current_route()
{
    if (state() != RUNNING) {
	XLOG_ERROR("Calling RouteWalker::next_route() whilst not in RUNNING "
		   "state.");
	return 0;
    }
    if (_pos == _route_db.routes().end()) {
	return 0;
    }
    return _pos->second.get();
}

template <typename A>
void
RouteWalker<A>::pause(uint32_t pause_ms)
{
    if (state() == PAUSED)
	return;

    _state = PAUSED;
    if (_pos == _route_db.routes().end()) {
	_last_visited = NO_NET;
	return;
    }

    // Check if route has a deletion timer and if so push it's expiry time
    // back to maximize the chance of the route still being valid when
    // resume is called.  Otherwise we have to do more work to find a good
    // point to resume from.  We're advertising the route at infinity
    // so advertising it once past it's original expiry is no big deal

    XorpTimer t = _pos->second->timer();
    if (t.scheduled() && _pos->second->cost() == RIP_INFINITY) {
	TimeVal next_run;
	_route_db.eventloop().current_time(next_run);
	next_run += TimeVal(0, 1000 * pause_ms * 2); // factor of 2 == slack
	if (t.expiry() <= next_run) {
	    t.schedule_at(next_run);
	    _pos->second->set_timer(t);
	}
    }
    _last_visited = _pos->second->net();
}

template <typename A>
void
RouteWalker<A>::resume()
{
    if (state() != PAUSED)
	return;

    _state = RUNNING;
    if (_last_visited == NO_NET) {
	_pos = _route_db.routes().end();
	return;
    }

    _pos = _route_db.routes().find(_last_visited);
    if (_pos == _route_db.routes().end()) {
	// Node got deleted despite our pushing back it's timer (???)
	_pos = _route_db.routes().upper_bound(_last_visited);
    }
}

template <typename A>
void
RouteWalker<A>::reset()
{
    _state = RUNNING;
    _pos = _route_db.routes().begin();
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class RouteDB<IPv4>;
template class RouteWalker<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class RouteDB<IPv6>;
template class RouteWalker<IPv6>;
#endif
