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

// $XORP: xorp/bgp/peer_data.hh,v 1.7 2003/09/27 03:42:20 atanu Exp $

#ifndef __BGP_PEER_DATA_HH__
#define __BGP_PEER_DATA_HH__

#include <config.h>
#include <sys/types.h>
#include <list>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/asnum.hh"
#include "iptuple.hh"
#include "parameter.hh"

const size_t BGPVERSION = 4;

/**
 * Data that applies to a specific peering.
 */
class BGPPeerData {
public:
    BGPPeerData(const Iptuple& iptuple,	AsNum as,
		const IPv4& next_hop, const uint16_t holdtime);
    ~BGPPeerData();

    const Iptuple& iptuple() const		{ return _iptuple; }

    const AsNum& as() const			{ return _as; }
    void set_as(const AsNum& a)			{ _as = a; }

    uint32_t get_hold_duration() const;
    void set_hold_duration(uint32_t d);

    uint32_t get_retry_duration() const;
    void set_retry_duration(uint32_t d);

    uint32_t get_keepalive_duration() const;
    void set_keepalive_duration(uint32_t d);

    const IPv4& id() const			{ return _id; }
    void set_id(const IPv4& i)			{ _id = i; }

    bool get_internal_peer() const;
    void set_internal_peer(bool p);

    void add_recv_parameter(const ParameterNode& p) {
	add_parameter(_recv_parameters, p);
    };
    void remove_recv_parameter(const ParameterNode& p) {
	remove_parameter(_recv_parameters, p);
    };

    void add_sent_parameter(const ParameterNode& p){
	add_parameter(_sent_parameters, p);
    };
    void remove_sent_parameter(const ParameterNode& p){
	remove_parameter(_sent_parameters, p);
    };

    void add_negotiated_parameter(const ParameterNode& p){
	add_parameter(_negotiated_parameters, p);
    };
    void remove_negotiated_parameter(const ParameterNode& p) {
	remove_parameter(_negotiated_parameters, p);
    };

    const ParameterList& parameter_recv_list() const {
	return _recv_parameters;
    }

    const ParameterList& parameter_sent_list() const {
	return _sent_parameters;
    }

    const ParameterList& parameter_negotiated_list() const {
	return _negotiated_parameters;
    }

    /**
     * Take the optional parameters out of an open packet and save
     * them in _recv_parameters. The list is accessible through the
     * parameter_recv_list method.
     */
    void save_parameters(const ParameterList& parameter_list);

    /**
     * Go through the parameters that we have sent and the ones that
     * our peer has sent us and drop state into the
     * _negotiated_parameters list.
     */
    void open_negotiation();

    bool unicast_ipv4() {
	return _unicast_ipv4;
    }

    bool unicast_ipv6() {
	return _unicast_ipv6;
    } 

    bool multicast_ipv4() {
	return _multicast_ipv4;
    }

    bool multicast_ipv6() {
	return _multicast_ipv6;
    }

    void set_v4_local_addr(const IPv4& addr) { _nexthop_ipv4 = addr; }
    void set_v6_local_addr(const IPv6& addr) { _nexthop_ipv6 = addr; }
    const IPv4& get_v4_local_addr() const { return _nexthop_ipv4; }
    const IPv6& get_v6_local_addr() const { return _nexthop_ipv6; }

    void set_configured_hold_time(uint16_t hold) {
	_configured_hold_time = hold;
    }

    uint16_t get_configured_hold_time() const {
	return _configured_hold_time;
    }

    void set_next_hop_rewrite(const IPv4& next_hop) { 
	_next_hop_rewrite = next_hop;
    }

    const IPv4 get_next_hop_rewrite() const { 
	return _next_hop_rewrite;
    }

    /**
     * Dump the state of the peer data (debugging).
     */
    void dump_peer_data() const;
protected:
private:
    void add_parameter(ParameterList& p_list, const ParameterNode& p);
    void remove_parameter(ParameterList& p_list, const ParameterNode& p);

    /**
     * Local Interface, Local Server Port, Peer Interface and
     * Peer Server Port tuple.
     */
    const Iptuple _iptuple;

    bool	_internal;	// set if our peer has the same _as
    AsNum	_as;

    /**
     * Holdtime in seconds. Value sent in open negotiation.
     */
    uint16_t _configured_hold_time;

    /**
     * Peer's BGP ID.
     */
    IPv4 _id;

    /*
    ** These values in milliseconds.
    */
    uint32_t _hold_duration;
    uint32_t _retry_duration;
    uint32_t _keepalive_duration;

    /**
     * IPv4 nexthop
     */
    IPv4 _nexthop_ipv4;

    /**
     * IPv6 nexthop
     */
    IPv6 _nexthop_ipv6;
    
    /**
     * Parameters received by our peer.
     */
    ParameterList _recv_parameters;

    /**
     * Parameters that we have sent.
     */
    ParameterList _sent_parameters;

    /**
     * The options that we have both agreed on.
     */
    ParameterList _negotiated_parameters;

    /**
     * The set of different topologies that we support.
     */
    bool _unicast_ipv4, _unicast_ipv6, _multicast_ipv4,  _multicast_ipv6;

    /* XXX
    ** Eventually we will have totally programmable filters. As a
    ** temporary hack store the re-write value here.
    */
    IPv4 _next_hop_rewrite;
};

#endif // __BGP_PEER_DATA_HH__
