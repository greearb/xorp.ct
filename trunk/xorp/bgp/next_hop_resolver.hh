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

// $XORP: xorp/bgp/next_hop_resolver.hh,v 1.18 2003/06/26 21:38:53 jcardona Exp $

#ifndef __BGP_NEXT_HOP_RESOLVER_HH__
#define __BGP_NEXT_HOP_RESOLVER_HH__

#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <list>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/ref_trie.hh"
#include "xrl/targets/ribclient_base.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/rib_xif.hh"

template<class A> class NhLookupTable;
template<class A> class NextHopCache;
template<class A> class NextHopResolver;
template<class A> class NextHopRibRequest;
template<class A> class DecisionTable;
template<class A> class NHRequest;

/**
 * Next hop resolvability and IGP distances are accessed through this class.
 *
 * Next hop resolvability and IGP distances are retrieved from the RIB
 * and cached here in BGP. This retrieval process implicitly registers
 * interest with the RIB regarding these next hops. Thus any changes in
 * these next hops is signalled by the RIB to BGP via callbacks.
 *
 * If the state of a next hop changes (resolvable/unresolvable), or an
 * IGP distance changes, then it is possible that a new route may now
 * win the decision process. The decision process must therefore be
 * re-run for all routes that are affected by a next hop change. This
 * re-run of the decision process is achieved calling
 * "igp_nexthop_changed" on the decision process.
 *
 * What questions can be asked about next hops? Is a next hop
 * resolvable and if it is, what is the IGP distance.
 *
 * To answer questions about next hops three interfaces are supported:
 * 1) An asynchronous interface that registers a callback which will be
 * called when a response becomes available. For use by the (next
 * hop) route table before decision. By the time a route gets to
 * decision it *must* be known if the route is resolvable.
 * 2) A synchronous interface for use by decision. It is a fatal error
 * if this interface is called and the next hop is not in the
 * cache. As by the time decision is called the cache should have been
 * populated by use of the asynchronous interface.
 * 3) A synchronous debugging interface.
 *
 * Cache maintainance:
 * Every stored SubnetRoute in every rib in has a next hop. Every
 * unique next hop has an entry in the cache. If a next hop lookup
 * through the asynchronous interface causes a cache miss then an entry
 * is created with a reference count of 1. Subsequent lookups through
 * the next hop interface will cause the reference count to be
 * incremented by 1. An interface to increase the reference count by
 * more than one also exists. All route deletions should explicitly
 * call a routine in here to decrement the reference count.
 */

class BGPMain;

template<class A>
class NextHopResolver {
public:
    NextHopResolver(XrlStdRouter *xrl_router, EventLoop& eventloop,
		    BGPMain& bgp);

    virtual ~NextHopResolver();

    /**
     * Add decision.
     *
     * Pass a pointer to the decision table into the next hop
     * resolver. This pointer is used to notify the decision table
     * when a next hop metric changes.
     *
     * @param decision Pointer to the decision table.
     */
    void add_decision(DecisionTable<A> *decision);

    /**
    * Set the rib's name, allows for having a dummy rib or not having
    * a RIB at all.
    */
    bool register_ribname(const string& r);

    /**
     * Register interest in this nexthop.
     *
     * @param nexthop Nexthop.
     * @param net_from_route The net that is associated with this
     * nexthop in the NextHopLookupTable. Treated as an opaque id.
     * @param requester Once the registration with the RIB suceeds the
     * requester is called back.
     * @return True if the registration succeed.
     */
    virtual bool register_nexthop(A nexthop, IPNet<A> net_from_route,
				  NhLookupTable<A> *requester);

    /**
     * De-Register interest in this nexthop.
     *
     * @param nexthop Nexthop.
     * @param net_from_route The net that is associated with this
     * nexthop in the NextHopLookupTable. Treated as an opaque id.
     * @param requester Original requester, not used.
     * @return True if the registration succeed.
     */
    virtual void deregister_nexthop(A nexthop, IPNet<A> net_from_route,
				    NhLookupTable<A> *requester);


