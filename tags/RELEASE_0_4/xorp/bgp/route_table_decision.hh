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

// $XORP: xorp/bgp/route_table_decision.hh,v 1.6 2003/05/29 17:59:08 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_DECISION_HH__
#define __BGP_ROUTE_TABLE_DECISION_HH__

#include <map>
#include "route_table_base.hh"
#include "peer_handler.hh"
#include "next_hop_resolver.hh"

template<class A>
class DecisionTable : public BGPRouteTable<A>  {
public:
    DecisionTable(string tablename, NextHopResolver<A>& next_hop_resolver);
    int add_parent(BGPRouteTable<A> *parent,
		   PeerHandler *peer_handler);
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
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net) const;

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

private:
    const SubnetRoute<A> *lookup_route(const BGPRouteTable<A>* ignore_parent,
				       const IPNet<A> &net,
				       const PeerHandler*& best_routes_peer,
				       BGPRouteTable<A>*& best_routes_parent
				       ) const;

    bool find_previous_winner(BGPRouteTable<A> *caller,
			      const IPNet<A>& net,
			      const SubnetRoute<A>*& best_route,
			      const PeerHandler*& best_routes_peer,
			      BGPRouteTable<A>*& best_routes_parent) const;
    uint32_t local_pref(const SubnetRoute<A> *route, const PeerHandler *peer)
	const;
    uint32_t med(const SubnetRoute<A> *route) const;
    bool resolvable(const A) const;
    uint32_t igp_distance(const A) const;
    bool route_is_better(const SubnetRoute<A> *our_route,
			 const PeerHandler *out_peer,
			 const SubnetRoute<A> *test_route,
			 const PeerHandler *test_peer) const;
    map<BGPRouteTable<A>*, PeerHandler* > _parents;

    NextHopResolver<A>& _next_hop_resolver;
};

#endif // __BGP_ROUTE_TABLE_DECISION_HH__
