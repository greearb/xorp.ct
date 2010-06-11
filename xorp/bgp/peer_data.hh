// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/bgp/peer_data.hh,v 1.28 2008/11/08 06:14:37 mjh Exp $

#ifndef __BGP_PEER_DATA_HH__
#define __BGP_PEER_DATA_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/asnum.hh"



#include "iptuple.hh"
#include "parameter.hh"
#include "local_data.hh"


const size_t BGPVERSION = 4;

/**
 * Types of BGP Peering Sessions 
 */
enum PeerType {
    PEER_TYPE_EBGP = 0,		// Standard E-BGP peering
    PEER_TYPE_IBGP = 1,		// Standard I-BGP peering
    PEER_TYPE_EBGP_CONFED = 2,	// Confederation E-BGP peering
    PEER_TYPE_IBGP_CLIENT = 3,	// Route reflection client I-BGP.
    PEER_TYPE_UNCONFIGURED = 4,	// The peer type has not yet been configured.
    PEER_TYPE_INTERNAL = 255 	// connects to RIB
} ;

/**
 * The value of a configuration variable and its state enable/disabled.
 */
template <typename A>
class ConfigVar {
 public:    
    ConfigVar(A var, bool enabled = false) :  _var(var), _enabled(enabled)
    {}

    void set_var(A var) {
	_var = var;
    }

    A get_var() const {
	return _var;
    }

    void set_enabled(bool enabled) {
	_enabled = enabled;
    }

    bool get_enabled() const {
	return _enabled;
    }
    
 private:
    A _var;
    bool _enabled;
};

/**
 * Data that applies to a specific peering.
 */
class BGPPeerData {
public:
    BGPPeerData(const LocalData& local_data, const Iptuple& iptuple, AsNum as,
		const IPv4& next_hop, const uint16_t holdtime);
    ~BGPPeerData();

    const Iptuple& iptuple() const		{ return _iptuple; }

    const AsNum& as() const			{ return _as; }
    void set_as(const AsNum& a)			{ _as = a; }
    bool use_4byte_asnums() const               { return _use_4byte_asnums; }
    void set_use_4byte_asnums(bool use)         { _use_4byte_asnums = use; }
    bool we_use_4byte_asnums() const            { return _local_data.use_4byte_asnums(); }

    uint32_t get_hold_duration() const;
    void set_hold_duration(uint32_t d);

    uint32_t get_retry_duration() const;
    void set_retry_duration(uint32_t d);

    uint32_t get_keepalive_duration() const;
    void set_keepalive_duration(uint32_t d);

    /**
     * The peers BGP ID.
     */
    const IPv4& id() const			{ return _id; }
    void set_id(const IPv4& i)			{ _id = i; }

    bool route_reflector() const 		{ return _route_reflector; }
    void set_route_reflector(bool rr) 		{ _route_reflector = rr; }

    bool confederation() const 			{ return _confederation; }
    void set_confederation(bool conf) 		{ _confederation = conf; }

    /**
     * @return the prefix limit.
     */
    ConfigVar<uint32_t>& get_prefix_limit() {
	return _prefix_limit;
    }

    /**
     * Return this routers AS number. Normally this is simple, but if
     * this router is in a confederation then the AS number depends on
     * the the type of the peering.
     */
    const AsNum my_AS_number() const;

    /**
     * Compute the type of this peering.
     */
    void compute_peer_type();

    string get_peer_type_str() const;
    PeerType get_peer_type() const;

    /**
     * true if peer type is either PEER_TYPE_IBGP or PEER_TYPE_IBGP_CLIENT
     */
    bool ibgp() const;		 	

    /**
     * true if peer type is PEER_TYPE_IEBGP 
     */
    bool ibgp_vanilla() const;		

    /**
     * true if peer type is PEER_TYPE_IBGP_CLIENT
     */
    bool ibgp_client() const;		
    
    /**
     * true if peer type is PEER_TYPE_EBGP
     */
    bool ebgp_vanilla() const;		

    /**
     * true if peer type is PEER_TYPE_EBGP_CONFED
     */
    bool ebgp_confed() const;		

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

    /**
     * Multiprotocol option negotiation.
     */
    enum Direction {
	SENT = 0,		// Sent by us.
	RECEIVED = 1,		// Received from peer.
	NEGOTIATED = 2,		// Both sides agree.
	// Note: ARRAY_SIZE must be larger than all previous values
	ARRAY_SIZE = 3
    };

