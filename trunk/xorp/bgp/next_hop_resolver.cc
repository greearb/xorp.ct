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

#ident "$XORP: xorp/bgp/next_hop_resolver.cc,v 1.12 2003/04/18 23:27:43 mjh Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/timer.hh"

#include "next_hop_resolver.hh"
#include "route_table_nhlookup.hh"
#include "route_table_decision.hh"

template <class A>
NextHopResolver<A>::NextHopResolver(XrlStdRouter *xrl_router,
				    EventLoop& event_loop)
    : _xrl_router(xrl_router),
      _event_loop(event_loop),
      _next_hop_rib_request(xrl_router, *this, _next_hop_cache)
{
}

template <class A>
NextHopResolver<A>::~NextHopResolver()
{
}

template <class A>
void
NextHopResolver<A>::add_decision(DecisionTable<A> *decision)
{
    _decision = decision;
}

template <class A>
bool
NextHopResolver<A>::register_ribname(const string& ribname)
{
    _ribname = ribname;
    return _next_hop_rib_request.register_ribname(ribname);
}

/*
 * The next hop lookup table uses this interface to discover if this
 * next hop is known. We don't care if this nexthop is resolvable or
 * not only that we have at some point asked the RIB and that we have
 * an answer.
 */
template <class A>
bool
NextHopResolver<A>::register_nexthop(A nexthop, IPNet<A> net_from_route,
				     NhLookupTable<A> *requester)
{
    debug_msg("nexthop %s net %s requested %p\n",
	      nexthop.str().c_str(), net_from_route.str().c_str(), requester);

    /*
    ** We haven't hooked up with a rib so just return true.
    */
    if ("" == _ribname)
	return true;

    /*
    ** The common case will be that we know about this next hop and
    ** its in our cache. Another option is that the nexthop falls into
    ** a range for which we already have an answer from the RIB.
    */
    if (_next_hop_cache.register_nexthop(nexthop))
	return true;

    /*
    ** We don't have an answer so make a request of the rib.
    */
    _next_hop_rib_request.register_nexthop(nexthop, net_from_route, requester);

    return false;
}

template <class A>
void
NextHopResolver<A>::deregister_nexthop(A nexthop, IPNet<A> net_from_route,
				    NhLookupTable<A> *requester)
{
    debug_msg("nexthop %s net %s requested %p\n",
	      nexthop.str().c_str(), net_from_route.str().c_str(), requester);
    /*
    ** We haven't hooked up with a rib so just return.
    */
    if ("" == _ribname)
	return;

    /*
    ** Somewhere there has to be a record of this registration. It is
    ** either in the "NextHopCache" or "NextHopRibRequest". Check
    ** first in the "NextHopCache".
    */
    bool last;
    A addr;
    uint32_t prefix_len;
    if (_next_hop_cache.deregister_nexthop(nexthop, last, addr, prefix_len)) {
	if (last) {
	    /*
	    ** We no longer have any interest in this entry.
	    */
	    _next_hop_rib_request.deregister_from_rib(addr, prefix_len);
	}
	return;
    }

    if (!_next_hop_rib_request.deregister_nexthop(nexthop, net_from_route, 
						  requester))
	XLOG_FATAL("Unknown nexthop %s", nexthop.str().c_str());
}

template <class A>
bool
NextHopResolver<A>::lookup(const A nexthop, bool& resolvable,
			   uint32_t& metric) const
{
    debug_msg("nexthop %s\n", nexthop.str().c_str());

    /*
    ** We haven't hooked up with a rib so just return.
    */
    if ("" == _ribname) {
	resolvable = true;
	metric = 1;
	return true;
    }

    /*
    ** If we are being called by the decision code this next hop must
    ** be in our cache.
    */
    if (_next_hop_cache.lookup_by_nexthop(nexthop, resolvable, metric))
	return true;

    /*
    ** There is a chance that this entry was in our cache but was
    ** invalidated by the RIB. In which case we are currently
    ** rerequesting the metrics. We will attempt to return the old
    ** values. As soon as the new values arrive we will call the hook
    ** in decision.
    */
    if (_next_hop_rib_request.lookup(nexthop, resolvable, metric)) {
	XLOG_INFO("FYI: Stale metrics supplied");
	return true;
    }

    return false;
}

template <class A>
bool
NextHopResolver<A>::rib_client_route_info_changed(const A& addr,
						  const uint32_t& real_prefix_len,
						  const A& nexthop,
						  const uint32_t& metric)
{
    debug_msg("addr %s prefix_len %d nexthop %s metric %d\n",
	      addr.str().c_str(), real_prefix_len, nexthop.str().c_str(), metric);

    /*
    ** Change the entries in the cache and make an upcall notifying
    ** the decision code that the metrics have changed.
    */
    map<A, int> m = _next_hop_cache.change_entry(addr, real_prefix_len, metric);
    typename map<A, int>::iterator i;
    for (i = m.begin(); i != m.end(); i++)
	next_hop_changed(i->first);

    return true;
}