    /**
     * Lookup next hop.
     *
     * If a "register_nexthop" request has been made and callback has
     * taken place via the "requester" pointer, then the lookup is
     * guaranteed to work.
     *
     * @param nexthop Next hop.
     * @param resolvable Is this route resolvable.
     * @param metric If this route is resolvable the metric of this
     * route.
     * @return True if this next hop is found.
     */
    virtual bool lookup(const A nexthop, bool& resolvable,
			uint32_t& metric) const;

    /**
     * Call from the RIB to notify us that a metric has changed.
     */
    bool rib_client_route_info_changed(const A& addr,
				       const uint32_t& real_prefix_len,
				       const A& nexthop,
				       const uint32_t& metric);

    /**
     * Call from the RIB to notify us that any registrations with this
     * address and prefix_len are now invalid.
     */
    bool rib_client_route_info_invalid(const A&	addr,
				       const uint32_t&	prefix_len);

    /**
     * Next hop changed.
     *
     * Whenever a next hop changes this method should be called and
     * the change will be rippled up to the decision process.
     *
     * @param nexthop The next hop that has changed.
     */
    void next_hop_changed(A nexthop);

    /**
     * Next hop changed.
     *
     * Whenever a next hop changes this method should be called and
     * the change will be rippled up to the decision process. However
     * if a change occurs but the metrics don't change don't bother to
     * ripple up the change there is no point.
     *
     * @param nexthop The next hop that has changed.
     * @param old_resolves The old resolve value.
     * @param old_metric The old metric value.
     */
    void next_hop_changed(A nexthop, bool old_resolves, uint32_t old_metric);

    /**
     * Get NextHopRibRequest pointer.
     *
     * Used for testing.
     */
    NextHopRibRequest<A> *get_next_hop_rib_request()
    {
	return &_next_hop_rib_request;
    }

    /**
     * Get a reference to the main timer list
     */
    EventLoop& eventloop() {return _eventloop;}

    /**
     * Get the status of the NextHopResolver
     *
     * @param reason the human-readable reason for any failure
     *
     * @return false if NextHopResolver has suffered a fatal error,
     * true otherwise 
     */
    bool status(string& reason) const;
protected:
    DecisionTable<A> *_decision;
private:
    string _ribname;	// RIB name to use in XRL calls
    XrlStdRouter *_xrl_router;
    EventLoop& _eventloop;
    BGPMain& _bgp;
    NextHopCache<A> _next_hop_cache;
    NextHopRibRequest<A> _next_hop_rib_request;
};

/**
 * A cache of next hop information.
 *
 * BGP requires information regarding resolvabilty and metrics for
 * next hops. This information is known by the RIB. Questions are
 * asked of the RIB and the results are cached here. The RIB notes
 * that questions have been asked and if the state of a next hop
 * changes then this is reported back to BGP. In order to save space the
 * RIB does not record information about each next hop but returns an
 * address/prefix_len range for which the answer is valid.
 *
 * Not only can the RIB report changes but can also report that a
 * previous entry is totally invalid. In the case that an entry is
 * invalid all the next hops need to be re-requested.
 *
 */
template<class A>
class NextHopCache {
public:
    ~NextHopCache();

    /**
     * Add an entry to our next hop table.
     *
     * @param addr Base address.
     * @param nexthop Next hop that is being added to the trie.
     * @param prefix_len The prefix_len that is masked with the nexhop.
     * @param real_prefix_len The actual prefix_len that this next hop
     * resolves too. This is only used to match with upcalls from the
     * RIB.
     * @param resolvable Is this route resolvable.
     * @param metric If this route is resolvable its metric.
     */
    void add_entry(A addr, A nexthop, int prefix_len, int real_prefix_len,
		   bool resolvable, uint32_t metric = 0);

