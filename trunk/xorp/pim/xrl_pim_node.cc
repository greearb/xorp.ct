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

#ident "$XORP: xorp/pim/xrl_pim_node.cc,v 1.13 2003/03/25 06:55:08 pavlin Exp $"

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
XrlPimNode::xrl_result_add_protocol_mfea(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add a protocol to MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlPimNode::xrl_result_allow_signal_messages(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send allow_signal_messages() to MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlPimNode::xrl_result_allow_mrib_messages(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send allow_mrib_messages() to MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlPimNode::xrl_result_delete_protocol_mfea(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete a protocol with MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

//
// Protocol node methods
//

/**
 * XrlPimNode::proto_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the destination of the message.
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
		       xorp_module_id	, // dst_module_id,
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
XrlPimNode::xrl_result_send_protocol_message(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send a protocol message: %s",
		   xrl_error.str().c_str());
	return;
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
	    xorp_module_name(family(), XORP_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mfea));
    } else {
	XrlMfeaV0p1Client::send_add_protocol6(
	    xorp_module_name(family(), XORP_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mfea));
    }
    
    //
    // Enable the receiving of kernel signal messages
    //
    XrlMfeaV0p1Client::send_allow_signal_messages(
	xorp_module_name(family(), XORP_MODULE_MFEA),
	my_xrl_target_name(),
	string(PimNode::module_name()),
	PimNode::module_id(),
	true,			// XXX: enable
	callback(this, &XrlPimNode::xrl_result_allow_signal_messages));
    
    //
    // Enable receiving of MRIB information
    //
    XrlMfeaV0p1Client::send_allow_mrib_messages(
	xorp_module_name(family(), XORP_MODULE_MFEA),
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
	    xorp_module_name(family(), XORP_MODULE_MFEA),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    callback(this, &XrlPimNode::xrl_result_delete_protocol_mfea));
    } else {
	XrlMfeaV0p1Client::send_delete_protocol6(
	    xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_start_protocol_kernel_vif(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to start a kernel vif with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_stop_protocol_kernel_vif(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to stop a kernel vif with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_join_multicast_group(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to join a multicast group with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_leave_multicast_group(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to leave a multicast group with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_add_mfc_to_kernel(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add MFC with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

int
XrlPimNode::delete_mfc_from_kernel(const PimMfc& pim_mfc)
{
    do {
	if (pim_mfc.source_addr().is_ipv4()) {
	    XrlMfeaV0p1Client::send_delete_mfc4(
		xorp_module_name(family(), XORP_MODULE_MFEA),
		my_xrl_target_name(),
		pim_mfc.source_addr().get_ipv4(),
		pim_mfc.group_addr().get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_delete_mfc_from_kernel));
	    break;
	}
	
	if (pim_mfc.source_addr().is_ipv6()) {
	    XrlMfeaV0p1Client::send_delete_mfc6(
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_delete_mfc_from_kernel(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete MFC with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
		xorp_module_name(family(), XORP_MODULE_MFEA),
		my_xrl_target_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		callback(this, &XrlPimNode::xrl_result_delete_all_dataflow_monitor));
	    break;
	}
	
	if (source_addr.is_ipv6()) {
	    XrlMfeaV0p1Client::send_delete_all_dataflow_monitor6(
		xorp_module_name(family(), XORP_MODULE_MFEA),
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
XrlPimNode::xrl_result_add_dataflow_monitor(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add dataflow monitor with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlPimNode::xrl_result_delete_dataflow_monitor(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete dataflow monitor with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlPimNode::xrl_result_delete_all_dataflow_monitor(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete all dataflow monitor with the MFEA: %s",
		   xrl_error.str().c_str());
	return;
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
	xorp_module_name(family(), XORP_MODULE_CLI),
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
XrlPimNode::xrl_result_add_cli_command(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add a command to CLI manager: %s",
		   xrl_error.str().c_str());
	return;
    }
}

int
XrlPimNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    XrlCliManagerV0p1Client::send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlPimNode::xrl_result_delete_cli_command));
    
    return (XORP_OK);
}

void
XrlPimNode::xrl_result_delete_cli_command(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
	return;
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
    const uint32_t&	vif_index)
{
    if (PimNode::add_vif(vif_name.c_str(), vif_index) != XORP_OK) {
	string msg = c_format("Failed to add vif %s with vif_index = %d",
			      vif_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    if (PimNode::delete_vif(vif_name.c_str()) != XORP_OK) {
	string msg = c_format("Failed to delete vif %s with vif_index = %d",
			      vif_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
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
    const IPv4&		peer)
{
    if (PimNode::add_vif_addr(vif_name.c_str(),
			      IPvX(addr),
			      IPvXNet(subnet),
			      IPvX(broadcast),
			      IPvX(peer))
	!= XORP_OK) {
	string msg = c_format("Failed to add address %s to vif %s",
			      cstring(addr), vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
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
    const IPv6&		peer)
{
    if (PimNode::add_vif_addr(vif_name.c_str(),
			      IPvX(addr),
			      IPvXNet(subnet),
			      IPvX(broadcast),
			      IPvX(peer))
	!= XORP_OK) {
	string msg = c_format("Failed to add address %s to vif %s",
			      cstring(addr), vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv4&		addr)
{
    if (PimNode::delete_vif_addr(vif_name.c_str(),
				      IPvX(addr))
	!= XORP_OK) {
	string msg = c_format("Failed to delete address %s from vif %s",
			      cstring(addr), vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	, // vif_index, 
    const IPv6&		addr)
{
    if (PimNode::delete_vif_addr(vif_name.c_str(),
				 IPvX(addr))
	!= XORP_OK) {
	string msg = c_format("Failed to delete address %s from vif %s",
			      cstring(addr), vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
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
    const bool&		is_up) 
{
    if (PimNode::set_vif_flags(vif_name.c_str(),
			       is_pim_register,
			       is_p2p,
			       is_loopback,
			       is_multicast,
			       is_broadcast,
			       is_up)
	!= XORP_OK) {
	string msg = c_format("Failed to set flags for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_set_vif_done(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	vif_index) 
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to complete setup for vif %s "
			      "with vif_index = %d: "
			      "no such vif",
			      vif_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_set_all_vifs_done()
{
    PimNode::set_vif_setup_completed(true);
    
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
	    xorp_module_name(family(), XORP_MODULE_MLD6IGMP),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    callback(this, &XrlPimNode::xrl_result_add_protocol_mld6igmp));
    } else {
	XrlMld6igmpV0p1Client::send_add_protocol6(
	    xorp_module_name(family(), XORP_MODULE_MLD6IGMP),
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
	    xorp_module_name(family(), XORP_MODULE_MLD6IGMP),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    callback(this, &XrlPimNode::xrl_result_delete_protocol_mld6igmp));
    } else {
	XrlMld6igmpV0p1Client::send_delete_protocol6(
	    xorp_module_name(family(), XORP_MODULE_MLD6IGMP),
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
XrlPimNode::xrl_result_add_protocol_mld6igmp(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add a protocol to MLD6IGMP: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlPimNode::xrl_result_delete_protocol_mld6igmp(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete a protocol with MLD6IGMP: %s",
		   xrl_error.str().c_str());
	return;
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
    const vector<uint8_t>& protocol_message)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(msg);
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
			&protocol_message[0],
			protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
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
    const vector<uint8_t>& protocol_message)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(msg);
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
			&protocol_message[0],
			protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
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
    const vector<uint8_t>&	protocol_message)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(msg);
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
				 &protocol_message[0],
				 protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
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
    const vector<uint8_t>&	protocol_message)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(msg);
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
				 &protocol_message[0],
				 protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
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
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    mrib.set_next_hop_router_addr(IPvX(next_hop_router_addr));
    mrib.set_next_hop_vif_index(next_hop_vif_index);
    mrib.set_metric_preference(metric_preference);
    mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(0, mrib);
    
    //
    // Success
    //
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
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    mrib.set_next_hop_router_addr(IPvX(next_hop_router_addr));
    mrib.set_next_hop_vif_index(next_hop_vif_index);
    mrib.set_metric_preference(metric_preference);
    mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(0, mrib);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_mrib4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4Net&	dest_prefix)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(0, mrib);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_mrib6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6Net&	dest_prefix)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(0, mrib);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_set_mrib_done(
    // Input values, 
    const string&	// xrl_sender_name, 
    )
{
    //
    // Commit all pending Mrib transactions
    //
    PimNode::pim_mrib_table().commit_pending_transactions(0);
    
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
    const bool&		is_leq_upcall)
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
    
    // XXX: we don't care if the signal delivery failed
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
    const bool&		is_leq_upcall)
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
    
    // XXX: we don't care if the signal delivery failed
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_start_transaction(
    // Output values, 
    uint32_t&	tid)
{
    if (_mrib_transaction_manager.start(tid) != true) {
	string msg = string("Resource limit on number of pending transactions hit");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_commit_transaction(
    // Input values, 
    const uint32_t&	tid)
{
    if (_mrib_transaction_manager.commit(tid) != true) {
	string msg = c_format("Cannot commit MRIB transaction for tid %u",
			      tid);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    PimNode::pim_mrib_table().commit_pending_transactions(tid);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_abort_transaction(
    // Input values, 
    const uint32_t&	tid)
{
    if (_mrib_transaction_manager.abort(tid) != true) {
	string msg = c_format("Cannot abort MRIB transaction for tid %u",
			      tid);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    PimNode::pim_mrib_table().abort_pending_transactions(tid);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_add_entry4(
    // Input values, 
    const uint32_t&	tid, 
    const IPv4Net&	dst, 
    const IPv4&		gateway, 
    const string&	/* ifname */, 
    const string&	vifname)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vifname);
    
    if (pim_vif == NULL) {
	string msg = c_format("Cannot add MRIB entry for vif %s: "
			      "no such vif",
			      vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // Verify the address family
    //
    do {
	bool is_invalid_family = false;
	
	if (family() != AF_INET)
	    is_invalid_family = true;
	
	if (is_invalid_family) {
	    // Invalid address family
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    mrib.set_next_hop_router_addr(IPvX(gateway));
    mrib.set_next_hop_vif_index(pim_vif->vif_index());
    // TODO: XXX: PAVPAVPAV: fix this!!
    // mrib.set_metric_preference(metric_preference);
    // mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(tid, mrib);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_add_entry6(
    // Input values, 
    const uint32_t&	tid, 
    const IPv6Net&	dst, 
    const IPv6&		gateway, 
    const string&	/* ifname */, 
    const string&	vifname)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vifname);
    
    if (pim_vif == NULL) {
	string msg = c_format("Cannot add MRIB entry for vif %s: "
			      "no such vif",
			      vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    mrib.set_next_hop_router_addr(IPvX(gateway));
    mrib.set_next_hop_vif_index(pim_vif->vif_index());
    // TODO: XXX: PAVPAVPAV: fix this!!
    // mrib.set_metric_preference(metric_preference);
    // mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(tid, mrib);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_delete_entry4(
    // Input values, 
    const uint32_t&	tid, 
    const IPv4Net&	dst)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(tid, mrib);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_delete_entry6(
    // Input values, 
    const uint32_t&	tid, 
    const IPv6Net&	dst)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(tid, mrib);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_delete_all_entries(
    // Input values, 
    const uint32_t&	/* tid */)
{
    PimNode::pim_mrib_table().clear();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_delete_all_entries4(
    // Input values, 
    const uint32_t&	/* tid */)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    PimNode::pim_mrib_table().clear();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_delete_all_entries6(
    // Input values, 
    const uint32_t&	/* tid */)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    PimNode::pim_mrib_table().clear();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_lookup_route4(
    // Input values, 
    const IPv4&		dst, 
    // Output values, 
    IPv4Net&		netmask, 
    IPv4&		gateway, 
    string&		ifname, 
    string&		vifname)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);

    //
    // Lookup
    //
    Mrib *mrib = PimNode::pim_mrib_table().find(IPvX(dst));
    if (mrib == NULL) {
	string msg = c_format("No routing entry for %s", cstring(dst));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // Find the vif
    //
    PimVif *pim_vif;
    pim_vif = PimNode::vif_find_by_vif_index(mrib->next_hop_vif_index());
    if (pim_vif == NULL) {
	string msg = c_format("Lookup error for %s: next-hop vif "
			      "has vif_index %d: "
			      "no such vif",
			      cstring(dst),
			      mrib->next_hop_vif_index());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // The return values
    //
    netmask = mrib->dest_prefix().get_ipv4Net();
    gateway = mrib->next_hop_router_addr().get_ipv4();
    ifname = pim_vif->ifname();
    vifname = pim_vif->name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_lookup_route6(
    // Input values, 
    const IPv6&		dst, 
    // Output values, 
    IPv6Net&		netmask, 
    IPv6&		gateway, 
    string&		ifname, 
    string&		vifname)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Lookup
    //
    Mrib *mrib = PimNode::pim_mrib_table().find(IPvX(dst));
    if (mrib == NULL) {
	string msg = c_format("No routing entry for %s", cstring(dst));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // Find the vif
    //
    PimVif *pim_vif;
    pim_vif = PimNode::vif_find_by_vif_index(mrib->next_hop_vif_index());
    if (pim_vif == NULL) {
	string msg = c_format("Lookup error for %s: next-hop vif "
			      "has vif_index %d: "
			      "no such vif",
			      cstring(dst),
			      mrib->next_hop_vif_index());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // The return values
    //
    netmask = mrib->dest_prefix().get_ipv6Net();
    gateway = mrib->next_hop_router_addr().get_ipv6();
    ifname = pim_vif->ifname();
    vifname = pim_vif->name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_lookup_entry4(
    // Input values, 
    const IPv4Net&	dst, 
    // Output values, 
    IPv4&		gateway, 
    string&		ifname, 
    string&		vifname)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Lookup
    //
    Mrib *mrib = PimNode::pim_mrib_table().find_exact(IPvXNet(dst));
    if (mrib == NULL) {
	string msg = c_format("No routing entry for %s", cstring(dst));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // Find the vif
    //
    PimVif *pim_vif;
    pim_vif = PimNode::vif_find_by_vif_index(mrib->next_hop_vif_index());
    if (pim_vif == NULL) {
	string msg = c_format("Lookup error for %s: next-hop vif "
			      "has vif_index %d: "
			      "no such vif",
			      cstring(dst),
			      mrib->next_hop_vif_index());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // The return values
    //
    gateway = mrib->next_hop_router_addr().get_ipv4();
    ifname = pim_vif->ifname();
    vifname = pim_vif->name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::fti_0_1_lookup_entry6(
    // Input values, 
    const IPv6Net&	dst, 
    // Output values, 
    IPv6&		gateway, 
    string&		ifname, 
    string&		vifname)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    //
    // Lookup
    //
    Mrib *mrib = PimNode::pim_mrib_table().find_exact(IPvXNet(dst));
    if (mrib == NULL) {
	string msg = c_format("No routing entry for %s", cstring(dst));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // Find the vif
    //
    PimVif *pim_vif;
    pim_vif = PimNode::vif_find_by_vif_index(mrib->next_hop_vif_index());
    if (pim_vif == NULL) {
	string msg = c_format("Lookup error for %s: next-hop vif "
			      "has vif_index %d: "
			      "no such vif",
			      cstring(dst),
			      mrib->next_hop_vif_index());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // The return values
    //
    gateway = mrib->next_hop_router_addr().get_ipv6();
    ifname = pim_vif->ifname();
    vifname = pim_vif->name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string msg = c_format("Failed to add membership for (%s, %s)",
			      cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string msg = c_format("Failed to add membership for (%s, %s)",
			      cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv4");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string msg = c_format("Failed to delete membership for (%s, %s)",
			      cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
    
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group)
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
	    string msg = string("Received protocol message with invalid "
				"address family: IPv6");
	    return XrlCmdError::COMMAND_FAILED(msg);
	}
    } while (false);
    
    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string msg = c_format("Failed to delete membership for (%s, %s)",
			      cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_vif(
    // Input values, 
    const string&	vif_name)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Cannot enable vif %s: no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    pim_vif->enable();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_vif(
    // Input values, 
    const string&	vif_name)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Cannot disable vif %s: no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    pim_vif->disable();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_vif(
    // Input values, 
    const string&	vif_name)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Cannot start vif %s: no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (pim_vif->start() != XORP_OK) {
	string msg = c_format("Failed to start vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_vif(
    // Input values, 
    const string&	vif_name)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Cannot stop vif %s: no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (pim_vif->stop() != XORP_OK) {
	string msg = c_format("Failed to stop vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_all_vifs()
{
    if (PimNode::enable_all_vifs() != XORP_OK) {
	string msg = c_format("Failed to enable all vifs");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_all_vifs()
{
    if (PimNode::disable_all_vifs() != XORP_OK) {
	string msg = c_format("Failed to disable all vifs");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_all_vifs()
{
    if (PimNode::start_all_vifs() < 0) {
	string msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_all_vifs()
{
    if (PimNode::stop_all_vifs() < 0) {
	string msg = c_format("Failed to stop all vifs");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_pim()
{
    if (enable_pim() != XORP_OK) {
	string msg = c_format("Failed to enable PIM");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_pim()
{
    if (disable_pim() != XORP_OK) {
	string msg = c_format("Failed to disable PIM");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_cli()
{
    if (enable_cli() != XORP_OK) {
	string msg = c_format("Failed to enable PIM CLI");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_cli()
{
    if (disable_cli() != XORP_OK) {
	string msg = c_format("Failed to disable PIM CLI");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_pim()
{
    if (start_pim() != XORP_OK) {
	string msg = c_format("Failed to start PIM");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_pim()
{
    if (stop_pim() != XORP_OK) {
	string msg = c_format("Failed to stop PIM");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_cli()
{
    if (start_cli() != XORP_OK) {
	string msg = c_format("Failed to start PIM CLI");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_cli()
{
    if (stop_cli() != XORP_OK) {
	string msg = c_format("Failed to stop PIM CLI");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_bsr()
{
    if (enable_bsr() != XORP_OK) {
	string msg = c_format("Failed to enable PIM BSR");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_bsr()
{
    if (disable_bsr() != XORP_OK) {
	string msg = c_format("Failed to disable PIM BSR");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_bsr()
{
    if (start_bsr() != XORP_OK) {
	string msg = c_format("Failed to start PIM BSR");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_bsr()
{
    if (stop_bsr() != XORP_OK) {
	string msg = c_format("Failed to stop PIM BSR");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_name4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const string&	vif_name)
{
    if (PimNode::add_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						   vif_name)
	< 0) {
	string msg = c_format("Failed to add scope zone %s on vif %s",
			      cstring(scope_zone_id),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const string&	vif_name)
{
    if (PimNode::add_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						   vif_name)
	< 0) {
	string msg = c_format("Failed to add scope zone %s on vif %s",
			      cstring(scope_zone_id),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const IPv4&		vif_addr)
{
    if (PimNode::add_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						   IPvX(vif_addr))
	< 0) {
	string msg = c_format("Failed to add scope zone %s on vif "
			      "with address %s",
			      cstring(scope_zone_id),
			      cstring(vif_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const IPv6&		vif_addr)
{
    if (PimNode::add_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						   IPvX(vif_addr))
	< 0) {
	string msg = c_format("Failed to add scope zone %s on vif "
			      "with address %s",
			      cstring(scope_zone_id),
			      cstring(vif_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_name4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const string&	vif_name)
{
    if (PimNode::delete_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						      vif_name)
	< 0) {
	string msg = c_format("Failed to delete scope zone %s on vif %s",
			      cstring(scope_zone_id),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const string&	vif_name)
{
    if (PimNode::delete_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						      vif_name)
	< 0) {
	string msg = c_format("Failed to delete scope zone %s on vif %s",
			      cstring(scope_zone_id),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const IPv4&		vif_addr)
{
    if (PimNode::delete_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						      IPvX(vif_addr))
	< 0) {
	string msg = c_format("Failed to delete scope zone %s on vif "
			      "with address %s",
			      cstring(scope_zone_id),
			      cstring(vif_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const IPv6&		vif_addr)
{
    if (PimNode::delete_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						      IPvX(vif_addr))
	< 0) {
	string msg = c_format("Failed to delete scope zone %s on vif "
			      "with address %s",
			      cstring(scope_zone_id),
			      cstring(vif_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_vif_name4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen)
{
    if (PimNode::add_config_cand_bsr_by_vif_name(IPvXNet(scope_zone_id),
						 is_scope_zone,
						 vif_name,
						 bsr_priority,
						 hash_masklen)
	< 0) {
	string msg = c_format("Failed to add cand-BSR for zone %s on vif %s",
			      cstring(PimScopeZoneId(scope_zone_id,
						     is_scope_zone)),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen)
{
    if (PimNode::add_config_cand_bsr_by_vif_name(IPvXNet(scope_zone_id),
						 is_scope_zone,
						 vif_name,
						 bsr_priority,
						 hash_masklen)
	< 0) {
	string msg = c_format("Failed to add cand-BSR for zone %s on vif %s",
			      cstring(PimScopeZoneId(scope_zone_id,
						     is_scope_zone)),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const IPv4&		cand_bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen)
{
    if (PimNode::add_config_cand_bsr_by_addr(IPvXNet(scope_zone_id),
					     is_scope_zone,
					     IPvX(cand_bsr_addr),
					     bsr_priority,
					     hash_masklen)
	< 0) {
	string msg = c_format("Failed to add cand-BSR for zone %s on vif "
			      "with address %s",
			      cstring(PimScopeZoneId(scope_zone_id,
						     is_scope_zone)),
			      cstring(cand_bsr_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const IPv6&		cand_bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen)
{
    if (PimNode::add_config_cand_bsr_by_addr(IPvXNet(scope_zone_id),
					     is_scope_zone,
					     IPvX(cand_bsr_addr),
					     bsr_priority,
					     hash_masklen)
	< 0) {
	string msg = c_format("Failed to add cand-BSR for zone %s on vif "
			      "with address %s",
			      cstring(PimScopeZoneId(scope_zone_id,
						     is_scope_zone)),
			      cstring(cand_bsr_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_bsr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone)
{
    if (PimNode::delete_config_cand_bsr(IPvXNet(scope_zone_id),
					is_scope_zone)
	< 0) {
	string msg = c_format("Failed to delete cand-BSR for zone %s",
			      cstring(PimScopeZoneId(scope_zone_id,
						     is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_bsr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone)
{
    if (PimNode::delete_config_cand_bsr(IPvXNet(scope_zone_id),
					is_scope_zone)
	< 0) {
	string msg = c_format("Failed to delete cand-BSR for zone %s",
			      cstring(PimScopeZoneId(scope_zone_id,
						     is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_vif_name4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (PimNode::add_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						is_scope_zone,
						vif_name,
						rp_priority,
						rp_holdtime)
	< 0) {
	string msg = c_format("Failed to add Cand-RP for prefix %s on vif %s",
			      cstring(group_prefix),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_vif_name6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (PimNode::add_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						is_scope_zone,
						vif_name,
						rp_priority,
						rp_holdtime)
	< 0) {
	string msg = c_format("Failed to add Cand-RP for prefix %s on vif %s",
			      cstring(group_prefix),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_addr4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv4&		cand_rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (PimNode::add_config_cand_rp_by_addr(IPvXNet(group_prefix),
					    is_scope_zone,
					    IPvX(cand_rp_addr),
					    rp_priority,
					    rp_holdtime)
	< 0) {
	string msg = c_format("Failed to add Cand-RP for prefix %s on vif "
			      "with address %s",
			      cstring(group_prefix),
			      cstring(cand_rp_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_addr6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv6&		cand_rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (PimNode::add_config_cand_rp_by_addr(IPvXNet(group_prefix),
					    is_scope_zone,
					    IPvX(cand_rp_addr),
					    rp_priority,
					    rp_holdtime)
	< 0) {
	string msg = c_format("Failed to add Cand-RP for prefix %s on vif "
			      "with address %s",
			      cstring(group_prefix),
			      cstring(cand_rp_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_vif_name4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name)
{
    if (PimNode::delete_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						   is_scope_zone,
						   vif_name)
	< 0) {
	string msg = c_format("Failed to delete Cand-RP for prefix %s on vif %s",
			      cstring(group_prefix),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_vif_name6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name)
{
    if (PimNode::delete_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						   is_scope_zone,
						   vif_name)
	< 0) {
	string msg = c_format("Failed to delete Cand-RP for prefix %s on vif %s",
			      cstring(group_prefix),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_addr4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv4&		cand_rp_addr)
{
    if (PimNode::delete_config_cand_rp_by_addr(IPvXNet(group_prefix),
					       is_scope_zone,
					       IPvX(cand_rp_addr))
	< 0) {
	string msg = c_format("Failed to delete Cand-RP for prefix %s on vif "
			      "with address %s",
			      cstring(group_prefix),
			      cstring(cand_rp_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_addr6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv6&		cand_rp_addr)
{
    if (PimNode::delete_config_cand_rp_by_addr(IPvXNet(group_prefix),
					       is_scope_zone,
					       IPvX(cand_rp_addr))
	< 0) {
	string msg = c_format("Failed to delete Cand-RP for prefix %s on vif "
			      "with address %s",
			      cstring(group_prefix),
			      cstring(cand_rp_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_masklen)
{
    if (PimNode::add_config_rp(IPvXNet(group_prefix),
			       IPvX(rp_addr),
			       rp_priority,
			       hash_masklen)
	< 0) {
	string msg = c_format("Failed to add %s to RP-Set for prefix %s",
			      cstring(rp_addr),
			      cstring(group_prefix));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_masklen)
{
    if (PimNode::add_config_rp(IPvXNet(group_prefix),
			       IPvX(rp_addr),
			       rp_priority,
			       hash_masklen)
	< 0) {
	string msg = c_format("Failed to add %s to RP-Set for prefix %s",
			      cstring(rp_addr),
			      cstring(group_prefix));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr)
{
    if (PimNode::delete_config_rp(IPvXNet(group_prefix),
				  IPvX(rp_addr))
	< 0) {
	string msg = c_format("Failed to delete %s from RP-Set for prefix %s",
			      cstring(rp_addr),
			      cstring(group_prefix));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr)
{
    if (PimNode::delete_config_rp(IPvXNet(group_prefix),
				  IPvX(rp_addr))
	< 0) {
	string msg = c_format("Failed to delete %s from RP-Set for prefix %s",
			      cstring(rp_addr),
			      cstring(group_prefix));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_config_rp_done()
{
    if (PimNode::config_rp_done() < 0) {
	string msg = string("Failed to complete the RP-Set configuration");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		proto_version)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get protocol version for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    proto_version = pim_vif->proto_version();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	proto_version)
{
    if (PimNode::set_vif_proto_version(vif_name, proto_version) < 0) {
	string msg = c_format("Failed to set protocol version for vif %s to %d",
			      vif_name.c_str(),
			      proto_version);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_proto_version(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_proto_version(vif_name) < 0) {
	string msg = c_format("Failed to reset protocol version for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_triggered_delay)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'Hello triggered delay' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    hello_triggered_delay = pim_vif->hello_triggered_delay().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_triggered_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_triggered_delay)
{
    if ((hello_triggered_delay > (uint16_t)~0)
	|| (PimNode::set_vif_hello_triggered_delay(vif_name,
						   hello_triggered_delay)
	    < 0)) {
	string msg = c_format("Failed to set 'Hello triggered delay' for vif %s to %d",
			      vif_name.c_str(),
			      hello_triggered_delay);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_triggered_delay(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_hello_triggered_delay(vif_name) < 0) {
	string msg = c_format("Failed to reset 'Hello triggered delay' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		hello_period)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'Hello period' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    hello_period = pim_vif->hello_period().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_period)
{
    if ((hello_period > (uint16_t)~0)
	|| (PimNode::set_vif_hello_period(vif_name, hello_period)
	    < 0)) {
	string msg = c_format("Failed to set 'Hello period' for vif %s to %d",
			      vif_name.c_str(),
			      hello_period);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_period(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_hello_period(vif_name) < 0) {
	string msg = c_format("Failed to reset 'Hello period' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		hello_holdtime)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'Hello holdtime' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    hello_holdtime = pim_vif->hello_holdtime().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_holdtime)
{
    if ((hello_holdtime > (uint16_t)~0)
	|| (PimNode::set_vif_hello_holdtime(vif_name, hello_holdtime)
	    < 0)) {
	string msg = c_format("Failed to set 'Hello holdtime' for vif %s to %d",
			      vif_name.c_str(),
			      hello_holdtime);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_hello_holdtime(vif_name) < 0) {
	string msg = c_format("Failed to reset 'Hello holdtime' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		dr_priority)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'DR priority' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    dr_priority = pim_vif->dr_priority().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	dr_priority)
{
    if ((dr_priority > (uint32_t)~0)
	|| (PimNode::set_vif_dr_priority(vif_name, dr_priority) < 0)) {
	string msg = c_format("Failed to set 'DR priority' for vif %s to %d",
			      vif_name.c_str(),
			      dr_priority);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_dr_priority(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_dr_priority(vif_name) < 0) {
	string msg = c_format("Failed to reset 'DR priority' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		lan_delay)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'LAN delay' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    lan_delay = pim_vif->lan_delay().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	lan_delay)
{
    if ((lan_delay > (uint16_t)~0)
	|| (PimNode::set_vif_lan_delay(vif_name, lan_delay) < 0)) {
	string msg = c_format("Failed to set 'LAN delay' for vif %s to %d",
			      vif_name.c_str(),
			      lan_delay);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_lan_delay(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_lan_delay(vif_name) < 0) {
	string msg = c_format("Failed to reset 'LAN delay' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		override_interval)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'override interval' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    override_interval = pim_vif->override_interval().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	override_interval)
{
    if ((override_interval > (uint16_t)~0)
	|| (PimNode::set_vif_override_interval(vif_name, override_interval)
	    < 0)) {
	string msg = c_format("Failed to set 'override interval' for vif %s to %d",
			      vif_name.c_str(),
			      override_interval);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_override_interval(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_override_interval(vif_name) < 0) {
	string msg = c_format("Failed to reset 'override interval' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		is_tracking_support_disabled)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'is_tracking_support_disabled' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    is_tracking_support_disabled = pim_vif->is_tracking_support_disabled().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    const bool&		is_tracking_support_disabled)
{
    if (PimNode::set_vif_is_tracking_support_disabled(vif_name,
						      is_tracking_support_disabled)
	< 0) {
	string msg = c_format("Failed to set 'is_tracking_support_disabled' for vif %s to %d",
			      vif_name.c_str(),
			      is_tracking_support_disabled);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_is_tracking_support_disabled(vif_name) < 0) {
	string msg = c_format("Failed to reset 'is_tracking_support_disabled' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		accept_nohello_neighbors)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'accept_nohello_neighbors' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    accept_nohello_neighbors = pim_vif->accept_nohello_neighbors().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    const bool&		accept_nohello_neighbors)
{
    if (PimNode::set_vif_accept_nohello_neighbors(vif_name,
						  accept_nohello_neighbors)
	< 0) {
	string msg = c_format("Failed to set 'accept_nohello_neighbors' for vif %s to %d",
			      vif_name.c_str(),
			      accept_nohello_neighbors);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_accept_nohello_neighbors(vif_name) < 0) {
	string msg = c_format("Failed to reset 'accept_nohello_neighbors' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		join_prune_period)
{
    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	string msg = c_format("Failed to get 'Join/Prune period' for vif %s: "
			      "no such vif",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    join_prune_period = pim_vif->join_prune_period().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	join_prune_period)
{
    if ((join_prune_period > (uint16_t)~0)
	|| (PimNode::set_vif_join_prune_period(vif_name, join_prune_period)
	    < 0)) {
	string msg = c_format("Failed to set 'Join/Prune period' for vif %s to %d",
			      vif_name.c_str(),
			      join_prune_period);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_join_prune_period(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::reset_vif_join_prune_period(vif_name) < 0) {
	string msg = c_format("Failed to reset 'Join/Prune period' for vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_switch_to_spt_threshold(
    // Output values, 
    bool&	is_enabled, 
    uint32_t&	interval_sec, 
    uint32_t&	bytes)
{
    is_enabled = PimNode::is_switch_to_spt_enabled().get();
    interval_sec = PimNode::switch_to_spt_threshold_interval_sec().get();
    bytes = PimNode::switch_to_spt_threshold_bytes().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_switch_to_spt_threshold(
    // Input values, 
    const bool&		is_enabled, 
    const uint32_t&	interval_sec, 
    const uint32_t&	bytes)
{
    PimNode::is_switch_to_spt_enabled().set(is_enabled);
    PimNode::switch_to_spt_threshold_interval_sec().set(interval_sec);
    PimNode::switch_to_spt_threshold_bytes().set(bytes);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_switch_to_spt_threshold()
{
    PimNode::is_switch_to_spt_enabled().reset();
    PimNode::switch_to_spt_threshold_interval_sec().reset();
    PimNode::switch_to_spt_threshold_bytes().reset();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_log_trace()
{
    PimNode::set_log_trace(true);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_disable_log_trace()
{
    PimNode::set_log_trace(false);
    
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
    const bool&		new_group_bool)
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
	string msg = c_format("Invalid entry type = %s",
			      mrt_entry_type.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
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
	string msg = c_format("Invalid action = %s",
			      action_jp.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    } while (false);
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   group_masklen, entry_type, action_type,
				   holdtime, new_group_bool)
	< 0) {
	string msg = c_format("Failed to add Join/Prune test entry for (%s, %s)",
			      cstring(source_addr),
			      cstring(group_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
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
    const bool&		new_group_bool)
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
	string msg = c_format("Invalid entry type = %s",
			      mrt_entry_type.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
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
	string msg = c_format("Invalid action = %s",
			      action_jp.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    } while (false);
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   group_masklen, entry_type, action_type,
				   holdtime, new_group_bool)
	< 0) {
	string msg = c_format("Failed to add Join/Prune test entry for (%s, %s)",
			      cstring(source_addr),
			      cstring(group_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry4(
    // Input values, 
    const IPv4&		nbr_addr)
{
    if (PimNode::send_test_jp_entry(IPvX(nbr_addr)) < 0) {
	string msg = c_format("Failed to send Join/Prune test message to %s",
			      cstring(nbr_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry6(
    // Input values, 
    const IPv6&		nbr_addr)
{
    if (PimNode::send_test_jp_entry(IPvX(nbr_addr)) < 0) {
	string msg = c_format("Failed to send Join/Prune test message to %s",
			      cstring(nbr_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_assert4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		source_addr, 
    const IPv4&		group_addr, 
    const bool&		rpt_bit, 
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
{
    if (PimNode::send_test_assert(vif_name,
				  IPvX(source_addr),
				  IPvX(group_addr),
				  rpt_bit,
				  metric_preference,
				  metric)
	< 0) {
	string msg = c_format("Failed to send Assert test message for (%s, %s) on vif %s",
			      cstring(source_addr),
			      cstring(group_addr),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_assert6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		source_addr, 
    const IPv6&		group_addr, 
    const bool&		rpt_bit, 
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
{
    if (PimNode::send_test_assert(vif_name,
				  IPvX(source_addr),
				  IPvX(group_addr),
				  rpt_bit,
				  metric_preference,
				  metric)
	< 0) {
	string msg = c_format("Failed to send Assert test message for (%s, %s) on vif %s",
			      cstring(source_addr),
			      cstring(group_addr),
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_zone4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4&		bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen, 
    const uint32_t&	fragment_tag)
{
    if (bsr_priority > 0xff) {
	string msg = c_format("Invalid BSR priority = %d",
			      bsr_priority);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (hash_masklen > 0xff) {
	string msg = c_format("Invalid hash masklen = %d",
			      hash_masklen);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (fragment_tag > 0xffff) {
	string msg = c_format("Invalid fragment tag = %d",
			      fragment_tag);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (PimNode::add_test_bsr_zone(PimScopeZoneId(zone_id_scope_zone_prefix,
						  zone_id_is_scope_zone),
				   IPvX(bsr_addr),
				   bsr_priority,
				   hash_masklen,
				   fragment_tag)
	< 0) {
	string msg = c_format("Failed to add BSR test zone %s with BSR address %s",
			      cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						     zone_id_is_scope_zone)),
			      cstring(bsr_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_zone6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6&		bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_masklen, 
    const uint32_t&	fragment_tag)
{
    if (bsr_priority > 0xff) {
	string msg = c_format("Invalid BSR priority = %d",
			      bsr_priority);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (hash_masklen > 0xff) {
	string msg = c_format("Invalid hash masklen = %d",
			      hash_masklen);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (fragment_tag > 0xffff) {
	string msg = c_format("Invalid fragment tag = %d",
			      fragment_tag);
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    if (PimNode::add_test_bsr_zone(PimScopeZoneId(zone_id_scope_zone_prefix,
						  zone_id_is_scope_zone),
				   IPvX(bsr_addr),
				   bsr_priority,
				   hash_masklen,
				   fragment_tag)
	< 0) {
	string msg = c_format("Failed to add BSR test zone %s with BSR address %s",
			      cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						     zone_id_is_scope_zone)),
			      cstring(bsr_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_group_prefix4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const uint32_t&	expected_rp_count)
{
    if (expected_rp_count > 0xff) {
	string msg = c_format("Invalid expected RP count = %d",
			      expected_rp_count);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (PimNode::add_test_bsr_group_prefix(
	PimScopeZoneId(zone_id_scope_zone_prefix,
		       zone_id_is_scope_zone),
	IPvXNet(group_prefix),
	is_scope_zone,
	expected_rp_count)
	< 0) {
	string msg = c_format("Failed to add group prefix %s for BSR test zone %s",
			      cstring(group_prefix),
			      cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						     zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_group_prefix6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const uint32_t&	expected_rp_count)
{
    if (expected_rp_count > 0xff) {
	string msg = c_format("Invalid expected RP count = %d",
			      expected_rp_count);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (PimNode::add_test_bsr_group_prefix(
	PimScopeZoneId(zone_id_scope_zone_prefix,
		       zone_id_is_scope_zone),
	IPvXNet(group_prefix),
	is_scope_zone,
	expected_rp_count)
	< 0) {
	string msg = c_format("Failed to add group prefix %s for BSR test zone %s",
			      cstring(group_prefix),
			      cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						     zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_rp4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (rp_priority > 0xff) {
	string msg = c_format("Invalid RP priority = %d",
			      rp_priority);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (rp_holdtime > 0xff) {
	string msg = c_format("Invalid RP holdtime = %d",
			      rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (PimNode::add_test_bsr_rp(PimScopeZoneId(zone_id_scope_zone_prefix,
						zone_id_is_scope_zone),
				 IPvXNet(group_prefix),
				 IPvX(rp_addr),
				 rp_priority,
				 rp_holdtime)
	< 0) {
	string msg = c_format("Failed to add test Cand-RP %s for group prefix %s for BSR zone %s",
			      cstring(rp_addr),
			      cstring(group_prefix),
			      cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						     zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_rp6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (rp_priority > 0xff) {
	string msg = c_format("Invalid RP priority = %d",
			      rp_priority);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (rp_holdtime > 0xff) {
	string msg = c_format("Invalid RP holdtime = %d",
			      rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    if (PimNode::add_test_bsr_rp(PimScopeZoneId(zone_id_scope_zone_prefix,
						zone_id_is_scope_zone),
				 IPvXNet(group_prefix),
				 IPvX(rp_addr),
				 rp_priority,
				 rp_holdtime)
	< 0) {
	string msg = c_format("Failed to add test Cand-RP %s for group prefix %s for BSR zone %s",
			      cstring(rp_addr),
			      cstring(group_prefix),
			      cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						     zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_bootstrap(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::send_test_bootstrap(vif_name) < 0) {
	string msg = c_format("Failed to send Bootstrap test message on vif %s",
			      vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_bootstrap_by_dest4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		dest_addr)
{
    if (PimNode::send_test_bootstrap_by_dest(vif_name, IPvX(dest_addr)) < 0) {
	string msg = c_format("Failed to send Bootstrap test message on vif %s "
			      "to address %s",
			      vif_name.c_str(),
			      cstring(dest_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_bootstrap_by_dest6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		dest_addr)
{
    if (PimNode::send_test_bootstrap_by_dest(vif_name, IPvX(dest_addr)) < 0) {
	string msg = c_format("Failed to send Bootstrap test message on vif %s "
			      "to address %s",
			      vif_name.c_str(),
			      cstring(dest_addr));
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_cand_rp_adv()
{
    if (PimNode::send_test_cand_rp_adv() < 0) {
	string msg = string("Failed to send Cand-RP-Adv test message");
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    return XrlCmdError::OKAY();
}