template <class A>
bool
NextHopResolver<A>::rib_client_route_info_invalid(const A& addr,
						  const uint32_t& prefix_len)
{
    debug_msg("addr %s prefix_len %d\n", addr.str().c_str(), prefix_len);

    /*
    ** This is a callback from the RIB telling us that this
    ** addr/prefix_len should no longer be used to satisfy nexthop
    ** requests.
    ** 1) As a sanity check verify that we have this add/prefix_len in our
    ** cache.
    ** 2) Delete the entry from our cache.
    ** 3) Reregister interest for this nexthop. It may be the case
    ** that we have an entry in the nexthop cache that satisfies this
    ** nexthop. This eventuality will be dealt with in the reregister
    ** code. Save the old metrics in case a lookup comes in while we
    ** are waiting for the new values.
    */

    bool resolvable;
    uint32_t metric;
    if (!_next_hop_cache.lookup_by_addr(addr, prefix_len, resolvable, metric)) {
	XLOG_WARNING("address not found in next hop cache: %s/%d",
		   addr.str().c_str(), prefix_len);
	return false;
    }

    map<A, int> m = _next_hop_cache.delete_entry(addr, prefix_len);
    typename map<A, int>::iterator i;
    for (i = m.begin(); i != m.end(); i++)
	_next_hop_rib_request.reregister_nexthop(i->first, i->second,
						 resolvable, metric);

    return true;
}

template <class A>
void
NextHopResolver<A>::next_hop_changed(A addr)
{
    debug_msg("next hop: %s\n", addr.str().c_str());
    if (_decision)
	_decision->igp_nexthop_changed(addr);
    else
	XLOG_FATAL("No pointer to the decision table.");
}

template <class A>
void
NextHopResolver<A>::next_hop_changed(A addr, bool old_resolves,
				     uint32_t old_metric)
{
    debug_msg("next hop: %s\n", addr.str().c_str());
    if (!_decision)
	XLOG_FATAL("No pointer to the decision table.");

    bool resolvable;
    uint32_t metric;
    if (!lookup(addr, resolvable, metric))
	XLOG_FATAL("Could not lookup %s", addr.str().c_str());
	
    bool changed = false;
	
    // If resolvability has changed we care.
    if (resolvable != old_resolves)
	changed = true;

    // Only if the nexthop resolves do we care if the metric has changed.
    if (resolvable && metric != old_metric)
	changed = true;

    if (changed)
	_decision->igp_nexthop_changed(addr);
}

/****************************************/

template<class A>
NextHopCache<A>::~NextHopCache()
{
    /*
    ** Free all the allocated NextHopEntrys.
    */
    PrefixIterator i;
    for (i = _next_hop_by_prefix.begin(); 
	 i != _next_hop_by_prefix.end(); i++){
	NextHopEntry *entry = i.payload();
	delete entry;
    }
}

template<class A>
void
NextHopCache<A>::add_entry(A addr, A nexthop, int prefix_len, int real_prefix_len,
			   bool resolvable, uint32_t metric)
{
    debug_msg("addr %s prefix_len %d real prefix_len %d\n",
	      addr.str().c_str(), prefix_len, real_prefix_len);

    XLOG_ASSERT(addr == nexthop.mask_by_prefix(prefix_len));

    PrefixEntry *entry = new PrefixEntry;
    entry->_address = addr;
#ifdef	USE_NEXTHOP
    entry->_nexthop = nexthop;
#endif
    // Don't create any map entries.
//  entry->_nexthop_references[nexthop] = 0;	// Initial Ref count of 0
    entry->_prefix_len = prefix_len;
    entry->_real_prefix_len = real_prefix_len;
    entry->_resolvable = resolvable;
    entry->_metric = metric;

    // This entry must not already be in the trie.
    XLOG_ASSERT(_next_hop_by_prefix.lookup_node(IPNet<A>(addr, 
							     prefix_len))
		== _next_hop_by_prefix.end());

    // This entry must not already be in the trie.
    RealPrefixIterator rpi = _next_hop_by_real_prefix.
	lookup_node(IPNet<A>(addr, real_prefix_len));

    if (rpi == _next_hop_by_real_prefix.end()) {
	RealPrefixEntry rpe;
	rpe.insert(entry);
	_next_hop_by_real_prefix.insert(IPNet<A>(addr, real_prefix_len), rpe);
    } else {
	RealPrefixEntry *rpep = &(rpi.payload());
	XLOG_ASSERT(0 == rpe_to_pe(*rpep, addr, real_prefix_len));
	rpep->insert(entry);
    }

    _next_hop_by_prefix.insert(IPNet<A>(addr, prefix_len), entry);
}