    /**
     * Validate an entry.
     *
     * add_entry creates an entry with no nexthop references. The
     * assumption is that a register_nexthop will follow shortly after
     * initial creation. It is possible due to a deregister_nexthop
     * coming in while we are waiting for a response from the RIB that
     * the register_nexthop never happens. This method checks that the
     * specified entry is referenced and if it isn't it is deleted.
     *
     * @param addr Base address.
     * @param nexthop Next hop that is being added to the trie.
     * @param prefix_len The prefix_len that is masked with the nexhop.
     * @param real_prefix_len The actual prefix_len that this next hop
     * @return true if the entry is in use.
     */
    bool validate_entry(A addr, A nexthop, int prefix_len, int real_prefix_len);

    /**
     * Change an entry in the next hop table.
     *
     * @param addr The base address.
     * @param real_prefix_len The actual prefix_len that this next hop
     * resolves too. This is only used to match with upcalls from the
     * RIB.
     * @param metric If this route is resolvable its metric.
     * @return The map of next hops with reference counts that were
     * covered by this entry.
     *
     */
    map <A, int> change_entry(A addr, int real_prefix_len, uint32_t metric);

    /**
     * Delete an entry from the nexthop table.
     *
     * It is a fatal error to attempt to delete an entry that doesn't
     * exist.
     *
     * @param addr Base address that is being removed from the trie.
     * @param prefix_len The prefix_len.
     * @return The map of next hops with reference counts that were
     * covered by this entry.
     */
    map <A, int> delete_entry(A addr, int prefix_len);

    /**
     * Lookup by base address
     *
     * @param addr Base address.
     * @param prefix_len Prefix_Len.
     * @param resolvable Is this route resolvable.
     * @param metric If this route is resolvable the metric of this route.
     * @return True if this next hop is found.
     *
     */
    bool lookup_by_addr(A addr, int prefix_len, bool& resolvable,
			uint32_t& metric) const;

    /**
     * Lookup next hop.
     *
     * @param nexthop Next hop.
     * @param resolvable Is this route resolvable.
     * @param metric If this route is resolvable the metric of this
     * route.
     * @return True if this next hop is found.
     *
     */
    bool lookup_by_nexthop(A nexthop, bool& resolvable, uint32_t& metric)
	const;

    /**
     * Lookup next hop without entry
     *
     * This lookup does not require that next hop is already
     * known. That is the next hop is not in _nexthop_references.
     *
     * @param nexthop Next hop.
     * @param resolvable Is this route resolvable.
     * @param metric If this route is resolvable the metric of this
     * route.
     * @return True if this next hop is found.
     *
     */
    bool lookup_by_nexthop_without_entry(A nexthop, bool& resolvable,
					 uint32_t& metric) const;

    /*
     * Try and register this next hop.
     *
     * If this next hop is known or covered then this next hop is
     * added to map of next hops that is associated with the
     * NextHopEntry.
     *
     * @param nexthop Next hop.
     * @param ref_cnt_incr How much to increase the reference count by.
     * @return True if this next hop is known or its in a covered
     * range.
     */
    bool register_nexthop(A nexthop, int ref_cnt_incr = 1);

    /*
     * Deregister this next hop.
     *
     * The NextHopEntry has a map of next hops with reference
     * counts. A deregister causes this next hop to be removed from
     * its associated NextHopEntry. If the map becomes empty then the
     * entry can be removed. If the map becomes empty then this fact
     * is signalled in the return value and this next hop can be
     * deregistered from the RIB.
     *
     * @param nexthop Next hop.
     * @param last True if this is the last next hop and the entry
     * has been freed.
     * @param addr If this was the last entry the base address.
     * @param prefix_len If this was the last entry the associated prefix_len.
     * @return True if an entry was found.
     */
    bool deregister_nexthop(A nexthop, bool& last, A& addr, uint32_t& prefix_len);
private:
    struct NextHopEntry {
	A _address;	// Base address as returned by the RIB
#ifdef	USE_NEXTHOP
	A _nexthop;	// The initial next hop. Used to find entry by
			// prefix_len
#endif
	map <A, int> _nexthop_references;
	int _prefix_len;
	int _real_prefix_len;
	bool _resolvable;
	int _metric;
    };

