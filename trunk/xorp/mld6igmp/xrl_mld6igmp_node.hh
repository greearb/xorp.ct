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

// $XORP: xorp/mld6igmp/xrl_mld6igmp_node.hh,v 1.12 2003/04/16 04:53:43 pavlin Exp $

#ifndef __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__
#define __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__


//
// MLD6IGMP XRL-aware node definition.
//

#include <string>

#include "libxorp/xlog.h"
#include "libxipc/xrl_router.hh"
#include "xrl/targets/mld6igmp_base.hh"
#include "xrl/interfaces/common_xif.hh"
#include "xrl/interfaces/mfea_xif.hh"
#include "xrl/interfaces/cli_manager_xif.hh"
#include "xrl/interfaces/mld6igmp_client_xif.hh"
#include "mld6igmp_node.hh"
#include "mld6igmp_node_cli.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlMld6igmpNode : public Mld6igmpNode,
			public XrlMld6igmpTargetBase,
			public XrlCommonV0p1Client,
			public XrlMfeaV0p1Client,
			public XrlCliManagerV0p1Client,
			public XrlMld6igmpClientV0p1Client,
			public Mld6igmpNodeCli {
public:
    XrlMld6igmpNode(int family, xorp_module_id module_id, 
		    EventLoop& eventloop, XrlRouter* xrl_router)
	: Mld6igmpNode(family, module_id, eventloop),
	  XrlMld6igmpTargetBase(xrl_router),
	  XrlCommonV0p1Client(xrl_router),
	  XrlMfeaV0p1Client(xrl_router),
	  XrlCliManagerV0p1Client(xrl_router),
	  XrlMld6igmpClientV0p1Client(xrl_router),
	  Mld6igmpNodeCli(*static_cast<Mld6igmpNode *>(this))
	{ }
    virtual ~XrlMld6igmpNode() { Mld6igmpNode::stop(); Mld6igmpNodeCli::stop(); }
    
    //
    // XrlMld6igmpNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();
    int enable_mld6igmp();
    int disable_mld6igmp();
    int start_mld6igmp();
    int stop_mld6igmp();
    
    
protected:
    //
    // Protocol node methods
    //
    void xrl_result_add_protocol(const XrlError& xrl_error);
    void xrl_result_allow_signal_messages(const XrlError& xrl_error);
    void xrl_result_delete_protocol(const XrlError& xrl_error);
    
    int	proto_send(const string& dst_module_instance_name,
		   xorp_module_id dst_module_id,
		   uint16_t vif_index,
		   const IPvX& src, const IPvX& dst,
		   int ip_ttl, int ip_tos, bool router_alert_bool,
		   const uint8_t * sndbuf, size_t sndlen);
    void xrl_result_send_protocol_message(const XrlError& xrl_error);
    
    int start_protocol_kernel();
    int stop_protocol_kernel();
    
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
    
    int send_add_membership(const string& dst_module_instance_name,
			    xorp_module_id dst_module_id,
			    uint16_t vif_index,
			    const IPvX& source,
			    const IPvX& group);
    void xrl_result_send_add_membership(const XrlError& xrl_error);

    int send_delete_membership(const string& dst_module_instance_name,
			       xorp_module_id dst_module_id,
			       uint16_t vif_index,
			       const IPvX& source,
			       const IPvX& group);
    void xrl_result_send_delete_membership(const XrlError& xrl_error);
    
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
     *  
     *  @param vif_index the index of the vif to delete.
     */
    XrlCmdError mfea_client_0_1_delete_vif(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

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
     */
    XrlCmdError mfea_client_0_1_add_vif_addr4(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	addr, 
	const IPv4Net&	subnet, 
	const IPv4&	broadcast, 
	const IPv4&	peer);

    XrlCmdError mfea_client_0_1_add_vif_addr6(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	addr, 
	const IPv6Net&	subnet, 
	const IPv6&	broadcast, 
	const IPv6&	peer);

    /**
     *  Delete an address from a vif.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param vif_index the index of the vif.
     *  
     *  @param addr the unicast address to delete.
     */
    XrlCmdError mfea_client_0_1_delete_vif_addr4(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	addr);
    
    XrlCmdError mfea_client_0_1_delete_vif_addr6(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	addr);

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
	const bool&	is_up);

    /**
     *  Complete a transaction with vif information.
     *  
     *  @param vif_name the name of the vif.
     *  
     *  @param vif_index the index of the vif.
     */
    XrlCmdError mfea_client_0_1_set_vif_done(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

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
	const string&		, // xrl_sender_name, 
	const string&		, // protocol_name, 
	const uint32_t&		, // protocol_id, 
	const uint32_t&		, // message_type, 
	const string&		, // vif_name, 
	const uint32_t&		, // vif_index, 
	const IPv4&		, // source_address, 
	const IPv4&		, // dest_address, 
	const vector<uint8_t>&	  // protocol_message
	) {
	string msg  = "Unexpected kernel signal message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    XrlCmdError mfea_client_0_1_recv_kernel_signal_message6(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const string&		, // protocol_name, 
	const uint32_t&		, // protocol_id, 
	const uint32_t&		, // message_type, 
	const string&		, // vif_name, 
	const uint32_t&		, // vif_index, 
	const IPv6&		, // source_address, 
	const IPv6&		, // dest_address, 
	const vector<uint8_t>&	  // protocol_message
	) {
	string msg = "Unexpected kernel signal message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // MRIB-related add/delete/done: not used by MLD or IGMP.
    //
    XrlCmdError mfea_client_0_1_add_mrib4(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv4Net&		, // dest_prefix, 
	const IPv4&		, // next_hop_router_addr, 
	const string&		, // next_hop_vif_name, 
	const uint32_t&		, // next_hop_vif_index, 
	const uint32_t&		, // metric_preference, 
	const uint32_t&		  // metric
	) {
	string msg = "Unexpected add_mrib() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    XrlCmdError mfea_client_0_1_add_mrib6(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv6Net&		, // dest_prefix, 
	const IPv6&		, // next_hop_router_addr, 
	const string&		, // next_hop_vif_name, 
	const uint32_t&		, // next_hop_vif_index, 
	const uint32_t&		, // metric_preference, 
	const uint32_t&		  // metric
	) {
	string msg = "Unexpected add_mrib() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    XrlCmdError mfea_client_0_1_delete_mrib4(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv4Net&		  // dest_prefix
	) {
	string msg = "Unexpected delete_mrib() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    XrlCmdError mfea_client_0_1_delete_mrib6(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv6Net&		  // dest_prefix
	) {
	string msg = "Unexpected delete_mrib() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    XrlCmdError mfea_client_0_1_set_mrib_done(
	// Input values, 
	const string&		  // xrl_sender_name
	) {
	string msg = "Unexpected set_mrib_done() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    //
    // A signal that a dataflow-related pre-condition is true:
    // not used by MLD or IGMP.
    //
    XrlCmdError mfea_client_0_1_recv_dataflow_signal4(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv4&		, // source_address, 
	const IPv4&		, // group_address, 
	const uint32_t&		, // threshold_interval_sec, 
	const uint32_t&		, // threshold_interval_usec, 
	const uint32_t&		, // measured_interval_sec, 
	const uint32_t&		, // measured_interval_usec, 
	const uint32_t&		, // threshold_packets, 
	const uint32_t&		, // threshold_bytes, 
	const uint32_t&		, // measured_packets, 
	const uint32_t&		, // measured_bytes, 
	const bool&		, // is_threshold_in_packets,
	const bool&		, // is_threshold_in_bytes,
	const bool&		, // is_geq_upcall,
	const bool&		  // is_leq_upcall
	) {
	string msg = "Unexpected recv_dataflow_signal4() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    XrlCmdError mfea_client_0_1_recv_dataflow_signal6(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv6&		, // source_address, 
	const IPv6&		, // group_address, 
	const uint32_t&		, // threshold_interval_sec, 
	const uint32_t&		, // threshold_interval_usec, 
	const uint32_t&		, // measured_interval_sec, 
	const uint32_t&		, // measured_interval_usec, 
	const uint32_t&		, // threshold_packets, 
	const uint32_t&		, // threshold_bytes, 
	const uint32_t&		, // measured_packets, 
	const uint32_t&		, // measured_bytes, 
	const bool&		, // is_threshold_in_packets,
	const bool&		, // is_threshold_in_bytes,
	const bool&		, // is_geq_upcall,
	const bool&		  // is_leq_upcall
	) {
	string msg = "Unexpected recv_dataflow_signal4() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    /**
     *  Enable/disable/start/stop a MLD6IGMP vif interface.
     *  
     *  @param vif_name the name of the vif to enable/disable/start/stop.
     */
    XrlCmdError mld6igmp_0_1_enable_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError mld6igmp_0_1_disable_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError mld6igmp_0_1_start_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError mld6igmp_0_1_stop_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Enable/disable/start/stop all MLD6IGMP vif interfaces.
     */
    XrlCmdError mld6igmp_0_1_enable_all_vifs();

    XrlCmdError mld6igmp_0_1_disable_all_vifs();

    XrlCmdError mld6igmp_0_1_start_all_vifs();

    XrlCmdError mld6igmp_0_1_stop_all_vifs();

    /**
     *  Enable/disable/start/stop MLD6IGMP protocol and MLD6IGMP CLI access.
     */
    XrlCmdError mld6igmp_0_1_enable_mld6igmp();

    XrlCmdError mld6igmp_0_1_disable_mld6igmp();

    XrlCmdError mld6igmp_0_1_enable_cli();

    XrlCmdError mld6igmp_0_1_disable_cli();

    XrlCmdError mld6igmp_0_1_start_mld6igmp();

    XrlCmdError mld6igmp_0_1_stop_mld6igmp();

    XrlCmdError mld6igmp_0_1_start_cli();

    XrlCmdError mld6igmp_0_1_stop_cli();

    /**
     *  Configure MLD6IGMP interface-related metrics. The 'set_foo' XRLs set
     *  the particular values. The 'reset_foo' XRLs reset the metrics to their
     *  default values.
     *  
     *  @param vif_name the name of the vif to configure.
     *  
     *  @param proto_version the protocol version.
     */
    XrlCmdError mld6igmp_0_1_get_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	proto_version);

    XrlCmdError mld6igmp_0_1_set_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	proto_version);

    XrlCmdError mld6igmp_0_1_reset_vif_proto_version(
	// Input values, 
	const string&	vif_name);

    /**
     *  Enable/disable the MLD6IGMP trace log.
     */
    XrlCmdError mld6igmp_0_1_enable_log_trace();

    XrlCmdError mld6igmp_0_1_disable_log_trace();

    /**
     *  Add/delete a client protocol in the MLD/IGMP protocol.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol to add/delete.
     *  
     *  @param protocol_id the ID of the protocol to add/delete (both sides
     *  must agree on the particular values).
     *  
     *  @param vif_name the name of the vif the protocol add/delete to apply
     *  to.
     *  
     *  @param vif_index the index of the vif the protocol add/delete to apply
     *  to. The added protocol will receive Join/Leave membership information
     *  about same-LAN members for the particular vif.
     */
    XrlCmdError mld6igmp_0_1_add_protocol4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mld6igmp_0_1_add_protocol6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mld6igmp_0_1_delete_protocol4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mld6igmp_0_1_delete_protocol6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);
    
    
private:
    const string& my_xrl_target_name() {
	return XrlMld6igmpTargetBase::name();
    }
    
    int family() const { return (Mld6igmpNode::family()); }
};

#endif // __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__