template<class A>
bool
NextHopCache<A>::validate_entry(A addr, A nexthop, int prefix_len, int real_prefix_len)
{
    debug_msg("addr %s nexthop %s prefix_len %d real prefix_len %d\n",
	      addr.str().c_str(), nexthop.str().c_str(), prefix_len, real_prefix_len);

    PrefixIterator pi =
	_next_hop_by_prefix.lookup_node(IPNet<A>(addr, prefix_len));
    XLOG_ASSERT(pi != _next_hop_by_prefix.end());

    PrefixEntry *en = pi.payload();
    XLOG_ASSERT(en->_address == addr);
#ifdef	USE_NEXTHOP
    XLOG_ASSERT(en->_nexthop == nexthop);
#else
    nexthop = nexthop;
#endif
    XLOG_ASSERT(en->_prefix_len == prefix_len);
    XLOG_ASSERT(en->_real_prefix_len == real_prefix_len);

    if (en->_nexthop_references.empty()) {
	delete_entry(addr, prefix_len);
 	debug_msg("No references\n");
	return false;
    }

    return true;
}

template<class A>
map<A, int>
NextHopCache<A>::change_entry(A addr, int real_prefix_len, uint32_t metric)
{
    RealPrefixIterator rpi =
	_next_hop_by_real_prefix.lookup_node(IPNet<A>(addr, real_prefix_len));
    XLOG_ASSERT(rpi != _next_hop_by_real_prefix.end());

    RealPrefixEntry rpe = rpi.payload();
    PrefixEntry *en = rpe_to_pe(rpe, addr, real_prefix_len);

    XLOG_ASSERT(en);
    XLOG_ASSERT(en->_address == addr);
    XLOG_ASSERT(en->_real_prefix_len == real_prefix_len);

    // Save all the next hops and reference counts that were covered
    // by this entry.
    map<A, int> ret = en->_nexthop_references;

    // Update the metric
    en->_metric = metric;

    return ret;
}

template<class A>
map<A, int>
NextHopCache<A>::delete_entry(A addr, int prefix_len)
{
    debug_msg("addr %s prefix_len %d\n", addr.str().c_str(), prefix_len);

    PrefixIterator pi =
	_next_hop_by_prefix.lookup_node(IPNet<A>(addr, prefix_len));
    XLOG_ASSERT(pi != _next_hop_by_prefix.end());

    PrefixEntry *en = pi.payload();

    XLOG_ASSERT(en->_address == addr);
    XLOG_ASSERT(en->_prefix_len == prefix_len);

    RealPrefixIterator rpi =
	_next_hop_by_real_prefix.lookup_node(IPNet<A>(addr, en->_real_prefix_len));
    XLOG_ASSERT(rpi != _next_hop_by_real_prefix.end());

    RealPrefixEntry *rpep = &(rpi.payload());
    // These should both point at the same entry.
    if (rpe_to_pe_delete(*rpep, addr, en->_real_prefix_len) != en)
	XLOG_FATAL("Entry was not present addr %s real_prefix_len %d",
		   addr.str().c_str(), en->_real_prefix_len);

    // Save all the next hops and reference counts that were covered
    // by this entry.
    map<A, int> ret = en->_nexthop_references;

    // Delete the entry
    delete en;

    // Delete the two pointers to this entry.
    if (rpep->empty())
	_next_hop_by_real_prefix.erase(rpi);
    _next_hop_by_prefix.erase(pi);

    return ret;
}

template<class A>
bool
NextHopCache<A>::lookup_by_addr(A addr, int prefix_len, bool& resolvable,
				uint32_t& metric) const
{
    PrefixIterator pi =
	_next_hop_by_prefix.lookup_node(IPNet<A>(addr, prefix_len));
    if (pi == _next_hop_by_prefix.end())
	return false;

    NextHopEntry *en = pi.payload();

    XLOG_ASSERT(en->_prefix_len == prefix_len);
    resolvable = en->_resolvable;
    metric = en->_metric;

    return true;
}

template<class A>
bool
NextHopCache<A>::lookup_by_nexthop(A nexthop, bool& resolvable,
				   uint32_t& metric)
    const
{
    PrefixIterator pi = _next_hop_by_prefix.find(nexthop);
    if (pi == _next_hop_by_prefix.end())
	return false;

    NextHopEntry *en = pi.payload();

    /*
    ** We have found an entry now check that this next hop is recorded
    ** in this entry.
    */
    if (en->_nexthop_references.find(nexthop) == en->_nexthop_references.end())
	return false;

    resolvable = en->_resolvable;
    metric = en->_metric;

    return true;
}

template<class A>
bool
NextHopCache<A>::lookup_by_nexthop_without_entry(A nexthop,
						 bool& resolvable,
						 uint32_t& metric)
    const
{
    PrefixIterator pi = _next_hop_by_prefix.find(nexthop);
    if (pi == _next_hop_by_prefix.end())
	return false;

    NextHopEntry *en = pi.payload();

    resolvable = en->_resolvable;
    metric = en->_metric;

    return true;
}

