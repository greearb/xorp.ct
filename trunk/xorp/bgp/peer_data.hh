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

// $XORP: xorp/bgp/peer_data.hh,v 1.4 2003/03/10 23:20:01 hodson Exp $

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

#define BGPVERSION 4

/**
 * Data that applies to a specific peering.
 */
class BGPPeerData {
public:
    BGPPeerData();
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

    void add_recv_parameter(const BGPParameter *p) {
	add_parameter(p, _recv_parameters);
    };
    void remove_recv_parameter(const BGPParameter *p) {
	remove_parameter(p, _recv_parameters);
    };

    void add_sent_parameter(const BGPParameter *p){
	add_parameter(p, _sent_parameters);
    };
    void remove_sent_parameter(const BGPParameter *p){
	remove_parameter(p, _sent_parameters);
    };

    void add_negotiated_parameter(const BGPParameter *p){
	add_parameter(p, _negotiated_parameters);
    };
    void remove_negotiated_parameter(const BGPParameter *p) {
	remove_parameter(p, _negotiated_parameters);
    };

    bool unsupported_parameters() const ;

    const list<const BGPParameter*>& parameter_recv_list() const {
	return _recv_parameters;
    }
    const list<const BGPParameter*>& parameter_sent_list() const {
	return _sent_parameters;
    }
    const list<const BGPParameter*>& parameter_negotiated_list() const {
	return _negotiated_parameters;
    }
    void clone_parameters(const list< BGPParameter*>& parameter_list);

    void dump_peer_data() const;
    void set_v4_local_addr(const IPv4& addr) { _local_v4_addr = addr; }
    void set_v6_local_addr(const IPv6& addr) { _local_v6_addr = addr; }
    const IPv4& get_v4_local_addr() const { return _local_v4_addr; }
    const IPv6& get_v6_local_addr() const { return _local_v6_addr; }

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

protected:
private:
    void add_parameter(const BGPParameter *,
	 list<const BGPParameter*>& p_list);
    void remove_parameter(const BGPParameter *p,
	 list<const BGPParameter*>& p_list);

    /*
     * Local Interface, Local Server Port, Peer Interface and
     * Peer Server Port tuple.
     */
    const Iptuple _iptuple;

    bool	_internal;	// set if our peer has the same _as
    AsNum	_as;

    /*
    ** Holdtime in seconds. Value sent in open negotiation.
    */
    uint16_t _configured_hold_time;

    /*
    ** Peer's BGP ID.
    */
    IPv4 _id;

    /*
    ** These values in milliseconds.
    */
    uint32_t _hold_duration;
    uint32_t _retry_duration;
    uint32_t _keepalive_duration;

    IPv4 _local_v4_addr; /* this is the address we advertise to
			    external peers as the nexthop */
    IPv6 _local_v6_addr; /* this is the address we advertise to
			    external peers as the nexthop */

    list <const BGPParameter*> _recv_parameters;
    list <const BGPParameter*> _sent_parameters;
    list <const BGPParameter*> _negotiated_parameters;
    bool _unsupported_parameters;
    uint8_t _num_parameters;
    uint8_t _param_length;

    /* XXX
    ** Eventually we will have totally programmable filters. As a
    ** temporary hack store the re-write value here.
    */
    IPv4 _next_hop_rewrite;
};

#endif // __BGP_PEER_DATA_HH__
