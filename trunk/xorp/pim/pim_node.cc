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

#ident "$XORP: xorp/pim/pim_node.cc,v 1.9 2003/05/19 00:20:23 pavlin Exp $"


//
// Protocol Independent Multicast (both PIM-SM and PIM-DM)
// node implementation (common part).
// PIM-SMv2 (draft-ietf-pim-sm-new-*), PIM-DM (new draft pending).
//


#include "pim_module.h"
#include "pim_private.hh"
#include "mfea/mfea_unix_kernel_messages.hh"		// TODO: XXX: yuck!
#include "pim_mre.hh"
#include "pim_node.hh"
#include "pim_vif.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


/**
 * PimNode::PimNode:
 * @family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @module_id: The module ID (must be either %XORP_MODULE_PIMSM
 * or %XORP_MODULE_PIMDM).
 * TODO: XXX: XORP_MODULE_PIMDM is not implemented yet.
 * @eventloop: The event loop.
 * 
 * PIM node constructor.
 **/
PimNode::PimNode(int family, xorp_module_id module_id,
		 EventLoop& eventloop)
    : ProtoNode<PimVif>(family, module_id, eventloop),
    _pim_mrt(*this),
    _pim_mrib_table(*this),
    _pim_bsr(*this),
    _rp_table(*this),
    _pim_scope_zone_table(*this),
    _is_switch_to_spt_enabled(false),	// XXX: disabled by defailt
    _switch_to_spt_threshold_interval_sec(0),
    _switch_to_spt_threshold_bytes(0),
    _is_log_trace(true),		// XXX: default to print trace logs
    _test_jp_header(*this)
{
    // TODO: XXX: PIMDM not implemented yet
    XLOG_ASSERT(module_id == XORP_MODULE_PIMSM);
    XLOG_ASSERT((module_id == XORP_MODULE_PIMSM)
		|| (module_id == XORP_MODULE_PIMDM));
    if ((module_id != XORP_MODULE_PIMSM) && (module_id != XORP_MODULE_PIMDM)) {
	XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_PIMSM' = %d "
		   "or 'XORP_MODULE_PIMDM' = %d",
		   module_id, XORP_MODULE_PIMSM, XORP_MODULE_PIMDM);
    }
    
    _pim_register_vif_index = Vif::VIF_INDEX_INVALID;
    
    _buffer_recv = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
}

/**
 * PimNode::~PimNode:
 * @void: 
 * 
 * PIM node destructor.
 * 
 **/
PimNode::~PimNode(void)
{
    stop();
    
    //
    // XXX: the MRT, MRIB, etc tables, and the PimVifs will be automatically
    // "destructed".
    
    delete_all_vifs();
    
    BUFFER_FREE(_buffer_recv);
}

/**
 * PimNode::set_proto_version:
 * @proto_version: The protocol version to set.
 * 
 * Set protocol version.
 * 
 * Return value: %XORP_OK is @proto_version is valid, otherwise %XORP_ERROR.
 **/
int
PimNode::set_proto_version(int proto_version)
{
    if ((proto_version < PIM_VERSION_MIN) || (proto_version > PIM_VERSION_MAX))
	return (XORP_ERROR);
    
    ProtoUnit::set_proto_version(proto_version);
    
    return (XORP_OK);
}

