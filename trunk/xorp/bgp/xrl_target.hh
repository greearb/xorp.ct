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

// $XORP: xorp/bgp/xrl_target.hh,v 1.20 2004/05/11 00:44:35 atanu Exp $

#ifndef __BGP_XRL_TARGET_HH__
#define __BGP_XRL_TARGET_HH__

#include "xrl/targets/bgp_base.hh"

class BGPMain;

class XrlBgpTarget :  XrlBgpTargetBase {
public:
    XrlBgpTarget(XrlRouter *r, BGPMain& bgp);

    XrlCmdError common_0_1_get_target_name(string& name);

    XrlCmdError common_0_1_get_version(string& version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(
				      // Output values,
				      uint32_t& status,
				      string&	reason);

    /**
     * Request target shut down cleanly
     */
    XrlCmdError common_0_1_shutdown();

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

    XrlCmdError bgp_0_2_set_bgp_id(
	// Input values,
	const IPv4&	id);

    XrlCmdError bgp_0_2_get_bgp_id(
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
	const uint32_t&	peer_port);

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

    XrlCmdError bgp_0_2_activate(
	// Input values,
	const string&	local_ip,
	const uint32_t&	local_port,
	const string&	peer_ip,
	const uint32_t&	peer_port);

    XrlCmdError bgp_0_2_next_hop_rewrite_filter(
	// Input values,
	const string&	local_ip,
	const uint32_t&	local_port,
	const string&	peer_ip,
	const uint32_t&	peer_port,
	const IPv4&	next_hop);

    XrlCmdError bgp_0_2_originate_route4(
	// Input values,
	const IPv4Net&	nlri,
	const IPv4&	next_hop,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError bgp_0_2_originate_route6(
	// Input values,
	const IPv6Net&	nlri,
	const IPv6&	next_hop,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError bgp_0_2_withdraw_route4(
	// Input values,
	const IPv4Net&	nlri,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError bgp_0_2_withdraw_route6(
	// Input values,
	const IPv6Net&	nlri,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError bgp_0_2_get_peer_list_start(
        // Output values,
        uint32_t& token,
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
	uint32_t& min_as_origin_interval,
	uint32_t& min_route_adv_interval);

    XrlCmdError bgp_0_2_register_rib(
	// Input values,
	const string&	name);

    XrlCmdError bgp_0_2_get_v4_route_list_start(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	// Output values,
	uint32_t& token);

    XrlCmdError bgp_0_2_get_v6_route_list_start(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	// Output values,
	uint32_t& token);

    XrlCmdError bgp_0_2_get_v4_route_list_next(
	// Input values,
	const uint32_t&	token,
	// Output values,
	IPv4&	peer_id,
	IPv4Net& net,
	uint32_t& best_and_origin,
	vector<uint8_t>& aspath,
	IPv4& nexthop,
	int32_t& med,
	int32_t& localpref,
	int32_t& atomic_agg,
	vector<uint8_t>& aggregator,
	int32_t& calc_localpref,
	vector<uint8_t>& attr_unknown,
	bool& valid,
	bool& unicast,
	bool& multicast);

    XrlCmdError bgp_0_2_get_v6_route_list_next(
	// Input values,
	const uint32_t&	token,
	// Output values,
	IPv4& peer_id,
	IPv6Net& net,
	uint32_t& best_and_origin,
	vector<uint8_t>& aspath,
	IPv6& nexthop,
	int32_t& med,
	int32_t& localpref,
	int32_t& atomic_agg,
	vector<uint8_t>& aggregator,
	int32_t& calc_localpref,
	vector<uint8_t>& attr_unknown,
	bool& valid,
	bool& unicast,
	bool& multicast);

    XrlCmdError rib_client_0_1_route_info_changed4(
	// Input values,
	const IPv4&	addr,
	const uint32_t&	prefix_len,
	const IPv4&	nexthop,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin);

    XrlCmdError rib_client_0_1_route_info_changed6(
	// Input values,
	const IPv6&	addr,
	const uint32_t&	prefix_len,
	const IPv6&	nexthop,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin);

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

    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

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
    bool _awaiting_bgp_id;
    /**
     * BGP id.
     */
    IPv4 _id;
    /**
     * Set to true if we should be exiting.
     */
    bool _done;
};

#endif // __BGP_XRL_TARGET_HH__
