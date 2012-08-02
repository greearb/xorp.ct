// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "mfea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "mfea_node.hh"
#include "mfea_node_cli.hh"
#include "mfea_kernel_messages.hh"
#include "mfea_vif.hh"
#include "xrl_mfea_node.hh"


//
// XrlMfeaNode front-end interface
//
XrlMfeaNode::XrlMfeaNode(FeaNode&	fea_node,
			 int		family,
			 xorp_module_id	module_id,
			 EventLoop&	eventloop,
			 const string&	class_name,
			 const string&	finder_hostname,
			 uint16_t	finder_port,
			 const string&	finder_target)
	: MfeaNode(fea_node, family, module_id, eventloop),
      XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlMfeaTargetBase(&xrl_router()),
      MfeaNodeCli(*static_cast<MfeaNode *>(this)),
      _eventloop(eventloop),
      _finder_target(finder_target),
      _xrl_mfea_client_client(&xrl_router()),
      _xrl_cli_manager_client(&xrl_router()),
      _xrl_finder_client(&xrl_router()),
      _lib_mfea_client_bridge(xrl_router(), MfeaNode::mfea_iftree_update_replicator()),
      _is_finder_alive(false)
{
}

XrlMfeaNode::~XrlMfeaNode()
{
    shutdown();
}