/**
 * PimNode::start:
 * @void: 
 * 
 * Start the PIM protocol.
 * TODO: This function should not start the protocol operation on the
 * interfaces. The interfaces must be activated separately.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
PimNode::start(void)
{
    if (ProtoNode<PimVif>::start() < 0)
	return (XORP_ERROR);
    
    //
    // Start the protocol with the kernel
    //
    if (start_protocol_kernel() != XORP_OK) {
	XLOG_ERROR("Error starting protocol with the kernel");
	ProtoNode<PimVif>::stop();
	return (XORP_ERROR);
    }
    
    // Start the pim_vifs
    start_all_vifs();
    
    // TODO: start the BSR module only if we are enabled to run the
    // BSR mechanism.
    if (_pim_bsr.is_enabled()) {
	if (_pim_bsr.start() < 0)
	    return (XORP_ERROR);
    }
    
    //
    // Perform misc. PIM-specific start operations
    //
    // TODO: IMPLEMENT IT!
    
    return (XORP_OK);
}

/**
 * PimNode::stop:
 * @void: 
 * 
 * Gracefully stop the PIM protocol.
 * XXX: The graceful stop will attempt to send Join/Prune, Assert, etc.
 * messages for all multicast routing entries to gracefully clean-up
 * state with neighbors.
 * XXX: After the multicast routing entries cleanup is completed,
 * PimNode::final_stop() is called to complete the job.
 * XXX: If this method is called one-after-another, the second one
 * will force calling immediately PimNode::final_stop() to quickly
 * finish the job. TODO: is this a desired semantic?
 * XXX: This function, unlike start(), will stop the protocol
 * operation on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::stop(void)
{
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
    
    //
    // Perform misc. PIM-specific stop operations
    //
    // TODO: IMPLEMENT IT!
    
    _pim_bsr.stop();
    
    // Stop the vifs
    stop_all_vifs();
    
    if (ProtoNode<PimVif>::pending_stop() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * PimNode::final_stop:
 * @void: 
 * 
 * Completely stop the PIM protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::final_stop(void)
{
    int ret_value = XORP_OK;
    
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
    
    //
    // Stop the protocol with the kernel
    //
    if (stop_protocol_kernel() != XORP_OK) {
	XLOG_ERROR("Error stopping protocol with the kernel. Ignored.");
    }
    
    if (ProtoUnit::stop() < 0)
	ret_value = XORP_ERROR;
    
    return (ret_value);
}

/**
 * PimNode::has_pending_stop_vif:
 * @void: 
 * 
 * Test if there is an unit that is in PENDING_DOWN state.
 * 
 * Return value: True if there is an unit that is in PENDING_DOWN state,
 * otherwise false.
 **/
bool
PimNode::has_pending_down_units(void)
{
    vector<PimVif *>::iterator iter;
    
    //
    // Test the interfaces
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (pim_vif->is_pending_down())
	    return (true);
    }
    
    //
    // TODO: XXX: PAVPAVPAV: test other units that may be waiting
    // in PENDING_DOWN state: PimMrt, PimMribTable, PimBsr, RpTable,
    // PimScopeZoneTable, etc.
    //
    
    return (false);
}

