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

#ident "$XORP: xorp/pim/xrl_pim_node.cc,v 1.1.1.1 2002/12/11 23:56:12 hodson Exp $"

#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mfc.hh"
#include "pim_node.hh"
#include "pim_node_cli.hh"
#include "pim_mrib_table.hh"
#include "pim_vif.hh"
#include "xrl_pim_node.hh"


//
// XrlPimNode front-end interface
//

int
XrlPimNode::enable_cli()
{
    int ret_code = XORP_OK;
    
    PimNodeCli::enable();
    
    return (ret_code);
}

int
XrlPimNode::disable_cli()
{
    int ret_code = XORP_OK;
    
    PimNodeCli::disable();
    
    return (ret_code);
}

int
XrlPimNode::start_cli()
{
    int ret_code = XORP_OK;
    
    if (PimNodeCli::start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlPimNode::stop_cli()
{
    int ret_code = XORP_OK;
    
    if (PimNodeCli::stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlPimNode::enable_pim()
{
    int ret_code = XORP_OK;
    
    PimNode::enable();
    
    return (ret_code);
}

int
XrlPimNode::disable_pim()
{
    int ret_code = XORP_OK;
    
    PimNode::disable();
    
    return (ret_code);
}

int
XrlPimNode::start_pim()
{
    int ret_code = XORP_OK;
    
    if (PimNode::start() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlPimNode::stop_pim()
{
    int ret_code = XORP_OK;
    
    if (PimNode::stop() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlPimNode::enable_bsr()
{
    int ret_code = XORP_OK;
    
    PimNode::enable_bsr();
    
    return (ret_code);
}

int
XrlPimNode::disable_bsr()
{
    int ret_code = XORP_OK;
    
    PimNode::disable_bsr();
    
    return (ret_code);
}

int
XrlPimNode::start_bsr()
{
    int ret_code = XORP_OK;
    
    if (PimNode::start_bsr() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
XrlPimNode::stop_bsr()
{
    int ret_code = XORP_OK;
    
    if (PimNode::stop_bsr() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}


void
XrlPimNode::xrl_result_add_protocol_mfea(const XrlError& xrl_error,
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
XrlPimNode::xrl_result_allow_signal_messages(const XrlError& xrl_error,
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
XrlPimNode::xrl_result_allow_mrib_messages(const XrlError& xrl_error,
					   const bool *fail,
					   const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to send allow_mrib_messages() to MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlPimNode::xrl_result_delete_protocol_mfea(const XrlError& xrl_error,
					    const bool *fail,
					    const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to delete a protocol with MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

//
// Protocol node methods
//

/**
 * XrlPimNode::proto_send:
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
XrlPimNode::proto_send(const string& dst_module_instance_name,
		       x_module_id	, // dst_module_id,
		       uint16_t vif_index,
		       const IPvX& src,
		       const IPvX& dst,
		       int ip_ttl,
		       int ip_tos,
		       bool router_alert_bool,
		       const uint8_t *sndbuf,
		       size_t sndlen)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
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
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlPimNode::xrl_result_send_protocol_message));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    XrlMfeaV0p1Client::send_send_protocol_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlPimNode::xrl_result_send_protocol_message));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
	
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_send_protocol_message(const XrlError& xrl_error,
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
XrlPimNode::start_protocol_kernel()
{    
    //
    // Register the protocol with the MFEA
    //
    if (family() == AF_INET) {
	XrlMfeaV0p1Client::send_add_protocol4(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mfea));
    } else {
	XrlMfeaV0p1Client::send_add_protocol6(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mfea));
    }
    
    //
    // Enable the receiving of kernel signal messages
    //
    XrlMfeaV0p1Client::send_allow_signal_messages(
	x_module_name(family(), X_MODULE_MFEA),
	my_xrl_target_name(),
	string(PimNode::module_name()),
	PimNode::module_id(),
	true,			// XXX: enable
	callback(this, &XrlPimNode::xrl_result_allow_signal_messages));
    
    //
    // Enable receiving of MRIB information
    //
    XrlMfeaV0p1Client::send_allow_mrib_messages(
	x_module_name(family(), X_MODULE_MFEA),
	my_xrl_target_name(),
	string(PimNode::module_name()),
	PimNode::module_id(),
	true,			// XXX: enable
	callback(this, &XrlPimNode::xrl_result_allow_mrib_messages));
    
    // TODO: explicitly start the XRL operations?
    
    return (XORP_OK);
}

int
XrlPimNode::stop_protocol_kernel()
{ 
    if (family() == AF_INET) {
	XrlMfeaV0p1Client::send_delete_protocol4(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_delete_protocol_mfea));
    } else {
	XrlMfeaV0p1Client::send_delete_protocol6(
	    x_module_name(family(), X_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_delete_protocol_mfea));
    }
    
    return (XORP_OK);
}

int
XrlPimNode::start_protocol_kernel_vif(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot start in the kernel vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (PimNode::is_ipv4()) {
	    XrlMfeaV0p1Client::send_start_protocol_vif4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::xrl_result_start_protocol_kernel_vif));
	    break;
	}
	
	if (PimNode::is_ipv6()) {
	    XrlMfeaV0p1Client::send_start_protocol_vif6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::xrl_result_start_protocol_kernel_vif));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}
void
XrlPimNode::xrl_result_start_protocol_kernel_vif(const XrlError& xrl_error,
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
XrlPimNode::stop_protocol_kernel_vif(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot stop in the kernel vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (PimNode::is_ipv4()) {
	    XrlMfeaV0p1Client::send_stop_protocol_vif4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::xrl_result_stop_protocol_kernel_vif));
	    break;
	}
	
	if (PimNode::is_ipv6()) {
	    XrlMfeaV0p1Client::send_stop_protocol_vif6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::xrl_result_stop_protocol_kernel_vif));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}
void
XrlPimNode::xrl_result_stop_protocol_kernel_vif(const XrlError& xrl_error,
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
XrlPimNode::join_multicast_group(uint16_t vif_index,
				 const IPvX& multicast_group)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot join group %s on vif with vif_index %d: "
		   "no such vif", cstring(multicast_group), vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (multicast_group.is_ipv4()) {
	    XrlMfeaV0p1Client::send_join_multicast_group4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		multicast_group.get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_join_multicast_group));
	    break;
	}
	
	if (multicast_group.is_ipv6()) {
	    XrlMfeaV0p1Client::send_join_multicast_group6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		multicast_group.get_ipv6(),
		callback(this, &XrlPimNode::xrl_result_join_multicast_group));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_join_multicast_group(const XrlError& xrl_error,
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
XrlPimNode::leave_multicast_group(uint16_t vif_index,
				  const IPvX& multicast_group)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot leave group %s on vif with vif_index %d: "
		   "no such vif", cstring(multicast_group), vif_index);
	return (XORP_ERROR);
    }
    
    do {
	if (multicast_group.is_ipv4()) {
	    XrlMfeaV0p1Client::send_leave_multicast_group4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		multicast_group.get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_leave_multicast_group));
	    break;
	}
	
	if (multicast_group.is_ipv6()) {
	    XrlMfeaV0p1Client::send_leave_multicast_group6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		multicast_group.get_ipv6(),
		callback(this, &XrlPimNode::xrl_result_leave_multicast_group));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while(false);
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_leave_multicast_group(const XrlError& xrl_error,
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
XrlPimNode::add_mfc_to_kernel(const PimMfc& pim_mfc)
{
    size_t max_vifs_oiflist = pim_mfc.olist().size();
    vector<uint8_t> oiflist_vector(max_vifs_oiflist);
    vector<uint8_t> oiflist_disable_wrongvif_vector(max_vifs_oiflist);
    
    mifset_to_vector(pim_mfc.olist(), oiflist_vector);
    mifset_to_vector(pim_mfc.olist_disable_wrongvif(),
		     oiflist_disable_wrongvif_vector);
    
    do {
	if (pim_mfc.source_addr().is_ipv4()) {
	    XrlMfeaV0p1Client::send_add_mfc4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		pim_mfc.source_addr().get_ipv4(),
		pim_mfc.group_addr().get_ipv4(),
		pim_mfc.iif_vif_index(),
		oiflist_vector,
		oiflist_disable_wrongvif_vector,
		max_vifs_oiflist,
		pim_mfc.rp_addr().get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_add_mfc_to_kernel));
	    break;
	}
	
	if (pim_mfc.source_addr().is_ipv6()) {
	    XrlMfeaV0p1Client::send_add_mfc6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		pim_mfc.source_addr().get_ipv6(),
		pim_mfc.group_addr().get_ipv6(),
		pim_mfc.iif_vif_index(),
		oiflist_vector,
		oiflist_disable_wrongvif_vector,
		max_vifs_oiflist,
		pim_mfc.rp_addr().get_ipv6(),
		callback(this, &XrlPimNode::xrl_result_add_mfc_to_kernel));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_add_mfc_to_kernel(const XrlError& xrl_error,
					 const bool *fail,
					 const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure add MFC with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlPimNode::delete_mfc_from_kernel(const PimMfc& pim_mfc)
{
    do {
	if (pim_mfc.source_addr().is_ipv4()) {
	    XrlMfeaV0p1Client::send_delete_mfc4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		pim_mfc.source_addr().get_ipv4(),
		pim_mfc.group_addr().get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_delete_mfc_from_kernel));
	    break;
	}
	
	if (pim_mfc.source_addr().is_ipv6()) {
	    XrlMfeaV0p1Client::send_delete_mfc6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		pim_mfc.source_addr().get_ipv6(),
		pim_mfc.group_addr().get_ipv6(),
		callback(this, &XrlPimNode::xrl_result_delete_mfc_from_kernel));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_delete_mfc_from_kernel(const XrlError& xrl_error,
					      const bool *fail,
					      const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure delete MFC with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

int
XrlPimNode::add_dataflow_monitor(const IPvX& source_addr,
				 const IPvX& group_addr,
				 uint32_t threshold_interval_sec,
				 uint32_t threshold_interval_usec,
				 uint32_t threshold_packets,
				 uint32_t threshold_bytes,
				 bool is_threshold_in_packets,
				 bool is_threshold_in_bytes,
				 bool is_geq_upcall,
				 bool is_leq_upcall)
{
    do {
	if (source_addr.is_ipv4()) {
	    XrlMfeaV0p1Client::send_add_dataflow_monitor4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		threshold_interval_sec,
		threshold_interval_usec,
		threshold_packets,
		threshold_bytes,
		is_threshold_in_packets,
		is_threshold_in_bytes,
		is_geq_upcall,
		is_leq_upcall,
		callback(this, &XrlPimNode::xrl_result_add_dataflow_monitor));
	    break;
	}
	
	if (source_addr.is_ipv6()) {
	    XrlMfeaV0p1Client::send_add_dataflow_monitor6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
		threshold_interval_sec,
		threshold_interval_usec,
		threshold_packets,
		threshold_bytes,
		is_threshold_in_packets,
		is_threshold_in_bytes,
		is_geq_upcall,
		is_leq_upcall,
		callback(this, &XrlPimNode::xrl_result_add_dataflow_monitor));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

int
XrlPimNode::delete_dataflow_monitor(const IPvX& source_addr,
				    const IPvX& group_addr,
				    uint32_t threshold_interval_sec,
				    uint32_t threshold_interval_usec,
				    uint32_t threshold_packets,
				    uint32_t threshold_bytes,
				    bool is_threshold_in_packets,
				    bool is_threshold_in_bytes,
				    bool is_geq_upcall,
				    bool is_leq_upcall)
{
    do {
	if (source_addr.is_ipv4()) {
	    XrlMfeaV0p1Client::send_delete_dataflow_monitor4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		threshold_interval_sec,
		threshold_interval_usec,
		threshold_packets,
		threshold_bytes,
		is_threshold_in_packets,
		is_threshold_in_bytes,
		is_geq_upcall,
		is_leq_upcall,
		callback(this, &XrlPimNode::xrl_result_delete_dataflow_monitor));
	    break;
	}
	
	if (source_addr.is_ipv6()) {
	    XrlMfeaV0p1Client::send_delete_dataflow_monitor6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
		threshold_interval_sec,
		threshold_interval_usec,
		threshold_packets,
		threshold_bytes,
		is_threshold_in_packets,
		is_threshold_in_bytes,
		is_geq_upcall,
		is_leq_upcall,
		callback(this, &XrlPimNode::xrl_result_delete_dataflow_monitor));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

int
XrlPimNode::delete_all_dataflow_monitor(const IPvX& source_addr,
					const IPvX& group_addr)
{
    do {
	if (source_addr.is_ipv4()) {
	    XrlMfeaV0p1Client::send_delete_all_dataflow_monitor4(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_delete_all_dataflow_monitor));
	    break;
	}
	
	if (source_addr.is_ipv6()) {
	    XrlMfeaV0p1Client::send_delete_all_dataflow_monitor6(
		x_module_name(family(), X_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
		callback(this, &XrlPimNode::xrl_result_delete_all_dataflow_monitor));
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_add_dataflow_monitor(const XrlError& xrl_error,
					    const bool *fail,
					    const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure add dataflow monitor with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlPimNode::xrl_result_delete_dataflow_monitor(const XrlError& xrl_error,
					       const bool *fail,
					       const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure delete dataflow monitor with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlPimNode::xrl_result_delete_all_dataflow_monitor(const XrlError& xrl_error,
						   const bool *fail,
						   const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure delete all dataflow monitor with the MFEA: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

//
// Protocol node CLI methods
//
int
XrlPimNode::add_cli_command_to_cli_manager(const char *command_name,
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
	callback(this, &XrlPimNode::xrl_result_add_cli_command));
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_add_cli_command(const XrlError& xrl_error,
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
XrlPimNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    XrlCliManagerV0p1Client::send_delete_cli_command(
	x_module_name(family(), X_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlPimNode::xrl_result_delete_cli_command));
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_delete_cli_command(const XrlError& xrl_error,
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
XrlPimNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = my_xrl_target_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::common_0_1_get_version(
    // Output values, 
    string&		version)
{
    version = XORP_MODULE_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::cli_processor_0_1_process_command(
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
    PimNodeCli::cli_process_command(processor_name,
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
XrlPimNode::mfea_client_0_1_new_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (PimNode::add_vif(vif_name.c_str(), vif_index) != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (PimNode::delete_vif(vif_name.c_str()) != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_add_vif_addr4(
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
    if (PimNode::add_vif_addr(vif_name.c_str(),
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
XrlPimNode::mfea_client_0_1_add_vif_addr6(
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
    if (PimNode::add_vif_addr(vif_name.c_str(),
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
XrlPimNode::mfea_client_0_1_delete_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv4&		addr, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (PimNode::delete_vif_addr(vif_name.c_str(),
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
XrlPimNode::mfea_client_0_1_delete_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv6&		addr, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    if (PimNode::delete_vif_addr(vif_name.c_str(),
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
XrlPimNode::mfea_client_0_1_set_vif_flags(
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
    if (PimNode::set_vif_flags(vif_name.c_str(),
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
XrlPimNode::mfea_client_0_1_set_vif_done(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    // Output values, 
    bool&		fail,
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
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
XrlPimNode::mfea_client_0_1_set_all_vifs_done(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    PimNode::set_vif_setup_completed(true);
    
    //
    // XXX: the origin of this XRL doesn't care what we can do with the vifs
    //
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_is_vif_setup_completed(
    // Output values, 
    bool&	is_completed)
{
    is_completed = PimNode::is_vif_setup_completed();
    
    return XrlCmdError::OKAY();
}

int
XrlPimNode::add_protocol_mld6igmp(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);		// Unknown vif
    
    //
    // Register the protocol with the MLD6IGMP for membership
    // change on this interface.
    //
    if (family() == AF_INET) {
	XrlMld6igmpV0p1Client::send_add_protocol4(
	    x_module_name(family(), X_MODULE_MLD6IGMP),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mld6igmp));
    } else {
	XrlMld6igmpV0p1Client::send_add_protocol6(
	    x_module_name(family(), X_MODULE_MLD6IGMP),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mld6igmp));
    }
    
    return (XORP_OK);
}

int
XrlPimNode::delete_protocol_mld6igmp(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);		// Unknown vif
    
    //
    // Deregister the protocol with the MLD6IGMP for membership
    // change on this interface.
    //
    if (family() == AF_INET) {
	XrlMld6igmpV0p1Client::send_delete_protocol4(
	    x_module_name(family(), X_MODULE_MLD6IGMP),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    callback(this, &XrlPimNode::xrl_result_delete_protocol_mld6igmp));
    } else {
	XrlMld6igmpV0p1Client::send_delete_protocol6(
	    x_module_name(family(), X_MODULE_MLD6IGMP),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    callback(this, &XrlPimNode::xrl_result_delete_protocol_mld6igmp));
    }
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_add_protocol_mld6igmp(const XrlError& xrl_error,
					     const bool *fail,
					     const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to add a protocol to MLD6IGMP: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

void
XrlPimNode::xrl_result_delete_protocol_mld6igmp(const XrlError& xrl_error,
						const bool *fail,
						const string *reason)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("XRL error: %s", xrl_error.str().c_str());
	return;
    }
    
    if (fail && *fail) {
	XLOG_ERROR("Failure to delete a protocol with MLD6IGMP: %s",
		   reason? reason->c_str(): "unknown reason");
    }
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_protocol_message4(
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
    PimNode::proto_recv(xrl_sender_name,
			src_module_id,
			vif_index,
			IPvX(source_address),
			IPvX(dest_address),
			ip_ttl,
			ip_tos,
			is_router_alert,
			(const uint8_t *)(protocol_message.begin()),
			protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_protocol_message6(
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
    PimNode::proto_recv(xrl_sender_name,
			src_module_id,
			vif_index,
			IPvX(source_address),
			IPvX(dest_address),
			ip_ttl,
			ip_tos,
			is_router_alert,
			(const uint8_t *)protocol_message.begin(),
			protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_kernel_signal_message4(
    // Input values, 
    const string&		xrl_sender_name, 
    const string&		, // protocol_name, 
    const uint32_t&		protocol_id, 
    const uint32_t&		message_type, 
    const string&		, // vif_name, 
    const uint32_t&		vif_index, 
    const IPv4&			source_address, 
    const IPv4&			dest_address, 
    const vector<uint8_t>&	protocol_message, 
    // Output values, 
    bool&			fail, 
    string&			reason)
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
    // Receive the kernel signal message
    //
    PimNode::signal_message_recv(xrl_sender_name,
				 src_module_id,
				 message_type,
				 vif_index,
				 IPvX(source_address),
				 IPvX(dest_address),
				 (const uint8_t *)(protocol_message.begin()),
				 protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_kernel_signal_message6(
    // Input values, 
    const string&		xrl_sender_name, 
    const string&		, // protocol_name, 
    const uint32_t&		protocol_id, 
    const uint32_t&		message_type, 
    const string&		, //  vif_name, 
    const uint32_t&		vif_index, 
    const IPv6&			source_address, 
    const IPv6&			dest_address, 
    const vector<uint8_t>&	protocol_message, 
    // Output values, 
    bool&			fail, 
    string&			reason)
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
    // Receive the kernel signal message
    //
    PimNode::signal_message_recv(xrl_sender_name,
				 src_module_id,
				 message_type,
				 vif_index,
				 IPvX(source_address),
				 IPvX(dest_address),
				 (const uint8_t *)(protocol_message.begin()),
				 protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_add_mrib4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4Net&	dest_prefix, 
    const IPv4&		next_hop_router_addr, 
    const string&	, // next_hop_vif_name, 
    const uint32_t&	next_hop_vif_index, 
    const uint32_t&	metric, 
    const uint32_t&	metric_preference, 
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
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    mrib.set_next_hop_router_addr(IPvX(next_hop_router_addr));
    mrib.set_next_hop_vif_index(next_hop_vif_index);
    mrib.set_metric(metric);
    mrib.set_metric_preference(metric_preference);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(mrib);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_add_mrib6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6Net&	dest_prefix, 
    const IPv6&		next_hop_router_addr, 
    const string&	, // next_hop_vif_name, 
    const uint32_t&	next_hop_vif_index, 
    const uint32_t&	metric, 
    const uint32_t&	metric_preference, 
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
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    mrib.set_next_hop_router_addr(IPvX(next_hop_router_addr));
    mrib.set_next_hop_vif_index(next_hop_vif_index);
    mrib.set_metric(metric);
    mrib.set_metric_preference(metric_preference);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(mrib);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_mrib4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4Net&	dest_prefix, 
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
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(mrib);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_mrib6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6Net&	dest_prefix, 
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
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(mrib);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_set_mrib_done(
    // Input values, 
    const string&	, // xrl_sender_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Commit all pending Mrib transactions
    //
    PimNode::pim_mrib_table().commit_pending_transactions();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_dataflow_signal4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	measured_interval_sec, 
    const uint32_t&	measured_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const uint32_t&	measured_packets, 
    const uint32_t&	measured_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Deliver a signal that a dataflow-related pre-condition is true.
    //
    PimNode::pim_mrt().signal_dataflow_recv(
	IPvX(source_address),
	IPvX(group_address),
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
	is_leq_upcall);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_dataflow_signal6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	measured_interval_sec, 
    const uint32_t&	measured_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const uint32_t&	measured_packets, 
    const uint32_t&	measured_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    //
    // Deliver a signal that a dataflow-related pre-condition is true.
    //
    PimNode::pim_mrt().signal_dataflow_recv(
	IPvX(source_address),
	IPvX(group_address),
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
	is_leq_upcall);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group, 
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
    
    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group, 
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
    
    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group, 
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
    
    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
    
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group, 
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
    
    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";	// TODO: return the xlog message?
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	reason = c_format("Cannot enable vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	pim_vif->enable();
	fail = false;
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	reason = c_format("Cannot disable vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	pim_vif->disable();
	fail = false;
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	reason = c_format("Cannot start vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	if (pim_vif->start() != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_vif(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	reason = c_format("Cannot stop vif %s: "
			  "no such vif", vif_name.c_str());
	XLOG_ERROR("%s", reason.c_str());
	fail = true;
    } else {
	if (pim_vif->stop() != XORP_OK) {
	    fail = true;
	} else {
	    fail = false;
	}
	reason = "";
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimNode::enable_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimNode::disable_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimNode::start_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_all_vifs(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    PimNode::stop_all_vifs();
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_pim(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (enable_pim() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_pim(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (disable_pim() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_cli(
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
XrlPimNode::pim_0_1_disable_cli(
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
XrlPimNode::pim_0_1_start_pim(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (start_pim() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_pim(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (stop_pim() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_cli(
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
XrlPimNode::pim_0_1_stop_cli(
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
XrlPimNode::pim_0_1_enable_bsr(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (enable_bsr() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_bsr(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (disable_bsr() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_bsr(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (start_bsr() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_bsr(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (stop_bsr() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_vif_name4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	admin_scope_zone_id, 
    const string&	vif_name, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_bsr_by_vif_name(is_admin_scope_zone,
						 IPvXNet(admin_scope_zone_id),
						 vif_name,
						 bsr_priority,
						 hash_masklen) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_vif_name6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	admin_scope_zone_id, 
    const string&	vif_name, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_bsr_by_vif_name(is_admin_scope_zone,
						 IPvXNet(admin_scope_zone_id),
						 vif_name,
						 bsr_priority,
						 hash_masklen) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_addr4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	admin_scope_zone_id, 
    const IPv4&		cand_bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_bsr_by_addr(is_admin_scope_zone,
					     IPvXNet(admin_scope_zone_id),
					     IPvX(cand_bsr_addr),
					     bsr_priority,
					     hash_masklen) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_addr6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	admin_scope_zone_id, 
    const IPv6&		cand_bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_bsr_by_addr(is_admin_scope_zone,
					     IPvXNet(admin_scope_zone_id),
					     IPvX(cand_bsr_addr),
					     bsr_priority,
					     hash_masklen) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_bsr4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	admin_scope_zone_id, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_cand_bsr(is_admin_scope_zone,
					IPvXNet(admin_scope_zone_id)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_bsr6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	admin_scope_zone_id, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_cand_bsr(is_admin_scope_zone,
					IPvXNet(admin_scope_zone_id)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_vif_name4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	group_prefix, 
    const string&	vif_name, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_rp_by_vif_name(is_admin_scope_zone,
						IPvXNet(group_prefix),
						vif_name,
						rp_priority,
						rp_holdtime) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_vif_name6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	group_prefix, 
    const string&	vif_name, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_rp_by_vif_name(is_admin_scope_zone,
						IPvXNet(group_prefix),
						vif_name,
						rp_priority,
						rp_holdtime) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_addr4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	group_prefix, 
    const IPv4&		cand_rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_rp_by_addr(is_admin_scope_zone,
					    IPvXNet(group_prefix),
					    IPvX(cand_rp_addr),
					    rp_priority,
					    rp_holdtime) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_addr6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	group_prefix, 
    const IPv6&		cand_rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_cand_rp_by_addr(is_admin_scope_zone,
					    IPvXNet(group_prefix),
					    IPvX(cand_rp_addr),
					    rp_priority,
					    rp_holdtime) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_vif_name4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	group_prefix, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_cand_rp_by_vif_name(is_admin_scope_zone,
						   IPvXNet(group_prefix),
						   vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_vif_name6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	group_prefix, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_cand_rp_by_vif_name(is_admin_scope_zone,
						   IPvXNet(group_prefix),
						   vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_addr4(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv4Net&	group_prefix, 
    const IPv4&		cand_rp_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_cand_rp_by_addr(is_admin_scope_zone,
					       IPvXNet(group_prefix),
					       IPvX(cand_rp_addr)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_addr6(
    // Input values, 
    const bool&		is_admin_scope_zone, 
    const IPv6Net&	group_prefix, 
    const IPv6&		cand_rp_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_cand_rp_by_addr(is_admin_scope_zone,
					       IPvXNet(group_prefix),
					       IPvX(cand_rp_addr)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_masklen, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::add_config_rp(IPvXNet(group_prefix),
			       IPvX(rp_addr),
			       rp_priority,
			       hash_masklen) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_masklen, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (PimNode::add_config_rp(IPvXNet(group_prefix),
			       IPvX(rp_addr),
			       rp_priority,
			       hash_masklen) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_rp(IPvXNet(group_prefix),
				  IPvX(rp_addr)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::delete_config_rp(IPvXNet(group_prefix),
				  IPvX(rp_addr)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_config_rp_done(
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::config_rp_done() < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		proto_version, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	proto_version = pim_vif->proto_version();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	proto_version, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::set_vif_proto_version(vif_name, proto_version) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_proto_version(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_triggered_delay, 
	bool&		fail, 
	string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	hello_triggered_delay = pim_vif->hello_triggered_delay().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_triggered_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_triggered_delay, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((hello_triggered_delay > (uint16_t)~0)
	|| (PimNode::set_vif_hello_triggered_delay(vif_name, hello_triggered_delay) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_triggered_delay(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_hello_triggered_delay(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		hello_period, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	hello_period = pim_vif->hello_period().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_period, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((hello_period > (uint16_t)~0)
	|| (PimNode::set_vif_hello_period(vif_name, hello_period) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_hello_period(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		hello_holdtime, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	hello_holdtime = pim_vif->hello_holdtime().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_holdtime, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((hello_holdtime > (uint16_t)~0)
	|| (PimNode::set_vif_hello_holdtime(vif_name, hello_holdtime) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_hello_holdtime(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		dr_priority, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	dr_priority = pim_vif->dr_priority().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	dr_priority, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((dr_priority > (uint32_t)~0)
	|| (PimNode::set_vif_dr_priority(vif_name, dr_priority) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_dr_priority(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		lan_delay, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	lan_delay = pim_vif->lan_delay().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	lan_delay, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((lan_delay > (uint16_t)~0)
	|| (PimNode::set_vif_lan_delay(vif_name, lan_delay) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_lan_delay(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		override_interval, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	override_interval = pim_vif->override_interval().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	override_interval, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((override_interval > (uint16_t)~0)
	|| (PimNode::set_vif_override_interval(vif_name, override_interval) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_override_interval(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		is_tracking_support_disabled, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	is_tracking_support_disabled = pim_vif->is_tracking_support_disabled().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    const bool&		is_tracking_support_disabled, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::set_vif_is_tracking_support_disabled(vif_name,
						      is_tracking_support_disabled)
	< 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_is_tracking_support_disabled(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		accept_nohello_neighbors, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	accept_nohello_neighbors = pim_vif->accept_nohello_neighbors().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    const bool&		accept_nohello_neighbors, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::set_vif_accept_nohello_neighbors(vif_name,
						  accept_nohello_neighbors)
	< 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_accept_nohello_neighbors(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		join_prune_period, 
    bool&		fail, 
    string&		reason)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	fail = true;
    } else {
	join_prune_period = pim_vif->join_prune_period().get();
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	join_prune_period, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if ((join_prune_period > (uint16_t)~0)
	|| (PimNode::set_vif_join_prune_period(vif_name, join_prune_period) < 0)) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::reset_vif_join_prune_period(vif_name) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_jp_entry4(
    // Input values, 
    const IPv4&		source_addr, 
    const IPv4&		group_addr, 
    const uint32_t&	group_masklen, 
    const string&	mrt_entry_type, 
    const string&	action_jp, 
    const uint32_t&	holdtime, 
    const bool&		new_group_bool, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    mrt_entry_type_t entry_type = MRT_ENTRY_UNKNOWN;
    action_jp_t action_type;
    
    //
    // Find the entry type
    //
    do {
	if (mrt_entry_type == "SG") {
	    entry_type = MRT_ENTRY_SG;
	    break;
	}
	if (mrt_entry_type == "SG_RPT") {
	    entry_type = MRT_ENTRY_SG_RPT;
	    break;
	}
	if (mrt_entry_type == "WC") {
	    entry_type = MRT_ENTRY_WC;
	    break;
	}
	if (mrt_entry_type == "RP") {
	    entry_type = MRT_ENTRY_RP;
	    break;
	}
	// Invalid entry
	fail = true;
	reason = "Invalid entry type";
	return XrlCmdError::OKAY();
    } while (false);
    
    //
    // Find the action type
    //
    do {
	if (action_jp == "JOIN") {
	    action_type = ACTION_JOIN;
	    break;
	}
	if (action_jp == "PRUNE") {
	    action_type = ACTION_PRUNE;
	    break;
	}
	// Invalid action
	fail = true;
	reason = "Invalid action";
	return XrlCmdError::OKAY();
    } while (false);
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   group_masklen, entry_type, action_type,
				   holdtime, new_group_bool) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_jp_entry6(
    // Input values, 
    const IPv6&		source_addr, 
    const IPv6&		group_addr, 
    const uint32_t&	group_masklen, 
    const string&	mrt_entry_type, 
    const string&	action_jp, 
    const uint32_t&	holdtime, 
    const bool&		new_group_bool, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    mrt_entry_type_t entry_type = MRT_ENTRY_UNKNOWN;
    action_jp_t action_type;
    
    //
    // Find the entry type
    //
    do {
	if (mrt_entry_type == "SG") {
	    entry_type = MRT_ENTRY_SG;
	    break;
	}
	if (mrt_entry_type == "SG_RPT") {
	    entry_type = MRT_ENTRY_SG_RPT;
	    break;
	}
	if (mrt_entry_type == "WC") {
	    entry_type = MRT_ENTRY_WC;
	    break;
	}
	if (mrt_entry_type == "RP") {
	    entry_type = MRT_ENTRY_RP;
	    break;
	}
	// Invalid entry
	fail = true;
	reason = "Invalid entry type";
	return XrlCmdError::OKAY();
    } while (false);
    
    //
    // Find the action type
    //
    do {
	if (action_jp == "JOIN") {
	    action_type = ACTION_JOIN;
	    break;
	}
	if (action_jp == "PRUNE") {
	    action_type = ACTION_PRUNE;
	    break;
	}
	// Invalid action
	fail = true;
	reason = "Invalid action";
	return XrlCmdError::OKAY();
    } while (false);
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   group_masklen, entry_type, action_type,
				   holdtime, new_group_bool) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry4(
    // Input values, 
    const IPv4&		nbr_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::send_test_jp_entry(IPvX(nbr_addr)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry6(
    // Input values, 
    const IPv6&		nbr_addr, 
    // Output values, 
    bool&		fail, 
    string&		reason)
{
    if (PimNode::send_test_jp_entry(IPvX(nbr_addr)) < 0) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_log_trace(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    PimNode::set_log_trace(true);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_log_trace(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    PimNode::set_log_trace(false);
    
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}
