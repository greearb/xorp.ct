// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/ospf/xrl_target.hh,v 1.42 2007/08/15 19:33:41 atanu Exp $

#ifndef __OSPF_XRL_TARGET_HH__
#define __OSPF_XRL_TARGET_HH__

#include "xrl/targets/ospfv2_base.hh"
#include "xrl/targets/ospfv3_base.hh"

#include "ospf.hh"

class XrlOspfV2Target : XrlOspfv2TargetBase {
 public:
    XrlOspfV2Target(XrlRouter *r, Ospf<IPv4>& ospf, XrlIO<IPv4>& io);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Receive an IPv4 packet from a raw socket.
     *
     *  @param if_name the interface name the packet arrived on.
     *
     *  @param vif_name the vif name the packet arrived on.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, then
     *  the received value is unknown.
     *
     *  @param ip_tos the Type of Service (Diffserv/ECN bits for IPv4). If it
     *  has a negative value, then the received value is unknown.
     *
     *  @param ip_router_alert if true, the IP Router Alert option was included
     *  in the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     */
    XrlCmdError raw_packet4_client_0_1_recv(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv4&	src_address,
	const IPv4&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const vector<uint8_t>&	payload);

    /**
     *  Configure a policy filter.
     *
     *  @param filter the identifier of the filter to configure.
     *
     *  @param conf the configuration of the filter.
     */
    XrlCmdError policy_backend_0_1_configure(
	// Input values,
	const uint32_t&	filter,
	const string&	conf);

    /**
     *  Reset a policy filter.
     *
     *  @param filter the identifier of the filter to reset.
     */
    XrlCmdError policy_backend_0_1_reset(
	// Input values,
	const uint32_t&	filter);

    /**
     *  Push all available routes through all filters for re-filtering.
     */
    XrlCmdError policy_backend_0_1_push_routes();

    /**
     *  Start route redistribution for an IPv4 route.
     *
     *  @param network the route to advertise.
     *
     *  @param unicast whether the route is unicast.
     *
     *  @param multicast whether the route is multicast.
     *
     *  @param nexthop the nexthop of the route.
     *
     *  @param metric the metric of the route.
     *
     *  @param policytags the set of policy-tags associated with the route.
     */
    XrlCmdError policy_redist4_0_1_add_route4(
	// Input values,
	const IPv4Net&	network,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4&	nexthop,
	const uint32_t&	metric,
	const XrlAtomList&	policytags);

    /**
     *  Terminate route redistribution for an IPv4 route.
     *
     *  @param network the route for which advertisements should cease.
     *
     *  @param unicast whether the route is unicast.
     *
     *  @param multicast whether the route is multicast.
     */
    XrlCmdError policy_redist4_0_1_delete_route4(
	// Input values,
	const IPv4Net&	network,
	const bool&	unicast,
	const bool&	multicast);

    /**
     *  Set router id
     */
    XrlCmdError ospfv2_0_1_set_router_id(
	// Input values,
	const IPv4&	id);

    /**
     *  Set RFC 1583 compatibility.
     */
    XrlCmdError ospfv2_0_1_set_rfc1583_compatibility(
	// Input values,
	const bool&	compatibility);

    /**
     *  Set the router alert in the IP options.
     */
    XrlCmdError ospfv2_0_1_set_ip_router_alert(
	// Input values,
	const bool&	ip_router_alert);

    /**
     *  @param type of area "normal", "stub", "nssa"
     */
    XrlCmdError ospfv2_0_1_create_area_router(
	// Input values,
	const IPv4&	area,
	const string&	type);

    /**
     *  Change area type.
     *
     *  @param area id of the area
     *
     *  @param type of area "border", "stub", "nssa"
     */
    XrlCmdError ospfv2_0_1_change_area_router_type(
	// Input values,
	const IPv4&	area,
	const string&	type);

    /**
     *  Destroy area.
     */
    XrlCmdError ospfv2_0_1_destroy_area_router(
	// Input values,
	const IPv4&	area);