/**
 * PimNode::add_vif:
 * @vif: Information about new PimVif to install.
 * @err: The error message (if error).
 * 
 * Install a new PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::add_vif(const Vif& vif, string& err)
{
    //
    // Create a new PimVif
    //
    PimVif *pim_vif = new PimVif(*this, vif);
    
    if (ProtoNode<PimVif>::add_vif(pim_vif) != XORP_OK) {
	// Cannot add this new vif
	err = c_format("Cannot add vif %s: internal error",
		       vif.name().c_str());
	XLOG_ERROR(err.c_str());
	
	delete pim_vif;
	return (XORP_ERROR);
    }
    
    // Set the PIM Register vif index if needed
    if (pim_vif->is_pim_register())
	_pim_register_vif_index = pim_vif->vif_index();
    
    XLOG_INFO("New vif: %s", pim_vif->str().c_str());
    
    return (XORP_OK);
}

/**
 * PimNode::add_vif:
 * @vif_name: The name of the new vif.
 * @vif_index: The vif index of the new vif.
 * @err: The error message (if error).
 * 
 * Install a new PIM vif. If the vif exists, nothing is installed.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::add_vif(const string& vif_name, uint16_t vif_index, string& err)
{
    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
    
    if ((pim_vif != NULL) && (pim_vif->name() == vif_name)) {
	return (XORP_OK);		// Already have this vif
    }

    //
    // Create a new Vif
    //
    Vif vif(vif_name);
    vif.set_vif_index(vif_index);
    if (add_vif(vif, err) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * PimNode::delete_vif:
 * @vif_name: The name of the vif to delete.
 * @err: The error message (if error).
 * 
 * Delete an existing PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::delete_vif(const string& vif_name, string& err)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	err = c_format("Cannot delete vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    if (ProtoNode<PimVif>::delete_vif(pim_vif) != XORP_OK) {
	err = c_format("Cannot delete vif %s: internal error",
		       pim_vif->name().c_str());
	XLOG_ERROR(err.c_str());
	delete pim_vif;
	return (XORP_ERROR);
    }
    
    // Reset the PIM Register vif index if needed
    if (_pim_register_vif_index == pim_vif->vif_index())
	_pim_register_vif_index = Vif::VIF_INDEX_INVALID;
    
    delete pim_vif;
    
    XLOG_INFO("Deleted vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

int
PimNode::set_vif_flags(const string& vif_name,
		       bool is_pim_register, bool is_p2p,
		       bool is_loopback, bool is_multicast,
		       bool is_broadcast, bool is_up,
		       string& err)
{
    bool is_changed = false;
    
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	err = c_format("Cannot set flags vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->is_pim_register() != is_pim_register) {
	pim_vif->set_pim_register(is_pim_register);
	is_changed = true;
    }
    if (pim_vif->is_p2p() != is_p2p) {
	pim_vif->set_p2p(is_p2p);
	is_changed = true;
    }
    if (pim_vif->is_loopback() != is_loopback) {
	pim_vif->set_loopback(is_loopback);
	is_changed = true;
    }
    if (pim_vif->is_multicast_capable() != is_multicast) {
	pim_vif->set_multicast_capable(is_multicast);
	is_changed = true;
    }
    if (pim_vif->is_broadcast_capable() != is_broadcast) {
	pim_vif->set_broadcast_capable(is_broadcast);
	is_changed = true;
    }
    if (pim_vif->is_underlying_vif_up() != is_up) {
	pim_vif->set_underlying_vif_up(is_up);
	is_changed = true;
    }
    
    // Set the PIM Register vif index if needed
    if (pim_vif->is_pim_register())
	_pim_register_vif_index = pim_vif->vif_index();
    
    if (is_changed)
	XLOG_INFO("Vif flags changed: %s", pim_vif->str().c_str());
    
    return (XORP_OK);
}

int
PimNode::add_vif_addr(const string& vif_name,
		      const IPvX& addr,
		      const IPvXNet& subnet_addr,
		      const IPvX& broadcast_addr,
		      const IPvX& peer_addr,
		      string &err)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	err = c_format("Cannot add address on vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr vif_addr(addr, subnet_addr, broadcast_addr, peer_addr);
    
    if (pim_vif->is_my_vif_addr(vif_addr)) {
	return (XORP_OK);		// Already have this address
    }
    
    if (! addr.is_unicast()) {
	err = c_format("Cannot add address on vif %s: "
		       "invalid unicast address: %s",
		       pim_vif->name().c_str(), addr.str().c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    if ((addr.af() != family())
	|| (subnet_addr.af() != family())
	|| (broadcast_addr.af() != family())
	|| (peer_addr.af() != family())) {
	err = c_format("Cannot add address on vif %s: "
		       "invalid address family: %s ",
		       pim_vif->name().c_str(), vif_addr.str().c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->add_address(vif_addr);
    
    XLOG_INFO("Added new address to vif %s: %s",
	      pim_vif->name().c_str(), vif_addr.str().c_str());
    
    return (XORP_OK);
}

int
PimNode::delete_vif_addr(const string& vif_name,
			 const IPvX& addr,
			 string& err)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	err = c_format("Cannot delete address on vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr *tmp_vif_addr = pim_vif->find_address(addr);
    if (tmp_vif_addr == NULL) {
	err = c_format("Cannot delete address on vif %s: "
		       "invalid address %s",
		       pim_vif->name().c_str(), addr.str().c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    VifAddr vif_addr = *tmp_vif_addr;	// Get a copy
    if (pim_vif->delete_address(addr) != XORP_OK) {
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Deleted address on vif %s: %s",
	      pim_vif->name().c_str(), vif_addr.str().c_str());
    
    return (XORP_OK);
}

/**
 * PimNode::start_all_vifs:
 * @void: 
 * 
 * Start PIM on all enabled interfaces.
 * 
 * Return value: The number of virtual interfaces PIM was started on,
 * or %XORP_ERROR if error occured.
 **/