    typedef set<NextHopEntry *> RealPrefixEntry;
    typedef RefTriePostOrderIterator<A, RealPrefixEntry> RealPrefixIterator;

    typedef NextHopEntry PrefixEntry;
    typedef RefTriePostOrderIterator<A, PrefixEntry *> PrefixIterator;

    /**
     * The NextHopEntry is indexed in two ways either by prefix_len or by
     * real prefix_len.
     *
     * Both of these data structures need to be kept in sync.
     */
    RefTrie<A, PrefixEntry *> _next_hop_by_prefix;
    RefTrie<A, RealPrefixEntry> _next_hop_by_real_prefix;

    /**
    * Given a real prefix_len entry return a prefix_len entry.
    *
    * @param pe A real prefix_len entry.
    * @param addr Address.
    * @param real_prefix_len The real prefix_len.
    * @return A prefix_len entry if found 0 otherwise.
    */
    PrefixEntry *rpe_to_pe(const RealPrefixEntry& pe, A addr,
			   int real_prefix_len) const;
    /**
    * Given a real prefix_len entry return a prefix_len entry.
    *
    * @param pe A real prefix_len entry.
    * @param addr Address.
    * @param real_prefix_len The real prefix_len.
    * @return A prefix_len entry if found 0 otherwise.
    */
    PrefixEntry *rpe_to_pe_delete(RealPrefixEntry& pe, A addr,
				  int real_prefix_len);
};

/**
 * The queue of outstanding requests to the RIB. Requests can have
 * arrived in this queue in two ways. A simple call down from the next
 * hop table or due to the previous result being marked invalid by
 * an upcall from the RIB. The class variables "_register" and
 * "_reregister" denote how the entry was created. It is possible
 * that an upcall from the RIB has caused a queue entry, followed
 * by a downcall from the next hop table in which case both
 * "_register" and "_reregister" will be true.
 */
template <class A>
class RibRequestQueueEntry {
public:
    typedef enum {REGISTER, DEREGISTER} RegisterMode;
    RibRequestQueueEntry(RegisterMode mode) : _register_mode(mode) {}
    virtual ~RibRequestQueueEntry() {}
protected:
    RegisterMode _register_mode;
};

template <class A>
class RibRegisterQueueEntry : public RibRequestQueueEntry<A> {
public:
    RibRegisterQueueEntry(A nexthop, IPNet<A> net_from_route, 
			 NhLookupTable<A> *requester)
	: RibRequestQueueEntry<A>(REGISTER),
	  _nexthop(nexthop), _new_register(true), 
	  _requests(net_from_route, requester),
	  _reregister(false), _ref_cnt(0)
    {}
    RibRegisterQueueEntry(A nexthop, uint32_t ref_cnt, bool resolvable, 
			   uint32_t metric)
	: RibRequestQueueEntry<A>(REGISTER),
	  _nexthop(nexthop), _new_register(false), _reregister(true),
	  _ref_cnt(ref_cnt), _resolvable(resolvable), _metric(metric)
    {}

    void register_nexthop(IPNet<A> net_from_route, 
			  NhLookupTable<A> *requester) {
	XLOG_ASSERT(true == _reregister || true == _new_register);
	XLOG_ASSERT(_register_mode == REGISTER);
	_new_register = true;
	_requests.add_request(net_from_route, requester);
    }

    bool deregister_nexthop(IPNet<A> net_from_route, 
			    NhLookupTable<A> *requester) {
	XLOG_ASSERT(true == _reregister || true == _new_register);
	XLOG_ASSERT(_register_mode == REGISTER);
	if (_new_register && _requests.remove_request(net_from_route, 
						      requester)) {
	    return true;
	}
	if (_reregister) {
	    XLOG_ASSERT(_ref_cnt > 0);
	    _ref_cnt--;
	    return true;
	}
	return false;
    }