int
XrlMfeaNode::startup()
{
    if (start_mfea() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlMfeaNode::shutdown()
{
    int ret_value = XORP_OK;

    if (stop_cli() != XORP_OK)
	ret_value = XORP_ERROR;

    if (stop_mfea() != XORP_OK)
	ret_value = XORP_ERROR;

    return (ret_value);
}

int
XrlMfeaNode::enable_cli()
{
    MfeaNodeCli::enable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::disable_cli()
{
    MfeaNodeCli::disable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::start_cli()
{
    if (MfeaNodeCli::start() != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMfeaNode::stop_cli()
{
    if (MfeaNodeCli::stop() != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMfeaNode::enable_mfea()
{
    MfeaNode::enable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::disable_mfea()
{
    MfeaNode::disable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::start_mfea()
{
    if (MfeaNode::start() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlMfeaNode::stop_mfea()
{
    int ret_code = XORP_OK;

    if (MfeaNode::stop() != XORP_OK)
	return (XORP_ERROR);
    
    return (ret_code);
}

//
// Finder-related events
//
/**
 * Called when Finder connection is established.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlMfeaNode::finder_connect_event()
{
    _is_finder_alive = true;
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlMfeaNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    _is_finder_alive = false;

    stop_mfea();
}

//
// Protocol node methods
//

/**
 * XrlMfeaNode::signal_message_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @message_type: The message type of the kernel signal.
 * At this moment, one of the following:
 * %MFEA_KERNEL_MESSAGE_NOCACHE (if a cache-miss in the kernel)
 * %MFEA_KERNEL_MESSAGE_WRONGVIF (multicast packet received on wrong vif)
 * %MFEA_KERNEL_MESSAGE_WHOLEPKT (typically, a packet that should be
 * encapsulated as a PIM-Register).
 * %MFEA_KERNEL_MESSAGE_BW_UPCALL (the bandwidth of a predefined
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
				 int message_type,
				 uint32_t vif_index,
				 const IPvX& src,
				 const IPvX& dst,
				 const uint8_t *sndbuf,
				 size_t sndlen)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(vif_index);

    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

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
	    _xrl_mfea_client_client.send_recv_kernel_signal_message4(
		dst_module_instance_name.c_str(),
		xrl_router().class_name(),
		message_type,
		mfea_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		snd_vector,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_kernel_signal_message_cb));
	    break;
	}
	
#ifdef HAVE_IPV6
	if (dst.is_ipv6()) {
	    _xrl_mfea_client_client.send_recv_kernel_signal_message6(
		dst_module_instance_name.c_str(),
		xrl_router().class_name(),
		message_type,
		mfea_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		snd_vector,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_kernel_signal_message_cb));
	    break;
	}
#endif
	
	XLOG_UNREACHABLE();
	break;
    } while(false);
    
    return (XORP_OK);
}

//
// Misc. callback functions for setup a vif.
// TODO: those are almost identical, and should be replaced by a single
// function.
//
void
XrlMfeaNode::mfea_client_client_send_recv_kernel_signal_message_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot send a kernel signal message to a protocol: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// XXX: if a transient error, then don't try again.
	// All kernel signal messages are soft-state
	// (i.e., they are retransmitted as appropriate by the kernel),
	// hence we don't retransmit them here if there was an error.
	//
	XLOG_ERROR("Failed to send a kernel signal message to a protocol: %s",
		   xrl_error.str().c_str());
	break;
    }
}

int
XrlMfeaNode::dataflow_signal_send(const string& dst_module_instance_name,
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
    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

    do {
	if (source_addr.is_ipv4()) {
	    _xrl_mfea_client_client.send_recv_dataflow_signal4(
		dst_module_instance_name.c_str(),
		xrl_router().class_name(),
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
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_dataflow_signal_cb));
	    break;
	}
	
#ifdef HAVE_IPV6
	if (source_addr.is_ipv6()) {
	    _xrl_mfea_client_client.send_recv_dataflow_signal6(
		dst_module_instance_name.c_str(),
		xrl_router().class_name(),
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
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_dataflow_signal_cb));
	    break;
	}
	XLOG_UNREACHABLE();
#endif
	
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMfeaNode::mfea_client_client_send_recv_dataflow_signal_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot send recv_dataflow_signal to a protocol: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// XXX: if a transient error, then don't try again.
	// All dataflow signals are soft-state (i.e., they are
	// retransmitted periodically by the originator),
	// hence we don't retransmit them here if there was an error.
	//
	XLOG_ERROR("Failed to send recv_dataflow_signal to a protocol: %s",
		   xrl_error.str().c_str());
	break;
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
    bool success = true;

    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

    success = _xrl_cli_manager_client.send_add_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	xrl_router().class_name(),
	string(command_name),
	string(command_help),
	is_command_cd,
	string(command_cd_prompt),
	is_command_processor,
	callback(this, &XrlMfeaNode::cli_manager_client_send_add_cli_command_cb));

    if (! success) {
	XLOG_ERROR("Failed to add CLI command '%s' to the CLI manager",
		   command_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlMfeaNode::cli_manager_client_send_add_cli_command_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot add a command to CLI manager: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("Cannot add a command to CLI manager: %s",
		   xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	//
	// TODO: if the command failed, then we should retransmit it
	//
	XLOG_ERROR("Failed to add a command to CLI manager: %s",
		   xrl_error.str().c_str());
	break;
    }
}

int
XrlMfeaNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    bool success = true;

    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

    success = _xrl_cli_manager_client.send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	xrl_router().class_name(),
	string(command_name),
	callback(this, &XrlMfeaNode::cli_manager_client_send_delete_cli_command_cb));

    if (! success) {
	XLOG_ERROR("Failed to delete CLI command '%s' with the CLI manager",
		   command_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlMfeaNode::cli_manager_client_send_delete_cli_command_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("Cannot delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	//
	// TODO: if the command failed, then we should retransmit it
	//
	XLOG_ERROR("Failed to delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
	break;
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
    name = xrl_router().class_name();
    
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
XrlMfeaNode::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&		reason)
{
    status = MfeaNode::node_status(reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::common_0_1_shutdown()
{
    string error_msg;

    if (shutdown() != XORP_OK) {
	error_msg = c_format("Failed to shutdown MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

/**
 *  Announce target birth to observer.
 *
 *  @param target_class the target class name.
 *
 *  @param target_instance the target instance name.
 */
XrlCmdError
XrlMfeaNode::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    UNUSED(target_class);
    UNUSED(target_instance);

    return XrlCmdError::OKAY();
}

/**
 *  Announce target death to observer.
 *
 *  @param target_class the target class name.
 *
 *  @param target_instance the target instance name.
 */
XrlCmdError
XrlMfeaNode::finder_event_observer_0_1_xrl_target_death(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    UNUSED(target_class);
    UNUSED(target_instance);

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
XrlMfeaNode::ifmgr_replicator_0_1_register_ifmgr_mirror(
    // Input values,
    const string&	clientname)
{
    string error_msg;

    if (_lib_mfea_client_bridge.add_libfeaclient_mirror(clientname) != XORP_OK) {
	error_msg = c_format("Cannot register ifmgr mirror client %s",
			     clientname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::ifmgr_replicator_0_1_unregister_ifmgr_mirror(
    // Input values,
    const string&	clientname)
{
    string error_msg;

    if (_lib_mfea_client_bridge.remove_libfeaclient_mirror(clientname)
	!= XORP_OK) {
	error_msg = c_format("Cannot unregister ifmgr mirror client %s",
			     clientname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_have_multicast_routing4(
    // Output values, 
    bool&	result)
{
    result = MfeaNode::have_multicast_routing4();
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlMfeaNode::mfea_0_1_have_multicast_routing6(
    // Output values, 
    bool&	result)
{
    result = MfeaNode::have_multicast_routing6();
    
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlMfeaNode::mfea_0_1_register_protocol6(
    // Input values,
    const string&	xrl_sender_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Register the protocol
    //
    if (MfeaNode::register_protocol(xrl_sender_name, if_name, vif_name,
				    ip_protocol, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_unregister_protocol6(
    // Input values,
    const string&	xrl_sender_name,
    const string&	if_name,
    const string&	vif_name)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Register the protocol
    //
    if (MfeaNode::unregister_protocol(xrl_sender_name, if_name, vif_name,
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
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
    const uint32_t&     distance)
{
    string error_msg;
    Mifset mifset;
    Mifset mifset_disable_wrongvif;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Check the number of covered interfaces
    if (max_vifs_oiflist > mifset.size()) {
	// Invalid number of covered interfaces
	error_msg = c_format("Received 'add_mfc' with invalid "
			     "'max_vifs_oiflist' = %u (expected <= %u)",
			     XORP_UINT_CAST(max_vifs_oiflist),
			     XORP_UINT_CAST(mifset.size()));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    // Get the set of outgoing interfaces
    vector_to_mifset(oiflist, mifset);
    
    // Get the set of interfaces to disable the WRONGVIF signal.
    vector_to_mifset(oiflist_disable_wrongvif, mifset_disable_wrongvif);
    
    if (MfeaNode::add_mfc(xrl_sender_name,
			  IPvX(source_address), IPvX(group_address),
			  iif_vif_index, mifset, mifset_disable_wrongvif,
			  max_vifs_oiflist,
			  IPvX(rp_address), distance, error_msg, true)
	!= XORP_OK) {
	error_msg += c_format("Cannot add MFC for "
			      "source %s and group %s "
			      "with iif_vif_index = %u",
			      source_address.str().c_str(),
			      group_address.str().c_str(),
			      XORP_UINT_CAST(iif_vif_index));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_mfc6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_mfc(xrl_sender_name,
			     IPvX(source_address), IPvX(group_address),
			     error_msg, true)
	!= XORP_OK) {
	error_msg += c_format("Cannot delete MFC for "
			      "source %s and group %s",
			      source_address.str().c_str(),
			      group_address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
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
    const bool&		is_leq_upcall)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::add_dataflow_monitor(xrl_sender_name,
				       IPvX(source_address),
				       IPvX(group_address),
				       TimeVal(threshold_interval_sec,
					       threshold_interval_usec),
				       threshold_packets,
				       threshold_bytes,
				       is_threshold_in_packets,
				       is_threshold_in_bytes,
				       is_geq_upcall,
				       is_leq_upcall,
				       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
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
    const bool&		is_leq_upcall)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_dataflow_monitor(xrl_sender_name,
					  IPvX(source_address),
					  IPvX(group_address),
					  TimeVal(threshold_interval_sec,
						  threshold_interval_usec),
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall,
					  error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_all_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_all_dataflow_monitor(xrl_sender_name,
					      IPvX(source_address),
					      IPvX(group_address),
					      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

#endif //ipv6

XrlCmdError
XrlMfeaNode::mfea_0_1_register_protocol4(
    // Input values,
    const string&	xrl_sender_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Register the protocol
    //
    if (MfeaNode::register_protocol(xrl_sender_name, if_name, vif_name,
				    ip_protocol, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_unregister_protocol4(
    // Input values,
    const string&	xrl_sender_name,
    const string&	if_name,
    const string&	vif_name)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Register the protocol
    //
    if (MfeaNode::unregister_protocol(xrl_sender_name, if_name, vif_name,
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
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
    const uint32_t&      distance)
{
    string error_msg;
    Mifset mifset;
    Mifset mifset_disable_wrongvif;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Check the number of covered interfaces
    if (max_vifs_oiflist > mifset.size()) {
	// Invalid number of covered interfaces
	error_msg = c_format("Received 'add_mfc' with invalid "
			     "'max_vifs_oiflist' = %u (expected <= %u)",
			     XORP_UINT_CAST(max_vifs_oiflist),
			     XORP_UINT_CAST(mifset.size()));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    // Get the set of outgoing interfaces
    vector_to_mifset(oiflist, mifset);
    
    // Get the set of interfaces to disable the WRONGVIF signal.
    vector_to_mifset(oiflist_disable_wrongvif, mifset_disable_wrongvif);
    
    if (MfeaNode::add_mfc(xrl_sender_name,
			  IPvX(source_address), IPvX(group_address),
			  iif_vif_index, mifset, mifset_disable_wrongvif,
			  max_vifs_oiflist,
			  IPvX(rp_address), distance, error_msg, true)
	!= XORP_OK) {
	error_msg += c_format("Cannot add MFC for "
			      "source %s and group %s "
			      "with iif_vif_index = %u",
			      source_address.str().c_str(),
			      group_address.str().c_str(),
			      XORP_UINT_CAST(iif_vif_index));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_mfc4_str(
    // Input values,
    const string&	xrl_sender_name,
    const IPv4&		source_address,
    const IPv4&		group_address,
    const string&       iif_ifname,
    const string&       oif_ifnames,
    const uint32_t&      distance)
{
    string error_msg;

    XLOG_INFO("received mfea add-mfc command, sender-name: %s input: %s  mcast-addr: %s  ifname: %s  output_ifs: %s\n",
	      xrl_sender_name.c_str(),
	      source_address.str().c_str(),
	      group_address.str().c_str(),
	      iif_ifname.c_str(),
	      oif_ifnames.c_str());
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::add_mfc_str(xrl_sender_name,
			      IPvX(source_address), IPvX(group_address),
			      iif_ifname, oif_ifnames, distance, error_msg, true) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_mfc4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_mfc(xrl_sender_name,
			     IPvX(source_address), IPvX(group_address),
			     error_msg, true)
	!= XORP_OK) {
	error_msg += c_format("Cannot delete MFC for "
			      "source %s and group %s",
			      source_address.str().c_str(),
			      group_address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
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
    const bool&		is_leq_upcall)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::add_dataflow_monitor(xrl_sender_name,
				       IPvX(source_address),
				       IPvX(group_address),
				       TimeVal(threshold_interval_sec,
					       threshold_interval_usec),
				       threshold_packets,
				       threshold_bytes,
				       is_threshold_in_packets,
				       is_threshold_in_bytes,
				       is_geq_upcall,
				       is_leq_upcall,
				       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
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
    const bool&		is_leq_upcall)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_dataflow_monitor(xrl_sender_name,
					  IPvX(source_address),
					  IPvX(group_address),
					  TimeVal(threshold_interval_sec,
						  threshold_interval_usec),
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall,
					  error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_all_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_all_dataflow_monitor(xrl_sender_name,
					      IPvX(source_address),
					      IPvX(group_address),
					      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_vif(
	// Input values,
	const string&	vif_name,
	const bool&	enable)
{
    string error_msg;
    int ret_code;

    if (enable)
	ret_code = MfeaNode::enable_vif(vif_name, error_msg);
    else
	ret_code = MfeaNode::disable_vif(vif_name, error_msg);

    if (ret_code != XORP_OK) {
        XLOG_ERROR("Mfea, enable/disable vif failed.  Allowing commit to succeed anyway since this is likely a race with a deleted interface, error: %s\n", error_msg.c_str());
	//return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (MfeaNode::start_vif(vif_name, error_msg) != XORP_OK) {
        XLOG_ERROR("Mfea, start vif failed.  Allowing commit to succeed anyway, error: %s\n", error_msg.c_str());
	//return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (MfeaNode::stop_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_all_vifs(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_code;

    if (enable)
	ret_code = MfeaNode::enable_all_vifs();
    else
	ret_code = MfeaNode::disable_all_vifs();

    if (ret_code != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable all vifs");
	else
	    error_msg = c_format("Failed to disable all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_all_vifs()
{
    string error_msg;

    if (MfeaNode::start_all_vifs() != XORP_OK) {
	error_msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_all_vifs()
{
    string error_msg;

    if (MfeaNode::stop_all_vifs() != XORP_OK) {
	error_msg = c_format("Failed to stop all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_mfea(
	// Input values,
	const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_mfea();
    else
	ret_value = disable_mfea();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable MFEA");
	else
	    error_msg = c_format("Failed to disable MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_mfea()
{
    string error_msg;

    if (start_mfea() != XORP_OK) {
	error_msg = c_format("Failed to start MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_mfea()
{
    string error_msg;

    if (stop_mfea() != XORP_OK) {
	error_msg = c_format("Failed to stop MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_cli(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_cli();
    else
	ret_value = disable_cli();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable MFEA CLI");
	else
	    error_msg = c_format("Failed to disable MFEA CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_cli()
{
    string error_msg;

    if (start_cli() != XORP_OK) {
	error_msg = c_format("Failed to start MFEA CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_cli()
{
    string error_msg;

    if (stop_cli() != XORP_OK) {
	error_msg = c_format("Failed to start MFEA CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_log_trace_all(
    // Input values,
    const bool&	enable)
{
    MfeaNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}