int
PimNode::start_all_vifs(void)
{
    int n = 0;
    vector<PimVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (pim_vif->start() == XORP_OK)
	    n++;
    }
    
    return (n);
}

/**
 * PimNode::stop_all_vifs:
 * @void: 
 * 
 * Stop PIM on all interfaces it was running on.
 * 
 * Return value: The number of virtual interfaces PIM was stopped on,
 * or %XORP_ERROR if error occured.
 **/
int
PimNode::stop_all_vifs(void)
{
    int n = 0;
    vector<PimVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (pim_vif->stop() == XORP_OK)
	    n++;
    }
    
    return (n);
}

/**
 * PimNode::enable_all_vifs:
 * @void: 
 * 
 * Enable PIM on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::enable_all_vifs(void)
{
    vector<PimVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	pim_vif->enable();
    }
    
    return (XORP_OK);
}

/**
 * PimNode::disable_all_vifs:
 * @void: 
 * 
 * Disable PIM on all interfaces. All running interfaces are stopped first.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::disable_all_vifs(void)
{
    vector<PimVif *>::iterator iter;
    
    stop_all_vifs();
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	pim_vif->disable();
    }
    
    return (XORP_OK);
}

/**
 * PimNode::delete_all_vifs:
 * @void: 
 * 
 * Delete all PIM vifs.
 **/
void
PimNode::delete_all_vifs(void)
{
    // XXX: here we must use proto_vifs().size() to end the iteration,
    // because the proto_vifs() array may be modified when a vif
    // is deleted.
    for (uint16_t i = 0; i < proto_vifs().size(); i++) {
	PimVif *pim_vif = vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	delete pim_vif;
    }
    
    proto_vifs().clear();
}

