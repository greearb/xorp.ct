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

#ident "$XORP: xorp/mfea/xrl_mfea_node.cc,v 1.5 2003/02/08 09:18:26 pavlin Exp $"

#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_node.hh"
#include "mfea_node_cli.hh"
#include "mfea_vif.hh"
#include "xrl_mfea_node.hh"


//
// XrlMfeaNode front-end interface
//
int
XrlMfeaNode::enable_cli()
{
    int ret_code = XORP_OK;
    
    MfeaNodeCli::enable();
    
    return (ret_code);
}

int
XrlMfeaNode::disable_cli()
{
    int ret_code = XORP_OK;
    
    MfeaNodeCli::disable();
    
    return (ret_code);
}

int
XrlMfeaNode::start_cli()
{
    int ret_code = XORP_OK;
    
    if (MfeaNodeCli::start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlMfeaNode::stop_cli()
{
    int ret_code = XORP_OK;
    
    if (MfeaNodeCli::stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlMfeaNode::enable_mfea()
{
    int ret_code = XORP_OK;
    
    MfeaNode::enable();
    
    return (ret_code);
}

int
XrlMfeaNode::disable_mfea()
{
    int ret_code = XORP_OK;
    
    MfeaNode::disable();
    
    return (ret_code);
}

int
XrlMfeaNode::start_mfea()
{
    int ret_code = XORP_OK;
    
    if (MfeaNode::start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlMfeaNode::stop_mfea()
{
    int ret_code = XORP_OK;
    
    if (MfeaNode::stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

//
// Protocol node methods
//

/**
 * XrlMfeaNode::proto_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #x_module_id of the protocol-destination of the message.
 * @vif_index: The vif index of the interface used to receive this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @router_alert_bool: If true, the ROUTER_ALERT IP option for the IP
 * packet of the incoming message was set.
 * @sndbuf: The data buffer with the message to send.
 * @sndlen: The data length in @sndbuf.
 * 
 * Send a protocol message received from the kernel to the user-level process.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::proto_send(const string& dst_module_instance_name,
			x_module_id	, // dst_module_id,
			uint16_t vif_index,
			const IPvX& src,
			const IPvX& dst,
			int ip_ttl,
			int ip_tos,
			bool router_alert_bool,
			const uint8_t *sndbuf,
			size_t sndlen
    )
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	XLOG_ERROR("Cannot send a protocol message on vif with vif_index %d: "
		   "no such vif",
		   vif_index);
	return (XORP_ERROR);
    }
    
    // Copy 'sndbuf' to a vector
    vector<uint8_t> snd_vector;
    snd_vector.resize(sndlen);
    for (size_t i = 0; i < sndlen; i++)
	snd_vector[i] = sndbuf[i];
    
    do {
	if (dst.is_ipv4()) {
	    XrlMfeaClientV0p1Client::send_recv_protocol_message4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		mfea_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMfeaNode::xrl_result_recv_protocol_message));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    XrlMfeaClientV0p1Client::send_recv_protocol_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		mfea_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMfeaNode::xrl_result_recv_protocol_message));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMfeaNode::xrl_result_recv_protocol_message(const XrlError& xrl_error,
					      const bool *fail,
					      const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send a data message to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

/**
 * XrlMfeaNode::signal_message_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #x_module_id of the protocol-destination of the message.
 * @message_type: The message type of the kernel signal.
 * At this moment, one of the following:
 * %MFEA_UNIX_KERNEL_MESSAGE_NOCACHE (if a cache-miss in the kernel)
 * %MFEA_UNIX_KERNEL_MESSAGE_WRONGVIF (multicast packet received on wrong vif)
 * %MFEA_UNIX_KERNEL_MESSAGE_WHOLEPKT (typically, a packet that should be
 * encapsulated as a PIM-Register).
 * %MFEA_UNIX_KERNEL_MESSAGE_BW_UPCALL (the bandwidth of a predefined
 * source-group flow is above or below a given threshold).
 * (XXX: The above types correspond to %IGMPMSG_* or %MRT6MSG_*).
 * @vif_index: The vif index of the related interface (message-specific
 * relation).
 * @src: The source address in the message.
 * @dst: The destination address in the message.
 * @sndbuf: The data buffer with the additional information in the message.
 * @sndlen: The data length in @sndbuf.
 * 
 * Send a kernel signal to an user-level protocol that expects it.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::signal_message_send(const string& dst_module_instance_name,
				 x_module_id	, // dst_module_id,
				 int message_type,
				 uint16_t vif_index,
				 const IPvX& src,
				 const IPvX& dst,
				 const uint8_t *sndbuf,
				 size_t sndlen)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	XLOG_ERROR("Cannot send a kernel signal message on vif with vif_index %d: "
		   "no such vif",
		   vif_index);
	return (XORP_ERROR);
    }

    // Copy 'sndbuf' to a vector
    vector<uint8_t> snd_vector;
    snd_vector.resize(sndlen);
    for (size_t i = 0; i < sndlen; i++)
	snd_vector[i] = sndbuf[i];
    
    do {
	if (dst.is_ipv4()) {
	    XrlMfeaClientV0p1Client::send_recv_kernel_signal_message4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		message_type,
		mfea_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		snd_vector,
		callback(this, &XrlMfeaNode::xrl_result_recv_kernel_signal_message));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    XrlMfeaClientV0p1Client::send_recv_kernel_signal_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		message_type,
		mfea_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		snd_vector,
		callback(this, &XrlMfeaNode::xrl_result_recv_kernel_signal_message));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while(false);
    
    return (XORP_OK);
}

/**
 * XrlMfeaNode::send_add_mrib:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #x_module_id of the protocol-destination of the message.
 * @mrib: The #Mrib entry to add.
 * 
 * Add a MRIB entry to an user-level protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::send_add_mrib(const string& dst_module_instance_name,
			   x_module_id ,	// dst_module_id,
			   const Mrib& mrib)
{
    string vif_name;
    Vif *vif = MfeaNode::vif_find_by_vif_index(mrib.next_hop_vif_index());
    
    if (vif != NULL)
	vif_name = vif->name();
    
    switch (family()) {
    case AF_INET:
	XrlMfeaClientV0p1Client::send_add_mrib4(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    mrib.dest_prefix().get_ipv4Net(),
	    mrib.next_hop_router_addr().get_ipv4(),
	    vif_name,
	    mrib.next_hop_vif_index(),
	    mrib.metric_preference(),
	    mrib.metric(),
	    callback(this, &XrlMfeaNode::xrl_result_add_mrib));
	break;
#ifdef HAVE_IPV6
	XrlMfeaClientV0p1Client::send_add_mrib6(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    mrib.dest_prefix().get_ipv6Net(),
	    mrib.next_hop_router_addr().get_ipv6(),
	    vif_name,
	    mrib.next_hop_vif_index(),
	    mrib.metric_preference(),
	    mrib.metric(),
	    callback(this, &XrlMfeaNode::xrl_result_add_mrib));
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	break;
    }
    
    return (XORP_OK);
}

/**
 * XrlMfeaNode::send_delete_mrib:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #x_module_id of the protocol-destination of the message.
 * @mrib: The #Mrib entry to delete.
 * 
 * Delete a MRIB entry from an user-level protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::send_delete_mrib(const string& dst_module_instance_name,
			      x_module_id ,	// dst_module_id,
			      const Mrib& mrib)
{
    switch (family()) {
    case AF_INET:
	XrlMfeaClientV0p1Client::send_delete_mrib4(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    mrib.dest_prefix().get_ipv4Net(),
	    callback(this, &XrlMfeaNode::xrl_result_delete_mrib));
	break;
#ifdef HAVE_IPV6
	XrlMfeaClientV0p1Client::send_delete_mrib6(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    mrib.dest_prefix().get_ipv6Net(),
	    callback(this, &XrlMfeaNode::xrl_result_delete_mrib));
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	break;
    }
    
    return (XORP_OK);
}

/**
 * XrlMfeaNode::send_set_mrib_done:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #x_module_id of the protocol-destination of the message.
 * 
 * Complete add/delete MRIB transaction.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::send_set_mrib_done(const string& dst_module_instance_name,
				x_module_id	// dst_module_id
    )
{
    XrlMfeaClientV0p1Client::send_set_mrib_done(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlMfeaNode::xrl_result_set_mrib_done));
    
    return (XORP_OK);
}

//
// Misc. callback functions for setup a vif.
// TODO: those are almost identical, and should be replaced by a single
// function.
//
void
XrlMfeaNode::xrl_result_recv_kernel_signal_message(const XrlError& xrl_error,
						   const bool *fail,
						   const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send a kernel signal message to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlMfeaNode::xrl_result_new_vif(const XrlError& xrl_error,
				const bool *fail,
				const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send new vif info to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}
void
XrlMfeaNode::xrl_result_delete_vif(const XrlError& xrl_error,
				   const bool *fail,
				   const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send delete_vif to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}
void
XrlMfeaNode::xrl_result_add_vif_addr(const XrlError& xrl_error,
				     const bool *fail,
				     const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send new vif address to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}
void
XrlMfeaNode::xrl_result_delete_vif_addr(const XrlError& xrl_error,
					const bool *fail,
					const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send delete vif address to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}
void
XrlMfeaNode::xrl_result_set_vif_flags(const XrlError& xrl_error,
				      const bool *fail,
				      const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send new vif flags to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}
void
XrlMfeaNode::xrl_result_set_vif_done(const XrlError& xrl_error,
				     const bool *fail,
				     const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send set_vif_done to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlMfeaNode::xrl_result_set_all_vifs_done(const XrlError& xrl_error,
					  const bool *fail,
					  const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send set_all_vifs_done to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMfeaNode::dataflow_signal_send(const string& dst_module_instance_name,
				  x_module_id	, // dst_module_id,
				  const IPvX& source_addr,
				  const IPvX& group_addr,
				  uint32_t threshold_interval_sec,
				  uint32_t threshold_interval_usec,
				  uint32_t measured_interval_sec,
				  uint32_t measured_interval_usec,
				  uint32_t threshold_packets,
				  uint32_t threshold_bytes,
				  uint32_t measured_packets,
				  uint32_t measured_bytes,
				  bool is_threshold_in_packets,
				  bool is_threshold_in_bytes,
				  bool is_geq_upcall,
				  bool is_leq_upcall)
{
    do {
	if (source_addr.is_ipv4()) {
	    XrlMfeaClientV0p1Client::send_recv_dataflow_signal4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		threshold_interval_sec,
		threshold_interval_usec,
		measured_interval_sec,
		measured_interval_usec,
		threshold_packets,
		threshold_bytes,
		measured_packets,
		measured_bytes,
		is_threshold_in_packets,
		is_threshold_in_bytes,
		is_geq_upcall,
		is_leq_upcall,
		callback(this, &XrlMfeaNode::xrl_result_recv_dataflow_signal));
	    break;
	}
	
	if (source_addr.is_ipv6()) {
	    XrlMfeaClientV0p1Client::send_recv_dataflow_signal6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
		threshold_interval_sec,
		threshold_interval_usec,
		measured_interval_sec,
		measured_interval_usec,
		threshold_packets,
		threshold_bytes,
		measured_packets,
		measured_bytes,
		is_threshold_in_packets,
		is_threshold_in_bytes,
		is_geq_upcall,
		is_leq_upcall,
		callback(this, &XrlMfeaNode::xrl_result_recv_dataflow_signal));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMfeaNode::xrl_result_recv_dataflow_signal(const XrlError& xrl_error,
					     const bool *fail,
					     const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send recv_dataflow_signal to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

//
// Protocol node CLI methods
//
int
XrlMfeaNode::add_cli_command_to_cli_manager(const char *command_name,
					    const char *command_help,
					    bool is_command_cd,
					    const char *command_cd_prompt,
					    bool is_command_processor
    )
{
    XrlCliManagerV0p1Client::send_add_cli_command(
	x_module_name(family(), X_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	string(command_help),
	is_command_cd,
	string(command_cd_prompt),
	is_command_processor,
	callback(this, &XrlMfeaNode::xrl_result_add_cli_command));
    
    return (XORP_OK);
}

void
XrlMfeaNode::xrl_result_add_cli_command(const XrlError& xrl_error,
					const bool *fail,
					const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to add a command to CLI manager: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMfeaNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    XrlCliManagerV0p1Client::send_delete_cli_command(
	x_module_name(family(), X_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlMfeaNode::xrl_result_delete_cli_command));
    
    return (XORP_OK);
}

void
XrlMfeaNode::xrl_result_delete_cli_command(const XrlError& xrl_error,
					   const bool *fail,
					   const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to delete a command from CLI manager: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}


//
// XRL target methods
//

XrlCmdError
XrlMfeaNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = my_xrl_target_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::common_0_1_get_version(
    // Output values, 
    string&		version)
{
    version = XORP_MODULE_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::cli_processor_0_1_process_command(
    // Input values, 
    const string&	processor_name, 
    const string&	cli_term_name, 
    const uint32_t&	cli_session_id,
    const string&	command_name, 
    const string&	command_args, 
    // Output values, 
    string&		ret_processor_name, 
    string&		ret_cli_term_name, 
    uint32_t&		ret_cli_session_id,
    string&		ret_command_output)
{
    MfeaNodeCli::cli_process_command(processor_name,
				     cli_term_name,
				     cli_session_id,
				     command_name,
				     command_args,
				     ret_processor_name,
				     ret_cli_term_name,
				     ret_cli_session_id,
				     ret_command_output);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_protocol4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    fail = true;
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	reason = c_format("Invalid module ID = %d", protocol_id);
	fail = true;
	return XrlCmdError::OKAY();
    }
    
    //
    // Add the protocol
    //
    if (MfeaNode::add_protocol(xrl_sender_name, src_module_id) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add protocol instance '%s' with module_id = %d",
			  xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Send info about all vifs
    //
    for (uint16_t i = 0; i < MfeaNode::maxvifs(); i++) {
	MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(i);
	XrlMfeaClientV0p1Client::send_new_vif(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    callback(this, &XrlMfeaNode::xrl_result_new_vif));
	list<VifAddr>::const_iterator iter;
	for (iter = mfea_vif->addr_list().begin();
	     iter != mfea_vif->addr_list().end();
	     ++iter) {
	    const VifAddr& vif_addr = *iter;
	    XrlMfeaClientV0p1Client::send_add_vif_addr4(
		xrl_sender_name.c_str(),
		mfea_vif->name(),
		mfea_vif->vif_index(),
		vif_addr.addr().get_ipv4(),
		vif_addr.subnet_addr().get_ipv4Net(),
		vif_addr.broadcast_addr().get_ipv4(),
		vif_addr.peer_addr().get_ipv4(),
		callback(this, &XrlMfeaNode::xrl_result_add_vif_addr));
	}
	XrlMfeaClientV0p1Client::send_set_vif_flags(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    mfea_vif->is_pim_register(),
	    mfea_vif->is_p2p(),
	    mfea_vif->is_loopback(),
	    mfea_vif->is_multicast_capable(),
	    mfea_vif->is_broadcast_capable(),
	    mfea_vif->is_up(),
	    callback(this, &XrlMfeaNode::xrl_result_set_vif_flags));
	XrlMfeaClientV0p1Client::send_set_vif_done(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    callback(this, &XrlMfeaNode::xrl_result_set_vif_done));
    }
    
    // We are done with the vifs
    XrlMfeaClientV0p1Client::send_set_all_vifs_done(
	xrl_sender_name.c_str(),
	callback(this, &XrlMfeaNode::xrl_result_set_all_vifs_done));
    
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_protocol6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    fail = true;
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Add the protocol
    //
    if (MfeaNode::add_protocol(xrl_sender_name, src_module_id) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add protocol instance '%s' with module_id = %d",
			  xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Send info about all vifs
    //
    for (uint16_t i = 0; i < MfeaNode::maxvifs(); i++) {
	MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(i);
	XrlMfeaClientV0p1Client::send_new_vif(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    callback(this, &XrlMfeaNode::xrl_result_new_vif));
	list<VifAddr>::const_iterator iter;
	for (iter = mfea_vif->addr_list().begin();
	     iter != mfea_vif->addr_list().end();
	     ++iter) {
	    const VifAddr& vif_addr = *iter;
	    XrlMfeaClientV0p1Client::send_add_vif_addr6(
		xrl_sender_name.c_str(),
		mfea_vif->name(),
		mfea_vif->vif_index(),
		vif_addr.addr().get_ipv6(),
		vif_addr.subnet_addr().get_ipv6Net(),
		vif_addr.broadcast_addr().get_ipv6(),
		vif_addr.peer_addr().get_ipv6(),
		callback(this, &XrlMfeaNode::xrl_result_add_vif_addr));
	}
	XrlMfeaClientV0p1Client::send_set_vif_flags(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    mfea_vif->is_pim_register(),
	    mfea_vif->is_p2p(),
	    mfea_vif->is_loopback(),
	    mfea_vif->is_multicast_capable(),
	    mfea_vif->is_broadcast_capable(),
	    mfea_vif->is_up(),
	    callback(this, &XrlMfeaNode::xrl_result_set_vif_flags));
	XrlMfeaClientV0p1Client::send_set_vif_done(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    callback(this, &XrlMfeaNode::xrl_result_set_vif_done));
    }
    
    // We are done with the vifs
    XrlMfeaClientV0p1Client::send_set_all_vifs_done(
	xrl_sender_name.c_str(),
	callback(this, &XrlMfeaNode::xrl_result_set_all_vifs_done));
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_protocol4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    fail = true;
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Delete the protocol
    //
    if (MfeaNode::delete_protocol(xrl_sender_name, src_module_id) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete protocol instance '%s' with module_id =  %d",
			  xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_protocol6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Delete the protocol
    //
    if (MfeaNode::delete_protocol(xrl_sender_name, src_module_id) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete protocol instance '%s' with module_id =  %d",
			  xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_protocol_vif4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    fail = true;
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Start the protocol on the vif
    //
    if (MfeaNode::start_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot start protocol instance '%s' "
			  "on vif_index %d",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_protocol_vif6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }

    //
    // Start the protocol on the vif
    //
    if (MfeaNode::start_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot start protocol instance '%s' "
			  "on vif_index %d",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_protocol_vif4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Stop the protocol on the vif
    //
    if (MfeaNode::stop_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot stop protocol instance '%s' "
			  "on vif_index %d",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_protocol_vif6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }

    //
    // Stop the protocol on the vif
    //
    if (MfeaNode::stop_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot stop protocol instance '%s' "
			  "on vif_index %d",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_allow_signal_messages(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const bool&		is_allow, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    if (is_allow) {
	if (MfeaNode::add_allow_kernel_signal_messages(xrl_sender_name,
						       src_module_id)
	    != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
    } else {
	if (MfeaNode::delete_allow_kernel_signal_messages(xrl_sender_name,
							  src_module_id)
	    != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_allow_mrib_messages(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const bool&		is_allow, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    if (is_allow) {
	if (MfeaNode::add_allow_mrib_messages(xrl_sender_name, src_module_id)
	    != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
    } else {
	if (MfeaNode::delete_allow_mrib_messages(xrl_sender_name, src_module_id)
	    != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
    }
    reason = "";	// TODO: return the xlog message?
    
    if (is_allow) {
	//
	// Send MRIB info
	//
	MribTable::iterator iter;
	for (iter = MfeaNode::mrib_table().begin();
	     iter != MfeaNode::mrib_table().end();
	     ++iter) {
	    Mrib *mrib = *iter;
	    if (mrib == NULL)
		continue;
	    string vif_name;
	    Vif *vif = MfeaNode::vif_find_by_vif_index(mrib->next_hop_vif_index());
	    if (vif != NULL)
		vif_name = vif->name();
	    
	    switch (family()) {
	    case AF_INET:
		XrlMfeaClientV0p1Client::send_add_mrib4(
		    xrl_sender_name.c_str(),
		    my_xrl_target_name(),
		    mrib->dest_prefix().get_ipv4Net(),
		    mrib->next_hop_router_addr().get_ipv4(),
		    vif_name,
		    mrib->next_hop_vif_index(),
		    mrib->metric_preference(),
		    mrib->metric(),
		    callback(this, &XrlMfeaNode::xrl_result_add_mrib));
		break;
#ifdef HAVE_IPV6
		XrlMfeaClientV0p1Client::send_add_mrib6(
		    xrl_sender_name.c_str(),
		    my_xrl_target_name(),
		    mrib->dest_prefix().get_ipv6Net(),
		    mrib->next_hop_router_addr().get_ipv6(),
		    vif_name,
		    mrib->next_hop_vif_index(),
		    mrib->metric_preference(),
		    mrib->metric(),
		    callback(this, &XrlMfeaNode::xrl_result_add_mrib));
		break;
#endif // HAVE_IPV6
	    default:
		XLOG_ASSERT(false);
		break;
	    }
	}
	//
	// Done
	//
	XrlMfeaClientV0p1Client::send_set_mrib_done(
	    xrl_sender_name.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlMfeaNode::xrl_result_set_mrib_done));
    }
    
    return XrlCmdError::OKAY();
}

void
XrlMfeaNode::xrl_result_add_mrib(const XrlError& xrl_error,
				 const bool *fail,
				 const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send add_mrib()  message to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlMfeaNode::xrl_result_delete_mrib(const XrlError& xrl_error,
				    const bool *fail,
				    const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send delete_mrib()  message to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlMfeaNode::xrl_result_set_mrib_done(const XrlError& xrl_error,
				      const bool *fail,
				      const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send set_mrib_done()  message to a protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

XrlCmdError
XrlMfeaNode::mfea_0_1_join_multicast_group4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    if (MfeaNode::join_multicast_group(xrl_sender_name, src_module_id, vif_index,
				       IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot join group %s on vif %s",
			  group_address.str().c_str(), vif_name.c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_join_multicast_group6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    if (MfeaNode::join_multicast_group(xrl_sender_name, src_module_id, vif_index,
				       IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot join group %s on vif %s",
			  group_address.str().c_str(), vif_name.c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_leave_multicast_group4(
    // Input values, 
    const string&	xrl_sender_name,
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    if (MfeaNode::leave_multicast_group(xrl_sender_name, src_module_id, vif_index,
					IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot leave group %s on vif %s",
			  group_address.str().c_str(), vif_name.c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_leave_multicast_group6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    if (MfeaNode::leave_multicast_group(xrl_sender_name, src_module_id, vif_index,
					IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot leave group %s on vif %s",
			  group_address.str().c_str(), vif_name.c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_mfc4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	iif_vif_index, 
    const vector<uint8_t>& oiflist, 
    const vector<uint8_t>& oiflist_disable_wrongvif, 
    const uint32_t&	max_vifs_oiflist, 
    const IPv4&		rp_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mifset mifset;
    Mifset mifset_disable_wrongvif;
    
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    // Check the number of covered interfaces
    if (max_vifs_oiflist > mifset.size()) {
	// Invalid number of covered interfaces
	fail = true;
	reason = c_format("Received 'add_mfc' with invalid "
			  "'max_vifs_oiflist' = %u (expected <= %u)",
			  max_vifs_oiflist, (uint32_t)mifset.size());
	return XrlCmdError::OKAY();
    }
    
    // Get the set of outgoing interfaces
    vector_to_mifset(oiflist, mifset);
    
    // Get the set of interfaces to disable the WRONGVIF signal.
    vector_to_mifset(oiflist_disable_wrongvif, mifset_disable_wrongvif);
    
    if (MfeaNode::add_mfc(xrl_sender_name,
			  IPvX(source_address), IPvX(group_address),
			  iif_vif_index, mifset, mifset_disable_wrongvif,
			  max_vifs_oiflist,
			  IPvX(rp_address))
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add MFC for source %s and group %s "
			  "with iif_vif_index = %d",
			  source_address.str().c_str(),
			  group_address.str().c_str(),
			  iif_vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_mfc6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	iif_vif_index, 
    const vector<uint8_t>& oiflist, 
    const vector<uint8_t>& oiflist_disable_wrongvif, 
    const uint32_t&	max_vifs_oiflist, 
    const IPv6&		rp_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mifset mifset;
    Mifset mifset_disable_wrongvif;
    
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    // Check the number of covered interfaces
    if (max_vifs_oiflist > mifset.size()) {
	// Invalid number of covered interfaces
	fail = true;
	reason = c_format("Received 'add_mfc' with invalid "
			  "'max_vifs_oiflist' = %u (expected <= %u)",
			  max_vifs_oiflist, (uint32_t)mifset.size());
	return XrlCmdError::OKAY();
    }
    
    // Get the set of outgoing interfaces
    vector_to_mifset(oiflist, mifset);
    
    // Get the set of interfaces to disable the WRONGVIF signal.
    vector_to_mifset(oiflist_disable_wrongvif, mifset_disable_wrongvif);
    
    if (MfeaNode::add_mfc(xrl_sender_name,
			  IPvX(source_address), IPvX(group_address),
			  iif_vif_index, mifset, mifset_disable_wrongvif,
			  max_vifs_oiflist,
			  IPvX(rp_address))
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add MFC for source %s and group %s "
			  "with iif_vif_index = %d",
			  source_address.str().c_str(),
			  group_address.str().c_str(),
			  iif_vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_mfc4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::delete_mfc(xrl_sender_name,
			     IPvX(source_address), IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete MFC for source %s and group %s",
			  source_address.str().c_str(),
			  group_address.str().c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_mfc6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::delete_mfc(xrl_sender_name,
			     IPvX(source_address), IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete MFC for source %s and group %s",
			  source_address.str().c_str(),
			  group_address.str().c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_send_protocol_message4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source_address, 
    const IPv4&		dest_address, 
    const int32_t&	ip_ttl, 
    const int32_t&	ip_tos, 
    const bool&		is_router_alert, 
    const vector<uint8_t>& protocol_message, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Receive the message
    //
    if (MfeaNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size()) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot send %s protocol message "
			  "from %s to %s on vif %s",
			  xrl_sender_name.c_str(),
			  source_address.str().c_str(),
			  dest_address.str().c_str(),
			  vif_name.c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_send_protocol_message6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source_address, 
    const IPv6&		dest_address, 
    const int32_t&	ip_ttl, 
    const int32_t&	ip_tos, 
    const bool&		is_router_alert, 
    const vector<uint8_t>& protocol_message, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    x_module_id src_module_id = static_cast<x_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	fail = true;
	reason = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::OKAY();
    }
    
    //
    // Receive the message
    //
    if (MfeaNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size()) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot send %s protocol message "
			  "from %s to %s on vif %s",
			  xrl_sender_name.c_str(),
			  source_address.str().c_str(),
			  dest_address.str().c_str(),
			  vif_name.c_str());
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::add_dataflow_monitor(xrl_sender_name,
				       IPvX(source_address),
				       IPvX(group_address),
				       mk_timeval(threshold_interval_sec,
						  threshold_interval_usec),
				       threshold_packets,
				       threshold_bytes,
				       is_threshold_in_packets,
				       is_threshold_in_bytes,
				       is_geq_upcall,
				       is_leq_upcall) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add dataflow monitoring for "
			  "source %s and group %s",
			  cstring(source_address),
			  cstring(group_address));
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::add_dataflow_monitor(xrl_sender_name,
				       IPvX(source_address),
				       IPvX(group_address),
				       mk_timeval(threshold_interval_sec,
						  threshold_interval_usec),
				       threshold_packets,
				       threshold_bytes,
				       is_threshold_in_packets,
				       is_threshold_in_bytes,
				       is_geq_upcall,
				       is_leq_upcall) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add dataflow monitoring for "
			  "source %s and group %s",
			  cstring(source_address),
			  cstring(group_address));
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::delete_dataflow_monitor(xrl_sender_name,
					  IPvX(source_address),
					  IPvX(group_address),
					  mk_timeval(threshold_interval_sec,
						     threshold_interval_usec),
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete dataflow monitoring for "
			  "source %s and group %s",
			  cstring(source_address),
			  cstring(group_address));
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::delete_dataflow_monitor(xrl_sender_name,
					  IPvX(source_address),
					  IPvX(group_address),
					  mk_timeval(threshold_interval_sec,
						     threshold_interval_usec),
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete dataflow monitoring for "
			  "source %s and group %s",
			  cstring(source_address),
			  cstring(group_address));
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_all_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv4");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::delete_all_dataflow_monitor(xrl_sender_name,
					      IPvX(source_address),
					      IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete all dataflow monitoring for "
			  "source %s and group %s",
			  cstring(source_address),
			  cstring(group_address));
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_all_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
#ifdef HAVE_IPV6
	if (family() != AF_INET6)
	    is_invalid_family = true;
#else
	is_invalid_family = true;
#endif
	
	if (is_invalid_family) {
	    // Invalid address family
	    fail = true;
	    reason = string("Received protocol message with invalid "
			      "address family: IPv6");
	    return XrlCmdError::OKAY();
	}
    } while (false);
    
    if (MfeaNode::delete_all_dataflow_monitor(xrl_sender_name,
					      IPvX(source_address),
					      IPvX(group_address)) < 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete all dataflow monitoring for "
			  "source %s and group %s",
			  cstring(source_address),
			  cstring(group_address));
	return XrlCmdError::OKAY();
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_name(vif_name);
    
    if (mfea_vif == NULL) {
	reason = c_format("Cannot enable vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	mfea_vif->enable();
	fail = false;
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_disable_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_name(vif_name);
    
    if (mfea_vif == NULL) {
	reason = c_format("Cannot disable vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	mfea_vif->disable();
	fail = false;
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_name(vif_name);
    
    if (mfea_vif == NULL) {
	reason = c_format("Cannot start vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	if (mfea_vif->start() != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_name(vif_name);
    
    if (mfea_vif == NULL) {
	reason = c_format("Cannot stop vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	if (mfea_vif->stop() != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaNode::enable_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_disable_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaNode::disable_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaNode::start_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    MfeaNode::stop_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_mfea(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (enable_mfea() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_disable_mfea(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (disable_mfea() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_cli(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (enable_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_disable_cli(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (disable_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_mfea(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (start_mfea() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_mfea(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (stop_mfea() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_cli(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (start_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_cli(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (stop_cli() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_log_trace(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    MfeaNode::set_log_trace(true);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_disable_log_trace(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    MfeaNode::set_log_trace(false);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_get_mrib_table_default_metric_preference(
    // Output values, 
    uint32_t&	metric_preference, 
    bool&	fail, 
    string&	reason)
{
    metric_preference = MfeaNode::mrib_table_default_metric_preference().get();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_set_mrib_table_default_metric_preference(
    // Input values, 
    const uint32_t&	metric_preference, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (MfeaNode::set_mrib_table_default_metric_preference(metric_preference)
	< 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_reset_mrib_table_default_metric_preference(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (MfeaNode::reset_mrib_table_default_metric_preference() < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_get_mrib_table_default_metric(
    // Output values, 
    uint32_t&	metric, 
    bool&	fail, 
    string&	reason)
{
    metric = MfeaNode::mrib_table_default_metric().get();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_set_mrib_table_default_metric(
    // Input values, 
    const uint32_t&	metric, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (MfeaNode::set_mrib_table_default_metric(metric) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_reset_mrib_table_default_metric(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (MfeaNode::reset_mrib_table_default_metric() < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}