template<class A>
bool
NextHopCache<A>::register_nexthop(A nexthop, int ref_cnt_incr)
{
    debug_msg("nexthop %s reference count %d\n", nexthop.str().c_str(),
	      ref_cnt_incr);

    XLOG_ASSERT(0 != ref_cnt_incr);

    /*
    ** Look for a covering entry. If there is none just return false.
    */
    PrefixIterator pi = _next_hop_by_prefix.find(nexthop);
    if (pi == _next_hop_by_prefix.end())
	return false;

    debug_msg("nexthop %s covered\n", nexthop.str().c_str());

    NextHopEntry *en = pi.payload();

    /*
    ** This may be a new nexthop in which case create a new map
    ** entry. If we already know about this nexthop then just
    ** increment the reference count.
    */
    if (en->_nexthop_references.find(nexthop) == en->_nexthop_references.end())
	en->_nexthop_references[nexthop] = ref_cnt_incr;
    else
	en->_nexthop_references[nexthop] += ref_cnt_incr;

    return true;
}

template<class A>
bool
NextHopCache<A>::deregister_nexthop(A nexthop, bool& last,
				    A& addr, uint32_t& prefix_len)
{
    PrefixIterator pi = _next_hop_by_prefix.find(nexthop);

    if (pi == _next_hop_by_prefix.end())
	return false;

    NextHopEntry *en = pi.payload();

    typename map<A, int>::iterator nhp = en->_nexthop_references.find(nexthop);
    if (nhp == en->_nexthop_references.end())
	return false;

    if (0 == --en->_nexthop_references[nexthop]) {
	en->_nexthop_references.erase(nhp);
	if (en->_nexthop_references.empty()) {
	    last = true;
	    addr = en->_address;
	    prefix_len = en->_prefix_len;
	    delete_entry(en->_address, en->_prefix_len);
	    return true;
	}
    }

    last = false;

    return true;
}

template<class A>
typename NextHopCache<A>::PrefixEntry *
NextHopCache<A>::rpe_to_pe(const RealPrefixEntry& rpe, A addr, int real_prefix_len)
    const
{
    debug_msg("addr %s real prefix_len %d\n", addr.str().c_str(), real_prefix_len);

    typename RealPrefixEntry::const_iterator si;

#ifdef	DEBUG_LOGGING
    for (si = rpe.begin(); si != rpe.end(); si++)
	debug_msg("addr %s prefix_len %d real prefix_len %d\n",
		  (*si)->_address.str().c_str(), (*si)->_prefix_len,
		  (*si)->_real_prefix_len);
#endif

    for (si = rpe.begin(); si != rpe.end(); si++)
 	if ((*si)->_real_prefix_len == real_prefix_len && (*si)->_address == addr)
 	    return *si;

    return 0;
}

template<class A>
typename NextHopCache<A>::PrefixEntry *
NextHopCache<A>::rpe_to_pe_delete(RealPrefixEntry& rpe, A addr,
				  int real_prefix_len)
{
    debug_msg("addr %s real prefix_len %d\n", addr.str().c_str(), 
	      real_prefix_len);

    typename RealPrefixEntry::const_iterator si;

#ifdef	DEBUG_LOGGING
    for (si = rpe.begin(); si != rpe.end(); si++)
	debug_msg("addr %s prefix_len %d real prefix_len %d\n",
		  (*si)->_address.str().c_str(), (*si)->_prefix_len,
		  (*si)->_real_prefix_len);
#endif

    for (si = rpe.begin(); si != rpe.end(); si++)
 	if ((*si)->_real_prefix_len == real_prefix_len && (*si)->_address == addr) {
	    PrefixEntry *pi = *si;
	    rpe.erase(si);
 	    return pi;
	}

    return 0;
}

/****************************************/

template<class A>
NextHopRibRequest<A>::NextHopRibRequest(XrlStdRouter *xrl_router,
					NextHopResolver<A>& next_hop_resolver,
					NextHopCache<A>& next_hop_cache)
    : _xrl_router(xrl_router), _next_hop_resolver(next_hop_resolver),
      _next_hop_cache(next_hop_cache), _busy(false), 
      _previously_successful(false), _interface_failed(false)
{
}

template<class A>
NextHopRibRequest<A>::~NextHopRibRequest()
{
    /*
    ** Free all the outstanding queue entries.
    */
    for_each(_queue.begin(), _queue.end(), &NextHopRibRequest::zapper);
}

template<class A>
void
NextHopRibRequest<A>::register_nexthop(A nexthop, IPNet<A> net_from_route,
				       NhLookupTable<A> *requester)
{
    debug_msg("nexthop %s net %s requested %p\n",
	      nexthop.str().c_str(), net_from_route.str().c_str(), requester);

    //don't do anything if we've already fatally failed
    if (_interface_failed)
	return;
    /*
    ** Make sure that we are not already waiting for a response for
    ** this sucker.
    */
    typename list<RibRequestQueueEntry<A> *>::iterator i;
    for (i = _queue.begin(); i != _queue.end(); i++) {
	RibRegisterQueueEntry<A> *rr = 
	    dynamic_cast<RibRegisterQueueEntry<A> *>(*i);
	if (rr && rr->nexthop() == nexthop) {
	    rr->register_nexthop(net_from_route, requester);
	    return;
	}
    }

    /*
    ** Construct a request.
    */
    RibRegisterQueueEntry<A> *rr 
	= new RibRegisterQueueEntry<A>(nexthop, net_from_route, requester);

    /*
    ** Add the request to the queue.
    */
    _queue.push_back(rr);

    /*
    ** We allow only one outstanding request to the RIB. The "_busy"
    ** flag marks if there is currently an outstanding request.
    */
    if (!_busy) {
	send_next_request();
    }
}