/**
 * PimNode::proto_recv:
 * @src_module_instance_name: The module instance name of the module-origin
 * of the message.
 * @src_module_id: The #xorp_module_id of the module-origin of the message.
 * @vif_index: The vif index of the interface used to receive this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @router_alert_bool: If true, the ROUTER_ALERT IP option for the IP
 * packet of the incoming message was set.
 * @rcvbuf: The data buffer with the received message.
 * @rcvlen: The data length in @rcvbuf.
 * 
 * Receive a protocol message from the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::proto_recv(const string&	, // src_module_instance_name,
		    xorp_module_id src_module_id,
		    uint16_t vif_index,
		    const IPvX& src, const IPvX& dst,
		    int ip_ttl, int ip_tos, bool router_alert_bool,
		    const uint8_t *rcvbuf, size_t rcvlen)
{
    PimVif *pim_vif = NULL;
    int ret_value = XORP_ERROR;
    
    debug_msg("Received message from %s to %s: "
	      "ip_ttl = %d ip_tos = %#x router_alert = %d rcvlen = %u\n",
	      cstring(src), cstring(dst),
	      ip_ttl, ip_tos, router_alert_bool, (uint32_t)rcvlen);
    
    //
    // Misc. checks
    //
    if (! is_up())
	return (XORP_ERROR);
    if ((src.af() != family()) || (dst.af() != family()))
	return (XORP_ERROR);	// Invalid address family
    if (! src.is_unicast())
	return (XORP_ERROR);	// The source address has to be valid
    if ((ip_ttl != MINTTL) || (! router_alert_bool)) {
	// TODO: only if we are running in secure mode we should ignore
	// messages with invalid ip_ttl?
#if 0
	return (XORP_ERROR);	// Invalid IP TTL
#endif
    }
    
    //
    // Find the vif for that packet
    //
    pim_vif = vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL) {
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    if (! pim_vif->is_up())
	return (XORP_ERROR);
    
    // Copy the data to the receiving #buffer_t
    BUFFER_RESET(_buffer_recv);
    BUFFER_PUT_DATA(rcvbuf, _buffer_recv, rcvlen);
    
    // Process the data by the vif
    ret_value = pim_vif->pim_recv(src, dst, ip_ttl, ip_tos,
				  router_alert_bool,
				  _buffer_recv);
    
    return (ret_value);
    
 buflen_error:
    XLOG_UNREACHABLE();
    return (XORP_ERROR);
    
    UNUSED(src_module_id);
}

/**
 * PimNode::pim_send:
 * @vif_index: The vif index of the interface to send this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * the TTL will be set by the lower layers.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * the TOS will be set by the lower layers.
 * @router_alert_bool: If true, set the ROUTER_ALERT IP option for the IP
 * packet of the outgoung message.
 * @buffer: The #buffer_t data buffer with the message to send.
 * 
 * Send a PIM message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::pim_send(uint16_t vif_index,
		  const IPvX& src, const IPvX& dst,
		  int ip_ttl, int ip_tos, bool router_alert_bool,
		  buffer_t *buffer)
{
    if (! (is_up() || is_pending_down()))
	return (XORP_ERROR);
    
    // TODO: the target name of the MFEA must be configurable.
    if (proto_send(xorp_module_name(family(), XORP_MODULE_MFEA),
		   XORP_MODULE_MFEA,
		   vif_index, src, dst,
		   ip_ttl, ip_tos, router_alert_bool,
		   BUFFER_DATA_HEAD(buffer),
		   BUFFER_DATA_SIZE(buffer)) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * PimNode::signal_message_recv:
 * @src_module_instance_name: The module instance name of the module-origin
 * of the message.
 * @src_module_id: The #xorp_module_id of the module-origin of the message.
 * @message_type: The message type of the kernel signal.
 * At this moment, one of the following:
 * %MFEA_KERNEL_MESSAGE_NOCACHE (if a cache-miss in the kernel)
 * %MFEA_KERNEL_MESSAGE_WRONGVIF (multicast packet received on wrong vif)
 * %MFEA_KERNEL_MESSAGE_WHOLEPKT (typically, a packet that should be
 * encapsulated as a PIM-Register).
 * @vif_index: The vif index of the related interface (message-specific
 * relation).
 * @src: The source address in the message.
 * @dst: The destination address in the message.
 * @rcvbuf: The data buffer with the additional information in the message.
 * @rcvlen: The data length in @rcvbuf.
 * 
 * Receive a signal from the kernel (e.g., NOCACHE, WRONGVIF, WHOLEPKT).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::signal_message_recv(const string& src_module_instance_name,
			     xorp_module_id src_module_id,
			     int message_type,
			     uint16_t vif_index,
			     const IPvX& src,
			     const IPvX& dst,
			     const uint8_t *rcvbuf,
			     size_t rcvlen)
{
    int ret_value = XORP_ERROR;
    
    do {
	if (message_type == MFEA_KERNEL_MESSAGE_NOCACHE) {
	    ret_value = pim_mrt().signal_message_nocache_recv(
		src_module_instance_name,
		src_module_id,
		vif_index,
		src,
		dst);
	    break;
	}
	if (message_type == MFEA_KERNEL_MESSAGE_WRONGVIF) {
	    ret_value = pim_mrt().signal_message_wrongvif_recv(
		src_module_instance_name,
		src_module_id,
		vif_index,
		src,
		dst);
	    break;
	}
	if (message_type == MFEA_KERNEL_MESSAGE_WHOLEPKT) {
	    ret_value = pim_mrt().signal_message_wholepkt_recv(
		src_module_instance_name,
		src_module_id,
		vif_index,
		src,
		dst,
		rcvbuf,
		rcvlen);
	    break;
	}
	
	XLOG_WARNING("RX signal message with invalid message type = %d: "
		     "src = %s dst = %s vif_index = %d src_module_name = %s",
		     message_type, cstring(src), cstring(dst), vif_index,
		     src_module_instance_name.c_str());
	return (XORP_ERROR);
    } while (false);
    
    return (ret_value);
}

/**
 * PimNode::add_membership:
 * @vif_index: The vif_index of the interface to add membership.
 * @source: The source address to add membership for
 * (IPvX::ZERO() for IGMPv1,2 and MLDv1).
 * @group: The group address to add membership for.
 * 
 * Add multicast membership on vif with vif_index of @vif_index for
 * source address of @source and group address of @group.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::add_membership(uint16_t vif_index, const IPvX& source,
			const IPvX& group)
{
    uint32_t lookup_flags = 0;
    uint32_t create_flags = 0;
    PimVif *pim_vif = NULL;
    
    //
    // Check the arguments: the vif, source and group addresses.
    //
    pim_vif = vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (XORP_ERROR);
    if (! (pim_vif->is_up()
	   || pim_vif->is_pending_up())) {
	return (XORP_ERROR);	// The vif is (going) DOWN
    }
    
    if (source != IPvX::ZERO(family())) {
	if (! source.is_unicast())
	    return (XORP_ERROR);
    }
    if (! group.is_multicast())
	return (XORP_ERROR);
    
    if (group.is_linklocal_multicast() || group.is_nodelocal_multicast())
	return (XORP_OK);		// XXX: don't route link or node-local groups
    
    XLOG_TRACE(is_log_trace(), "Add membership for (%s,%s) on vif %s",
	       cstring(source), cstring(group), pim_vif->name().c_str());
    
    //
    // Setup the MRE lookup and create flags
    //
    lookup_flags |= PIM_MRE_WC;
    if (source != IPvX::ZERO(family()))
	lookup_flags |= PIM_MRE_SG;
    create_flags = lookup_flags;
    
    PimMre *pim_mre = pim_mrt().pim_mre_find(source, group, lookup_flags,
					     create_flags);
    
    if (pim_mre == NULL)
	return (XORP_ERROR);
    
    //
    // Add to the local membership state
    //
    pim_mre->set_local_receiver_include(pim_vif->vif_index(), true);
    // TODO: figure-out what to do for (S,G) Join:
    // remove from local_receiver_exclude or something else?
    
    return (XORP_OK);
}

/**
 * PimNode::delete_membership:
 * @vif_index: The vif_index of the interface to delete membership.
 * @source: The source address to delete membership for
 * (IPvX::ZERO() for IGMPv1,2 and MLDv1).
 * @group: The group address to delete membership for.
 * 
 * Delete multicast membership on vif with vif_index of @vif_index for
 * source address of @source and group address of @group.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::delete_membership(uint16_t vif_index, const IPvX& source,
			   const IPvX& group)
{
    uint32_t lookup_flags = 0;
    uint32_t create_flags = 0;
    PimVif *pim_vif = NULL;
    
    //
    // Check the arguments: the vif, source and group addresses.
    //
    pim_vif = vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (XORP_ERROR);
    if (! (pim_vif->is_up()
	   || pim_vif->is_pending_down()
	   || pim_vif->is_pending_up())) {
	return (XORP_ERROR);	// The vif is DOWN
    }
    
    if (source != IPvX::ZERO(family())) {
	if (! source.is_unicast())
	    return (XORP_ERROR);
    }
    if (! group.is_multicast())
	return (XORP_ERROR);
    
    if (group.is_linklocal_multicast() || group.is_nodelocal_multicast())
	return (XORP_OK);		// XXX: don't route link or node-local groups
    
    XLOG_TRACE(is_log_trace(), "Delete membership for (%s,%s) on vif %s",
	       cstring(source), cstring(group), pim_vif->name().c_str());
    
    //
    // Setup the MRE lookup and create flags
    //
    lookup_flags |= PIM_MRE_WC;
    if (source != IPvX::ZERO(family()))
	lookup_flags |= PIM_MRE_SG;
    create_flags = 0;
    
    PimMre *pim_mre = pim_mrt().pim_mre_find(source, group, lookup_flags,
					     create_flags);
    
    if (pim_mre == NULL)
	return (XORP_ERROR);
    
    //
    // Delete from the local membership state
    //
    pim_mre->set_local_receiver_include(pim_vif->vif_index(), false);
    // TODO: XXX: PAVPAVPAV: figure-out what to do for (S,G) Leave:
    // remove from local_receiver_include or add to local_receiver_exclude
    // or something else?
    
    // pim_mrt().remove(pim_mre);
    // delete pim_mre;
    
    return (XORP_OK);
}

/**
 * PimNode::vif_find_pim_register:
 * @void: 
 * 
 * Return the PIM Register virtual interface.
 * 
 * Return value: The PIM Register virtual interface if exists, otherwise NULL.
 **/
