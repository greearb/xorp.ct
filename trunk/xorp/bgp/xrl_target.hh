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

// $XORP: xorp/bgp/xrl_target.hh,v 1.1.1.1 2002/12/11 23:55:50 hodson Exp $

#ifndef __BGP_XRL_TARGET_HH__
#define __BGP_XRL_TARGET_HH__

#include "xrl/targets/bgp_base.hh"
#include "iptuple.hh"
#include "main.hh"

class XrlBgpTarget :  XrlBgpTargetBase {
public:
    XrlBgpTarget(XrlRouter *r, BGPMain& bgp);

    XrlCmdError common_0_1_get_target_name(string& name) {
	name = "bgp";
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_get_version(string& version) {
	version = "0.1";
	return XrlCmdError::OKAY();
    }

    XrlCmdError bgp_0_2_get_bgp_version(
					// Output values, 
					uint32_t& version);

    XrlCmdError bgp_0_2_local_config(
	// Input values, 
	const uint32_t&	as_num, 
	const IPv4&	id);

    XrlCmdError bgp_0_2_set_local_as(
	// Input values, 
	const uint32_t&	as);

    XrlCmdError bgp_0_2_get_local_as(
				     // Output values, 
				     uint32_t& as);

    XrlCmdError bgp_0_2_set_bgpid(
	// Input values, 
	const IPv4&	id);

    XrlCmdError bgp_0_2_get_bgpid(
				  // Output values, 
				  IPv4& id);

    XrlCmdError bgp_0_2_add_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const uint32_t&	as, 
	const IPv4&	next_hop,
	const uint32_t&	holdtime);

    XrlCmdError bgp_0_2_delete_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const uint32_t&	as);

    XrlCmdError bgp_0_2_enable_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port);

    XrlCmdError bgp_0_2_disable_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port);

    XrlCmdError bgp_0_2_set_peer_state(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const bool&	state);

    XrlCmdError bgp_0_2_get_peer_list(
        // Output values,
        uint32_t& token, 
	IPv4&	local_ip, 
	uint32_t& local_port, 
	IPv4&	peer_ip, 
	uint32_t& peer_port, 
	bool& more);

    XrlCmdError bgp_0_2_get_peer_list_next(
        // Input values,
        const uint32_t&	token, 
	// Output values, 
	IPv4&	local_ip, 
	uint32_t&	local_port, 
	IPv4&	peer_ip, 
	uint32_t&	peer_port, 
	bool&	more);

    XrlCmdError bgp_0_2_get_peer_id(
        // Input values, 
        const IPv4&	local_ip, 
	const uint32_t&	local_port, 
	const IPv4&	peer_ip, 
	const uint32_t&	peer_port, 
	// Output values, 
	IPv4&	peer_id);

    XrlCmdError bgp_0_2_get_peer_status(
        // Input values, 
        const IPv4&	local_ip, 
	const uint32_t&	local_port, 
	const IPv4&	peer_ip, 
	const uint32_t&	peer_port, 
	// Output values, 
	uint32_t&	peer_state, 
	uint32_t&	admin_status);

    XrlCmdError bgp_0_2_get_peer_negotiated_version(
        // Input values, 
        const IPv4& local_ip, 
	const uint32_t& local_port, 
	const IPv4& peer_ip, 
	const uint32_t& peer_port, 
	// Output values, 
	int32_t& neg_version);

    XrlCmdError bgp_0_2_get_peer_as(
        // Input values, 
        const IPv4& local_ip, 
	const uint32_t& local_port, 
	const IPv4&	peer_ip, 
	const uint32_t& peer_port, 
	// Output values, 
	uint32_t& peer_as);

    XrlCmdError bgp_0_2_get_peer_msg_stats(
        // Input values, 
        const IPv4& local_ip, 
	const uint32_t& local_port, 
	const IPv4& peer_ip, 
	const uint32_t& peer_port, 
	// Output values, 
	uint32_t&	in_updates, 
	uint32_t&	out_updates, 
	uint32_t&	in_msgs, 
	uint32_t&	out_msgs, 
	uint32_t&	last_error, 
	uint32_t&	in_update_elapsed);

    XrlCmdError bgp_0_2_get_peer_established_stats(
        // Input values, 
        const IPv4& local_ip, 
	const uint32_t& local_port, 
	const IPv4& peer_ip, 
	const uint32_t& peer_port, 
	// Output values, 
	uint32_t& transitions, 
	uint32_t& established_time);

    XrlCmdError bgp_0_2_get_peer_timer_config(
        // Input values,
        const IPv4&	local_ip, 
	const uint32_t& local_port, 
	const IPv4&	peer_ip, 
	const uint32_t& peer_port, 
	// Output values, 
	uint32_t& retry_interval, 
	uint32_t& hold_time, 
	uint32_t& keep_alive, 
	uint32_t& hold_time_conf, 
	uint32_t& keep_alive_conf, 
	uint32_t& min_as_origin_interval);

    XrlCmdError bgp_0_2_register_rib(
	// Input values, 
	const string&	name);

    XrlCmdError bgp_0_2_add_route(
	// Input values, 
	const int32_t&	origin, 
	const int32_t&	asnum, 
	const IPv4&	next_hop, 
	const IPv4Net&	nlri);

    XrlCmdError bgp_0_2_delete_route(
	// Input values, 
	const IPv4Net&	nlri);

    XrlCmdError bgp_0_2_terminate();

    XrlCmdError rib_client_0_1_route_info_changed4(
	// Input values, 
	const IPv4&	addr, 
	const uint32_t&	prefix_len, 
	const IPv4&	nexthop, 
	const uint32_t&	metric);

    XrlCmdError rib_client_0_1_route_info_changed6(
	// Input values, 
	const IPv6&	addr, 
	const uint32_t&	prefix_len, 
	const IPv6&	nexthop, 
	const uint32_t&	metric);

    XrlCmdError rib_client_0_1_route_info_invalid4(
	// Input values, 
	const IPv4&	addr, 
	const uint32_t&	prefix_len);

    XrlCmdError rib_client_0_1_route_info_invalid6(
	// Input values, 
	const IPv6&	addr, 
	const uint32_t&	prefix_len);
    
    XrlCmdError bgp_0_2_set_parameter(
        // Input values,
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port,
        const string& parameter);
				     
    bool waiting();
    bool done();

private:
    /**
    * The main object that all requests go to.
    */
    BGPMain& _bgp;
    /**
    * Waiting for configuration. Such as our own AS number.
    */
    bool _awaiting_config;
    /**
     * Waiting for AS number.
     */
    bool _awaiting_as;
    /**
     * Local AS number.
     */
    uint32_t _as;
    /**
     * Waiting for BGP id
     */
    bool _awaiting_bgpid;
    /**
     * BGP id.
     */
    IPv4 _id;
    /**
    * Set to true if we should be exiting.
    */
    bool _done;
};

#endif //  __BGP_XRL_TARGET_HH__