template<class A>
void
NextHopRibRequest<A>::deregister_from_rib(const A& base_addr, 
					  uint32_t prefix_len)
{
    //don't do anything if we've already fatally failed
    if (_interface_failed)
	return;

    /*
    ** Construct a request.
    */
    RibDeregisterQueueEntry<A> *rr 
	= new RibDeregisterQueueEntry<A>(base_addr, prefix_len);

    /*
    ** Add the request to the queue.
    */
    _queue.push_back(rr);

    /*
    ** We allow only one outstanding request to the RIB. The "_busy"
    ** flag marks if there is currently an outstanding request.
    */
    if (!_busy) {
	send_next_request();
    }
}

template<>
void
NextHopRibRequest<IPv4>::register_interest(IPv4 nexthop)
{
    debug_msg("nexthop %s\n", nexthop.str().c_str());
    if(0 == _xrl_router)	// The test code sets _xrl_router to zero
	return;

    XrlRibV0p1Client rib(_xrl_router);
    rib.send_register_interest4(_ribname.c_str(), _xrl_router->name(),
				nexthop,
				::callback(this,
	       &NextHopRibRequest::register_interest_response,
	       c_format("nexthop: %s", nexthop.str().c_str())));
}

template<>
void
NextHopRibRequest<IPv6>::register_interest(IPv6 nexthop)
{
    debug_msg("nexthop %s\n", nexthop.str().c_str());
    if(0 == _xrl_router)	// The test code sets _xrl_router to zero
	return;

    XrlRibV0p1Client rib(_xrl_router);
    rib.send_register_interest6(_ribname.c_str(), _xrl_router->name(),
				nexthop,
				::callback(this,
	       &NextHopRibRequest::register_interest_response,
	       c_format("nexthop: %s", nexthop.str().c_str())));
}