PimVif *
PimNode::vif_find_pim_register(void) const
{
    return (vif_find_by_vif_index(pim_register_vif_index()));
}

/**
 * PimNode::set_pim_vifs_dr:
 * @vif_index: The vif index to set/reset the DR flag.
 * @v: true if set the DR flag, otherwise false.
 * 
 * Set/reset the DR flag for vif index @vif_index.
 **/
void
PimNode::set_pim_vifs_dr(uint16_t vif_index, bool v)
{
    if (vif_index >= pim_vifs_dr().size())
	return;			// TODO: return an error instead?
    
    if (pim_vifs_dr().test(vif_index) == v)
	return;			// Nothing changed
    
    if (v)
	pim_vifs_dr().set(vif_index);
    else
	pim_vifs_dr().reset(vif_index);
    
    pim_mrt().add_task_i_am_dr(vif_index);
}

/**
 * PimNode::pim_nbr_rpf_find:
 * @dst_addr: The address of the destination to search for.
 * 
 * Find the RPF PIM neighbor for a destination address.
 * 
 * Return value: The #PimNbr entry for the RPF neighbor to @dst_addr if found,
 * otherwise %NULL.
 **/
PimNbr *
PimNode::pim_nbr_rpf_find(const IPvX& dst_addr)
{
    Mrib *mrib;
    
    //
    // Do the MRIB lookup
    //
    mrib = pim_mrib_table().find(dst_addr);
    
    //
    // Seach for the RPF neighbor router
    //
    return (pim_nbr_rpf_find(dst_addr, mrib));
}

