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

#ident "$XORP: xorp/mld6igmp/xrl_mld6igmp_node.cc,v 1.2 2003/01/16 19:32:21 pavlin Exp $"

#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mld6igmp_node.hh"
#include "mld6igmp_node_cli.hh"
#include "mld6igmp_vif.hh"
#include "xrl_mld6igmp_node.hh"


//
// XrlMld6igmpNode front-end interface
//
int
XrlMld6igmpNode::enable_cli()
{
    int ret_code = XORP_OK;
    
    Mld6igmpNodeCli::enable();
    
    return (ret_code);
}

int
XrlMld6igmpNode::disable_cli()
{
    int ret_code = XORP_OK;
    
    Mld6igmpNodeCli::disable();
    
    return (ret_code);
}

int
XrlMld6igmpNode::start_cli()
{
    int ret_code = XORP_OK;
    
    if (Mld6igmpNodeCli::start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlMld6igmpNode::stop_cli()
{
    int ret_code = XORP_OK;
    
    if (Mld6igmpNodeCli::stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlMld6igmpNode::enable_mld6igmp()
{
    int ret_code = XORP_OK;
    
    Mld6igmpNode::enable();
    
    return (ret_code);
}

int
XrlMld6igmpNode::disable_mld6igmp()
{
    int ret_code = XORP_OK;
    
    Mld6igmpNode::disable();
    
    return (ret_code);
}

int
XrlMld6igmpNode::start_mld6igmp()
{
    int ret_code = XORP_OK;
    
    if (Mld6igmpNode::start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlMld6igmpNode::stop_mld6igmp()
{
    int ret_code = XORP_OK;
    
    if (Mld6igmpNode::stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

void
XrlMld6igmpNode::xrl_result_add_protocol(const XrlError& xrl_error,
					 const bool *fail,
					 const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to add a protocol to MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlMld6igmpNode::xrl_result_allow_signal_messages(const XrlError& xrl_error,
						  const bool *fail,
						  const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send allow_signal_messages() to MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlMld6igmpNode::xrl_result_delete_protocol(const XrlError& xrl_error,
					    const bool *fail,
					    const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to delete a protocol from MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

//
// Protocol node methods
//

/**
 * XrlMld6igmpNode::proto_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #x_module_id of the destination of the message.
 * @vif_index: The vif index of the interface to send this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * the TTL will be set by the lower layers.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * the TOS will be set by the lower layers.
 * @router_alert_bool: If true, the ROUTER_ALERT IP option for the IP
 * packet of the incoming message should be set.
 * @sndbuf: The data buffer with the message to send.
 * @sndlen: The data length in @sndbuf.
 * 
 * Send a protocol message through the FEA/MFEA.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMld6igmpNode::proto_send(const string& dst_module_instance_name,
			    x_module_id		, // dst_module_id,
			    uint16_t vif_index,
			    const IPvX& src,
			    const IPvX& dst,
			    int ip_ttl,
			    int ip_tos,
			    bool router_alert_bool,
			    const uint8_t *sndbuf,
			    size_t sndlen)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
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
	    XrlMfeaV0p1Client::send_send_protocol_message4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMld6igmpNode::xrl_result_send_protocol_message));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    XrlMfeaV0p1Client::send_send_protocol_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMld6igmpNode::xrl_result_send_protocol_message));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::xrl_result_send_protocol_message(const XrlError& xrl_error,
						  const bool *fail,
						  const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send a protocol message: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMld6igmpNode::start_protocol_kernel()
{    
    //
    // Register the protocol with the MFEA
    //
    if (family() == AF_INET) {
	XrlMfeaV0p1Client::send_add_protocol4(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(Mld6igmpNode::module_name()),
	    Mld6igmpNode::module_id(),
	    callback(this, &XrlMld6igmpNode::xrl_result_add_protocol));
    } else {
	XrlMfeaV0p1Client::send_add_protocol6(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(Mld6igmpNode::module_name()),
	    Mld6igmpNode::module_id(),
	    callback(this, &XrlMld6igmpNode::xrl_result_add_protocol));
    }
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::stop_protocol_kernel()
{   
    if (family() == AF_INET) {
	XrlMfeaV0p1Client::send_delete_protocol4(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(Mld6igmpNode::module_name()),
	    Mld6igmpNode::module_id(),
	    callback(this, &XrlMld6igmpNode::xrl_result_delete_protocol));
    } else {
	XrlMfeaV0p1Client::send_delete_protocol6(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(Mld6igmpNode::module_name()),
	    Mld6igmpNode::module_id(),
	    callback(this, &XrlMld6igmpNode::xrl_result_delete_protocol));
    }
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::start_protocol_kernel_vif(uint16_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot start in the kernel vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (Mld6igmpNode::is_ipv4()) {
	    XrlMfeaV0p1Client::send_start_protocol_vif4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::xrl_result_start_protocol_kernel_vif));
	    break;
	}
	
	if (Mld6igmpNode::is_ipv6()) {
	    XrlMfeaV0p1Client::send_start_protocol_vif6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::xrl_result_start_protocol_kernel_vif));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}
void
XrlMld6igmpNode::xrl_result_start_protocol_kernel_vif(const XrlError& xrl_error,
						      const bool *fail,
						      const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure start a kernel vif with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMld6igmpNode::stop_protocol_kernel_vif(uint16_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot stop in the kernel vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (Mld6igmpNode::is_ipv4()) {
	    XrlMfeaV0p1Client::send_stop_protocol_vif4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::xrl_result_stop_protocol_kernel_vif));
	    break;
	}
	
	if (Mld6igmpNode::is_ipv6()) {
	    XrlMfeaV0p1Client::send_stop_protocol_vif6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::xrl_result_stop_protocol_kernel_vif));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}
void
XrlMld6igmpNode::xrl_result_stop_protocol_kernel_vif(const XrlError& xrl_error,
						     const bool *fail,
						     const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure stop a kernel vif with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMld6igmpNode::join_multicast_group(uint16_t vif_index,
				      const IPvX& multicast_group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot join group %s on vif with vif_index %d: "
		   "no such vif", cstring(multicast_group), vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (multicast_group.is_ipv4()) {
	    XrlMfeaV0p1Client::send_join_multicast_group4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		multicast_group.get_ipv4(),
		callback(this, &XrlMld6igmpNode::xrl_result_join_multicast_group));
	    break;
	}
	
	if (multicast_group.is_ipv6()) {
	    XrlMfeaV0p1Client::send_join_multicast_group6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		multicast_group.get_ipv6(),
		callback(this, &XrlMld6igmpNode::xrl_result_join_multicast_group));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}
void
XrlMld6igmpNode::xrl_result_join_multicast_group(const XrlError& xrl_error,
						 const bool *fail,
						 const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure join a multicast group with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMld6igmpNode::leave_multicast_group(uint16_t vif_index,
				       const IPvX& multicast_group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot leave group %s on vif with vif_index %d: "
		   "no such vif", cstring(multicast_group), vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (multicast_group.is_ipv4()) {
	    XrlMfeaV0p1Client::send_leave_multicast_group4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		multicast_group.get_ipv4(),
		callback(this, &XrlMld6igmpNode::xrl_result_leave_multicast_group));
	    break;
	}
	
	if (multicast_group.is_ipv6()) {
	    XrlMfeaV0p1Client::send_leave_multicast_group6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		multicast_group.get_ipv6(),
		callback(this, &XrlMld6igmpNode::xrl_result_leave_multicast_group));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}
void
XrlMld6igmpNode::xrl_result_leave_multicast_group(const XrlError& xrl_error,
						  const bool *fail,
						  const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure leave a multicast group with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}


int
XrlMld6igmpNode::send_add_membership(const string& dst_module_instance_name,
				     x_module_id , // dst_module_id,
				     uint16_t vif_index,
				     const IPvX& source,
				     const IPvX& group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send add_membership to %s for (%s,%s) on vif with "
		   "vif_index %d: no such vif",
		   dst_module_instance_name.c_str(),
		   cstring(source),
		   cstring(group),
		   vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (group.is_ipv4()) {
	    XrlMld6igmpClientV0p1Client::send_add_membership4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		mld6igmp_vif->name(),
		vif_index,
		source.get_ipv4(),
		group.get_ipv4(),
		callback(this, &XrlMld6igmpNode::xrl_result_send_add_membership));
	    break;
	}
	
	if (group.is_ipv6()) {
	    XrlMld6igmpClientV0p1Client::send_add_membership6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		mld6igmp_vif->name(),
		vif_index,
		source.get_ipv6(),
		group.get_ipv6(),
		callback(this, &XrlMld6igmpNode::xrl_result_send_add_membership));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::xrl_result_send_add_membership(const XrlError& xrl_error,
						const bool *fail,
						const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure send add_membership to a client protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMld6igmpNode::send_delete_membership(const string& dst_module_instance_name,
					x_module_id , // dst_module_id,
					uint16_t vif_index,
					const IPvX& source,
					const IPvX& group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send delete_membership to %s for (%s,%s) on vif with "
		   "vif_index %d: no such vif",
		   dst_module_instance_name.c_str(),
		   cstring(source),
		   cstring(group),
		   vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (group.is_ipv4()) {
	    XrlMld6igmpClientV0p1Client::send_delete_membership4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		mld6igmp_vif->name(),
		vif_index,
		source.get_ipv4(),
		group.get_ipv4(),
		callback(this, &XrlMld6igmpNode::xrl_result_send_delete_membership));
	    break;
	}
	
	if (group.is_ipv6()) {
	    XrlMld6igmpClientV0p1Client::send_delete_membership6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		mld6igmp_vif->name(),
		vif_index,
		source.get_ipv6(),
		group.get_ipv6(),
		callback(this, &XrlMld6igmpNode::xrl_result_send_delete_membership));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while(false);
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::xrl_result_send_delete_membership(const XrlError& xrl_error,
						   const bool *fail,
						   const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure send delete_membership to a client protocol: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}


//
// Protocol node CLI methods
//
int
XrlMld6igmpNode::add_cli_command_to_cli_manager(const char *command_name,
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
	callback(this, &XrlMld6igmpNode::xrl_result_add_cli_command));
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::xrl_result_add_cli_command(const XrlError& xrl_error,
					    const bool *fail,
					    const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure add a command to CLI manager: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlMld6igmpNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    XrlCliManagerV0p1Client::send_delete_cli_command(
	x_module_name(family(), X_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlMld6igmpNode::xrl_result_delete_cli_command));
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::xrl_result_delete_cli_command(const XrlError& xrl_error,
					       const bool *fail,
					       const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure delete a command from CLI manager: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}


//
// XRL target methods
//

XrlCmdError
XrlMld6igmpNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = my_xrl_target_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::common_0_1_get_version(
    // Output values, 
    string&		version)
{
    version = XORP_MODULE_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::cli_processor_0_1_process_command(
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
    Mld6igmpNodeCli::cli_process_command(processor_name,
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
XrlMld6igmpNode::mfea_client_0_1_new_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::add_vif(vif_name.c_str(), vif_index) != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_delete_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::delete_vif(vif_name.c_str()) != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_add_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv4&		addr, 
    const IPv4Net&	subnet, 
    const IPv4&		broadcast, 
    const IPv4&		peer, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::add_vif_addr(vif_name.c_str(),
				   IPvX(addr),
				   IPvXNet(subnet),
				   IPvX(broadcast),
				   IPvX(peer))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_add_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv6&		addr, 
    const IPv6Net&	subnet, 
    const IPv6&		broadcast, 
    const IPv6&		peer, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::add_vif_addr(vif_name.c_str(),
				   IPvX(addr),
				   IPvXNet(subnet),
				   IPvX(broadcast),
				   IPvX(peer))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_delete_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv4&		addr, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::delete_vif_addr(vif_name.c_str(),
				      IPvX(addr))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_delete_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv6&		addr, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::delete_vif_addr(vif_name.c_str(),
				      IPvX(addr))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_set_vif_flags(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const bool&		is_pim_register, 
    const bool&		is_p2p, 
    const bool&		is_loopback, 
    const bool&		is_multicast, 
    const bool&		is_broadcast, 
    const bool&		is_up, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (Mld6igmpNode::set_vif_flags(vif_name.c_str(),
				    is_pim_register,
				    is_p2p,
				    is_loopback,
				    is_multicast,
				    is_broadcast,
				    is_up)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_set_vif_done(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_name(vif_name.c_str());
    
    if (mld6igmp_vif == NULL) {
	fail = true;
	reason = "Unknown vif";
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
XrlMld6igmpNode::mfea_client_0_1_set_all_vifs_done(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    Mld6igmpNode::set_vif_setup_completed(true);
    
#if 0		// TODO: XXX: PAVPAVPAV: don't start the vifs implicitly?
    //
    // If the protocol node is running, probably time to start the vifs as well
    //
    if (Mld6igmpNode::is_up()) {
	for (uint16_t i = 0; i < Mld6igmpNode::maxvifs(); i++) {
	    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(i);
	    if (mld6igmp_vif == NULL)
		continue;
	    mld6igmp_vif->start();
	}
    }
#endif
    
    //
    // XXX: the origin of this XRL doesn't care what we can do with the vifs
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_is_vif_setup_completed(
    // Output values, 
    bool&	is_completed)
{
    is_completed = Mld6igmpNode::is_vif_setup_completed();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_recv_protocol_message4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
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
	    reason = 
		"Received protocol message with invalid address family: IPv4";
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
    // Receive the message
    //
    Mld6igmpNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_recv_protocol_message6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
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
	    reason = 
		"Received protocol message with invalid address family: IPv6";
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
    // Receive the message
    //
    Mld6igmpNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_add_protocol4(
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
	    reason = 
		"Received protocol message with invalid address family: IPv4";
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
    
    if (Mld6igmpNode::add_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add protocol instance '%s' on vif_index %d",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Send info about all existing membership on the particular vif.
    //
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index);
	fail = true;
	reason = c_format("Cannot add protocol instance '%s' on vif_index %d: "
			  "no such vif",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    list<MemberQuery *>::const_iterator iter;
    for (iter = mld6igmp_vif->members().begin();
	 iter != mld6igmp_vif->members().end();
	 ++iter) {
	const MemberQuery *member_query = *iter;
	XrlMld6igmpClientV0p1Client::send_add_membership4(
	    xrl_sender_name.c_str(),
	    my_xrl_target_name(),
	    mld6igmp_vif->name(),
	    mld6igmp_vif->vif_index(),
	    member_query->source().get_ipv4(),
	    member_query->group().get_ipv4(),
	    callback(this, &XrlMld6igmpNode::xrl_result_send_add_membership));
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_add_protocol6(
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
	    reason = 
		"Received protocol message with invalid address family: IPv6";
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
    
    if (Mld6igmpNode::add_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot add protocol instance '%s' on vif_index %d",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    //
    // Send info about all existing membership on the particular vif.
    //
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index);
	fail = true;
	reason = c_format("Cannot add protocol instance '%s' on vif_index %d: "
			  "no such vif",
			  xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::OKAY();
    }
    
    list<MemberQuery *>::const_iterator iter;
    for (iter = mld6igmp_vif->members().begin();
	 iter != mld6igmp_vif->members().end();
	 ++iter) {
	const MemberQuery *member_query = *iter;
	XrlMld6igmpClientV0p1Client::send_add_membership6(
	    xrl_sender_name.c_str(),
	    my_xrl_target_name(),
	    mld6igmp_vif->name(),
	    mld6igmp_vif->vif_index(),
	    member_query->source().get_ipv6(),
	    member_query->group().get_ipv6(),
	    callback(this, &XrlMld6igmpNode::xrl_result_send_add_membership));
    }
    
    //
    // Success
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_delete_protocol4(
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
	    reason = 
		"Received protocol message with invalid address family: IPv4";
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
    
    if (Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete protocol instance '%s' on vif_index %d",
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
XrlMld6igmpNode::mld6igmp_0_1_delete_protocol6(
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
	    reason = 
		"Received protocol message with invalid address family: IPv6";
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
    
    if (Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	fail = true;
	// TODO: must find-out and return the reason for failure
	reason = c_format("Cannot delete protocol instance '%s' on vif_index %d",
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
XrlMld6igmpNode::mld6igmp_0_1_enable_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	reason = c_format("Cannot enable vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	mld6igmp_vif->enable();
	fail = false;
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_disable_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	reason = c_format("Cannot disable vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	mld6igmp_vif->disable();
	fail = false;
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_start_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	reason = c_format("Cannot start vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	if (mld6igmp_vif->start() != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	reason = c_format("Cannot stop vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	if (mld6igmp_vif->stop() != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpNode::enable_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_disable_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpNode::disable_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_start_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpNode::start_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    Mld6igmpNode::stop_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_mld6igmp(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (enable_mld6igmp() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_disable_mld6igmp(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (disable_mld6igmp() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_cli(
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
XrlMld6igmpNode::mld6igmp_0_1_disable_cli(
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
XrlMld6igmpNode::mld6igmp_0_1_start_mld6igmp(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (start_mld6igmp() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_mld6igmp(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (stop_mld6igmp() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_start_cli(
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
XrlMld6igmpNode::mld6igmp_0_1_stop_cli(
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
XrlMld6igmpNode::mld6igmp_0_1_enable_log_trace(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    Mld6igmpNode::set_log_trace(true);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_disable_log_trace(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    Mld6igmpNode::set_log_trace(false);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}
