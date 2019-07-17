// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __BGP_ROUTE_TABLE_DECISION_HH__
#define __BGP_ROUTE_TABLE_DECISION_HH__


#include "route_table_base.hh"
#include "dump_iterators.hh"
#include "peer_handler.hh"
#include "next_hop_resolver.hh"
#include "peer_route_pair.hh"

/**
 * Container for a route and the meta-data about the origin of a route
 * used in the DecisionTable decision process.
 */

template<class A>
class RouteData {
public:
    RouteData(const SubnetRoute<A>* route, 
	      FPAListRef pa_list,
	      BGPRouteTable<A>* parent_table,
	      const PeerHandler* peer_handler,
	      uint32_t genid) 
	: _route(route), _pa_list(pa_list), _parent_table(parent_table), 
	  _peer_handler(peer_handler), _genid(genid) {}

    RouteData(const RouteData<A>& him) :
	    _route(him._route), _pa_list(him._pa_list), _parent_table(him._parent_table),
	    _peer_handler(him._peer_handler), _genid(him._genid) { }

    /* main reason for defining operator= is to keep the refcount
       correct on _pa_list */
    RouteData<A>& operator=(const RouteData<A>& him) {
	_route = him._route;
	_pa_list = him._pa_list;
	_parent_table = him._parent_table;
	_peer_handler = him._peer_handler;
	_genid = him._genid;
	return *this;
    }

    void set_is_not_winner() {
	_parent_table->route_used(_route, false);
	_route->set_is_not_winner();
    }

    void set_is_winner(int igp_distance) {
	_parent_table->route_used(_route, true);
	_route->set_is_winner(igp_distance);
    }
    const SubnetRoute<A>* route() const { return _route; }
    const FPAListRef& attributes() const { return _pa_list; }
    const PeerHandler* peer_handler() const { return _peer_handler; }
    BGPRouteTable<A>* parent_table() const { return _parent_table; }
    uint32_t genid() const { return _genid; }
private:
    const SubnetRoute<A>* _route;
    FPAListRef _pa_list;
    BGPRouteTable<A>* _parent_table;
    const PeerHandler* _peer_handler;
    uint32_t _genid;
};

/**
 * @short BGPRouteTable which receives routes from all peers and
 * decided which routes win.
 *
 * The XORP BGP is internally implemented as a set of pipelines
 * consisting of a series of BGPRouteTables.  Each pipeline receives
 * routes from a BGP peer, stores them, and applies filters to them to
 * modify the routes.  Then the pipelines converge on a single
 * decision process, which decides which route wins amongst possible
 * alternative routes.  
 *
 * DecisionTable is a BGPRouteTable which performs this decision
 * process.  It has many upstream BGPRouteTables and a single
 * downstream BGPRouteTable.  Only the winning routes according to the
 * BGP decision process are propagated downstream.
 *
 * When a new route reaches DecisionTable from one peer, we must
 * lookup that route in all the other upstream branches to see if this
 * route wins, or even if it doesn't win, if it causes a change of
 * winning route.  Similarly for route deletions coming from a peer,
 * etc.
 */

template<class A>
class DecisionTable : public BGPRouteTable<A>  {
public:
    DecisionTable(string tablename,
		  Safi safi,
		  NextHopResolver<A>& next_hop_resolver);
    ~DecisionTable();
    int add_parent(BGPRouteTable<A> *parent,
		   PeerHandler *peer_handler,
		   uint32_t genid);
    int remove_parent(BGPRouteTable<A> *parent);

    int add_route(InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(InternalMessage<A> &old_rtmsg,
		      InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int route_dump(InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *peer);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid,
				       FPAListRef& pa_list) const;

    //don't call this on a DecisionTable - it's meaningless
    BGPRouteTable<A> *parent() { abort(); return NULL; }

    RouteTableType type() const {return DECISION_TABLE;}
    string str() const;

    bool get_next_message(BGPRouteTable<A> */*next_table*/) {
	abort();
	return false;
    }

    bool dump_next_route(DumpIterator<A>& dump_iter);
    /**
     * Notification that the status of this next hop has changed.
     *
     * @param bgp_nexthop The next hop that has changed.
     */
    virtual void igp_nexthop_changed(const A& bgp_nexthop);

    void peering_went_down(const PeerHandler *peer, uint32_t genid,
			   BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
			       BGPRouteTable<A> *caller);
    void peering_came_up(const PeerHandler *peer, uint32_t genid,
			 BGPRouteTable<A> *caller);

private:
    const SubnetRoute<A> *lookup_route(const BGPRouteTable<A>* ignore_parent,
				       const IPNet<A> &net,
				       const PeerHandler*& best_routes_peer,
				       BGPRouteTable<A>*& best_routes_parent
				       ) const;

    RouteData<A>* 
        find_alternative_routes(const BGPRouteTable<A> *caller,
				const IPNet<A>& net,
				list <RouteData<A> >& alternatives) const;
    uint32_t local_pref(const FPAListRef& pa_list) const;
    uint32_t med(const FPAListRef& pa_list) const;
    bool resolvable(const A) const;
    uint32_t igp_distance(const A) const;
    RouteData<A>* find_winner(list<RouteData<A> >& alternatives) const;
    map<BGPRouteTable<A>*, PeerTableInfo<A>* > _parents;
    map<uint32_t, PeerTableInfo<A>* > _sorted_parents;

    NextHopResolver<A>& _next_hop_resolver;
};

#endif // __BGP_ROUTE_TABLE_DECISION_HH__