template<class A>
void
NextHopRibRequest<A>::register_interest_response(const XrlError& error,
						 const bool *resolves,
						 const A *addr,
						 const uint32_t *prefix_len,
						 const uint32_t *real_prefix_len,
						 const A *actual_nexthop,
						 const uint32_t *metric,
						 const string comment)
{
    /*
    ** We attempted to register a next hop with the RIB and an error
    ** ocurred. Its not clear that we should continue.
    */
    if (XrlError::OKAY() != error) {
	if (error == XrlError::NO_FINDER()
	    /* || (error == FATAL_TRANSPORT_ERROR()) */
	    || (error == XrlError::RESOLVE_FAILED() 
		&& _previously_successful) ) {
	    //A NO_FINDER or FATAL_TRANSPORT_ERROR error is always
	    //unrecoverable.  A RESOLVE_FAILED error when it had
	    //previously been successful is also unrecoverable.
	    _interface_failed = true;
	    while (!_queue.empty()) {
		delete _queue.front();
		_queue.pop_front();
	    }
	    return;
	}
	if (error == XrlError::RESOLVE_FAILED()
	    || error == XrlError::SEND_FAILED()) {
	    //SEND_FAILED can be a transient error.  RESOLVE_FAILED
	    //when we hadn't been previously successful might indicate
	    //an ordering problem at startup.  In both cases, we resend.
	    XLOG_WARNING("%s %s", comment.c_str(), error.str().c_str());
	    
	    //The request will still be on the request queue.
	    //All we need to do is resend it after a respectable delay
	    A nexthop = *addr;
	    _rtx_delay_timer = 
                 _next_hop_resolver.event_loop().new_oneoff_after_ms(1000,
                      ::callback(this,
				 &NextHopRibRequest<A>::register_interest,
				 nexthop));
	    return;
	}
	//We received an application-level error when attempting to
	//register.  For now, this is fatal.
	XLOG_FATAL("%s %s", comment.c_str(), error.str().c_str());
    } else {
	_previously_successful = true;
    }

    debug_msg("Error %s resolves %d addr %s "
	      "prefix_len %d real prefix_len %d actual nexthop %s metric %d %s\n",
 	      error.str().c_str(), *resolves, addr->str().c_str(),
	      *prefix_len, *real_prefix_len, actual_nexthop->str().c_str(), *metric,
	      comment.c_str());

    /*
    ** Make sure that there is something on our queue.
    */
    XLOG_ASSERT(!_queue.empty());
    RibRegisterQueueEntry<A> *first_rr =
	dynamic_cast<RibRegisterQueueEntry<A> *>(_queue.front());

    /*
    ** Make sure the front entry is a register rather than a deregister
    */
    XLOG_ASSERT(first_rr != NULL);

    /*
    ** At this point I would really like to directly compare the
    ** nexthop that we actually registered interest for with the
    ** nexthop that was returned. Unfortunately what is returned is
    ** a base address for the covered region. So the comparison has to
    ** be masked by the prefix_len that is returned.
    */
    
    XLOG_ASSERT(IPNet<A>(*addr, *prefix_len) ==
		IPNet<A>(first_rr->nexthop(), *prefix_len));

    /*
    ** We have a lot to do here. This response may have satisfied
    ** other requests on the queue. Also there are two reasons for
    ** making requests. The simple case is that the next hop table has
    ** called down to us. In which case we should run the call
    ** backs. The other reason is that a previous entry has become
    ** invalid in which case we need to call the next hop changed hook
    ** in the decision table. It is possible (legal) for an entry to be here
    ** for both reasons.
    **
    ** The simplest strategy is to insert this result into the
    ** NextHopCache. Then traverse the queue removing all entries that
    ** are satisfied by the NextHopCache.
    */
    A first_nexthop = first_rr->nexthop();
    _next_hop_cache.add_entry(*addr, first_nexthop, *prefix_len, 
			      *real_prefix_len, *resolves, *metric);

    /*
    ** As soon as we come across an entry in the queue that we
    ** can't lookup in the cache we bail. It may be the case that
    ** there are other entries later in the queue that may
    ** resolve. Don't worry about it we will get there eventually.
    */
    typename list<RibRequestQueueEntry<A> *>::iterator i;
    i = _queue.begin();
    while (i != _queue.end()) {
	bool lookup_succeeded;
	uint32_t m;
	RibRegisterQueueEntry<A> *rr
	    = dynamic_cast<RibRegisterQueueEntry<A> *>(*i);
	if (rr) {
	    if (_next_hop_cache.lookup_by_nexthop_without_entry(rr->nexthop(),
								lookup_succeeded, m)) {
		XLOG_ASSERT(rr->new_register() || rr->reregister());
		/*
		** See if this request was caused by a downcall from the next hop
		** table.
	    */
		if (rr->new_register()) {
		    NHRequest<A>* request_data = &rr->requests();
		    /*
		    ** If nobody is interested then don't register or run
		    ** the callbacks.
		    */
		    if(0 != request_data->requests()) {
			_next_hop_cache.register_nexthop(rr->nexthop(),
							 request_data->requests());

			typename set <NhLookupTable<A> *>::const_iterator req_iter;
			for (req_iter = request_data->requesters().begin();
			     req_iter != request_data->requesters().end();
			     req_iter++) {
			    NhLookupTable<A> *requester = (*req_iter);
			    requester->RIB_lookup_done(rr->nexthop(),
						       request_data->request_nets(requester),
						       lookup_succeeded);
			}
		    }
		}
		/*
	    ** See if this request was caused by an upcall from the
	    ** RIB. If it was then notify decision that this next hop
	    ** has changed.
	    */
		if (rr->reregister() && 0 != rr->ref_cnt()) {
		    _next_hop_cache.register_nexthop(rr->nexthop(), rr->ref_cnt());
		    /*
		    ** Start the upcall with the old metrics. Only if the
		    ** state has changed will the upcall be made.
		    */
		    _next_hop_resolver.next_hop_changed(rr->nexthop(),
							rr->resolvable(),
							rr->metric());
		}
		delete rr;
		i = _queue.erase(i);
	    } else {
		break;
	    }
	} else {
	    RibDeregisterQueueEntry<A> *rd
		= dynamic_cast<RibDeregisterQueueEntry<A> *>(*i);
	    assert(rd != NULL);
	    if ((rd->base_addr() == *addr) 
		&& (rd->prefix_len() == *prefix_len)) {
		//We got an answer back, and there was a matching
		//deregister in the queue.  Remove the deregister from
		//the queue.  It's possible this was actually needed,
		//but at this point we can't tell for sure.  Later
		//we'll validate the entry, and back a deregister if
		//it was needed.
		delete rd;
		i = _queue.erase(i);
	    } else {
		//skip the deregister
		i++;
	    }
	}
    }

    /*
    ** A NextHopResolver::register_nexthop caused us to make a request
    ** of the RIB. In the meantime a NextHopResolver::deregister_nexthop
    ** call took place. We removed the reference but couldn't stop the
    ** call to RIB. It may be that this response may satisfy other
    ** outstanding queries in the queue. If it hasn't then the entry
    ** will be invalid so deregister interest with the RIB.
    */
    if (!_next_hop_cache.validate_entry(*addr, first_nexthop, *prefix_len,
					*real_prefix_len)) {
	RibDeregisterQueueEntry<A> *rd
	    = new RibDeregisterQueueEntry<A>(*addr, *prefix_len);
	_queue.push_back(rd);
    }


    /*
    ** There are entries left on the queue, so, fire off another request.
    */
    send_next_request();
}

