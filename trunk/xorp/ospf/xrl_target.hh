// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/xrl_target.hh,v 1.7 2005/10/03 20:24:06 atanu Exp $

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
	const vector<uint8_t>&	payload);

    /**
     *  Set router id
     */
    XrlCmdError ospfv2_0_1_set_router_id(
	// Input values,
	const IPv4&	id);

    /**
     *  @param type of area "normal", "stub", "nssa"
     */
    XrlCmdError ospfv2_0_1_create_area_router(
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
     *  @param prefix_len the prefix length XXX temporary.
     *
     *  @param mtu maximum transmission unit XXX temporary.
     *
     *  @param type of link "p2p", "broadcast", "nbma", "p2m", "vlink"
     */
    XrlCmdError ospfv2_0_1_create_peer(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t&	prefix_len,
	const uint32_t&	mtu,
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
     *  The edge cost of this interface.
     */
    XrlCmdError ospfv2_0_1_set_interface_cost(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	area,
	const uint32_t&	cost);

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

 private:
    Ospf<IPv4>& _ospf;
    XrlIO<IPv4>& _xrl_io;
};

class XrlOspfV3Target : XrlOspfv3TargetBase {
 public:
    XrlOspfV3Target(XrlRouter *r,
		    Ospf<IPv4>& ospf_ipv4,
		    Ospf<IPv6>& ospf_ipv6,
		    XrlIO<IPv4>& io_ipv4,
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
	const vector<uint8_t>&	payload);

    /**
     *  Set router id
     */
    XrlCmdError ospfv3_0_1_set_router_id(
	// Input values,
	const uint32_t&	id);

 private:
    Ospf<IPv4>& _ospf_ipv4;
    Ospf<IPv6>& _ospf_ipv6;
    XrlIO<IPv4>& _xrl_io_ipv4;
    XrlIO<IPv6>& _xrl_io_ipv6;
};
#endif // __OSPF_XRL_TARGET_HH__