    /**
     * Which multiprotocol parameters did we send,receive and negotiate
     *
     * @param safi - Subsequent address family identifier
     * @param d - direction, SENT, RECEIVED or NEGOTIATED
     *
     * @return true if this parameter was set
     */
    template <class A> bool multiprotocol(Safi safi,
					  Direction d = NEGOTIATED) const {
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv4_unicast));
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv4_multicast));
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv6_unicast));
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv6_multicast));

	switch(A::ip_version()) {
	case 4:
	    switch(safi) {
	    case SAFI_UNICAST:
		return _ipv4_unicast[d];
		break;
	    case SAFI_MULTICAST:
		return _ipv4_multicast[d];
		break;
	    }
	    break;
	case 6:
	    switch(safi) {
	    case SAFI_UNICAST:
		return _ipv6_unicast[d];
		break;
	    case SAFI_MULTICAST:
		return _ipv6_multicast[d];
		break;
	    }
	    break;
	default:
	    XLOG_FATAL("Unknown IP version %u",
		       XORP_UINT_CAST(A::ip_version()));
	    break;
	}
	XLOG_UNREACHABLE();
	return false;
    }

    /*
     * this is for debugging only
     */
    template <class A> void set_multiprotocol(Safi safi,
					  Direction d = NEGOTIATED) {
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv4_unicast));
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv4_multicast));
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv6_unicast));
	XLOG_ASSERT(static_cast<size_t>(d) < sizeof(_ipv6_multicast));

	switch(A::ip_version()) {
	case 4:
	    switch(safi) {
	    case SAFI_UNICAST:
		_ipv4_unicast[d] = true;
		return;
	    case SAFI_MULTICAST:
		_ipv4_multicast[d] = true;
		return;
	    }
	    break;
	case 6:
	    switch(safi) {
	    case SAFI_UNICAST:
		_ipv6_unicast[d] = true;
		return;
	    case SAFI_MULTICAST:
		_ipv6_multicast[d] = true;
		return;
	    }
	    break;
	default:
	    XLOG_FATAL("Unknown IP version %u",
		       XORP_UINT_CAST(A::ip_version()));
	    break;
	}
	XLOG_UNREACHABLE();
	return;
    }

    

#if	0
    bool ipv4_unicast(Direction d = NEGOTIATED) const {
	return _ipv4_unicast[d];
    }

    bool ipv6_unicast(Direction d = NEGOTIATED) const {
	return _ipv6_unicast[d];
    } 

    bool ipv4_multicast(Direction d = NEGOTIATED) const {
	return _ipv4_multicast[d];
    }

    bool ipv6_multicast(Direction d = NEGOTIATED) const {
	return _ipv6_multicast[d];
    }
#endif

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

    void set_delay_open_time(uint32_t delay_open_time) {
	_delay_open_time = delay_open_time;
    }

    uint32_t get_delay_open_time() const {
	return _delay_open_time;
    }

    void set_next_hop_rewrite(const IPv4& next_hop) { 
	_next_hop_rewrite = next_hop;
    }

    const IPv4 get_next_hop_rewrite() const { 
	return _next_hop_rewrite;
    }

    void set_md5_password(const string &password) { _md5_password = password; }
    const string& get_md5_password() const { return _md5_password; }

    /**
     * Dump the state of the peer data (debugging).
     */
    void dump_peer_data() const;
protected:
private:
    const LocalData& _local_data;

    void add_parameter(ParameterList& p_list, const ParameterNode& p);
    void remove_parameter(ParameterList& p_list, const ParameterNode& p);

    /**
     * Local Interface, Local Server Port, Peer Interface and
     * Peer Server Port tuple.
     */
    const Iptuple _iptuple;

    AsNum _as;			// Peer's AS number.
    bool _use_4byte_asnums;      // True if we negotiated to use 4byte ASnums.
    bool _route_reflector;	// True if this is a route reflector client.
    bool _confederation;	// True if this is a confederation peer.

    /**
     * If enabled the maximum number of prefixes that will be accepted
     * on a peer before the session is torn down.
     */
    ConfigVar<uint32_t>  _prefix_limit;

    /**
     * Holdtime in seconds. Value sent in open negotiation.
     */
    uint16_t _configured_hold_time;

    /**
     * The number of seconds to wait before sending an open message.
     */
    uint32_t _delay_open_time;

    /**
     * Peer's BGP ID.
     */
    IPv4 _id;

    /*
    ** These values in seconds.
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


    PeerType _peer_type;  // the type of this peering session 
    
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
    bool _ipv4_unicast[ARRAY_SIZE];
    bool _ipv6_unicast[ARRAY_SIZE];
    bool _ipv4_multicast[ARRAY_SIZE];
    bool _ipv6_multicast[ARRAY_SIZE];

    /* XXX
    ** Eventually we will have totally programmable filters. As a
    ** temporary hack store the re-write value here.
    */
    IPv4 _next_hop_rewrite;

    /**
     * The password for TCP-MD5 authentication.
     */
    string _md5_password;
};

#endif // __BGP_PEER_DATA_HH__