    void reregister_nexthop(uint32_t ref_cnt, bool resolvable,
			    uint32_t metric) {
	XLOG_ASSERT(false == _reregister);
	XLOG_ASSERT(0 == _ref_cnt);
	XLOG_ASSERT(_register_mode == REGISTER);
	_reregister = true;
	_ref_cnt = ref_cnt;
	_resolvable = resolvable;
	_metric = metric;
    }

    bool resolvable() const {
	assert(_register_mode == REGISTER);
	return _resolvable;
    }
    bool reregister() const {
	assert(_register_mode == REGISTER);
	return _reregister;
    }

    bool new_register() const {
	assert(_register_mode == REGISTER);
	return _new_register;
    }
    bool metric() const {
	assert(_register_mode == REGISTER);
	return _metric;
    }
    const A& nexthop() const {
	return _nexthop;
    }
    uint32_t ref_cnt() const {
	return _ref_cnt;
    }
    NHRequest<A>& requests() {
	return _requests;
    }
private:
    /*
    ** Register info.
    */
    A _nexthop;
    bool _new_register;
    NHRequest<A> _requests;

    /*
    ** Reregister info.
    */
    bool _reregister;
    uint32_t _ref_cnt;
    /**
     * The old answer if we are in the process of reregistering so
     * that lookups will be satisfied with this old answer.
	 */
    bool _resolvable;
    uint32_t _metric;
};

template <class A>
class RibDeregisterQueueEntry : public RibRequestQueueEntry<A> {
public:
    RibDeregisterQueueEntry(A base_addr, uint32_t prefix_len)
	: RibRequestQueueEntry<A>(DEREGISTER), 
	_base_addr(base_addr), _prefix_len(prefix_len)

    {}
    const A& base_addr() const { return _base_addr;}
    uint32_t prefix_len() const { return _prefix_len;}
private:
    /*
    ** Deregister info.
    */
    A _base_addr;
    uint32_t _prefix_len;
};


/**
 * Make requests of the RIB and get responses.
 *
 * At any time there is only ever one outstanding request to the
 * RIB. Firstly we don't want to overrun the RIB with
 * requests. Secondly it is possible that different next hops in the
 * queue of requests may resolve to the same address/prefix_len answer
 * (see below).
 */
template<class A>
class NextHopRibRequest {
public:
    NextHopRibRequest(XrlStdRouter *,
		      NextHopResolver<A>& next_hop_resolver,
		      NextHopCache<A>& next_hop_cache,
		      BGPMain& bgp);
    ~NextHopRibRequest();

    bool register_ribname(const string& r) { _ribname = r; return true; }

    /**
     * Register interest with the RIB about this next hop.
     *
     * @param nexthop The next hop that we are attempting to resolve.
     * @param net The subnet that this next hop is associated with.
     * @param requester The lookup table that wants to be notified
     * when the response comes back.
     */
    void register_nexthop(A nexthop, IPNet<A> net,
			  NhLookupTable<A> *requester);

    /**
     * Send the next queued request
     */
    void send_next_request();

    /**
     * Actually register interest with the RIB.
     *
     * A small method that will be specialized to differentiate
     * between IPv4 and IPv6.
     *
     * @param nexthop The next hop that we are attempting to resolve.
     */
    void register_interest(A nexthop);

    /**
     * XRL callback from register_interest.
     */
    void register_interest_response(const XrlError& error,
				    const bool *resolves,
				    const A *addr,
				    const uint32_t *prefix_len,
				    const uint32_t *real_prefix_len,
				    const A *actual_nexthop,
				    const uint32_t *metric,
				    const A nexthop_interest,
				    const string comment);


