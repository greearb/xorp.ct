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

// $XORP: xorp/pim/xrl_pim_node.hh,v 1.32 2003/09/30 18:52:21 pavlin Exp $

#ifndef __PIM_XRL_PIM_NODE_HH__
#define __PIM_XRL_PIM_NODE_HH__


//
// PIM XRL-aware node definition.
//

#include <string>

#include "libxorp/xlog.h"
#include "libxorp/transaction.hh"
#include "libxipc/xrl_router.hh"
#include "xrl/targets/pim_base.hh"
#include "xrl/interfaces/common_xif.hh"
#include "xrl/interfaces/mfea_xif.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/mld6igmp_xif.hh"
#include "xrl/interfaces/cli_manager_xif.hh"
#include "pim_node.hh"
#include "pim_node_cli.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlPimNode : public PimNode,
		   public XrlPimTargetBase,
		   public XrlCommonV0p1Client,
		   public XrlMfeaV0p1Client,
		   public XrlRibV0p1Client,
		   public XrlMld6igmpV0p1Client,
		   public XrlCliManagerV0p1Client,
		   public PimNodeCli {
public:
    XrlPimNode(int family, xorp_module_id module_id,
	       EventLoop& eventloop, XrlRouter* xrl_router)
	: PimNode(family, module_id, eventloop),
	  XrlPimTargetBase(xrl_router),
	  XrlCommonV0p1Client(xrl_router),
	  XrlMfeaV0p1Client(xrl_router),
	  XrlRibV0p1Client(xrl_router),
	  XrlMld6igmpV0p1Client(xrl_router),
	  XrlCliManagerV0p1Client(xrl_router),
	  PimNodeCli(*static_cast<PimNode *>(this)),
	  _mrib_transaction_manager(eventloop)
	{ }
    virtual ~XrlPimNode() { PimNode::stop(); PimNodeCli::stop(); }
    
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
    
    
protected:
    //
    // Protocol node methods
    //
    int	proto_send(const string& dst_module_instance_name,
		   xorp_module_id dst_module_id,
		   uint16_t vif_index,
		   const IPvX& src, const IPvX& dst,
		   int ip_ttl,int ip_tos,  bool router_alert_bool,
		   const uint8_t * sndbuf, size_t sndlen);
    void xrl_result_send_protocol_message(const XrlError& xrl_error);
    
    int start_protocol_kernel();
    void xrl_result_add_protocol_mfea(const XrlError& xrl_error);
    void xrl_result_allow_signal_messages(const XrlError& xrl_error);
    void xrl_result_allow_mrib_messages(const XrlError& xrl_error);
    void xrl_result_add_rib_client(const XrlError& xrl_error);
    
    int stop_protocol_kernel();
    void xrl_result_delete_protocol_mfea(const XrlError& xrl_error);
    
    int start_protocol_kernel_vif(uint16_t vif_index);
    void xrl_result_start_protocol_kernel_vif(const XrlError& xrl_error);

    int stop_protocol_kernel_vif(uint16_t vif_index);
    void xrl_result_stop_protocol_kernel_vif(const XrlError& xrl_error);
    
    int join_multicast_group(uint16_t vif_index,
			     const IPvX& multicast_group);
    void xrl_result_join_multicast_group(const XrlError& xrl_error);
    
    int leave_multicast_group(uint16_t vif_index,
			      const IPvX& multicast_group);
    void xrl_result_leave_multicast_group(const XrlError& xrl_error);
    
    int add_mfc_to_kernel(const PimMfc& pim_mfc);
    void xrl_result_add_mfc_to_kernel(const XrlError& xrl_error);
    
    int delete_mfc_from_kernel(const PimMfc& pim_mfc);
    void xrl_result_delete_mfc_from_kernel(const XrlError& xrl_error);
    
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
    void xrl_result_add_dataflow_monitor(const XrlError& xrl_error);
    
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
    void xrl_result_delete_dataflow_monitor(const XrlError& xrl_error);
    
    int delete_all_dataflow_monitor(const IPvX& source_addr,
				    const IPvX& group_addr);
    void xrl_result_delete_all_dataflow_monitor(const XrlError& xrl_error);

    int add_protocol_mld6igmp(uint16_t vif_index);
    void xrl_result_add_protocol_mld6igmp(const XrlError& xrl_error);
    
    int delete_protocol_mld6igmp(uint16_t vif_index);
    void xrl_result_delete_protocol_mld6igmp(const XrlError& xrl_error);
    
    //
    // Protocol node CLI methods
    //
    int add_cli_command_to_cli_manager(const char *command_name,
				       const char *command_help,
				       bool is_command_cd,
				       const char *command_cd_prompt,
				       bool is_command_processor);
    void xrl_result_add_cli_command(const XrlError& xrl_error);
    int delete_cli_command_from_cli_manager(const char *command_name);
    void xrl_result_delete_cli_command(const XrlError& xrl_error);
    
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
     * shutdown cleanly
     */
    XrlCmdError common_0_1_shutdown();

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
     *  Add a new vif.
     *  
     *  @param vif_name the name of the new vif.
     *  
     *  @param vif_index the index of the new vif.
     */
    XrlCmdError mfea_client_0_1_new_vif(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    /**
     *  Delete an existing vif.
     *  
     *  @param vif_name the name of the vif to delete.
     */
    XrlCmdError mfea_client_0_1_delete_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Add an address to a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param addr the unicast address to add.
     *  
     *  @param subnet the subnet address to add.
     *  
     *  @param broadcast the broadcast address (when applicable).
     *  
     *  @param peer the peer address (when applicable).
     */
    XrlCmdError mfea_client_0_1_add_vif_addr4(
	// Input values, 
	const string&	vif_name, 
	const IPv4&	addr, 
	const IPv4Net&	subnet, 
	const IPv4&	broadcast, 
	const IPv4&	peer);

    XrlCmdError mfea_client_0_1_add_vif_addr6(
	// Input values, 
	const string&	vif_name, 
	const IPv6&	addr, 
	const IPv6Net&	subnet, 
	const IPv6&	broadcast, 
	const IPv6&	peer);

    /**
     *  Delete an address from a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param addr the unicast address to delete.
     */
    XrlCmdError mfea_client_0_1_delete_vif_addr4(
	// Input values, 
	const string&	vif_name, 
	const IPv4&	addr);
    
    XrlCmdError mfea_client_0_1_delete_vif_addr6(
	// Input values, 
	const string&	vif_name, 
	const IPv6&	addr);

    /**
     *  Set flags to a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param is_pim_register true if this is a PIM Register vif.
     *  
     *  @param is_p2p true if this is a point-to-point vif.
     *  
     *  @param is_loopback true if this is a loopback interface.
     *  
     *  @param is_multicast true if the vif is multicast-capable.
     *  
     *  @param is_broadcast true if the vif is broadcast-capable.
     *  
     *  @param is_up true if the vif is UP and running.
     */
    XrlCmdError mfea_client_0_1_set_vif_flags(
	// Input values, 
	const string&	vif_name, 
	const bool&	is_pim_register, 
	const bool&	is_p2p, 
	const bool&	is_loopback, 
	const bool&	is_multicast, 
	const bool&	is_broadcast, 
	const bool&	is_up);

    /**
     *  Complete all transactions with vif information.
     */
    XrlCmdError mfea_client_0_1_set_all_vifs_done();

    /**
     *  Test if the vif setup is completed.
     *  
     *  @param is_completed if true the vif setup is completed.
     */
    XrlCmdError mfea_client_0_1_is_vif_setup_completed(
	// Output values, 
	bool&	is_completed);

    /**
     *  Receive a protocol message from the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol that sends a message.
     *  
     *  @param protocol_id the ID of the protocol that sends a message (both
     *  sides must agree on the particular values).
     *  
     *  @param vif_name the name of the vif the message was received on.
     *  
     *  @param vif_index the index of the vif the message was received on.
     *  
     *  @param source_address the address of the sender.
     *  
     *  @param dest_address the destination address.
     *  
     *  @param ip_ttl the TTL of the received IP packet. If it has a negative
     *  value, it should be ignored.
     *  
     *  @param ip_tos the TOS of the received IP packet. If it has a negative
     *  value, it should be ignored.
     *  
     *  @param is_router_alert if true, the IP Router Alert option in
     *  the IP packet was set (when applicable).
     *  
     *  @param protocol_message the protocol message.
     */
    XrlCmdError mfea_client_0_1_recv_protocol_message4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source_address, 
	const IPv4&	dest_address, 
	const int32_t&	ip_ttl, 
	const int32_t&	ip_tos, 
	const bool&	is_router_alert, 
	const vector<uint8_t>&	protocol_message);

    XrlCmdError mfea_client_0_1_recv_protocol_message6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source_address, 
	const IPv6&	dest_address, 
	const int32_t&	ip_ttl, 
	const int32_t&	ip_tos, 
	const bool&	is_router_alert, 
	const vector<uint8_t>&	protocol_message);
    
    /**
     *  
     *  Receive a kernel signal message from the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol that sends a message.
     *  
     *  @param protocol_id the ID of the protocol that sends a message (both
     *  sides must agree on the particular values).
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
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const uint32_t&	message_type, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source_address, 
	const IPv4&	dest_address, 
	const vector<uint8_t>&	protocol_message);

    XrlCmdError mfea_client_0_1_recv_kernel_signal_message6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const uint32_t&	message_type, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source_address, 
	const IPv6&	dest_address, 
	const vector<uint8_t>&	protocol_message);
    
    /**
     *  Add Multicast Routing Information Base information.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param dest_prefix the destination prefix to add.
     *  
     *  @param next_hop_router_addr the address of the next-hop router toward
     *  the destination prefix.
     *  
     *  @param next_hop_vif_name the name of the vif toward the destination
     *  prefix.
     *  
     *  @param next_hop_vif_index the index of the vif toward the destination
     *  prefix.
     *  
     *  @param metric_preference the metric preference for this entry.
     *  
     *  @param metric the metric for this entry.
     */
    XrlCmdError mfea_client_0_1_add_mrib4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4Net&	dest_prefix, 
	const IPv4&	next_hop_router_addr, 
	const string&	next_hop_vif_name, 
	const uint32_t&	next_hop_vif_index, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric);
    
    XrlCmdError mfea_client_0_1_add_mrib6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6Net&	dest_prefix, 
	const IPv6&	next_hop_router_addr, 
	const string&	next_hop_vif_name, 
	const uint32_t&	next_hop_vif_index, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric);
    
    /**
     *  Delete Multicast Routing Information Base information.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param dest_prefix the destination prefix to delete.
     */
    XrlCmdError mfea_client_0_1_delete_mrib4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4Net&	dest_prefix);
    
    XrlCmdError mfea_client_0_1_delete_mrib6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6Net&	dest_prefix);

    /**
     *  Complete a transaction with MRIB information.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     */
    XrlCmdError mfea_client_0_1_set_mrib_done(
	// Input values, 
	const string&	xrl_sender_name);
    
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
    XrlCmdError fti_0_2_start_transaction(
	// Output values, 
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *  
     *  @param tid the transaction ID of this transaction.
     */
    virtual XrlCmdError fti_0_2_commit_transaction(
	// Input values, 
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *  
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError fti_0_2_abort_transaction(
	// Input values, 
	const uint32_t&	tid);

    /**
     *  Add a routing entry.
     *  
     *  @param tid the transaction ID of this transaction.
     *  
     *  @param dst the destination subnet address of the entry.
     *  
     *  @param gateway the address of the next-hop router toward dst.
     *  
     *  @param ifname the name of the physical interface toward dst.
     *  
     *  @param vifname the name of the virtual interface toward dst.
     *  
     *  @param metric the routing metric toward dst.
     *  
     *  @param admin_distance the administratively defined distance toward dst.
     *  
     *  @param protocol_origin the name of the protocol that originated this
     *  entry.
     */
    XrlCmdError fti_0_2_add_entry4(
	// Input values, 
	const uint32_t&	tid, 
	const IPv4Net&	dst, 
	const IPv4&	gateway, 
	const string&	ifname, 
	const string&	vifname, 
	const uint32_t&	metric, 
	const uint32_t&	admin_distance, 
	const string&	protocol_origin);

    /**
     *  Add a routing entry.
     *  
     *  @param tid the transaction ID of this transaction.
     *  
     *  @param dst the destination subnet address of the entry.
     *  
     *  @param gateway the address of the next-hop router toward dst.
     *  
     *  @param ifname the name of the physical interface toward dst.
     *  
     *  @param vifname the name of the virtual interface toward dst.
     *  
     *  @param metric the routing metric toward dst.
     *  
     *  @param admin_distance the administratively defined distance toward dst.
     *  
     *  @param protocol_origin the name of the protocol that originated this
     *  entry.
     */
    XrlCmdError fti_0_2_add_entry6(
	// Input values, 
	const uint32_t&	tid, 
	const IPv6Net&	dst, 
	const IPv6&	gateway, 
	const string&	ifname, 
	const string&	vifname, 
	const uint32_t&	metric, 
	const uint32_t&	admin_distance, 
	const string&	protocol_origin);

    /**
     *  Delete a routing entry.
     *  
     *  @param tid the transaction ID of this transaction.
     *  
     *  @param dst the destination subnet address of the entry.
     */
    XrlCmdError fti_0_2_delete_entry4(
	// Input values, 
	const uint32_t&	tid, 
	const IPv4Net&	dst);

    /**
     *  Delete a routing entry.
     *  
     *  @param tid the transaction ID of this transaction.
     *  
     *  @param dst the destination subnet address of the entry.
     */
    XrlCmdError fti_0_2_delete_entry6(
	// Input values, 
	const uint32_t&	tid, 
	const IPv6Net&	dst);

    /**
     *  Delete all routing entries for all address families.
     *  
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError fti_0_2_delete_all_entries(
	// Input values, 
	const uint32_t&	tid);

    /**
     *  Delete all routing entries for the IPv4 address family.
     *  
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError fti_0_2_delete_all_entries4(
	// Input values, 
	const uint32_t&	tid);

    /**
     *  Delete all routing entries for the IPv6 address family.
     *  
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError fti_0_2_delete_all_entries6(
	// Input values, 
	const uint32_t&	tid);

    /**
     *  Lookup a route for a destination host address.
     *  
     *  @param dst the destination host address to lookup.
     *  
     *  @param gateway the address of the next-hop router toward dst.
     *  
     *  @param ifname the name of the physical interface toward dst.
     *  
     *  @param vifname the name of the virtual interface toward dst.
     *  
     *  @param metric the routing metric toward dst.
     *  
     *  @param admin_distance the administratively defined distance toward dst.
     *  
     *  @param protocol_origin the name of the protocol that originated this
     *  entry.
     */
    XrlCmdError fti_0_2_lookup_route4(
	// Input values, 
	const IPv4&	dst, 
	// Output values, 
	IPv4Net&	netmask, 
	IPv4&		gateway, 
	string&		ifname, 
	string&		vifname, 
	uint32_t&	metric, 
	uint32_t&	admin_distance, 
	string&		protocol_origin);

    /**
     *  Lookup a route for a destination host address.
     *  
     *  @param dst the destination host address to lookup.
     *  
     *  @param gateway the address of the next-hop router toward dst.
     *  
     *  @param ifname the name of the physical interface toward dst.
     *  
     *  @param vifname the name of the virtual interface toward dst.
     *  
     *  @param metric the routing metric toward dst.
     *  
     *  @param admin_distance the administratively defined distance toward dst.
     *  
     *  @param protocol_origin the name of the protocol that originated this
     *  entry.
     */
    XrlCmdError fti_0_2_lookup_route6(
	// Input values, 
	const IPv6&	dst, 
	// Output values, 
	IPv6Net&	netmask, 
	IPv6&		gateway, 
	string&		ifname, 
	string&		vifname, 
	uint32_t&	metric, 
	uint32_t&	admin_distance, 
	string&		protocol_origin);

    /**
     *  Lookup a route for a destination subnet address.
     *  
     *  @param dst the destination subnet address to lookup.
     *  
     *  @param gateway the address of the next-hop router toward dst.
     *  
     *  @param ifname the name of the physical interface toward dst.
     *  
     *  @param vifname the name of the virtual interface toward dst.
     *  
     *  @param metric the routing metric toward dst.
     *  
     *  @param admin_distance the administratively defined distance toward dst.
     *  
     *  @param protocol_origin the name of the protocol that originated this
     *  entry.
     */
    XrlCmdError fti_0_2_lookup_entry4(
	// Input values, 
	const IPv4Net&	dst, 
	// Output values, 
	IPv4&		gateway, 
	string&		ifname, 
	string&		vifname, 
	uint32_t&	metric, 
	uint32_t&	admin_distance, 
	string&		protocol_origin);

    /**
     *  Lookup a route for a destination subnet address.
     *  
     *  @param dst the destination subnet address to lookup.
     *  
     *  @param gateway the address of the next-hop router toward dst.
     *  
     *  @param ifname the name of the physical interface toward dst.
     *  
     *  @param vifname the name of the virtual interface toward dst.
     *  
     *  @param metric the routing metric toward dst.
     *  
     *  @param admin_distance the administratively defined distance toward dst.
     *  
     *  @param protocol_origin the name of the protocol that originated this
     *  entry.
     */
    XrlCmdError fti_0_2_lookup_entry6(
	// Input values, 
	const IPv6Net&	dst, 
	// Output values, 
	IPv6&		gateway, 
	string&		ifname, 
	string&		vifname, 
	uint32_t&	metric, 
	uint32_t&	admin_distance, 
	string&		protocol_origin);

    /**
     *  Test if the underlying system supports IPv4.
     *  
     *  Note: this is a dummy method that doesn't apply to this target.
     *  
     *  @param result true if the underlying system supports IPv4, otherwise
     *  false.
     */
    XrlCmdError fti_0_2_have_ipv4(
	// Output values, 
	bool&	result);

    /**
     *  Test if the underlying system supports IPv6.
     *  
     *  Note: this is a dummy method that doesn't apply to this target.
     *  
     *  @param result true if the underlying system supports IPv6, otherwise
     *  false.
     */
    XrlCmdError fti_0_2_have_ipv6(
	// Output values, 
	bool&	result);

    /**
     *  Test whether the IPv4 unicast forwarding engine is enabled or disabled
     *  to forward packets.
     *  
     *  Note: this is a dummy method that doesn't apply to this target.
     *  
     *  @param enabled if true, then the IPv4 unicast forwarding is enabled,
     *  otherwise is disabled.
     */
    XrlCmdError fti_0_2_get_unicast_forwarding_enabled4(
	// Output values, 
	bool&	enabled);

    /**
     *  Test whether the IPv6 unicast forwarding engine is enabled or disabled
     *  to forward packets.
     *  
     *  Note: this is a dummy method that doesn't apply to this target.
     *  
     *  @param enabled if true, then the IPv6 unicast forwarding is enabled,
     *  otherwise is disabled.
     */
    XrlCmdError fti_0_2_get_unicast_forwarding_enabled6(
	// Output values, 
	bool&	enabled);

    /**
     *  Pure-virtual function that needs to be implemented to:
     *  
     *  Set the IPv4 unicast forwarding engine to enable or disable forwarding
     *  of packets.
     *  
     *  Note: this is a dummy method that doesn't apply to this target.
     *  
     *  @param enabled if true, then enable IPv4 unicast forwarding, otherwise
     *  disable it.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_enabled4(
	// Input values, 
	const bool&	enabled);

    /**
     *  Set the IPv6 unicast forwarding engine to enable or disable forwarding
     *  of packets.
     *  
     *  Note: this is a dummy method that doesn't apply to this target.
     *  
     *  @param enabled if true, then enable IPv6 unicast forwarding, otherwise
     *  disable it.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_enabled6(
	// Input values, 
	const bool&	enabled);

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
     */
    XrlCmdError pim_0_1_enable_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_disable_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_start_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError pim_0_1_stop_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Enable/disable/start/stop all PIM vif interfaces.
     */
    XrlCmdError pim_0_1_enable_all_vifs();

    XrlCmdError pim_0_1_disable_all_vifs();

    XrlCmdError pim_0_1_start_all_vifs();

    XrlCmdError pim_0_1_stop_all_vifs();

    /**
     *  Enable/disable/start/stop PIM protocol and PIM CLI access.
     */
    XrlCmdError pim_0_1_enable_pim();

    XrlCmdError pim_0_1_disable_pim();

    XrlCmdError pim_0_1_enable_cli();

    XrlCmdError pim_0_1_disable_cli();

    XrlCmdError pim_0_1_start_pim();

    XrlCmdError pim_0_1_stop_pim();

    XrlCmdError pim_0_1_start_cli();

    XrlCmdError pim_0_1_stop_cli();

    /**
     *  Enable/disable/start/stop BSR.
     */
    XrlCmdError pim_0_1_enable_bsr();

    XrlCmdError pim_0_1_disable_bsr();

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
     *  @param bsr_priority the BSR priority (larger is better).
     *  
     *  @param hash_mask_len the hash mask length.
     */
    XrlCmdError pim_0_1_add_config_cand_bsr_by_vif_name4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_add_config_cand_bsr_by_vif_name6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_add_config_cand_bsr_by_addr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const IPv4&	cand_bsr_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_mask_len);

    XrlCmdError pim_0_1_add_config_cand_bsr_by_addr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const IPv6&	cand_bsr_addr, 
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
     *  @param rp_priority the Cand-RP priority (smaller is better).
     *  
     *  @param rp_holdtime the Cand-RP holdtime (in seconds).
     */
    XrlCmdError pim_0_1_add_config_cand_rp_by_vif_name4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_add_config_cand_rp_by_vif_name6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_add_config_cand_rp_by_addr4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv4&	cand_rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_add_config_cand_rp_by_addr6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv6&	cand_rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_vif_name4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_vif_name6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_addr4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv4&	cand_rp_addr);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_addr6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv6&	cand_rp_addr);

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

    XrlCmdError pim_0_1_config_static_rp_done();

    /**
     *  Configure PIM Hello-related metrics. The 'set_foo' XRLs set the
     *  particular values. The 'reset_foo' XRLs reset the metrics to their
     *  default values.
     *  
     *  @param vif_name the name of the vif to configure.
     *  
     *  @param proto_version the protocol version.
     */
    XrlCmdError pim_0_1_get_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	proto_version);

    XrlCmdError pim_0_1_set_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	proto_version);

    XrlCmdError pim_0_1_reset_vif_proto_version(
	// Input values, 
	const string&	vif_name);

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

    XrlCmdError pim_0_1_get_vif_lan_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	lan_delay);

    XrlCmdError pim_0_1_set_vif_lan_delay(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	lan_delay);

    XrlCmdError pim_0_1_reset_vif_lan_delay(
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
	const bool&	new_group_bool);

    XrlCmdError pim_0_1_add_test_jp_entry6(
	// Input values, 
	const IPv6&	source_addr, 
	const IPv6&	group_addr, 
	const uint32_t&	group_mask_len, 
	const string&	mrt_entry_type, 
	const string&	action_jp, 
	const uint32_t&	holdtime, 
	const bool&	new_group_bool);

    XrlCmdError pim_0_1_send_test_jp_entry4(
	// Input values, 
	const IPv4&	nbr_addr);

    XrlCmdError pim_0_1_send_test_jp_entry6(
	// Input values, 
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
    const string& my_xrl_target_name() {
	return XrlPimTargetBase::name();
    }
    
    int family() const { return (PimNode::family()); }
    
    TransactionManager _mrib_transaction_manager;
};

#endif // __PIM_XRL_PIM_NODE_HH__