    /**
     *  Create a binding to an interface.
     *
     *  @param ifname the interface that owns vif that has address.
     *
     *  @param vifname virtual interface owning address.
     *
     *  @param addr the address to be added.
     *
     *  @param type of link "p2p", "broadcast", "nbma", "p2m", "vlink"
     */
    XrlCmdError ospfv2_0_1_create_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const string&	type,
	const IPv4&	area);

    /**
     *  Delete peer.
     */
    XrlCmdError ospfv2_0_1_delete_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname);

    /**
     *  Set the peer state up or down.
     */
    XrlCmdError ospfv2_0_1_set_peer_state(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	enable);

    /**
     *  Add a neighbour to the peer.
     */
    XrlCmdError ospfv2_0_1_add_neighbour(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv4&	neighbour_address,
	const IPv4&	neighbour_id);

    /**
     *  Remove a neighbour from the peer.
     */
    XrlCmdError ospfv2_0_1_remove_neighbour(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv4&	neighbour_address,
	const IPv4&	neighbour_id);

    /**
     *  Create a virtual link.
     *
     *  @param neighbour_id the router ID of the other end of the link.
     *
     *  @param area in which an attempt has been made to configure a virtual
     *  link it has to be the backbone. Its just being passed in so it can be
     *  checked by the protocol.
     */
    XrlCmdError ospfv2_0_1_create_virtual_link(
	// Input values,
	const IPv4&	neighbour_id,
	const IPv4&	area);

    /**
     *  Delete virtual link
     *
     *  @param neighbour_id the router ID of the other end of the link.
     */
    XrlCmdError ospfv2_0_1_delete_virtual_link(
	// Input values,
	const IPv4&	neighbour_id);

    /**
     *  The area through which the virtual link transits.
     *
     *  @param neighbour_id the router ID of the other end of the link.
     *
     *  @param transit_area that the virtual link transits.
     */
    XrlCmdError ospfv2_0_1_transit_area_virtual_link(
	// Input values,
	const IPv4&	neighbour_id,
	const IPv4&	transit_area);

    /**
     *  The edge cost of this interface.
     */
    XrlCmdError ospfv2_0_1_set_interface_cost(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	cost);

    /**
     *  The RxmtInterval.
     */
    XrlCmdError ospfv2_0_1_set_retransmit_interval(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  Update packet will have their age incremented by this amount before
     *  transmission. This value should take into account transmission and
     *  propagation delays; it must be greater than zero.
     */
    XrlCmdError ospfv2_0_1_set_inftransdelay(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	delay);

    /**
     *  Used in the designated router election.
     */
    XrlCmdError ospfv2_0_1_set_router_priority(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  The interval between hello messages.
     */
    XrlCmdError ospfv2_0_1_set_hello_interval(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  The period to wait before considering a router dead.
     */
    XrlCmdError ospfv2_0_1_set_router_dead_interval(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  Set simple password authentication key.
     *
     *  @param ifname the interface name.
     *
     *  @param vifname the vif name.
     *
     *  @param area the area ID.
     *
     *  @param password the authentication password.
     */
    XrlCmdError ospfv2_0_1_set_simple_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const string&	password);

    /**
     *  Delete simple password authentication key.
     *
     *  @param ifname the interface name.
     *
     *  @param vifname the vif name.
     *
     *  @param area the area ID.
     */
    XrlCmdError ospfv2_0_1_delete_simple_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area);

    /**
     *  Set MD5 authentication key.
     *
     *  @param ifname the interface name.
     *
     *  @param vifname the vif name.
     *
     *  @param area the area ID.
     *
     *  @param key_id the key ID (must be an integer in the interval [0, 255]).
     *
     *  @param password the authentication password.
     *
     *  @param start_time the authentication start time (YYYY-MM-DD.HH:MM).
     *
     *  @param end_time the authentication end time (YYYY-MM-DD.HH:MM).
     *
     *  @param max_time_drift the maximum time drift (in seconds) among
     *  all routers. Allowed values are [0--65534] seconds or 65535 for
     *  unlimited time drift.
     */
    XrlCmdError ospfv2_0_1_set_md5_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	key_id,
	const string&	password,
	const string&	start_time,
	const string&	end_time,
	const uint32_t&	max_time_drift);

    /**
     *  Delete MD5 authentication key.
     *
     *  @param ifname the interface name.
     *
     *  @param vifname the vif name.
     *
     *  @param area the area ID.
     *
     *  @param key_id the key ID (must be an integer in the interval [0, 255]).
     */
    XrlCmdError ospfv2_0_1_delete_md5_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	key_id);

    /**
     *  Toggle the passive status of an interface.
     */
    XrlCmdError ospfv2_0_1_set_passive(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const bool&	passive);

    /**
     *  If this is a "stub" or "nssa" area toggle the sending of a default
     *  route.
     */
    XrlCmdError ospfv2_0_1_originate_default_route(
	// Input values,
	const IPv4&	area,
	const bool&	enable);

    /**
     *  Set the StubDefaultCost, the default cost sent in a default route in a
     *  "stub" or "nssa" area.
     */
    XrlCmdError ospfv2_0_1_stub_default_cost(
	// Input values,
	const IPv4&	area,
	const uint32_t&	cost);

    /**
     *  Toggle the sending of summaries into "stub" or "nssa" areas.
     */
    XrlCmdError ospfv2_0_1_summaries(
	// Input values,
	const IPv4&	area,
	const bool&	enable);

    /**
     *  Add area range.
     */
    XrlCmdError ospfv2_0_1_area_range_add(
	// Input values,
	const IPv4&	area,
	const IPv4Net&	net,
	const bool&	advertise);

    /**
     *  Delete area range.
     */
    XrlCmdError ospfv2_0_1_area_range_delete(
	// Input values,
	const IPv4&	area,
	const IPv4Net&	net);

    /**
     *  Change the advertised state of this area.
     */
    XrlCmdError ospfv2_0_1_area_range_change_state(
	// Input values,
	const IPv4&	area,
	const IPv4Net&	net,
	const bool&	advertise);

    /**
     *  Enable/Disable tracing.
     *
     *  @param tvar trace variable.
     *
     *  @param enable set to true to enable false to disable.
     */
    XrlCmdError ospfv2_0_1_trace(
	// Input values,
	const string&	tvar,
	const bool&	enable);

    /**
     *  Get a single lsa from an area. A stateless mechanism to get LSAs. The
     *  client of this interface should start from zero and continue to request
     *  LSAs (incrementing index) until toohigh becomes true.
     *
     *  @param area database that is being searched.
     *
     *  @param index into database starting from 0.
     *
     *  @param valid true if a LSA has been returned. Some index values do not
     *  contain LSAs. This should not be considered an error.
     *
     *  @param toohigh true if no more LSA exist after this index.
     *
     *  @param self if true this LSA was originated by this router.
     *
     *  @param lsa if valid is true the LSA at index.
     */
    XrlCmdError ospfv2_0_1_get_lsa(
	// Input values,
	const IPv4&	area,
	const uint32_t&	index,
	// Output values,
	bool&	valid,
	bool&	toohigh,
	bool&	self,
	vector<uint8_t>&	lsa);

    /**
     * Get a list of all the configured areas.
     */
    XrlCmdError ospfv2_0_1_get_area_list(XrlAtomList& areas);

    /**
     *  Get the list of neighbours.
     */
    XrlCmdError ospfv2_0_1_get_neighbour_list(
	// Output values,
	XrlAtomList&	areas);

    /**
     *  Get information on a neighbour.
     *
     *  @param nid neighbour ID returned by the get_neighbour_list.
     *
     *  @param valid true if valid information has been returned.
     *
     *  @param address of neighbour in txt to allow IPv4 and IPv6.
     *
     *  @param interface with which the neighbour forms the adjacency.
     *
     *  @param state of the adjacency.
     *
     *  @param rid router ID of the neighbour.
     *
     *  @param priority of the neighbour (used for DR election).
     *
     *  @param area the neighbour is in.
     *
     *  @param opt value in the neighbours hello packet.
     *
     *  @param dr designated router.
     *
     *  @param bdr backup designated router.
     *
     *  @param up time in seconds that the neigbour has been up.
     *
     *  @param adjacent time in seconds that there has been an adjacency.
     */
    XrlCmdError ospfv2_0_1_get_neighbour_info(
	// Input values,
	const uint32_t&	nid,
	// Output values,
	string&	address,
	string&	interface,
	string&	state,
	IPv4&	rid,
	uint32_t& priority,
	uint32_t& deadtime,
	IPv4&	area,
	uint32_t&	opt,
	IPv4&	dr,
	IPv4&	bdr,
	uint32_t&	up,
	uint32_t&	adjacent);

    /**
     *  Clear the OSPF database.
     */
    XrlCmdError ospfv2_0_1_clear_database();

 private:
    Ospf<IPv4>& _ospf;
    XrlIO<IPv4>& _xrl_io;
};