/**
 * PimNode::pim_nbr_rpf_find:
 * @dst_addr: The address of the destination to search for.
 * @mrib: The MRIB information that was lookup already.
 * 
 * Find the RPF PIM neighbor for a destination address by using the
 * information in a pre-lookup MRIB.
 * 
 * Return value: The #PimNbr entry for the RPF neighbor to @dst_addr if found,
 * otherwise %NULL.
 **/
PimNbr *
PimNode::pim_nbr_rpf_find(const IPvX& dst_addr, const Mrib *mrib)
{
    PimNbr *pim_nbr;
    
    //
    // Check the MRIB information
    //
    if (mrib == NULL)
	return (NULL);
    
    //
    // Find the vif toward the destination address
    //
    PimVif *pim_vif = vif_find_by_vif_index(mrib->next_hop_vif_index());
    
    //
    // Search for the neighbor router
    //
    if (mrib->next_hop_router_addr() != IPvX::ZERO(family())) {
	// Not a destination on the same subnet
	if (pim_vif != NULL)
	    pim_nbr = pim_vif->pim_nbr_find(mrib->next_hop_router_addr());
    } else {
	// Probably a destination on the same subnet
	if (pim_vif != NULL)
	    pim_nbr = pim_vif->pim_nbr_find(dst_addr);
	else
	    pim_nbr = pim_nbr_find(dst_addr);
    }
    
    return (pim_nbr);
}

