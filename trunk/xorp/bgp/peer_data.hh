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

// $XORP: xorp/bgp/peer_data.hh,v 1.12 2004/06/10 22:40:32 hodson Exp $

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
	    XLOG_FATAL("Unknown IP version %d", A::ip_version());
	    break;
	}
	XLOG_UNREACHABLE();
	return false;
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
