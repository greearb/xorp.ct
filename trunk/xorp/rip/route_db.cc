// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rip/route_db.cc,v 1.17 2004/09/17 20:02:27 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>

#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "constants.hh"
#include "route_db.hh"
#include "update_queue.hh"
#include "rip_varrw.hh"

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
RouteDB<A>::RouteDB(EventLoop& e, PolicyFilters& pfs)
    : _eventloop(e),
      _policy_filters(pfs)
{
    _uq = new UpdateQueue<A>();
}

template <typename A>
RouteDB<A>::~RouteDB()
{
    _routes.erase(_routes.begin(), _routes.end());

    for (typename RouteContainerNoRef::iterator i = _rib_routes.begin();
	i != _rib_routes.end(); ++i) {
	delete (*i).second;
    }	
    
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

    //
    // Check if we have rib routes to replace the delete route.
    // XXX: might be more correct to do all this in expire route.
    //
    typename RouteContainerNoRef::iterator iter = _rib_routes.find(r->net());
  
    _routes.erase(i);

    // add possible rib route
    if (iter != _rib_routes.end()) {
        r = iter->second;

	debug_msg("[RIP] Deleted route, but re-added from RIB routes: %s\n",
		  r->net().str().c_str());

        update_route(r->net(), r->nexthop(), r->cost(), r->tag(), _rib_origin,
		     r->policytags());
    }
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
			      r->origin(), r->policytags())) {
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
RouteDB<A>::do_filtering(Route* r)
{
    try {
	RIPVarRW<A> varrw(*r);

	ostringstream trace;

	debug_msg("[RIP] Running import filter on route %s\n",
		  r->net().str().c_str());

	bool accepted = _policy_filters.run_filter(filter::IMPORT,
						   varrw, &trace);

	debug_msg("[RIP] Filter trace:\n%s\nDone.. Accepted: %d\n",
		  trace.str().c_str(),accepted);

	// clear trace
	trace.str("");

	if (!accepted)
	    return false;


	RIPVarRW<A> varrw2(*r);


	debug_msg("[RIP] Running source match filter on route %s\n",
		  r->net().str().c_str());
	_policy_filters.run_filter(filter::EXPORT_SOURCEMATCH,
				   varrw2, &trace);


	debug_msg("[RIP] Filter trace:\n%s\nDone..\n",
		  trace.str().c_str());

	return true;
    } catch(const PolicyException& e) {
	XLOG_FATAL("PolicyException: %s",e.str().c_str());
	XLOG_UNFINISHED();
    }
}

template <typename A>
bool
RouteDB<A>::update_route(const Net&	    net,
			 const Addr&	    nexthop,
			 uint32_t	    cost,
			 uint32_t	    tag,
			 RouteOrigin*	    o,
			 const PolicyTags&  policytags)
{
    UNUSED(policytags);

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
	    r->set_policytags(policytags);

	    set_expiry_timer(r);
	    bool ok(_routes.insert(typename
				   RouteContainer::value_type(net, r)).second);
	    
	    XLOG_ASSERT(ok);
	    
	    bool accepted = do_filtering(r);
	    r->set_filtered(!accepted);

	    if (!accepted)
		return false;

	    _uq->push_back(r);
	    return true;
	}

	// Resurrect route
	
	bool ok(_routes.insert(typename
			       RouteContainer::value_type(net, r)).second);
	XLOG_ASSERT(ok);

	// XXX: this is wrong
	bool accepted = do_filtering(r);
	r->set_filtered(!accepted);

	if (accepted)
	    updated = true;
    } else {
	r = i->second.get();
    }

    RouteEntryOrigin<A>* no_origin = NULL;
    RouteEntry<A>* new_route = new RouteEntry<A>(r->net(), nexthop,
						 cost, no_origin, tag,
						 policytags);
    // XXX: lost origin
    bool accepted = do_filtering(new_route);

    // XXX: this whole section of code is too entangled.
    if (r->origin() == o) {
	
	updated |= r->set_nexthop(new_route->nexthop());
	updated |= r->set_tag(new_route->tag());
	updated |= r->set_cost(new_route->cost());
	updated |= r->set_policytags(new_route->policytags());

	delete new_route;

	if (cost == RIP_INFINITY) {
	    set_deletion_timer(r);
	} else {
	    set_expiry_timer(r);
	}
	
	bool was_filtered = r->filtered();
	r->set_filtered(!accepted);

	debug_msg("[RIP] Was filtered: %d, Accepted: %d\n",
		  was_filtered, accepted);
	
	if (accepted) {
	    if (was_filtered) {
		updated = true;
	    } else {
	    }
	} else {
	    if (was_filtered) {
		return false;
	    } else {
		if (cost != RIP_INFINITY) {
		    expire_route(r);
		    return true;
		}    
//		delete_route(r);
	    }
	}
    } else {
	// route from other origin
	if (!accepted) {
	    delete new_route;
	    return false;
	}
	// this is "RIP's decision" -- where one route wins another.
	// Source-match filtering should go here.
	if (r->cost() > new_route->cost()) {
	    r->set_nexthop(new_route->nexthop());
	    r->set_tag(new_route->tag());
	    r->set_cost(new_route->cost());
	    r->set_policytags(new_route->policytags());
	    r->set_origin(o);
	    set_expiry_timer(r);
	    updated = true;

	}    
	delete new_route;
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


/*
 * XXX: ok... we need to delete all the routes we got... as we
 * do not store original routes.
 * all we can do is re-add rib routes.
 * and wait for "real rip peers" to readvertise...
 * we can probably send out a request to get the advertisements though...
 */
template <typename A>
void
RouteDB<A>::push_routes()
{
    debug_msg("[RIP] Push routes\n");

    for (typename RouteContainer::iterator i = _routes.begin();
	i != _routes.end(); ++i) {
	Route* r = (*i).second.get();

	debug_msg("[RIP] route: %s, filtered: %d\n",
		  r->net().str().c_str(), r->filtered());

	if (!r->filtered())
	    expire_route(r);

//	delete_route(r);
    }	

    // XXX may have got RIB route adds because of delete_route
    // flush is probably not necessary...

    debug_msg("[RIP] Pushing the RIB routes we have\n");
   
    for (typename RouteContainerNoRef::iterator i = _rib_routes.begin();
	i != _rib_routes.end(); ++i) {

	Route* r = (*i).second;

	debug_msg("[RIP] Pushing RIB route %s\n", r->net().str().c_str());
	
	update_route(r->net(), r->nexthop(), r->cost(), r->tag(),
		     _rib_origin, r->policytags());
    }
}

template <typename A>
void
RouteDB<A>::add_rib_route(const Net& net, const Addr& nexthop, uint32_t cost,
			  uint32_t tag, RouteOrigin* origin,
			  const PolicyTags& policytags)
{
    debug_msg("[RIP] adding RIB route %s\n",net.str().c_str());
  
    _rib_origin = origin;
         
    typename RouteContainerNoRef::iterator i = _rib_routes.find(net);

    if (i != _rib_routes.end()) {
	Route* prev = (*i).second;
	delete prev;
    }

    //
    // XXX: We are cheating here NULL origin so we don't get association.
    //
    RouteOrigin* no_origin = NULL;
    Route* r = new Route(net, nexthop, cost, no_origin, tag, policytags);

    _rib_routes[net] = r;
}

template <typename A>
void
RouteDB<A>::delete_rib_route(const Net& net)
{
    debug_msg("[RIP] deleting RIB route %s\n",net.str().c_str());

    typename RouteContainerNoRef::iterator i = _rib_routes.find(net);
    XLOG_ASSERT(i != _rib_routes.end());

    Route* r = (*i).second;

    delete r;

    _rib_routes.erase(i);
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