template<class A>
void
NextHopRibRequest<A>::send_next_request() {
    if (_queue.empty()) {
	_busy = false;
	return;
    }
    _busy = true;
    RibRegisterQueueEntry<A> *rr = 
	dynamic_cast<RibRegisterQueueEntry<A> *>(_queue.front());
    if (rr) {
	register_interest(rr->nexthop());
	return;
    }
    
    RibDeregisterQueueEntry<A> *rd = 
	dynamic_cast<RibDeregisterQueueEntry<A> *>(_queue.front());
    if (rd) {
	deregister_interest(rd->base_addr(), rd->prefix_len());
	return;
    }
    abort();
}

template<class A>
bool
NextHopRibRequest<A>::deregister_nexthop(A nexthop, IPNet<A> net_from_route,
					 NhLookupTable<A> *requester)
{
    debug_msg("nexthop %s net %s requested %p\n",
	      nexthop.str().c_str(), net_from_route.str().c_str(), requester);

    typename list<RibRequestQueueEntry<A> *>::iterator i;

    /*
    ** The deregister may mean that there are no more interested parties.
    ** It may therefore be tempting to remove this entry from the
    ** queue DON'T as a request to the RIB might be in progress. The
    ** register_interest_response will tidy up. Worst case we
    ** occasionally make a request for a next hop for which there are
    ** no requesters.
    */
    for (i = _queue.begin(); i != _queue.end(); i++) {
	RibRegisterQueueEntry<A> *rr = 
	    dynamic_cast<RibRegisterQueueEntry<A> *>(*i);
	if (rr && rr->nexthop() == nexthop)
	    return rr->deregister_nexthop(net_from_route, requester);
    }
    return false;
}

template<class A>
void
NextHopRibRequest<A>::reregister_nexthop(A nexthop, uint32_t ref_cnt,
					 bool resolvable, uint32_t metric)
{
    debug_msg(" nexthop %s ref_cnf %d resolvable %d metric %d\n",
	      nexthop.str().c_str(), ref_cnt, resolvable, metric);

    /*
    ** It may be the case that we already have an entry that covers
    ** this next hop.
    */
    if (_next_hop_cache.register_nexthop(nexthop, ref_cnt)) {
	bool new_resolvable;
	uint32_t new_metric;
	if (!_next_hop_cache.lookup_by_nexthop(nexthop, new_resolvable,
					       new_metric))
	    XLOG_FATAL("This nexthop %s must be in the cache",
		       nexthop.str().c_str());

	/*
	** The entry is already covered but the metrics may have
	** changed. If the metrics have changed we need to make an
	** upcall to the BGP decision process.
	*/
	_next_hop_resolver.next_hop_changed(nexthop, resolvable, metric);

	return;
    }

    /*
    ** Make sure that we are not already waiting for a response for
    ** this sucker.
    */
    typename list<RibRequestQueueEntry<A> *>::iterator i;
    for (i = _queue.begin(); i != _queue.end(); i++) {
	RibRegisterQueueEntry<A> *rr = 
	    dynamic_cast<RibRegisterQueueEntry<A> *>(*i);
	if (rr && rr->nexthop() == nexthop) {
	    rr->reregister_nexthop(ref_cnt, resolvable, metric);
	    return;
	}
    }

    /*
    ** Construct a request.
    */
    RibRegisterQueueEntry<A> *rr 
	= new RibRegisterQueueEntry<A>(nexthop, ref_cnt, resolvable, metric);

    /*
    ** Add the request to the queue.
    */
    _queue.push_back(rr);

    /*
    ** We allow only one outstanding request to the RIB. The "_busy"
    ** flag marks if there is currently an outstanding request.
    */
    if (!_busy) {
	send_next_request();
    }
}

template<class A>
bool
NextHopRibRequest<A>::lookup(const A& nexthop, 
			     bool& resolvable, uint32_t& metric) const
{
    /*
    ** Make sure that we are not already waiting for a response for
    ** this sucker.
    */
    typename list<RibRequestQueueEntry<A> *>::const_iterator i;
    for (i = _queue.begin(); i != _queue.end(); i++) {
	RibRegisterQueueEntry<A> *rr = 
	    dynamic_cast<RibRegisterQueueEntry<A> *>(*i);
	if (rr && rr->reregister() && rr->nexthop() == nexthop) {
	    resolvable = rr->resolvable();
	    metric = rr->metric();
	    debug_msg("nexthop %s resolvable %d metric %d\n",
		      nexthop.str().c_str(), resolvable, metric);
	    return true;
	}
    }

    debug_msg("nexthop %s not resolvable\n", nexthop.str().c_str());

    return false;
}

template<>
void
NextHopRibRequest<IPv4>::deregister_interest(IPv4 addr, 
					     uint32_t prefix_len)
{
    debug_msg("addr %s/%d\n", addr.str().c_str(), prefix_len);
    if(0 == _xrl_router)	// The test code sets _xrl_router to zero
	return;

    XrlRibV0p1Client rib(_xrl_router);
    rib.send_deregister_interest4(_ribname.c_str(),
				  _xrl_router->name(),
				  addr,
				  prefix_len,
	    ::callback(this,&NextHopRibRequest::deregister_interest_response,
		       addr, prefix_len,
		       c_format("deregister_from_rib: addr %s/%d",
				addr.str().c_str(), prefix_len)));
}