class XrlOspfV3Target : XrlOspfv3TargetBase {
 public:
    XrlOspfV3Target(XrlRouter *r,
		    /* Ospf<IPv4>& ospf_ipv4, */
		    Ospf<IPv6>& ospf_ipv6,
		    /* XrlIO<IPv4>& io_ipv4, */
		    XrlIO<IPv6>& io_ipv6);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Receive an IPv4 packet from a raw socket.
     *
     *  @param if_name the interface name the packet arrived on.
     *
     *  @param vif_name the vif name the packet arrived on.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, then
     *  the received value is unknown.
     *
     *  @param ip_tos the Type of Service (Diffserv/ECN bits for IPv4). If it
     *  has a negative value, then the received value is unknown.
     *
     *  @param ip_router_alert if true, the IP Router Alert option was included
     *  in the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     */
    XrlCmdError raw_packet4_client_0_1_recv(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv4&	src_address,
	const IPv4&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const vector<uint8_t>&	payload);

    /**
     *  Receive an IPv6 packet from a raw socket.
     *
     *  @param if_name the interface name the packet arrived on.
     *
     *  @param vif_name the vif name the packet arrived on.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, then
     *  the received value is unknown.
     *
     *  @param ip_tos the Type Of Service (IP traffic class for IPv4). If it
     *  has a negative value, then the received value is unknown.
     *
     *  @param ip_router_alert if true, the IP Router Alert option was included
     *  in the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     *
     *  @param ext_headers_type a list of u32 integers with the types of the
     *  optional extention headers.
     *
     *  @param ext_headers_payload a list of payload data, one for each
     *  optional extention header. The number of entries must match
     *  ext_headers_type.
     */
    XrlCmdError raw_packet6_client_0_1_recv(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv6&	src_address,
	const IPv6&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const XrlAtomList&	ext_headers_type,
	const XrlAtomList&	ext_headers_payload,
	const vector<uint8_t>&	payload);

