// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/bgp/plumbing.hh,v 1.27 2002/12/09 18:28:46 hodson Exp $

#ifndef __BGP_PLUMBING_HH__
#define __BGP_PLUMBING_HH__

#include <map>
#include "route_table_ribin.hh"
#include "route_table_ribout.hh"
#include "route_table_decision.hh"
#include "route_table_fanout.hh"
#include "route_table_filter.hh"
#include "route_table_cache.hh"
#include "route_table_nhlookup.hh"
#include "peer.hh"
#include "rib_ipc_handler.hh"
#include "next_hop_resolver.hh"

class BGPPlumbing;

template <class A>
class BGPPlumbingAF {
public:
    BGPPlumbingAF(string ribname, BGPPlumbing& master, XrlStdRouter *);

    int add_peering(PeerHandler* peer_handler);
    int stop_peering(PeerHandler* peer_handler);
    int peering_went_down(PeerHandler* peer_handler);
    int peering_came_up(PeerHandler* peer_handler);
    int delete_peering(PeerHandler* peer_handler);

    int add_route(const InternalMessage<A> &rtmsg, 
		  PeerHandler* peer_handler);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     PeerHandler* peer_handler);
    int delete_route(const IPNet<A> &net, 
		     PeerHandler* peer_handler);
    const SubnetRoute<A>* 
      lookup_route(const IPNet<A> &net) const;
    void push(PeerHandler* peer_handler);
    void output_no_longer_busy(PeerHandler* peer_handler);
    /**
     * Hook to the next hop resolver so that xrl calls from the RIB
     * can be passed through.
     */
    NextHopResolver<A>& next_hop_resolver() {return _next_hop_resolver;}
private:
    const A& get_local_nexthop(const PeerHandler *peer_handler) const;

    map <PeerHandler*, BGPRibInTable<A>* > _in_map;
    map <BGPRibOutTable<A>*,  PeerHandler*> _reverse_out_map;
    map <PeerHandler*, BGPRibOutTable<A>*> _out_map;
    BGPDecisionTable<A> *_decision_table;
    BGPFanoutTable<A> *_fanout_table;
    BGPRibInTable<A> *_ipc_rib_in_table;
    BGPRibOutTable<A> *_ipc_rib_out_table;
    /* _tables is all the tables not covered above*/
    set <BGPRouteTable<A>*> _tables;

    bool _awaits_push;
    string _ribname;
    BGPPlumbing& _master;

    NextHopResolver<A> _next_hop_resolver;
};


/*
 * BGP plumbing is the class that sets up all the BGP route tables,
 * from RIB-In to RIB-Out, plus everything in between.
 */

class RibIpcHandler;

class BGPPlumbing {
public:
    BGPPlumbing(XrlStdRouter *, RibIpcHandler* rib_handler);
    void set_my_as_number(const AsNum& my_AS_number);

    int add_peering(PeerHandler* peer_handler);
    int stop_peering(PeerHandler* peer_handler);
    int peering_went_down(PeerHandler* peer_handler);
    int peering_came_up(PeerHandler* peer_handler);
    int delete_peering(PeerHandler* peer_handler);

    int add_route(const InternalMessage<IPv4> &rtmsg, 
		  PeerHandler* peer_handler);
    int add_route(const InternalMessage<IPv6> &rtmsg, 
		  PeerHandler* peer_handler);
    int delete_route(const InternalMessage<IPv4> &rtmsg, 
		     PeerHandler* peer_handler);
    int delete_route(const InternalMessage<IPv6> &rtmsg, 
		     PeerHandler* peer_handler);
    int delete_route(const IPNet<IPv4> &net, 
		     PeerHandler* peer_handler);
    int delete_route(const IPNet<IPv6> &net,
		     PeerHandler* peer_handler);
    void push(PeerHandler* peer_handler);
    void output_no_longer_busy(PeerHandler* peer_handler);
    const SubnetRoute<IPv4>* 
      lookup_route(const IPNet<IPv4> &net) const;
    const SubnetRoute<IPv6>* 
      lookup_route(const IPNet<IPv6> &net) const;
    const AsNum& my_AS_number() const {return _my_AS_number;}
    RibIpcHandler *rib_handler() const {return _rib_handler;}
    BGPPlumbingAF<IPv4>& plumbing4() {return _v4_plumbing;}
    BGPPlumbingAF<IPv6>& plumbing6() {return _v6_plumbing;}
private:
    RibIpcHandler *_rib_handler;

    BGPPlumbingAF<IPv4> _v4_plumbing;
    BGPPlumbingAF<IPv6> _v6_plumbing;

    AsNum _my_AS_number;
};

#endif // __BGP_PLUMBING_HH__