template<>
void
NextHopRibRequest<IPv6>::deregister_interest(IPv6 addr, 
					     uint32_t prefix_len)
{
    debug_msg("addr %s/%d\n", addr.str().c_str(), prefix_len);
    if(0 == _xrl_router)	// The test code sets _xrl_router to zero
	return;

    XrlRibV0p1Client rib(_xrl_router);
    rib.send_deregister_interest6(_ribname.c_str(),
				  _xrl_router->name(),
				  addr,
				  prefix_len,
	    ::callback(this,&NextHopRibRequest::deregister_interest_response,
		       addr, prefix_len,
		       c_format("deregister_from_rib: addr %s/%d",
				addr.str().c_str(), prefix_len)));
}

template <class A>
void
NextHopRibRequest<A>::deregister_interest_response(const XrlError& error, 
						   A addr,
						   uint32_t prefix_len,
						   string comment)
{
    //Check that this answer is for the question on the front of the queue
    XLOG_ASSERT(!_queue.empty());
    RibDeregisterQueueEntry<A> *rd = 
	dynamic_cast<RibDeregisterQueueEntry<A> *>(_queue.front());
    XLOG_ASSERT(rd != NULL);
    XLOG_ASSERT(addr == rd->base_addr());
    XLOG_ASSERT(prefix_len == rd->prefix_len());

    debug_msg("%s %s\n", comment.c_str(), error.str().c_str());
    if (error != XrlError::OKAY()) {
	if (error == XrlError::NO_FINDER()
	    /* || (error == FATAL_TRANSPORT_ERROR()) */
	    || (error == XrlError::RESOLVE_FAILED() 
		&& _previously_successful) ) {
	    //A NO_FINDER or FATAL_TRANSPORT_ERROR error is always
	    //unrecoverable.  A RESOLVE_FAILED error when it had
	    //previously been successful is also unrecoverable.
	    _interface_failed = true;
	    while (!_queue.empty()) {
		delete _queue.front();
		_queue.pop_front();
	    }
	    return;
	}
	if (error == XrlError::RESOLVE_FAILED()
	    || error == XrlError::SEND_FAILED()) {
	    //SEND_FAILED can be a transient error.  RESOLVE_FAILED
	    //when we hadn't been previously successful might indicate
	    //an ordering problem at startup.  In both cases, we resend.
	    XLOG_WARNING("%s %s", comment.c_str(), error.str().c_str());
	    
	    //The request will still be on the request queue.
	    //All we need to do is resend it after a respectable delay
	    _rtx_delay_timer = 
		_next_hop_resolver.event_loop().new_oneoff_after_ms(1000,
                      ::callback(this,
				 &NextHopRibRequest<A>::deregister_interest,
				 addr, prefix_len));
	    return;
	}

	//If we fell through to here, something bad happened, but it's
	//not clear exactly what.  Log a warning, and carry on
	//regardless.
	XLOG_WARNING("callback: %s %s",  comment.c_str(), error.str().c_str());
    } else {
	//We were successful
	_previously_successful = true;
    }

    //remove this request from the queue
    delete rd;
    _queue.pop_front();

    //if there's anything else queued, send it.
    if (_queue.empty())
	_busy = false;
    else
	send_next_request();

    return;
}

/****************************************/

template <class A>
NHRequest<A>::NHRequest()
    : _request_total(0)
{
}

template <class A>
NHRequest<A>::NHRequest(IPNet<A> net, NhLookupTable<A> *requester)
    : _request_total(0)
{
    add_request(net, requester);
}

template <class A>
void
NHRequest<A>::add_request(IPNet<A> net, NhLookupTable<A> *requester)
{
    _request_total++;
    if (_request_map.find(requester) == _request_map.end()) {
	_requesters.insert(requester);
	_request_map[requester].insert(net);
    } else {
	_request_map[requester].insert(net);
    }
}

template <class A>
bool
NHRequest<A>::remove_request(IPNet<A> net, NhLookupTable<A> *requester)
{
    typename map <NhLookupTable<A> *, set<IPNet<A> > >::iterator i;
    i = _request_map.find(requester);
    if (i == _request_map.end())
	return false;
    set<IPNet<A> >* nets = &(i->second);
    typename set<IPNet<A> >::iterator nets_iter = nets->find(net);
    if (nets_iter == nets->end())
	return false;
    nets->erase(nets_iter);
    _request_total--;

    return true;
}

template <class A>
const set <IPNet<A> >&
NHRequest<A>::request_nets(NhLookupTable<A>* requester) const
{
    typename map <NhLookupTable<A> *, set<IPNet<A> > >::const_iterator i;
    i = _request_map.find(requester);
    assert(i != _request_map.end());

    return i->second;
}

//force these templates to be built
template class NextHopResolver<IPv4>;
template class NextHopResolver<IPv6>;
