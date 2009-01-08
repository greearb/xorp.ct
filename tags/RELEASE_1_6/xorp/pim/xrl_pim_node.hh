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

// $XORP: xorp/pim/xrl_pim_node.hh,v 1.80 2008/10/02 21:57:56 bms Exp $

#ifndef __PIM_XRL_PIM_NODE_HH__
#define __PIM_XRL_PIM_NODE_HH__


//
// PIM XRL-aware node definition.
//

#include <set>

#include "libxorp/transaction.hh"

#include "libxipc/xrl_std_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/fea_rawpkt4_xif.hh"
#include "xrl/interfaces/fea_rawpkt6_xif.hh"
#include "xrl/interfaces/mfea_xif.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/mld6igmp_xif.hh"
#include "xrl/interfaces/cli_manager_xif.hh"
#include "xrl/targets/pim_base.hh"

#include "pim_node.hh"
#include "pim_node_cli.hh"
#include "pim_mfc.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlPimNode : public PimNode,
		   public XrlStdRouter,
		   public XrlPimTargetBase,
		   public PimNodeCli {
public:
    XrlPimNode(int		family,
	       xorp_module_id	module_id,
	       EventLoop&	eventloop,
	       const string&	class_name,
	       const string&	finder_hostname,
	       uint16_t		finder_port,
	       const string&	finder_target,
	       const string&	fea_target,
	       const string&	mfea_target,
	       const string&	rib_target,
	       const string&	mld6igmp_target);
    virtual ~XrlPimNode();

    /**
     * Startup the node operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown the node operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Get a reference to the XrlRouter instance.
     *
     * @return a reference to the XrlRouter (@ref XrlRouter) instance.
     */
    XrlRouter&	xrl_router() { return *this; }

    /**
     * Get a const reference to the XrlRouter instance.
     *
     * @return a const reference to the XrlRouter (@ref XrlRouter) instance.
     */
    const XrlRouter& xrl_router() const { return *this; }

    //
    // XrlPimNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();
    int enable_pim();
    int disable_pim();
    int start_pim();
    int stop_pim();
    int enable_bsr();
    int disable_bsr();
    int start_bsr();
    int stop_bsr();

    // XrlTask relatedMethods that need to be public
    void send_register_unregister_interest();
    void send_register_unregister_receiver();
    void send_register_unregister_protocol();
    void send_join_leave_multicast_group();
    void send_protocol_message();
    void send_add_delete_mfc();
    void send_add_delete_dataflow_monitor();

protected:    
    //
    // XRL target methods
    //

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
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(// Output values,
				      uint32_t& status,
				      string&	reason);
    
    /**
     * Shutdown cleanly
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Announce target birth to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Announce target death to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Process a CLI command.
     *  
     *  @param processor_name the processor name for this command.
     *  
     *  @param cli_term_name the terminal name the command was entered from.
     *  
     *  @param cli_session_id the CLI session ID the command was entered from.
     *  
     *  @param command_name the command name to process.
     *  
     *  @param command_args the command arguments to process.
     *  
     *  @param ret_processor_name the processor name to return back to the CLI.
     *  
     *  @param ret_cli_term_name the terminal name to return back.
     *  
     *  @param ret_cli_session_id the CLI session ID to return back.
     *  
     *  @param ret_command_output the command output to return back.
     */
    XrlCmdError cli_processor_0_1_process_command(
	// Input values, 
	const string&	processor_name, 
	const string&	cli_term_name, 
	const uint32_t&	cli_session_id,
	const string&	command_name, 
	const string&	command_args, 
	// Output values, 
	string&	ret_processor_name, 
	string&	ret_cli_term_name, 
	uint32_t& ret_cli_session_id,
	string&	ret_command_output);

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
     *  
     *  Receive a kernel signal message from the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param message_type the type of the kernel signal message (TODO:
     *  integer for now: the particular types are well-known by both sides).
     *  
     *  @param vif_name the name of the vif the message was received on.
     *  
     *  @param vif_index the index of the vif the message was received on.
     *  
     *  @param source_address the address of the sender.
     *  
     *  @param dest_address the destination address.
     *  
     *  @param protocol_message the protocol message.
     */
    XrlCmdError mfea_client_0_1_recv_kernel_signal_message4(
	// Input values, 
	const string&	xrl_sender_name, 
	const uint32_t&	message_type, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source_address, 
	const IPv4&	dest_address, 
	const vector<uint8_t>&	protocol_message);

    XrlCmdError mfea_client_0_1_recv_kernel_signal_message6(
	// Input values, 
	const string&	xrl_sender_name, 
	const uint32_t&	message_type, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source_address, 
	const IPv6&	dest_address, 
	const vector<uint8_t>&	protocol_message);
    
    /**
     *  A signal that a dataflow-related pre-condition is true.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param source_address the source address of the dataflow.
     *  
     *  @param group_address the group address of the dataflow.
     *  
     *  @param threshold_interval_sec the number of seconds in the interval
     *  requested for measurement.
     *  
     *  @param threshold_interval_usec the number of microseconds in the
     *  interval requested for measurement.
     *  
     *  @param measured_interval_sec the number of seconds in the last measured
     *  interval that has triggered the signal.
     *  
     *  @param measured_interval_usec the number of microseconds in the last
     *  measured interval that has triggered the signal.
     *  
     *  @param threshold_packets the threshold value to trigger a signal (in
     *  number of packets).
     *  
     *  @param threshold_bytes the threshold value to trigger a signal (in
     *  bytes).
     *  
     *  @param measured_packets the number of packets measured within the
     *  measured interval.
     *  
     *  @param measured_bytes the number of bytes measured within the measured
     *  interval.
     *  
     *  @param is_threshold_in_packets if true, threshold_packets is valid.
     *  
     *  @param is_threshold_in_bytes if true, threshold_bytes is valid.
     *  
     *  @param is_geq_upcall if true, the operation for comparison is ">=".
     *  
     *  @param is_leq_upcall if true, the operation for comparison is "<=".
     */
    XrlCmdError mfea_client_0_1_recv_dataflow_signal4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	measured_interval_sec, 
	const uint32_t&	measured_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const uint32_t&	measured_packets, 
	const uint32_t&	measured_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_client_0_1_recv_dataflow_signal6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	measured_interval_sec, 
	const uint32_t&	measured_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const uint32_t&	measured_packets, 
	const uint32_t&	measured_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    /**
     *  Start transaction.
     *
     *  @param tid the transaction ID to use for this transaction.
     */
    XrlCmdError redist_transaction4_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction4_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction4_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add/delete a routing entry.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param dst destination network.
     *
     *  @param nexthop nexthop router address.
     *
     *  @param ifname interface name associated with nexthop.
     *
     *  @param vifname virtual interface name with nexthop.
     *
     *  @param metric origin routing protocol metric for route.
     *
     *  @param admin_distance administrative distance of origin routing
     *  protocol.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  routing entry.
     */
    XrlCmdError redist_transaction4_0_1_add_route(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    XrlCmdError redist_transaction4_0_1_delete_route(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    /**
     *  Delete all routing entries.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     */
    XrlCmdError redist_transaction4_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie);

    /**
     *  Start transaction.
     *
     *  @param tid the transaction ID to use for this transaction.
     */
    XrlCmdError redist_transaction6_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction6_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction6_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add/delete a routing entry.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param dst destination network.
     *
     *  @param nexthop nexthop router address.
     *
     *  @param ifname interface name associated with nexthop.
     *
     *  @param vifname virtual interface name with nexthop.
     *
     *  @param metric origin routing protocol metric for route.
     *
     *  @param admin_distance administrative distance of origin routing
     *  protocol.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  routing entry.
     */
    XrlCmdError redist_transaction6_0_1_add_route(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    XrlCmdError redist_transaction6_0_1_delete_route(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    /**
     *  Delete all routing entries.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     */
    XrlCmdError redist_transaction6_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie);

    /**
     *  Add/delete membership information.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param vif_name the name of the new vif.
     *  
     *  @param vif_index the index of the new vif.
     *  
     *  @param source the source address that has been joined/left.
     *  
     *  @param group the group address that has been joined/left.
     */
    XrlCmdError mld6igmp_client_0_1_add_membership4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source, 
	const IPv4&	group);

    XrlCmdError mld6igmp_client_0_1_add_membership6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source, 
	const IPv6&	group);

    XrlCmdError mld6igmp_client_0_1_delete_membership4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source, 
	const IPv4&	group);

    XrlCmdError mld6igmp_client_0_1_delete_membership6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source, 
	const IPv6&	group);

    /**
     *  Enable/disable/start/stop a PIM vif interface.
     *
     *  @param vif_name the name of the vif to enable/disable/start/stop.
     *
     *  @param enable if true, then enable the vif, otherwise disable it.
     */
    XrlCmdError pim_0_1_enable_vif(
	// Input values,
	const string&	vif_name,
	const bool&	enable);

    XrlCmdError pim_0_1_start_vif(
	// Input values,
	const string&	vif_name);

    XrlCmdError pim_0_1_stop_vif(
	// Input values,
	const string&	vif_name);

    /**
     *  Enable/disable/start/stop all PIM vif interfaces.
     *
     *  @param enable if true, then enable the vifs, otherwise disable them.
     */
    XrlCmdError pim_0_1_enable_all_vifs(
	// Input values,
	const bool&	enable);

    XrlCmdError pim_0_1_disable_all_vifs();

    XrlCmdError pim_0_1_start_all_vifs();

    XrlCmdError pim_0_1_stop_all_vifs();

    /**
     *  Enable/disable/start/stop the PIM protocol.
     *
     *  @param enable if true, then enable the PIM protocol, otherwise disable
     *  it.
     */
    XrlCmdError pim_0_1_enable_pim(
	// Input values,
	const bool&	enable);

    XrlCmdError pim_0_1_start_pim();

    XrlCmdError pim_0_1_stop_pim();

    /**
     *  Enable/disable/start/stop the PIM CLI access.
     *
     *  @param enable if true, then enable the PIM CLI access, otherwise
     *  disable it.
     */
    XrlCmdError pim_0_1_enable_cli(
	// Input values,
	const bool&	enable);

    XrlCmdError pim_0_1_start_cli();

    XrlCmdError pim_0_1_stop_cli();

    /**
     *  Enable/disable/start/stop BSR.
     *
     *  @param enable if true, then enable the BSR, otherwise disable it.
     */
    XrlCmdError pim_0_1_enable_bsr(
	// Input values,
	const bool&	enable);

    XrlCmdError pim_0_1_start_bsr();

    XrlCmdError pim_0_1_stop_bsr();

    /**
     *  Add/delete scope zone.
     *  
     *  @param scope_zone_id the ID of the configured zone.
     *  
     *  @param vif_name the name of the vif to use as a bondary of the scope
     *  zone.
     */
    XrlCmdError pim_0_1_add_config_scope_zone_by_vif_name4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const string&	vif_name);

    XrlCmdError pim_0_1_add_config_scope_zone_by_vif_name6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const string&	vif_name);

    XrlCmdError pim_0_1_add_config_scope_zone_by_vif_addr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const IPv4&	vif_addr);

    XrlCmdError pim_0_1_add_config_scope_zone_by_vif_addr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const IPv6&	vif_addr);

    XrlCmdError pim_0_1_delete_config_scope_zone_by_vif_name4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const string&	vif_name);

    XrlCmdError pim_0_1_delete_config_scope_zone_by_vif_name6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const string&	vif_name);

    XrlCmdError pim_0_1_delete_config_scope_zone_by_vif_addr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const IPv4&	vif_addr);

    XrlCmdError pim_0_1_delete_config_scope_zone_by_vif_addr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const IPv6&	vif_addr);

    /**
     *  Add/delete candidate-BSR configuration.
     *  
     *  @param scope_zone_id the ID of the configured zone.
     *  
     *  @param is_scope_zone true if configuring administratively scoped zone.
     *  
     *  @param vif_name the name of the vif to use its address as a
     *  candidate-BSR.
     *  
     *  @param vif_addr the address of the vif to use as a candidate-BSR.
     *  
     *  @param bsr_priority the BSR priority (larger is better).
     *  
     *  @param hash_mask_len the hash mask length.
     */
    XrlCmdError pim_0_1_add_config_cand_bsr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const IPv4&	vif_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_add_config_cand_bsr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const IPv6&	vif_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_delete_config_cand_bsr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone);

    XrlCmdError pim_0_1_delete_config_cand_bsr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone);

    /**
     *  Add/delete Candidate-RP configuration.
     *  
     *  @param group_prefix the group prefix of the configured zone.
     *  
     *  @param is_scope_zone true if configuring administratively scoped zone.
     *  
     *  @param vif_name the name of the vif to use its address as a
     *  candidate-RP.
     *  
     *  @param vif_addr the address of the vif to use as a candidate-RP.
     *  
     *  @param rp_priority the Cand-RP priority (smaller is better).
     *  
     *  @param rp_holdtime the Cand-RP holdtime (in seconds).
     */
    XrlCmdError pim_0_1_add_config_cand_rp4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const IPv4&	vif_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_add_config_cand_rp6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const IPv6&	vif_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_delete_config_cand_rp4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const IPv4&	vif_addr);

    XrlCmdError pim_0_1_delete_config_cand_rp6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const IPv6&	vif_addr);

    /**
     *  Add/delete/complete static RP configuration.
     *  
     *  @param group_prefix the group prefix for the RP.
     *  
     *  @param rp_addr the RP address.
     *  
     *  @param rp_priority the RP priority (smaller is better).
     *  
     *  @param hash_mask_len the hash mask length used in computing an RP for a
     *  group. It should be same across all RPs. If set to zero, the default
     *  one will be used.
     */
    XrlCmdError pim_0_1_add_config_static_rp4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const IPv4&	rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_add_config_static_rp6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const IPv6&	rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_delete_config_static_rp4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const IPv4&	rp_addr);

    XrlCmdError pim_0_1_delete_config_static_rp6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const IPv6&	rp_addr);

    XrlCmdError pim_0_1_delete_config_all_static_group_prefixes_rp4(
	// Input values, 
	const IPv4&	rp_addr);

    XrlCmdError pim_0_1_delete_config_all_static_group_prefixes_rp6(
	// Input values, 
	const IPv6&	rp_addr);

    XrlCmdError pim_0_1_delete_config_all_static_rps();

    XrlCmdError pim_0_1_config_static_rp_done();

    /**
     *  Get the configured protocol version per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param proto_version the protocol version.
     */
    XrlCmdError pim_0_1_get_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	proto_version);

    /**
     *  Set the protocol version per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param proto_version the protocol version.
     */
    XrlCmdError pim_0_1_set_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	proto_version);

    /**
     *  Reset the protocol version per interface to its default value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError pim_0_1_reset_vif_proto_version(
	// Input values, 
	const string&	vif_name);

    /**
     *  Configure PIM Hello-related metrics. The 'set_foo' XRLs set the
     *  particular values. The 'reset_foo' XRLs reset the metrics to their
     *  default values.
     *  
     *  @param vif_name the name of the vif to configure.
     *  
     *  @param proto_version the protocol version.
     */
    XrlCmdError pim_0_1_get_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_triggered_delay);

    XrlCmdError pim_0_1_set_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	hello_triggered_delay);

    XrlCmdError pim_0_1_reset_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_hello_period(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_period);

    XrlCmdError pim_0_1_set_vif_hello_period(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	hello_period);

    XrlCmdError pim_0_1_reset_vif_hello_period(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_hello_holdtime(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_holdtime);

    XrlCmdError pim_0_1_set_vif_hello_holdtime(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	hello_holdtime);

    XrlCmdError pim_0_1_reset_vif_hello_holdtime(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_dr_priority(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	dr_priority);

    XrlCmdError pim_0_1_set_vif_dr_priority(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	dr_priority);

    XrlCmdError pim_0_1_reset_vif_dr_priority(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_propagation_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	propagation_delay);

    XrlCmdError pim_0_1_set_vif_propagation_delay(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	propagation_delay);

    XrlCmdError pim_0_1_reset_vif_propagation_delay(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_override_interval(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	override_interval);

    XrlCmdError pim_0_1_set_vif_override_interval(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	override_interval);

    XrlCmdError pim_0_1_reset_vif_override_interval(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_is_tracking_support_disabled(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	is_tracking_support_disabled);

    XrlCmdError pim_0_1_set_vif_is_tracking_support_disabled(
	// Input values, 
	const string&	vif_name, 
	const bool&	is_tracking_support_disabled);

    XrlCmdError pim_0_1_reset_vif_is_tracking_support_disabled(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_get_vif_accept_nohello_neighbors(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	accept_nohello_neighbors);

    XrlCmdError pim_0_1_set_vif_accept_nohello_neighbors(
	// Input values, 
	const string&	vif_name, 
	const bool&	accept_nohello_neighbors);

    XrlCmdError pim_0_1_reset_vif_accept_nohello_neighbors(
	// Input values, 
	const string&	vif_name);

    /**
     *  Configure PIM Join/Prune-related metrics. The 'set_foo' XRLs set the
     *  particular values. The 'reset_foo' XRLs reset the metrics to their
     *  default values.
     *  
     *  @param vif_name the name of the vif to configure.
     *  
     *  @param join_prune_period the period between Join/Prune messages.
     */
    XrlCmdError pim_0_1_get_vif_join_prune_period(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	join_prune_period);

    XrlCmdError pim_0_1_set_vif_join_prune_period(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	join_prune_period);

    XrlCmdError pim_0_1_reset_vif_join_prune_period(
	// Input values, 
	const string&	vif_name);

    /**
     *  Configure SPT-switch threshold. The 'set_foo' XRLs set the particular
     *  values. The 'reset_foo' XRLs reset the metrics to their default values.
     *  
     *  @param is_enabled if true, enable SPT-switch, otherwise disable it.
     *  
     *  @param interval_sec if the SPT-switch is enabled, the interval (in
     *  number of seconds) to measure the bandwidth to consider whether to
     *  switch to the SPT.
     */
    XrlCmdError pim_0_1_get_switch_to_spt_threshold(
	// Output values, 
	bool&		is_enabled, 
	uint32_t&	interval_sec, 
	uint32_t&	bytes);

    XrlCmdError pim_0_1_set_switch_to_spt_threshold(
	// Input values, 
	const bool&	is_enabled, 
	const uint32_t&	interval_sec, 
	const uint32_t&	bytes);

    XrlCmdError pim_0_1_reset_switch_to_spt_threshold();

    /**
     *  Add or delete an alternative subnet on a PIM vif. An alternative subnet
     *  is used to make incoming traffic with a non-local source address appear
     *  as it is coming from a local subnet. Note: add alternative subnets with
     *  extreme care, only if you know what you are really doing!
     *
     *  @param vif_name the name of the vif to add or delete an alternative
     *  subnet.
     *
     *  @param subnet the subnet address to add or delete.
     */
    XrlCmdError pim_0_1_add_alternative_subnet4(
	// Input values,
	const string&	vif_name,
	const IPv4Net&	subnet);

    XrlCmdError pim_0_1_add_alternative_subnet6(
	// Input values,
	const string&	vif_name,
	const IPv6Net&	subnet);

    XrlCmdError pim_0_1_delete_alternative_subnet4(
	// Input values,
	const string&	vif_name,
	const IPv4Net&	subnet);

    XrlCmdError pim_0_1_delete_alternative_subnet6(
	// Input values,
	const string&	vif_name,
	const IPv6Net&	subnet);

    XrlCmdError pim_0_1_remove_all_alternative_subnets(
	// Input values,
	const string&	vif_name);

    /**
     *  Enable/disable the PIM trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError pim_0_1_log_trace_all(
	// Input values,
	const bool&	enable);

    /**
     *  Test-related methods: add Join/Prune entries, and send them to a
     *  neighbor.
     */
    XrlCmdError pim_0_1_add_test_jp_entry4(
	// Input values, 
	const IPv4&	source_addr, 
	const IPv4&	group_addr, 
	const uint32_t&	group_mask_len, 
	const string&	mrt_entry_type, 
	const string&	action_jp, 
	const uint32_t&	holdtime, 
	const bool&	is_new_group);

    XrlCmdError pim_0_1_add_test_jp_entry6(
	// Input values, 
	const IPv6&	source_addr, 
	const IPv6&	group_addr, 
	const uint32_t&	group_mask_len, 
	const string&	mrt_entry_type, 
	const string&	action_jp, 
	const uint32_t&	holdtime, 
	const bool&	is_new_group);

    XrlCmdError pim_0_1_send_test_jp_entry4(
	// Input values, 
	const string&	vif_name, 
	const IPv4&	nbr_addr);

    XrlCmdError pim_0_1_send_test_jp_entry6(
	// Input values, 
	const string&	vif_name, 
	const IPv6&	nbr_addr);

    /**
     *  Test-related methods: send an Assert message on an interface.
     *  
     *  @param vif_name the name of the vif to send the Assert on.
     *  
     *  @param source_addr the source address inside the Assert message.
     *  
     *  @param group_addr the group address inside the Assert message.
     *  
     *  @param rpt_bit the RPT-bit inside the Assert message.
     *  
     *  @param metric_preference the metric preference inside the Assert
     *  message.
     *  
     *  @param metric the metric inside the Assert message.
     */
    XrlCmdError pim_0_1_send_test_assert4(
	// Input values, 
	const string&	vif_name, 
	const IPv4&	source_addr, 
	const IPv4&	group_addr, 
	const bool&	rpt_bit, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric);

    XrlCmdError pim_0_1_send_test_assert6(
	// Input values, 
	const string&	vif_name, 
	const IPv6&	source_addr, 
	const IPv6&	group_addr, 
	const bool&	rpt_bit, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric);

    /**
     *  Test-related methods: send Bootstrap and Cand-RP-Adv messages.
     *  
     *  @param zone_id_scope_zone_prefix the zone prefix of the zone ID.
     *  
     *  @param zone_id_is_scope_zone true if the zone is scoped.
     *  
     *  @param bsr_addr the address of the Bootstrap router.
     *  
     *  @param bsr_priority the priority of the Bootstrap router.
     *  
     *  @param hash_mask_len the hash mask length inside the Bootstrap
     *  messages.
     *  
     *  @param fragment_tag the fragment tag inside the Bootstrap messages.
     */
    XrlCmdError pim_0_1_add_test_bsr_zone4(
	// Input values, 
	const IPv4Net&	zone_id_scope_zone_prefix, 
	const bool&	zone_id_is_scope_zone, 
	const IPv4&	bsr_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len, 
	const uint32_t&	fragment_tag);

    XrlCmdError pim_0_1_add_test_bsr_zone6(
	// Input values, 
	const IPv6Net&	zone_id_scope_zone_prefix, 
	const bool&	zone_id_is_scope_zone, 
	const IPv6&	bsr_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len, 
	const uint32_t&	fragment_tag);

    XrlCmdError pim_0_1_add_test_bsr_group_prefix4(
	// Input values, 
	const IPv4Net&	zone_id_scope_zone_prefix, 
	const bool&	zone_id_is_scope_zone, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const uint32_t&	expected_rp_count);

    XrlCmdError pim_0_1_add_test_bsr_group_prefix6(
	// Input values, 
	const IPv6Net&	zone_id_scope_zone_prefix, 
	const bool&	zone_id_is_scope_zone, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const uint32_t&	expected_rp_count);

    XrlCmdError pim_0_1_add_test_bsr_rp4(
	// Input values, 
	const IPv4Net&	zone_id_scope_zone_prefix, 
	const bool&	zone_id_is_scope_zone, 
	const IPv4Net&	group_prefix, 
	const IPv4&	rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_add_test_bsr_rp6(
	// Input values, 
	const IPv6Net&	zone_id_scope_zone_prefix, 
	const bool&	zone_id_is_scope_zone, 
	const IPv6Net&	group_prefix, 
	const IPv6&	rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_send_test_bootstrap(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_send_test_bootstrap_by_dest4(
	// Input values, 
	const string&	vif_name, 
	const IPv4&	dest_addr);

    XrlCmdError pim_0_1_send_test_bootstrap_by_dest6(
	// Input values, 
	const string&	vif_name, 
	const IPv6&	dest_addr);

    XrlCmdError pim_0_1_send_test_cand_rp_adv();

    /**
     *  Retrieve information about all PIM neighbors.
     *  
     *  @param nbrs_number the number of PIM neighbors
     *  
     *  @param vifs the list of vif names for all neighbors (one vif name per
     *  neighbor).
     *  
     *  @param pim_versions the list of PIM protocol versions for all neighbors
     *  (one number per neighbor).
     *  
     *  @param dr_priorities the list of DR priorities of all neighbors (one
     *  number per neighbor).
     *  
     *  @param holdtimes the list of configured holdtimes (in seconds) of all
     *  neighbors (one number per neighbor).
     *  
     *  @param timeouts the list of timeout values (in seconds) of all
     *  neighbors (one number per neighbor).
     *  
     *  @param uptimes the list of uptime values (in seconds) of all neighbors
     *  (one number per neighbor).
     */
    XrlCmdError pim_0_1_pimstat_neighbors4(
	// Output values, 
	uint32_t&	nbrs_number, 
	XrlAtomList&	vifs, 
	XrlAtomList&	addresses, 
	XrlAtomList&	pim_versions, 
	XrlAtomList&	dr_priorities, 
	XrlAtomList&	holdtimes, 
	XrlAtomList&	timeouts, 
	XrlAtomList&	uptimes);

    XrlCmdError pim_0_1_pimstat_neighbors6(
	// Output values, 
	uint32_t&	nbrs_number, 
	XrlAtomList&	vifs, 
	XrlAtomList&	addresses, 
	XrlAtomList&	pim_versions, 
	XrlAtomList&	dr_priorities, 
	XrlAtomList&	holdtimes, 
	XrlAtomList&	timeouts, 
	XrlAtomList&	uptimes);

    /**
     *  Retrieve information about PIM interfaces.
     *  
     *  @param vif_name the name of the vif to retrieve information about.
     *  
     *  @param pim_version the PIM protocol version on that vif.
     *  
     *  @param is_dr true if this router is the DR for the subnet the vif is
     *  connected to.
     *  
     *  @param dr_priority the DR priority configured on that vif.
     *  
     *  @param dr_address the address of the DR for the subnet the vif is
     *  connected to.
     *  
     *  @param pim_nbrs_number the number of PIM neighbors on the subnet
     *  the vif is connected to.
     */
    XrlCmdError pim_0_1_pimstat_interface4(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	pim_version, 
	bool&		is_dr, 
	uint32_t&	dr_priority, 
	IPv4&		dr_address, 
	uint32_t&	pim_nbrs_number);

    XrlCmdError pim_0_1_pimstat_interface6(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	pim_version, 
	bool&		is_dr, 
	uint32_t&	dr_priority, 
	IPv6&		dr_address, 
	uint32_t&	pim_nbrs_number);

    /**
     *  Retrieve information about the RP-Set.
     *  
     *  @param rps_number the number of RPs in the RP-Set.
     *  
     *  @param addresses the list of addresses of all RPs (one IPv4 or IPv6
     *  address per RP).
     *  
     *  @param types the list of textual description about the origin of each
     *  RP (one keyword per RP: "bootstrap", "static" or "unknown").
     *  
     *  @param priorities the list of RP priorities of all RPs (one number per
     *  RP).
     *  
     *  @param holdtimes the list of configured holdtimes (in seconds) of all
     *  RPs (one number per RP).
     *  
     *  @param timeouts the list of timeout values (in seconds) of all RPs (one
     *  number per RP).
     *  
     *  @param group_prefixes the list of all group prefixes (one network
     *  IPv4Net or IPv6Net address per RP). Note that if an RP is configured
     *  for more than one group prefixes, there will be a number of entries for
     *  that RP: one per group prefix.
     */
    XrlCmdError pim_0_1_pimstat_rps4(
	// Output values, 
	uint32_t&	rps_number, 
	XrlAtomList&	addresses, 
	XrlAtomList&	types, 
	XrlAtomList&	priorities, 
	XrlAtomList&	holdtimes, 
	XrlAtomList&	timeouts, 
	XrlAtomList&	group_prefixes);

    XrlCmdError pim_0_1_pimstat_rps6(
	// Output values, 
	uint32_t&	rps_number, 
	XrlAtomList&	addresses, 
	XrlAtomList&	types, 
	XrlAtomList&	priorities, 
	XrlAtomList&	holdtimes, 
	XrlAtomList&	timeouts, 
	XrlAtomList&	group_prefixes);

    /**
     *  Clear all statistics
     */
    XrlCmdError pim_0_1_clear_pim_statistics();

    /**
     *  Clear all statistics on a specific interface.
     *  
     *  @param vif_name the interface to clear the statistics of.
     */
    XrlCmdError pim_0_1_clear_pim_statistics_per_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Statistics-related counters and values
     */
    XrlCmdError pim_0_1_pimstat_hello_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_hello_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_hello_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_stop_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_stop_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_stop_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_join_prune_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_join_prune_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_join_prune_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bootstrap_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bootstrap_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bootstrap_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_assert_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_assert_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_assert_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_ack_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_ack_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_ack_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_candidate_rp_messages_received(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_candidate_rp_messages_sent(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_candidate_rp_messages_rx_errors(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_unknown_type_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_unknown_version_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_neighbor_unknown_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bad_length_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bad_checksum_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bad_receive_interface_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_interface_disabled_messages(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_register_not_rp(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rp_filtered_source(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_unknown_register_stop(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_prune_no_state(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_graft_graft_ack_no_state(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_graft_on_upstream_interface(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_candidate_rp_not_bsr(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_bsr_when_bsr(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_bsr_not_rpf_interface(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_unknown_hello_option(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_data_no_state(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_rp_no_state(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_aggregate(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_malformed_packet(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_no_rp(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_no_route_upstream(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rp_mismatch(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rpf_neighbor_unknown(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_rp(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_rp(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_wc(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_wc(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_sg(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_sg(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_sg_rpt(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_sg_rpt(
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_hello_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_hello_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_hello_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_stop_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_stop_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_register_stop_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_join_prune_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_join_prune_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_join_prune_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bootstrap_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bootstrap_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bootstrap_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_assert_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_assert_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_assert_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_ack_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_ack_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_graft_ack_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_candidate_rp_messages_received_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_candidate_rp_messages_sent_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_candidate_rp_messages_rx_errors_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_unknown_type_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_unknown_version_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_neighbor_unknown_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bad_length_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bad_checksum_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_bad_receive_interface_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_interface_disabled_messages_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_register_not_rp_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rp_filtered_source_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_unknown_register_stop_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_prune_no_state_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_graft_graft_ack_no_state_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_graft_on_upstream_interface_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_candidate_rp_not_bsr_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_bsr_when_bsr_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_bsr_not_rpf_interface_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_unknown_hello_option_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_data_no_state_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_rp_no_state_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_aggregate_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_malformed_packet_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_no_rp_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_no_route_upstream_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rp_mismatch_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rpf_neighbor_unknown_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_rp_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_rp_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_wc_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_wc_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_sg_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_sg_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_join_sg_rpt_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

    XrlCmdError pim_0_1_pimstat_rx_prune_sg_rpt_per_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	value);

private:
    class XrlTaskBase;

    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(&_ifmgr);
    }
    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr.iftree(); }

    /**
     * Called when Finder connection is established.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_connect_event();

    /**
     * Called when Finder disconnect occurs.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_disconnect_event();

    //
    // Methods to handle the XRL tasks
    //
    void add_task(XrlTaskBase* xrl_task);
    void send_xrl_task();
    void pop_xrl_task();
    void retry_xrl_task();

    void fea_register_startup();
    void mfea_register_startup();
    void fea_register_shutdown();
    void mfea_register_shutdown();
    void finder_send_register_unregister_interest_cb(const XrlError& xrl_error);

    void rib_register_startup();
    void finder_register_interest_rib_cb(const XrlError& xrl_error);
    void rib_register_shutdown();
    void finder_deregister_interest_rib_cb(const XrlError& xrl_error);

    void send_rib_redist_transaction_enable();
    void rib_client_send_redist_transaction_enable_cb(const XrlError& xrl_error);
    void send_rib_redist_transaction_disable();
    void rib_client_send_redist_transaction_disable_cb(const XrlError& xrl_error);

    //
    // Protocol node methods
    //
    int register_receiver(const string& if_name, const string& vif_name,
			  uint8_t ip_protocol,
			  bool enable_multicast_loopback);
    int unregister_receiver(const string& if_name, const string& vif_name,
			    uint8_t ip_protocol);
    void fea_client_send_register_unregister_receiver_cb(const XrlError& xrl_error);

    int register_protocol(const string& if_name, const string& vif_name,
			  uint8_t ip_protocol);
    int unregister_protocol(const string& if_name, const string& vif_name);
    void mfea_client_send_register_unregister_protocol_cb(const XrlError& xrl_error);

    int join_multicast_group(const string& if_name, const string& vif_name,
			     uint8_t ip_protocol, const IPvX& group_address);
    int leave_multicast_group(const string& if_name, const string& vif_name,
			      uint8_t ip_protocol, const IPvX& group_address);
    void fea_client_send_join_leave_multicast_group_cb(const XrlError& xrl_error);

    int	proto_send(const string& if_name,
		   const string& vif_name,
		   const IPvX& src_address,
		   const IPvX& dst_address,
		   uint8_t ip_protocol,
		   int32_t ip_ttl,
		   int32_t ip_tos,
		   bool ip_router_alert,
		   bool ip_internet_control,
		   const uint8_t* sndbuf,
		   size_t sndlen,
		   string& error_msg);
    void fea_client_send_protocol_message_cb(const XrlError& xrl_error);

    int add_mfc_to_kernel(const PimMfc& pim_mfc);
    int delete_mfc_from_kernel(const PimMfc& pim_mfc);
    void mfea_client_send_add_delete_mfc_cb(const XrlError& xrl_error);

    int add_dataflow_monitor(const IPvX& source_addr,
			     const IPvX& group_addr,
			     uint32_t threshold_interval_sec,
			     uint32_t threshold_interval_usec,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall);
    int delete_dataflow_monitor(const IPvX& source_addr,
				const IPvX& group_addr,
				uint32_t threshold_interval_sec,
				uint32_t threshold_interval_usec,
				uint32_t threshold_packets,
				uint32_t threshold_bytes,
				bool is_threshold_in_packets,
				bool is_threshold_in_bytes,
				bool is_geq_upcall,
				bool is_leq_upcall);
    int delete_all_dataflow_monitor(const IPvX& source_addr,
				    const IPvX& group_addr);
    void mfea_client_send_add_delete_dataflow_monitor_cb(const XrlError& xrl_error);

    int add_protocol_mld6igmp(uint32_t vif_index);
    int delete_protocol_mld6igmp(uint32_t vif_index);
    void send_add_delete_protocol_mld6igmp();
    void mld6igmp_client_send_add_delete_protocol_mld6igmp_cb(const XrlError& xrl_error);
    void schedule_add_protocol_mld6igmp();

    //
    // Protocol node CLI methods
    //
    int add_cli_command_to_cli_manager(const char *command_name,
				       const char *command_help,
				       bool is_command_cd,
				       const char *command_cd_prompt,
				       bool is_command_processor);
    void cli_manager_client_send_add_cli_command_cb(const XrlError& xrl_error);
    int delete_cli_command_from_cli_manager(const char *command_name);
    void cli_manager_client_send_delete_cli_command_cb(const XrlError& xrl_error);
    
    int family() const { return PimNode::family(); }


    /**
     * A base class for handling tasks for sending XRL requests.
     */
    class XrlTaskBase {
    public:
	XrlTaskBase(XrlPimNode& xrl_pim_node)
	    : _xrl_pim_node(xrl_pim_node) {}
	virtual ~XrlTaskBase() {}

	virtual void dispatch() = 0;
	virtual const char* operation_name() const = 0;

    protected:
	XrlPimNode&	_xrl_pim_node;
    private:
    };

    /**
     * Class for handling the task to register/unregister interest
     * in the FEA or MFEA with the Finder.
     */
    class RegisterUnregisterInterest : public XrlTaskBase {
    public:
	RegisterUnregisterInterest(XrlPimNode&		xrl_pim_node,
				   const string&	target_name,
				   bool			is_register)
	    : XrlTaskBase(xrl_pim_node),
	      _target_name(target_name),
	      _is_register(is_register) {}

	void		dispatch() {
	    _xrl_pim_node.send_register_unregister_interest();
	}
	const char*	operation_name() const {
	    return ((_is_register)? "register" : "unregister");
	}
	const string&	target_name() const { return _target_name; }
	bool		is_register() const { return _is_register; }

    private:
	string		_target_name;
	bool		_is_register;
    };

    /**
     * Class for handling the task to register/unregister with the FEA
     * as a receiver on an interface.
     */
    class RegisterUnregisterReceiver : public XrlTaskBase {
    public:
	RegisterUnregisterReceiver(XrlPimNode&		xrl_pim_node,
				   const string&	if_name,
				   const string&	vif_name,
				   uint8_t		ip_protocol,
				   bool			enable_multicast_loopback,
				   bool			is_register)
	    : XrlTaskBase(xrl_pim_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _ip_protocol(ip_protocol),
	      _enable_multicast_loopback(enable_multicast_loopback),
	      _is_register(is_register) {}

	void		dispatch() {
	    _xrl_pim_node.send_register_unregister_receiver();
	}
	const char*	operation_name() const {
	    return ((_is_register)? "register" : "unregister");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	bool		enable_multicast_loopback() const {
	    return _enable_multicast_loopback;
	}
	bool		is_register() const { return _is_register; }

    private:
	string		_if_name;
	string		_vif_name;
	uint8_t		_ip_protocol;
	bool		_enable_multicast_loopback;
	bool		_is_register;
    };

    /**
     * Class for handling the task to register/unregister with the MFEA
     * as a protocol on an interface.
     */
    class RegisterUnregisterProtocol : public XrlTaskBase {
    public:
	RegisterUnregisterProtocol(XrlPimNode&		xrl_pim_node,
				   const string&	if_name,
				   const string&	vif_name,
				   uint8_t		ip_protocol,
				   bool			is_register)
	    : XrlTaskBase(xrl_pim_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _ip_protocol(ip_protocol),
	      _is_register(is_register) {}

	void		dispatch() {
	    _xrl_pim_node.send_register_unregister_protocol();
	}
	const char*	operation_name() const {
	    return ((_is_register)? "register" : "unregister");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	bool		is_register() const { return _is_register; }

    private:
	string		_if_name;
	string		_vif_name;
	uint8_t		_ip_protocol;
	bool		_is_register;
    };

    /**
     * Class for handling the task of join/leave multicast group requests
     */
    class JoinLeaveMulticastGroup : public XrlTaskBase {
    public:
	JoinLeaveMulticastGroup(XrlPimNode&		xrl_pim_node,
				const string&		if_name,
				const string&		vif_name,
				uint8_t			ip_protocol,
				const IPvX&		group_address,
				bool			is_join)
	    : XrlTaskBase(xrl_pim_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _ip_protocol(ip_protocol),
	      _group_address(group_address),
	      _is_join(is_join) {}

	void		dispatch() {
	    _xrl_pim_node.send_join_leave_multicast_group();
	}
	const char*	operation_name() const {
	    return ((_is_join)? "join" : "leave");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	const IPvX&	group_address() const { return _group_address; }
	bool		is_join() const { return _is_join; }

    private:
	string		_if_name;
	string		_vif_name;
	uint8_t		_ip_protocol;
	IPvX		_group_address;
	bool		_is_join;
    };

    /**
     * Class for handling the task of sending protocol messages
     */
    class SendProtocolMessage : public XrlTaskBase {
    public:
	SendProtocolMessage(XrlPimNode&		xrl_pim_node,
			    const string&	if_name,
			    const string&	vif_name,
			    const IPvX&		src_address,
			    const IPvX&		dst_address,
			    uint8_t		ip_protocol,
			    int32_t		ip_ttl,
			    int32_t		ip_tos,
			    bool		ip_router_alert,
			    bool		ip_internet_control,
			    const uint8_t*	sndbuf,
			    size_t		sndlen)
	    : XrlTaskBase(xrl_pim_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _src_address(src_address),
	      _dst_address(dst_address),
	      _ip_protocol(ip_protocol),
	      _ip_ttl(ip_ttl),
	      _ip_tos(ip_tos),
	      _ip_router_alert(ip_router_alert),
	      _ip_internet_control(ip_internet_control) {
	    _payload.resize(sndlen);
	    for (size_t i = 0; i < sndlen; i++)
		_payload[i] = sndbuf[i];
	}

	void		dispatch() {
	    _xrl_pim_node.send_protocol_message();
	}
	const char*	operation_name() const {
	    return ("send");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	const IPvX&	src_address() const { return _src_address; }
	const IPvX&	dst_address() const { return _dst_address; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	int32_t		ip_ttl() const { return _ip_ttl; }
	int32_t		ip_tos() const { return _ip_tos; }
	bool		ip_router_alert() const { return _ip_router_alert; }
	bool		ip_internet_control() const { return _ip_internet_control; }
	const vector<uint8_t>& payload() const { return _payload; }

    private:
	string		_if_name;
	string		_vif_name;
	IPvX		_src_address;
	IPvX		_dst_address;
	uint8_t		_ip_protocol;
	int32_t		_ip_ttl;
	int32_t		_ip_tos;
	bool		_ip_router_alert;
	bool		_ip_internet_control;
	vector<uint8_t>	_payload;
    };

    /**
     * Class for handling the task of add/delete MFC requests
     */
    class AddDeleteMfc : public XrlTaskBase {
    public:
	AddDeleteMfc(XrlPimNode&	xrl_pim_node,
		     const PimMfc&	pim_mfc,
		     bool		is_add)
	    : XrlTaskBase(xrl_pim_node),
	      _source_addr(pim_mfc.source_addr()),
	      _group_addr(pim_mfc.group_addr()),
	      _rp_addr(pim_mfc.rp_addr()),
	      _iif_vif_index(pim_mfc.iif_vif_index()),
	      _olist(pim_mfc.olist()),
	      _olist_disable_wrongvif(pim_mfc.olist_disable_wrongvif()),
	      _is_add(is_add) {}

	void		dispatch() {
	    _xrl_pim_node.send_add_delete_mfc();
	}
	const char*	operation_name() const {
	    return ((_is_add)? "add" : "delete");
	}
	const IPvX&	source_addr() const { return _source_addr; }
	const IPvX&	group_addr() const { return _group_addr; }
	const IPvX&	rp_addr() const { return _rp_addr; }
	uint32_t	iif_vif_index() const { return _iif_vif_index; }
	const Mifset&	olist() const { return _olist; }
	const Mifset&	olist_disable_wrongvif() const { return _olist_disable_wrongvif; }
	bool is_add()	const { return _is_add; }

    private:
	IPvX		_source_addr;
	IPvX		_group_addr;
	IPvX		_rp_addr;
	uint32_t	_iif_vif_index;
	Mifset		_olist;
	Mifset		_olist_disable_wrongvif;
	bool		_is_add;
    };

    /**
     * Class for handling the task of add/delete dataflow monitor requests
     */
    class AddDeleteDataflowMonitor : public XrlTaskBase {
    public:
	AddDeleteDataflowMonitor(XrlPimNode&	xrl_pim_node,
				 const IPvX&	source_addr,
				 const IPvX&	group_addr,
				 uint32_t	threshold_interval_sec,
				 uint32_t	threshold_interval_usec,
				 uint32_t	threshold_packets,
				 uint32_t	threshold_bytes,
				 bool		is_threshold_in_packets,
				 bool		is_threshold_in_bytes,
				 bool		is_geq_upcall,
				 bool		is_leq_upcall,
				 bool		is_add)
	    : XrlTaskBase(xrl_pim_node),
	      _source_addr(source_addr),
	      _group_addr(group_addr),
	      _threshold_interval_sec(threshold_interval_sec),
	      _threshold_interval_usec(threshold_interval_usec),
	      _threshold_packets(threshold_packets),
	      _threshold_bytes(threshold_bytes),
	      _is_threshold_in_packets(is_threshold_in_packets),
	      _is_threshold_in_bytes(is_threshold_in_bytes),
	      _is_geq_upcall(is_geq_upcall),
	      _is_leq_upcall(is_leq_upcall),
	      _is_add(is_add),
	      _is_delete_all(false) {}
	AddDeleteDataflowMonitor(XrlPimNode&	xrl_pim_node,
				 const IPvX&	source_addr,
				 const IPvX&	group_addr)
	    : XrlTaskBase(xrl_pim_node),
	      _source_addr(source_addr),
	      _group_addr(group_addr),
	      _threshold_interval_sec(0),
	      _threshold_interval_usec(0),
	      _threshold_packets(0),
	      _threshold_bytes(0),
	      _is_threshold_in_packets(false),
	      _is_threshold_in_bytes(false),
	      _is_geq_upcall(false),
	      _is_leq_upcall(false),
	      _is_add(false),
	      _is_delete_all(true) {}

	void		dispatch() {
	    _xrl_pim_node.send_add_delete_dataflow_monitor();
	}
	const char*	operation_name() const {
	    return ((_is_delete_all)? "delete all" : (_is_add)? "add" : "delete");
	}

	const IPvX&	source_addr() const { return _source_addr; }
	const IPvX&	group_addr() const { return _group_addr; }
	uint32_t	threshold_interval_sec() const { return _threshold_interval_sec; }
	uint32_t	threshold_interval_usec() const { return _threshold_interval_usec; }
	uint32_t	threshold_packets() const { return _threshold_packets; }
	uint32_t	threshold_bytes() const { return _threshold_bytes; }
	bool		is_threshold_in_packets() const { return _is_threshold_in_packets; }
	bool		is_threshold_in_bytes() const { return _is_threshold_in_bytes; }
	bool		is_geq_upcall() const { return _is_geq_upcall; }
	bool		is_leq_upcall() const { return _is_leq_upcall; }
	bool		is_add() const { return _is_add; }
	bool		is_delete_all() const { return _is_delete_all; }

    private:
	IPvX		_source_addr;
	IPvX		_group_addr;
	uint32_t	_threshold_interval_sec;
	uint32_t	_threshold_interval_usec;
	uint32_t	_threshold_packets;
	uint32_t	_threshold_bytes;
	bool		_is_threshold_in_packets;
	bool		_is_threshold_in_bytes;
	bool		_is_geq_upcall;
	bool		_is_leq_upcall;
	bool		_is_add;
	bool		_is_delete_all;
    };

    EventLoop&			_eventloop;
    const string		_finder_target;
    const string		_fea_target;
    const string		_mfea_target;
    const string		_rib_target;
    const string		_mld6igmp_target;

    IfMgrXrlMirror		_ifmgr;

    TransactionManager		_mrib_transaction_manager;
    XrlRawPacket4V0p1Client	_xrl_fea_client4;
    XrlRawPacket6V0p1Client	_xrl_fea_client6;
    XrlMfeaV0p1Client		_xrl_mfea_client;
    XrlRibV0p1Client		_xrl_rib_client;
    XrlMld6igmpV0p1Client	_xrl_mld6igmp_client;
    XrlCliManagerV0p1Client	_xrl_cli_manager_client;
    XrlFinderEventNotifierV0p1Client	_xrl_finder_client;

    static const TimeVal	RETRY_TIMEVAL;

    bool			_is_finder_alive;

    bool			_is_fea_alive;
    bool			_is_fea_registered;

    bool			_is_mfea_alive;
    bool			_is_mfea_registered;

    bool			_is_rib_alive;
    bool			_is_rib_registered;
    bool			_is_rib_registering;
    bool			_is_rib_deregistering;
    XorpTimer			_rib_register_startup_timer;
    XorpTimer			_rib_register_shutdown_timer;
    bool			_is_rib_redist_transaction_enabled;
    XorpTimer			_rib_redist_transaction_enable_timer;

    bool			_is_mld6igmp_alive;
    bool			_is_mld6igmp_registered;
    bool			_is_mld6igmp_registering;
    bool			_is_mld6igmp_deregistering;
    XorpTimer			_mld6igmp_register_startup_timer;
    XorpTimer			_mld6igmp_register_shutdown_timer;

    list<XrlTaskBase* >		_xrl_tasks_queue;
    XorpTimer			_xrl_tasks_queue_timer;
    list<pair<uint32_t, bool> >	_add_delete_protocol_mld6igmp_queue;
    XorpTimer			_add_delete_protocol_mld6igmp_queue_timer;

    // The set of vifs to add to MLD6IGMP if it goes away and comes back
    set<uint32_t>		_add_protocol_mld6igmp_vif_index_set;
};

#endif // __PIM_XRL_PIM_NODE_HH__
