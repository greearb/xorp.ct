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

// $XORP: xorp/pim/xrl_pim_node.hh,v 1.7 2003/02/06 22:21:32 hodson Exp $

#ifndef __PIM_XRL_PIM_NODE_HH__
#define __PIM_XRL_PIM_NODE_HH__


//
// PIM XRL-aware node definition.
//

#include <string>

#include "libxorp/xlog.h"
#include "libxipc/xrl_router.hh"
#include "xrl/targets/pim_base.hh"
#include "xrl/interfaces/common_xif.hh"
#include "xrl/interfaces/mfea_xif.hh"
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
		   public XrlMld6igmpV0p1Client,
		   public XrlCliManagerV0p1Client,
		   public PimNodeCli {
public:
    XrlPimNode(int family, x_module_id module_id,
	       EventLoop& event_loop, XrlRouter* r)
	: PimNode(family, module_id, event_loop),
	  XrlPimTargetBase(r),
	  XrlCommonV0p1Client(r),
	  XrlMfeaV0p1Client(r),
	  XrlMld6igmpV0p1Client(r),
	  XrlCliManagerV0p1Client(r),
	  PimNodeCli(*static_cast<PimNode *>(this))
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
    void xrl_result_add_protocol_mfea(const XrlError& xrl_error,
				      const bool *fail,
				      const string *reason);
    void xrl_result_delete_protocol_mfea(const XrlError& xrl_error,
					 const bool *fail,
					 const string *reason);
    
    int add_protocol_mld6igmp(uint16_t vif_index);
    int delete_protocol_mld6igmp(uint16_t vif_index);
    void xrl_result_add_protocol_mld6igmp(const XrlError& xrl_error,
					  const bool *fail,
					  const string *reason);
    void xrl_result_delete_protocol_mld6igmp(const XrlError& xrl_error,
					     const bool *fail,
					     const string *reason);
    
    void xrl_result_allow_signal_messages(const XrlError& xrl_error,
					  const bool *fail,
					  const string *reason);
    void xrl_result_allow_mrib_messages(const XrlError& xrl_error,
					const bool *fail,
					const string *reason);
    
    int	proto_send(const string& dst_module_instance_name,
		   x_module_id dst_module_id,
		   uint16_t vif_index,
		   const IPvX& src, const IPvX& dst,
		   int ip_ttl,int ip_tos,  bool router_alert_bool,
		   const uint8_t * sndbuf, size_t sndlen);
    void xrl_result_send_protocol_message(const XrlError& xrl_error,
					  const bool *fail,
					  const string *reason);
    
    int start_protocol_kernel();
    int stop_protocol_kernel();
    
    int start_protocol_kernel_vif(uint16_t vif_index);
    void xrl_result_start_protocol_kernel_vif(const XrlError& xrl_error,
					      const bool *fail,
					      const string *reason);

    int stop_protocol_kernel_vif(uint16_t vif_index);
    void xrl_result_stop_protocol_kernel_vif(const XrlError& xrl_error,
					     const bool *fail,
					     const string *reason);
    
    int join_multicast_group(uint16_t vif_index,
			     const IPvX& multicast_group);
    void xrl_result_join_multicast_group(const XrlError& xrl_error,
					 const bool *fail,
					 const string *reason);
    
    int leave_multicast_group(uint16_t vif_index,
			      const IPvX& multicast_group);
    void xrl_result_leave_multicast_group(const XrlError& xrl_error,
					  const bool *fail,
					  const string *reason);
    
    int add_mfc_to_kernel(const PimMfc& pim_mfc);
    void xrl_result_add_mfc_to_kernel(const XrlError& xrl_error,
				      const bool *fail,
				      const string *reason);
    
    int delete_mfc_from_kernel(const PimMfc& pim_mfc);
    void xrl_result_delete_mfc_from_kernel(const XrlError& xrl_error,
					   const bool *fail,
					   const string *reason);
    
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
    void xrl_result_add_dataflow_monitor(const XrlError& xrl_error,
					 const bool *fail,
					 const string *reason);
    void xrl_result_delete_dataflow_monitor(const XrlError& xrl_error,
					    const bool *fail,
					    const string *reason);
    void xrl_result_delete_all_dataflow_monitor(const XrlError& xrl_error,
						const bool *fail,
						const string *reason);
    
    //
    // Protocol node CLI methods
    //
    int add_cli_command_to_cli_manager(const char *command_name,
				       const char *command_help,
				       bool is_command_cd,
				       const char *command_cd_prompt,
				       bool is_command_processor);
    void xrl_result_add_cli_command(const XrlError& xrl_error,
				    const bool *fail,
				    const string *reason);
    int delete_cli_command_from_cli_manager(const char *command_name);
    void xrl_result_delete_cli_command(const XrlError& xrl_error,
				       const bool *fail,
				       const string *reason);
    
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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_new_vif(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	// Output values, 
	bool&	fail,
	string& reason);

    /**
     *  Delete an existing vif.
     *  
     *  @param vif_name the name of the vif to delete.
     *  
     *  @param vif_index the index of the vif to delete.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_delete_vif(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	// Output values, 
	bool&	fail,
	string& reason);

    /**
     *  Add an address to a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param vif_index the index of the vif.
     *  
     *  @param addr the unicast address to add.
     *  
     *  @param subnet the subnet address to add.
     *  
     *  @param broadcast the broadcast address (when applicable).
     *  
     *  @param peer the peer address (when applicable).
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_add_vif_addr4(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	addr, 
	const IPv4Net&	subnet, 
	const IPv4&	broadcast, 
	const IPv4&	peer, 
	// Output values, 
	bool&	fail,
	string& reason);

    XrlCmdError mfea_client_0_1_add_vif_addr6(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	addr, 
	const IPv6Net&	subnet, 
	const IPv6&	broadcast, 
	const IPv6&	peer, 
	// Output values, 
	bool&	fail,
	string& reason);

    /**
     *  Delete an address from a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param vif_index the index of the vif.
     *  
     *  @param addr the unicast address to delete.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_delete_vif_addr4(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	addr, 
	// Output values, 
	bool&	fail,
	string& reason);
    
    XrlCmdError mfea_client_0_1_delete_vif_addr6(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	addr, 
	// Output values, 
	bool&	fail,
	string& reason);

    /**
     *  Set flags to a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param vif_index the index of the vif.
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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_set_vif_flags(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const bool&	is_pim_register, 
	const bool&	is_p2p, 
	const bool&	is_loopback, 
	const bool&	is_multicast, 
	const bool&	is_broadcast, 
	const bool&	is_up, 
	// Output values, 
	bool&	fail,
	string& reason);

    /**
     *  Complete a transaction with vif information.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param vif_index the index of the vif.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_set_vif_done(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	// Output values, 
	bool&	fail,
	string& reason);

    /**
     *  Complete all transactions with vif information.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_set_all_vifs_done(
	// Output values, 
	bool&	fail, 
	string&	reason);

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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
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
	const vector<uint8_t>&	protocol_message, 
	// Output values, 
	bool&	fail, 
	string&	reason);

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
	const vector<uint8_t>&	protocol_message, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
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
	const vector<uint8_t>&	protocol_message, 
	// Output values, 
	bool&	fail, 
	string&	reason);

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
	const vector<uint8_t>&	protocol_message, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_add_mrib4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4Net&	dest_prefix, 
	const IPv4&	next_hop_router_addr, 
	const string&	next_hop_vif_name, 
	const uint32_t&	next_hop_vif_index, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
    XrlCmdError mfea_client_0_1_add_mrib6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6Net&	dest_prefix, 
	const IPv6&	next_hop_router_addr, 
	const string&	next_hop_vif_name, 
	const uint32_t&	next_hop_vif_index, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
    /**
     *  Delete Multicast Routing Information Base information.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param dest_prefix the destination prefix to delete.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_delete_mrib4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4Net&	dest_prefix, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
    XrlCmdError mfea_client_0_1_delete_mrib6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6Net&	dest_prefix, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Complete a transaction with MRIB information.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mfea_client_0_1_set_mrib_done(
	// Input values, 
	const string&	xrl_sender_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
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
	const bool&	is_leq_upcall, 
	// Output values, 
	bool&	fail, 
	string&	reason);

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
	const bool&	is_leq_upcall, 
	// Output values, 
	bool&	fail, 
	string&	reason);

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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError mld6igmp_client_0_1_add_membership4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source, 
	const IPv4&	group, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError mld6igmp_client_0_1_add_membership6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source, 
	const IPv6&	group, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError mld6igmp_client_0_1_delete_membership4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source, 
	const IPv4&	group, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError mld6igmp_client_0_1_delete_membership6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source, 
	const IPv6&	group, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Enable/disable/start/stop a PIM vif interface.
     *  
     *  @param vif_name the name of the vif to enable/disable/start/stop.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_enable_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_disable_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_start_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_stop_vif(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Enable/disable/start/stop all PIM vif interfaces.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_enable_all_vifs(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_disable_all_vifs(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_start_all_vifs(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_stop_all_vifs(
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Enable/disable/start/stop PIM protocol and PIM CLI access.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_enable_pim(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_disable_pim(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_enable_cli(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_disable_cli(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_start_pim(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_stop_pim(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_start_cli(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_stop_cli(
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Enable/disable/start/stop BSR.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_enable_bsr(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_disable_bsr(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_start_bsr(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_stop_bsr(
	// Output values, 
	bool&	fail, 
	string&	reason);

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
     *  @param hash_masklen the hash mask length.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_add_config_cand_bsr_by_vif_name4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_masklen, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_cand_bsr_by_vif_name6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_masklen, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_cand_bsr_by_addr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const IPv4&	cand_bsr_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_masklen, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_cand_bsr_by_addr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	const IPv6&	cand_bsr_addr, 
	const uint32_t&	bsr_priority, 
	const uint32_t&	hash_masklen, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_cand_bsr4(
	// Input values, 
	const IPv4Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_cand_bsr6(
	// Input values, 
	const IPv6Net&	scope_zone_id, 
	const bool&	is_scope_zone, 
	// Output values, 
	bool&	fail, 
	string&	reason);

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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_add_config_cand_rp_by_vif_name4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_cand_rp_by_vif_name6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_cand_rp_by_addr4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv4&	cand_rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_cand_rp_by_addr6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv6&	cand_rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	rp_holdtime, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_vif_name4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_vif_name6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_addr4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv4&	cand_rp_addr, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_cand_rp_by_addr6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const bool&	is_scope_zone, 
	const IPv6&	cand_rp_addr, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Add/delete/complete static RP configuration.
     *  
     *  @param group_prefix the group prefix for the RP.
     *  
     *  @param rp_addr the RP address.
     *  
     *  @param rp_priority the RP priority (smaller is better).
     *  
     *  @param hash_masklen the hash masklen used in computing an RP for a
     *  group. It should be same across all RPs. If set to zero, the default
     *  one will be used.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_add_config_rp4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const IPv4&	rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	hash_masklen, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_add_config_rp6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const IPv6&	rp_addr, 
	const uint32_t&	rp_priority, 
	const uint32_t&	hash_masklen, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_rp4(
	// Input values, 
	const IPv4Net&	group_prefix, 
	const IPv4&	rp_addr, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_delete_config_rp6(
	// Input values, 
	const IPv6Net&	group_prefix, 
	const IPv6&	rp_addr, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_config_rp_done(
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Configure PIM Hello-related metrics. The 'set_foo' XRLs set the
     *  particular values. The 'reset_foo' XRLs reset the metrics to their
     *  default values.
     *  
     *  @param vif_name the name of the vif to configure.
     *  
     *  @param proto_version the protocol version.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_get_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	proto_version, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	proto_version, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_triggered_delay, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	hello_triggered_delay, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_hello_period(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_period, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_hello_period(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	hello_period, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_hello_period(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_hello_holdtime(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_holdtime, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_hello_holdtime(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	hello_holdtime, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_hello_holdtime(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_dr_priority(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	dr_priority, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_dr_priority(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	dr_priority, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_dr_priority(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_lan_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	lan_delay, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_lan_delay(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	lan_delay, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_lan_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_override_interval(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	override_interval, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_override_interval(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	override_interval, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_override_interval(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_is_tracking_support_disabled(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	is_tracking_support_disabled, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_is_tracking_support_disabled(
	// Input values, 
	const string&	vif_name, 
	const bool&	is_tracking_support_disabled, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_is_tracking_support_disabled(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_accept_nohello_neighbors(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	accept_nohello_neighbors, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_accept_nohello_neighbors(
	// Input values, 
	const string&	vif_name, 
	const bool&	accept_nohello_neighbors, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_accept_nohello_neighbors(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_get_vif_join_prune_period(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	join_prune_period, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_set_vif_join_prune_period(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	join_prune_period, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_reset_vif_join_prune_period(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Enable/disable the PIM trace log.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_enable_log_trace(
	// Output values, 
	bool&	fail, 
	string&	reason);

    XrlCmdError pim_0_1_disable_log_trace(
	// Output values, 
	bool&	fail, 
	string&	reason);

    /**
     *  Test-related methods: add Join/Prune entries, and send them to a
     *  neighbor.
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_add_test_jp_entry4(
	// Input values, 
	const IPv4&	source_addr, 
	const IPv4&	group_addr, 
	const uint32_t&	group_masklen, 
	const string&	mrt_entry_type, 
	const string&	action_jp, 
	const uint32_t&	holdtime, 
	const bool&	new_group_bool, 
	// Output values, 
	bool&		fail, 
	string&		reason);

    XrlCmdError pim_0_1_add_test_jp_entry6(
	// Input values, 
	const IPv6&	source_addr, 
	const IPv6&	group_addr, 
	const uint32_t&	group_masklen, 
	const string&	mrt_entry_type, 
	const string&	action_jp, 
	const uint32_t&	holdtime, 
	const bool&	new_group_bool, 
	// Output values, 
	bool&		fail, 
	string&		reason);

    XrlCmdError pim_0_1_send_test_jp_entry4(
	// Input values, 
	const IPv4&	nbr_addr, 
	// Output values, 
	bool&		fail, 
	string&		reason);

    XrlCmdError pim_0_1_send_test_jp_entry6(
	// Input values, 
	const IPv6&	nbr_addr, 
	// Output values, 
	bool&		fail, 
	string&		reason);

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
     *  
     *  @param fail true if failure has occured.
     *  
     *  @param reason contains failure reason if it occured.
     */
    XrlCmdError pim_0_1_send_test_assert4(
	// Input values, 
	const string&	vif_name, 
	const IPv4&	source_addr, 
	const IPv4&	group_addr, 
	const bool&	rpt_bit, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric, 
	// Output values, 
	bool&		fail, 
	string&		reason);

    XrlCmdError pim_0_1_send_test_assert6(
	// Input values, 
	const string&	vif_name, 
	const IPv6&	source_addr, 
	const IPv6&	group_addr, 
	const bool&	rpt_bit, 
	const uint32_t&	metric_preference, 
	const uint32_t&	metric, 
	// Output values, 
	bool&		fail, 
	string&		reason);

private:
    const string& my_xrl_target_name() {
	return XrlPimTargetBase::name();
    }
    
    int family() const { return (PimNode::family()); }
};

#endif // __PIM_XRL_PIM_NODE_HH__