    /**
     *  Pure-virtual function that needs to be implemented to:
     *
     *  Configure a policy filter.
     *
     *  @param filter the identifier of the filter to configure.
     *
     *  @param conf the configuration of the filter.
     */
    XrlCmdError policy_backend_0_1_configure(
	// Input values,
	const uint32_t&	filter,
	const string&	conf);

    /**
     *  @param filter the identifier of the filter to reset.
     */
    XrlCmdError policy_backend_0_1_reset(
	// Input values,
	const uint32_t&	filter);

    /**
     *  Push all available routes through all filters for re-filtering.
     */
    XrlCmdError policy_backend_0_1_push_routes();

    /**
     *  Start route redistribution for an IPv6 route.
     *
     *  @param network the route to advertise.
     *
     *  @param unicast whether the route is unicast.
     *
     *  @param multicast whether the route is multicast.
     *
     *  @param nexthop the nexthop of the route.
     *
     *  @param metric the metric of the route.
     *
     *  @param policytags the set of policy-tags associated with the route.
     */
    XrlCmdError policy_redist6_0_1_add_route6(
	// Input values,
	const IPv6Net&	network,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6&	nexthop,
	const uint32_t&	metric,
	const XrlAtomList&	policytags);

    /**
     *  Terminate route redistribution for an IPv6 route.
     *
     *  @param network the route for which advertisements should cease.
     *
     *  @param unicast whether the route is unicast.
     *
     *  @param multicast whether the route is multicast.
     */
    XrlCmdError policy_redist6_0_1_delete_route6(
	// Input values,
	const IPv6Net&	network,
	const bool&	unicast,
	const bool&	multicast);

    /**
     *  Set instance id
     */
    XrlCmdError ospfv3_0_1_set_instance_id(
	// Input values,
	const uint32_t&	id);

    /**
     *  Set router id
     */
    XrlCmdError ospfv3_0_1_set_router_id(
	// Input values,
	const IPv4&	id);

    /**
     *  Set the router alert in the IP options.
     */
    XrlCmdError ospfv3_0_1_set_ip_router_alert(
	// Input values,
	const bool&	ip_router_alert);

    /**
     *  @param type of area "normal", "stub", "nssa"
     */
    XrlCmdError ospfv3_0_1_create_area_router(
	// Input values,
	const IPv4&	area,
	const string&	type);