/**
 * PimNode::pim_nbr_find:
 * @nbr_addr: The address of the neighbor to search for.
 * 
 * Find a PIM neighbor by its address.
 * 
 * Return value: The #PimNbr entry for the neighbor if found, otherwise %NULL.
 **/
PimNbr *
PimNode::pim_nbr_find(const IPvX& nbr_addr)
{
    for (uint16_t i = 0; i < maxvifs(); i++) {
	PimVif *pim_vif = vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(nbr_addr);
	if (pim_nbr != NULL)
	    return (pim_nbr);
    }
    
    return (NULL);
}

//
// Add the PimMre to the dummy PimNbr with addr of IPvX::ZERO()
//
void
PimNode::add_pim_mre_no_pim_nbr(PimMre *pim_mre)
{
    IPvX ipvx_zero(IPvX::ZERO(family()));
    PimNbr *pim_nbr = NULL;
    
    // Find the dummy PimNbr with addr of IPvX::ZERO()
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	pim_nbr = *iter;
	if (pim_nbr->addr() == ipvx_zero)
	    break;
	else
	    pim_nbr = NULL;
    }
    
    if (pim_nbr == NULL) {
	// Find the first vif
	PimVif *pim_vif = NULL;
	for (uint16_t i = 0; i < maxvifs(); i++) {
	    pim_vif = vif_find_by_vif_index(i);
	    if (pim_vif != NULL)
		break;
	}
	XLOG_ASSERT(pim_vif != NULL);
	pim_nbr = new PimNbr(*pim_vif, ipvx_zero, PIM_VERSION_DEFAULT);
	processing_pim_nbr_list().push_back(pim_nbr);
    }
    XLOG_ASSERT(pim_nbr != NULL);
    
    pim_nbr->add_pim_mre(pim_mre);
}

//
// Delete the PimMre from the dummy PimNbr with addr of IPvX::ZERO()
//
void
PimNode::delete_pim_mre_no_pim_nbr(PimMre *pim_mre)
{
    IPvX ipvx_zero(IPvX::ZERO(family()));
    PimNbr *pim_nbr = NULL;
    
    // Find the dummy PimNbr with addr of IPvX::ZERO()
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	pim_nbr = *iter;
	if (pim_nbr->addr() == ipvx_zero)
	    break;
	else
	    pim_nbr = NULL;
    }
    
    if (pim_nbr != NULL)
	pim_nbr->delete_pim_mre(pim_mre);
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (*,*,RP) PimMre entries.
//
void
PimNode::init_processing_pim_mre_rp(uint16_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_rp();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_rp();
    }
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (*,G) PimMre entries.
//
void
PimNode::init_processing_pim_mre_wc(uint16_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_wc();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_wc();
    }
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (S,G) PimMre entries.
//
void
PimNode::init_processing_pim_mre_sg(uint16_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_sg();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_sg();
    }
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (S,G,rpt) PimMre entries.
//
void
PimNode::init_processing_pim_mre_sg_rpt(uint16_t vif_index,
					const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_sg_rpt();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_sg_rpt();
    }
}

PimNbr *
PimNode::find_processing_pim_mre_rp(uint16_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_rp_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_rp_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

PimNbr *
PimNode::find_processing_pim_mre_wc(uint16_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_wc_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_wc_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

PimNbr *
PimNode::find_processing_pim_mre_sg(uint16_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_sg_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_sg_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

PimNbr *
PimNode::find_processing_pim_mre_sg_rpt(uint16_t vif_index,
					const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_sg_rpt_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_sg_rpt_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}
