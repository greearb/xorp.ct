// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/bgp/route_table_cache.hh,v 1.15 2004/06/10 22:40:33 hodson Exp $

#ifndef __BGP_ROUTE_TABLE_CACHE_HH__
#define __BGP_ROUTE_TABLE_CACHE_HH__

#include <queue>
#include "libxorp/timer.hh"
#include "route_table_base.hh"
#include "libxorp/ref_trie.hh"
#include "peer_handler.hh"

/**
 * Used in CacheTable to store a SubnetRoute and the genid of the
 * RibIn that generated the route.
 */

template<class A>
class CacheRoute {
public:
    CacheRoute(const SubnetRoute<A>* route, uint32_t genid) 
	: _routeref(route), _genid(genid) {}
    inline const SubnetRoute<A>* route() const {return _routeref.route();}
    inline uint32_t genid() const {return _genid;}
private:
    SubnetRouteConstRef<A> _routeref;
    uint32_t _genid;
};


class EventLoop;

/**
 * @short specialized BGPRouteTable that stores routes modified by a
 * FilterTable.
 *
 * The XORP BGP is internally implemented as a set of pipelines
 * consisting of a series of BGPRouteTables.  Each pipeline receives
 * routes from a BGP peer, stores them, and applies filters to them to
 * modify the routes.  Then the pipelines converge on a single
 * decision process, which decides which route wins amongst possible
 * alternative routes.  After decision, the winning routes fanout
 * again along a set of pipelines, again being filtered, before being
 * transmitted to peers.
 *
 * Typically there are two FilterTables for each peer which modify
 * routes passing down the pipeline, one in the input branch from that
 * peer, and one in the output branch to that peer.  A FilterTable
 * does not store the routes it modifies, so a CacheTable is placed
 * downstream of a FilterTable to store routes that are modified.
 *
 * In the current code, a CacheTable isn't strictly necessary, but it
 * simplifies life for all downstream tables, because they know that
 * all routes they receive are stored in stable storage and won't go
 * away without explicit notification.  
 *
 * In the future, it is likely that the CacheTables in the outgoing
 * branches will be removed, as they waste quite a bit of memory for
 * very little gain.  However, they have not yet been removed because
 * their internal sanity checks have served as a valuable debugging
 * device to ensure consistency in the messages travelling down the
 * outgoing branches.
 */

template<class A>
class CacheTable : public BGPRouteTable<A>  {
public:
    CacheTable(string tablename, Safi safi, BGPRouteTable<A> *parent,
	       const PeerHandler *peer);
    ~CacheTable();
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    int route_dump(const InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);

    void flush_cache();
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return CACHE_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    int route_count() const {
	return _route_table->route_count();
    }
    EventLoop& eventloop() const;

private:
    RefTrie<A, const CacheRoute<A> > *_route_table;
    const PeerHandler *_peer;
};

/**
 * @short Delete nodes in the cache table trie.
 */
template<class A>
class DeleteAllNodes {
public:
    typedef RefTrie<A, const CacheRoute<A> > RouteTable;
    typedef queue<RouteTable *> RouteTables;

    DeleteAllNodes(const PeerHandler *peer, 
		   RefTrie<A, const CacheRoute<A> > *route_table)
	: _peer(peer) {

	    bool empty = _route_tables.empty();
	    _route_tables.push(route_table);

	    if (empty) {
		_deleter =  peer->eventloop().
		    new_periodic(1000,
				 callback(this,
				       &DeleteAllNodes<A>::delete_some_nodes));
	    } else {
		delete this;
	    }
	}

    /**
     * Background task that deletes nodes it trie.
     *
     * @return true if there are routes to delete.
     */
    bool delete_some_nodes() {
	RouteTable *route_table = _route_tables.front();
	typename RouteTable::iterator current = route_table->begin();
	for(int i = 0; i < _deletions_per_call; i++) {
	    route_table->erase(current);
	    if (current == route_table->end()) {
		_route_tables.pop();
		route_table->delete_self();
		break;
	    }
	}

	bool empty = _route_tables.empty();
	if (empty)
	    delete this;

	return !empty;
    }

    /**
     * @return true if route tables exist and are still being deleted.
     */
    static bool running() {
	return !_route_tables.empty();
    }
private:
    static RouteTables _route_tables;	// Queue of route tables to delete.
    XorpTimer _deleter;			// 
    const PeerHandler *_peer;		// Handle to the EventLoop.
    static int _deletions_per_call;	// Number of nodes deleted per call.
};

#endif // __BGP_ROUTE_TABLE_CACHE_HH__