    /**
     * Deregister interest with the RIB about this next hop.
     *
     * @param nexthop The next hop that we are attempting to resolve.
     * @param net The subnet that this next hop is associated with.
     * @param requester The lookup table that wants to be notified
     * when the response comes back.
     * @return True if an entry was found to remove.
     */
    bool deregister_nexthop(A nexthop, IPNet<A> net,
			    NhLookupTable<A> *requester);

    /**
     * Reregister interest with the RIB about this next hop.
     *
     * This method is used when the RIB tells us that all previous
     * registrations have become invalid. This forces us to re-request
     * information. We save the old state (resolvable, metric) just in
     * case the following events occur:
     * <PRE>
     * 1) Register from next hop table.
     * 2) route_info_invalid from RIB.
     * 3) lookup from decision.
     * </PRE>
     * This ordering of events may not be possible just in case it is
     * save the old result and return it in a lookup.
     *
     * @param nexthop The next hop that we are attempting to resolve.
     * @param ref_cnt The number of subnets using this nexthop.
     * @param resolvable Was the previous result resolvable.
     * @param metric If the previous result was resolvable the metric.
     */
    void reregister_nexthop(A nexthop, uint32_t ref_cnt, bool resolvable,
			    uint32_t metric);

    /**
     * lookup next hop.
     *
     * @param nexthop Next hop.
     * @param resolvable Is this route resolvable.
     * @param metric If this route is resolvable the metric of this
     * route.
     * @return True if this next hop is found.
     *
     */
    bool lookup(const A& nexthop, bool& resolvable, uint32_t& metric) const;

    /*
     * Deregister ourselves from the RIB for this next hop
     *
     * @param nexthop The next hop.
     * @param prefix_len The prefix_len we registered with.
     */
    void deregister_from_rib(const A& nexthop, uint32_t prefix_len);

    /*
     * Deregister ourselves from the RIB for this next hop
     *
     * @param nexthop The next hop.
     * @param prefix_len The prefix_len we registered with.
     */
    void deregister_interest(A nexthop, uint32_t prefix_len);

    /**
     * XRL response method.
     *
     * @param error Error returned by xrl call.
     * @param comment Comment string used for diagnostic purposes.
     */
    void deregister_interest_response(const XrlError& error, 
				      A addr,
				      uint32_t prefix_len,
				      string comment);

    /**
     * Get the status of the NextHopRibRequest
     *
     * @param reason the human-readable reason for any failure
     *
     * @return false if NextHopRibRequest has suffered a fatal error,
     * true otherwise 
     */
    bool status(string& reason) const;
private:
    string _ribname;
    XrlStdRouter *_xrl_router;
    NextHopResolver<A>& _next_hop_resolver;
    NextHopCache<A>& _next_hop_cache;
    BGPMain& _bgp;

    /**
     * Are we currently waiting for a response from the RIB.
     */
    bool _busy;
    /**
     * The queue of outstanding requests.
     */
    list<RibRequestQueueEntry<A> *> _queue;
    /**
     * Retransmit delay timer for resending (de)registrations.
     */
    XorpTimer _rtx_delay_timer;

    bool _interface_failed; /* true if we've received a fatal error */

    /**
     * Used by the destructor to delete all the "RibRequestQueueEntry" objects
     * that have been allocated.
     */
    static void zapper(RibRequestQueueEntry<A> *req) { delete req; }
};

template<class A>
class NHRequest {
public:
    NHRequest();
    NHRequest(IPNet<A> net,
	      NhLookupTable<A> *requester);
    void add_request(IPNet<A> net,
		     NhLookupTable<A> *requester);
    bool remove_request(IPNet<A> net,
			NhLookupTable<A> *requester);
    const set <NhLookupTable<A>*>& requesters() const {
	return _requesters;
    }
    const set <IPNet<A> >& request_nets(NhLookupTable<A>* requester) const;
    const int requests() { return _request_total; }
private:
    set <NhLookupTable<A> *> _requesters;
    map <NhLookupTable<A> *, set<IPNet<A> > > _request_map;
    int _request_total;
};

#endif // __BGP_NEXT_HOP_RESOLVER_HH__
