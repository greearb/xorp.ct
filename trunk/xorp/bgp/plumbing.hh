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

// $XORP: xorp/bgp/plumbing.hh,v 1.15 2003/10/13 23:42:26 atanu Exp $

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
class RouteTableReader;

template <class A>
class BGPPlumbingAF {
public:
    BGPPlumbingAF(const string& ribname, BGPPlumbing& master,
		  NextHopResolver<A>& next_hop_resolver);
    ~BGPPlumbingAF();
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
    uint32_t create_route_table_reader();
    bool read_next_route(uint32_t token, 
			 const SubnetRoute<A>*& route, 
			 IPv4& peer_id);


    /**
     * Get the status of the Plumbing
     *
     * @param reason the human-readable reason for any failure
     *
     * @return false if Plumbing has suffered a fatal error,
     * true otherwise 
     */
    bool status(string& reason) const;
private:
    const A& get_local_nexthop(const PeerHandler *peer_handler) const;
    list <RibInTable<A>*> ribin_list() const;

    map <PeerHandler*, RibInTable<A>* > _in_map;
    map <RibOutTable<A>*,  PeerHandler*> _reverse_out_map;
    map <PeerHandler*, RibOutTable<A>*> _out_map;
    DecisionTable<A> *_decision_table;
    FanoutTable<A> *_fanout_table;
    RibInTable<A> *_ipc_rib_in_table;
    RibOutTable<A> *_ipc_rib_out_table;
    /* _tables is all the tables not covered above*/
    set <BGPRouteTable<A>*> _tables;

    uint32_t _max_reader_token;
    map <uint32_t, RouteTableReader<A>*> _route_table_readers;

    bool _awaits_push;
    string _ribname;
    BGPPlumbing& _master;

    NextHopResolver<A>& _next_hop_resolver;
};


/*
 * BGP plumbing is the class that sets up all the BGP route tables,
 * from RIB-In to RIB-Out, plus everything in between.
 */

class RibIpcHandler;

class BGPPlumbing {
public:
    BGPPlumbing(const string& safi,
		RibIpcHandler* rib_handler,
		NextHopResolver<IPv4>&,
		NextHopResolver<IPv6>&);
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
    void push_ipv4(PeerHandler* peer_handler);
    void push_ipv6(PeerHandler* peer_handler);
    void output_no_longer_busy(PeerHandler* peer_handler);
    const SubnetRoute<IPv4>* 
      lookup_route(const IPNet<IPv4> &net) const;
    const SubnetRoute<IPv6>* 
      lookup_route(const IPNet<IPv6> &net) const;
    const AsNum& my_AS_number() const {return _my_AS_number;}
    RibIpcHandler *rib_handler() const {return _rib_handler;}
    BGPPlumbingAF<IPv4>& plumbing_ipv4() {
	return _plumbing_ipv4;
    }
    BGPPlumbingAF<IPv6>& plumbing_ipv6() {
	return _plumbing_ipv6;
    }

    uint32_t create_ipv4_route_table_reader();
    uint32_t create_ipv6_route_table_reader();
    bool read_next_route(uint32_t token, 
			 const SubnetRoute<IPv4>*& route, 
			 IPv4& peer_id);
    bool read_next_route(uint32_t token, 
			 const SubnetRoute<IPv6>*& route, 
			 IPv4& peer_id);

    /**
     * Get the status of the Plumbing
     *
     * @param reason the human-readable reason for any failure
     *
     * @return false if Plumbing has suffered a fatal error,
     * true otherwise 
     */
    bool status(string& reason) const;
private:
    RibIpcHandler *_rib_handler;

    NextHopResolver<IPv4>& _next_hop_resolver_ipv4;
    NextHopResolver<IPv6>& _next_hop_resolver_ipv6;

    BGPPlumbingAF<IPv4> _plumbing_ipv4;
    BGPPlumbingAF<IPv6> _plumbing_ipv6;

    AsNum _my_AS_number;
};

#endif // __BGP_PLUMBING_HH__