    /**
     *  Change area type.
     *
     *  @param area id of the area
     *
     *  @param type of area "border", "stub", "nssa"
     */
    XrlCmdError ospfv3_0_1_change_area_router_type(
	// Input values,
	const IPv4&	area,
	const string&	type);

    /**
     *  Destroy area.
     */
    XrlCmdError ospfv3_0_1_destroy_area_router(
	// Input values,
	const IPv4&	area);

    /**
     *  Create a binding to an interface.
     *
     *  @param ifname the interface.
     *
     *  @param vifname virtual interface.
     *
     *  @param type of link "p2p", "broadcast", "nbma", "p2m", "vlink"
     */
    XrlCmdError ospfv3_0_1_create_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const string&	type,
	const IPv4&	area);

    /**
     *  Delete peer.
     */
    XrlCmdError ospfv3_0_1_delete_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname);

    /**
     *  Set the peer state up or down.
     */
    XrlCmdError ospfv3_0_1_set_peer_state(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	enable);

    /**
     *  Add an address to the peer.
     */
    XrlCmdError ospfv3_0_1_add_address_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv6&	addr);

    /**
     *  Remove an address from the peer.
     */
    XrlCmdError ospfv3_0_1_remove_address_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv6&	addr);

    /**
     *  Set the address state up or down.
     */
    XrlCmdError ospfv3_0_1_set_address_state_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv6&	addr,
	const bool&	enable);

    /**
     *  Activate peer. Called once the peer and child nodes have been
     *  configured.
     */
    XrlCmdError ospfv3_0_1_activate_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area);

    /**
     *  Update peer. Called if the peer and child nodes are modified.
     */
    XrlCmdError ospfv3_0_1_update_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area);

    /**
     *  Add a neighbour to the peer.
     */
    XrlCmdError ospfv3_0_1_add_neighbour(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv6&	neighbour_address,
	const IPv4&	neighbour_id);

    /**
     *  Remove a neighbour from the peer.
     */
    XrlCmdError ospfv3_0_1_remove_neighbour(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const IPv6&	neighbour_address,
	const IPv4&	neighbour_id);

    /**
     *  Create a virtual link.
     *
     *  @param neighbour_id the router ID of the other end of the link.
     *
     *  @param area in which an attempt has been made to configure a virtual
     *  link it has to be the backbone. Its just being passed in so it can be
     *  checked by the protocol.
     */
    XrlCmdError ospfv3_0_1_create_virtual_link(
	// Input values,
	const IPv4&	neighbour_id,
	const IPv4&	area);

    /**
     *  Delete virtual link
     *
     *  @param neighbour_id the router ID of the other end of the link.
     */
    XrlCmdError ospfv3_0_1_delete_virtual_link(
	// Input values,
	const IPv4&	neighbour_id);

    /**
     *  The area through which the virtual link transits.
     *
     *  @param neighbour_id the router ID of the other end of the link.
     *
     *  @param transit_area that the virtual link transits.
     */
    XrlCmdError ospfv3_0_1_transit_area_virtual_link(
	// Input values,
	const IPv4&	neighbour_id,
	const IPv4&	transit_area);

    /**
     *  The edge cost of this interface.
     */
    XrlCmdError ospfv3_0_1_set_interface_cost(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	cost);

    /**
     *  The RxmtInterval.
     */
    XrlCmdError ospfv3_0_1_set_retransmit_interval(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  Update packet will have their age incremented by this amount before
     *  transmission. This value should take into account transmission and
     *  propagation delays; it must be greater than zero.
     */
    XrlCmdError ospfv3_0_1_set_inftransdelay(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	delay);

    /**
     *  Used in the designated router election.
     */
    XrlCmdError ospfv3_0_1_set_router_priority(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  The interval between hello messages.
     */
    XrlCmdError ospfv3_0_1_set_hello_interval(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  The period to wait before considering a router dead.
     */
    XrlCmdError ospfv3_0_1_set_router_dead_interval(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	interval);

    /**
     *  Toggle the passive status of an interface.
     */
    XrlCmdError ospfv3_0_1_set_passive(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const bool&	passive);

    /**
     *  If this is a "stub" or "nssa" area toggle the sending of a default
     *  route.
     */
    XrlCmdError ospfv3_0_1_originate_default_route(
	// Input values,
	const IPv4&	area,
	const bool&	enable);

    /**
     *  Set the StubDefaultCost, the default cost sent in a default route in a
     *  "stub" or "nssa" area.
     */
    XrlCmdError ospfv3_0_1_stub_default_cost(
	// Input values,
	const IPv4&	area,
	const uint32_t&	cost);

    /**
     *  Toggle the sending of summaries into "stub" or "nssa" areas.
     */
    XrlCmdError ospfv3_0_1_summaries(
	// Input values,
	const IPv4&	area,
	const bool&	enable);

    /**
     *  Add area range.
     */
    XrlCmdError ospfv3_0_1_area_range_add(
	// Input values,
	const IPv4&	area,
	const IPv6Net&	net,
	const bool&	advertise);

    /**
     *  Delete area range.
     */
    XrlCmdError ospfv3_0_1_area_range_delete(
	// Input values,
	const IPv4&	area,
	const IPv6Net&	net);

    /**
     *  Change the advertised state of this area.
     */
    XrlCmdError ospfv3_0_1_area_range_change_state(
	// Input values,
	const IPv4&	area,
	const IPv6Net&	net,
	const bool&	advertise);

    /**
     *  Enable/Disable tracing.
     *
     *  @param tvar trace variable.
     *
     *  @param enable set to true to enable false to disable.
     */
    XrlCmdError ospfv3_0_1_trace(
	// Input values,
	const string&	tvar,
	const bool&	enable);

    /**
     *  Get a single lsa from an area. A stateless mechanism to get LSAs. The
     *  client of this interface should start from zero and continue to request
     *  LSAs (incrementing index) until toohigh becomes true.
     *
     *  @param area database that is being searched.
     *
     *  @param index into database starting from 0.
     *
     *  @param valid true if a LSA has been returned. Some index values do not
     *  contain LSAs. This should not be considered an error.
     *
     *  @param toohigh true if no more LSA exist after this index.
     *
     *  @param self if true this LSA was originated by this router.
     *
     *  @param lsa if valid is true the LSA at index.
     */
    XrlCmdError ospfv3_0_1_get_lsa(
	// Input values,
	const IPv4&	area,
	const uint32_t&	index,
	// Output values,
	bool&	valid,
	bool&	toohigh,
	bool&	self,
	vector<uint8_t>&	lsa);

    /**
     * Get a list of all the configured areas.
     */
    XrlCmdError ospfv3_0_1_get_area_list(XrlAtomList& areas);

    /**
     *  Get the list of neighbours.
     */
    XrlCmdError ospfv3_0_1_get_neighbour_list(
	// Output values,
	XrlAtomList&	areas);

    /**
     *  Get information on a neighbour.
     *
     *  @param nid neighbour ID returned by the get_neighbour_list.
     *
     *  @param valid true if valid information has been returned.
     *
     *  @param address of neighbour in txt to allow IPv4 and IPv6.
     *
     *  @param interface with which the neighbour forms the adjacency.
     *
     *  @param state of the adjacency.
     *
     *  @param rid router ID of the neighbour.
     *
     *  @param priority of the neighbour (used for DR election).
     *
     *  @param area the neighbour is in.
     *
     *  @param opt value in the neighbours hello packet.
     *
     *  @param dr designated router.
     *
     *  @param bdr backup designated router.
     *
     *  @param up time in seconds that the neigbour has been up.
     *
     *  @param adjacent time in seconds that there has been an adjacency.
     */
    XrlCmdError ospfv3_0_1_get_neighbour_info(
	// Input values,
	const uint32_t&	nid,
	// Output values,
	string&	address,
	string&	interface,
	string&	state,
	IPv4&	rid,
	uint32_t& priority,
	uint32_t& deadtime,
	IPv4&	area,
	uint32_t&	opt,
	IPv4&	dr,
	IPv4&	bdr,
	uint32_t&	up,
	uint32_t&	adjacent);

    /**
     *  Clear the OSPF database.
     */
    XrlCmdError ospfv3_0_1_clear_database();

 private:
//     Ospf<IPv4>& _ospf_ipv4;
    Ospf<IPv6>& _ospf_ipv6;
//     XrlIO<IPv4>& _xrl_io_ipv4;
    XrlIO<IPv6>& _xrl_io_ipv6;
};
#endif // __OSPF_XRL_TARGET_HH__
