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

// $XORP: xorp/bgp/route_table_decision.hh,v 1.13 2004/05/15 15:12:16 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_DECISION_HH__
#define __BGP_ROUTE_TABLE_DECISION_HH__

#include <map>
#include "route_table_base.hh"
#include "peer_handler.hh"
#include "next_hop_resolver.hh"
#include "peer_route_pair.hh"

template<class A>
class RouteData {
public:
    RouteData(const SubnetRoute<A>* route, 
	      BGPRouteTable<A>* parent_table,
	      const PeerHandler* peer_handler,
	      uint32_t genid) 
	: _route(route), _parent_table(parent_table), 
	  _peer_handler(peer_handler), _genid(genid) {}

    inline void set_is_not_winner() {
	_parent_table->route_used(_route, false);
	_route->set_is_not_winner();
    }
    inline void set_is_winner(int igp_distance) {
	_parent_table->route_used(_route, true);
	_route->set_is_winner(igp_distance);
    }
    inline const SubnetRoute<A>* route() const {
	return _route;
    }
    inline  const PeerHandler* peer_handler() const {
	return _peer_handler;
    }
    inline BGPRouteTable<A>* parent_table() const {
	return _parent_table;
    }
    inline uint32_t genid() const {
	return _genid;
    }
private:
    const SubnetRoute<A>* _route;
    BGPRouteTable<A>* _parent_table;
    const PeerHandler* _peer_handler;
    uint32_t _genid;
};

template<class A>
class DecisionTable : public BGPRouteTable<A>  {
public:
    DecisionTable(string tablename,
		  Safi safi,
		  NextHopResolver<A>& next_hop_resolver);
    int add_parent(BGPRouteTable<A> *parent,
		   PeerHandler *peer_handler,
		   uint32_t genid);
    int remove_parent(BGPRouteTable<A> *parent);

    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int route_dump(const InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *peer);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid) const;

    //don't call this on a DecisionTable - it's meaningless
    BGPRouteTable<A> *parent() { abort(); return NULL; }

    RouteTableType type() const {return DECISION_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool /*busy*/, BGPRouteTable<A> */*next_table*/) {
	abort();
    }

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
    uint32_t local_pref(const SubnetRoute<A> *route, const PeerHandler *peer)
	const;
    uint32_t med(const SubnetRoute<A> *route) const;
    bool resolvable(const A) const;
    uint32_t igp_distance(const A) const;
    bool route_wins(const SubnetRoute<A> *test_route,
		    const PeerHandler *test_peer,
		    list<RouteData<A> >& alternatives) const;
    RouteData<A>* find_winner(list<RouteData<A> >& alternatives) const;
    map<BGPRouteTable<A>*, PeerTableInfo<A>* > _parents;
    map<PeerHandler*, PeerTableInfo<A>* > _rev_parents;

    NextHopResolver<A>& _next_hop_resolver;
};

#endif // __BGP_ROUTE_TABLE_DECISION_HH__
