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

#ident "$XORP: xorp/mld6igmp/mld6igmp_node.cc,v 1.1.1.1 2002/12/11 23:56:06 hodson Exp $"


//
// Multicast Listener Discovery and Internet Group Management Protocol
// node implementation (common part).
// MLDv1 (RFC 2710), IGMPv1 and IGMPv2 (RFC 2236).
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mld6igmp_node.hh"
#include "mld6igmp_vif.hh"



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
 * Mld6igmpNode::Mld6igmpNode:
 * @family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @module_id: The module ID (must be %X_MODULE_MLD6IGMP).
 * @event_loop: The event loop.
 * 
 * MLD6IGMP node constructor.
 **/
Mld6igmpNode::Mld6igmpNode(int family, x_module_id module_id,
			   EventLoop& event_loop)
    : ProtoNode<Mld6igmpVif>(family, module_id, event_loop),
    _is_log_trace(true)			// XXX: default to print trace logs
{
    XLOG_ASSERT(module_id == X_MODULE_MLD6IGMP);
    if (module_id != X_MODULE_MLD6IGMP) {
	XLOG_FATAL("Invalid module ID = %d (must be 'X_MODULE_MLD6IGMP' = %d)",
		   module_id, X_MODULE_MLD6IGMP);
    }
    
    _buffer_recv = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
}

/**
 * Mld6igmpNode::~Mld6igmpNode:
 * @void: 
 * 
 * MLD6IGMP node destructor.
 * 
 **/
Mld6igmpNode::~Mld6igmpNode(void)
{
    stop();
    
    delete_all_vifs();
    
    BUFFER_FREE(_buffer_recv);
}

/**
 * Mld6igmpNode::start:
 * @void: 
 * 
 * Start the MLD6 or IGMP protocol.
 * TODO: This function should not start the protocol operation on the
 * interfaces. The interfaces must be activated separately.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
Mld6igmpNode::start(void)
{
    if (ProtoNode<Mld6igmpVif>::start() < 0)
	return (XORP_ERROR);
    
    //
    // Start the protocol with the kernel
    //
    if (start_protocol_kernel() != XORP_OK) {
	XLOG_ERROR("Error starting protocol with the kernel");
	ProtoNode<Mld6igmpVif>::stop();
	return (XORP_ERROR);
    }
    
    // Start the mld6igmp_vifs
    start_all_vifs();
    
    //
    // Perform misc. MLD6IGMP-specific start operations
    //
    // TODO: IMPLEMENT IT!
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::stop:
 * @void: 
 * 
 * Stop the MLD6 or IGMP protocol.
 * XXX: This function, unlike start(), will stop the protocol
 * operation on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::stop(void)
{
    if (! is_up())
	return (XORP_ERROR);
    
    //
    // Perform misc. MLD6IGMP-specific stop operations
    //
    // TODO: IMPLEMENT IT!
    
    // Stop the vifs
    stop_all_vifs();
    
    //
    // Stop the protocol with the kernel
    //
    if (stop_protocol_kernel() != XORP_OK) {
	XLOG_ERROR("Error stopping protocol with the kernel. Ignored.");
    }
    
    if (ProtoNode<Mld6igmpVif>::stop() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::add_vif:
 * @vif: Information about new Mld6igmpVif to install.
 * 
 * Install a new MLD6/IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::add_vif(const Vif& vif)
{
    //
    // Create a new Mld6igmpVif
    //
    Mld6igmpVif *mld6igmp_vif = new Mld6igmpVif(*this, vif);
    
    if (ProtoNode<Mld6igmpVif>::add_vif(mld6igmp_vif) != XORP_OK) {
	// Cannot add this new vif
	delete mld6igmp_vif;
	return (XORP_ERROR);
    }
    
    XLOG_INFO("New vif: %s", mld6igmp_vif->str().c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::add_vif:
 * @vif_name: The name of the new vif.
 * @vif_index: The vif index of the new vif.
 * 
 * Install a new MLD6/IGMP vif. If the vif exists, nothing is installed.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::add_vif(const char *vif_name, uint32_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(vif_index);
    
    if ((mld6igmp_vif != NULL)
	&& (mld6igmp_vif->name() == string(vif_name))) {
	return (XORP_OK);		// Already have this vif
    }
    
    //
    // Create a new Vif
    //
    Vif vif(vif_name);
    vif.set_vif_index(vif_index);
    if (add_vif(vif) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::delete_vif:
 * @vif_name: The name of the vif to delete.
 * 
 * Delete an existing MLD6/IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::delete_vif(const char *vif_name)
{
    if (vif_name == NULL) {
	XLOG_ERROR("Cannot delete vif with vif_name = NULL");
	return (XORP_ERROR);
    }
    
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot delete vif %s: no such vif",
		   vif_name);
	return (XORP_ERROR);
    }
    
    if (ProtoNode<Mld6igmpVif>::delete_vif(mld6igmp_vif) != XORP_OK) {
	XLOG_ERROR("Cannot delete vif %s: internal error",
		   mld6igmp_vif->name().c_str());
	delete mld6igmp_vif;
	return (XORP_ERROR);
    }
    
    delete mld6igmp_vif;
    
    XLOG_INFO("Deleted vif: %s", vif_name);
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_flags(const char *vif_name,
			    bool is_pim_register, bool is_p2p,
			    bool is_loopback, bool is_multicast,
			    bool is_broadcast, bool is_up)
{
    bool is_changed = false;
    
    if (vif_name == NULL) {
	XLOG_ERROR("Cannot set flags on vif with vif_name = NULL");
	return (XORP_ERROR);
    }
    
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot set flags vif %s: no such vif",
		   vif_name);
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->is_pim_register() != is_pim_register) {
	mld6igmp_vif->set_pim_register(is_pim_register);
	is_changed = true;
    }
    if (mld6igmp_vif->is_p2p() != is_p2p) {
	mld6igmp_vif->set_p2p(is_p2p);
	is_changed = true;
    }
    if (mld6igmp_vif->is_loopback() != is_loopback) {
	mld6igmp_vif->set_loopback(is_loopback);
	is_changed = true;
    }
    if (mld6igmp_vif->is_multicast_capable() != is_multicast) {
	mld6igmp_vif->set_multicast_capable(is_multicast);
	is_changed = true;
    }
    if (mld6igmp_vif->is_broadcast_capable() != is_broadcast) {
	mld6igmp_vif->set_broadcast_capable(is_broadcast);
	is_changed = true;
    }
    if (mld6igmp_vif->is_underlying_vif_up() != is_up) {
	mld6igmp_vif->set_underlying_vif_up(is_up);
	is_changed = true;
    }
    
    if (is_changed)
	XLOG_INFO("Vif flags changed: %s", mld6igmp_vif->str().c_str());
    
    return (XORP_OK);
}

int
Mld6igmpNode::add_vif_addr(const char *vif_name,
			   const IPvX& addr,
			   const IPvXNet& subnet_addr,
			   const IPvX& broadcast_addr,
			   const IPvX& peer_addr)
{
    if (vif_name == NULL) {
	XLOG_ERROR("Cannot add address on vif with vif_name = NULL");
	return (XORP_ERROR);
    }
    
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot add address on vif %s: no such vif",
		   vif_name);
	return (XORP_ERROR);
    }
    
    const VifAddr vif_addr(addr, subnet_addr, broadcast_addr, peer_addr);
    
    if (mld6igmp_vif->is_my_vif_addr(vif_addr)) {
	return (XORP_OK);		// Already have this address
    }
    
    if (! addr.is_unicast()) {
	XLOG_ERROR("Cannot add address on vif %s: "
		   "invalid unicast address: %s",
		   mld6igmp_vif->name().c_str(), addr.str().c_str());
	return (XORP_ERROR);
    }
    
    if ((addr.af() != family())
	|| (subnet_addr.af() != family())
	|| (broadcast_addr.af() != family())
	|| (peer_addr.af() != family())) {
	XLOG_ERROR("Cannot add address on vif %s: "
		   "invalid address family: %s ",
		   mld6igmp_vif->name().c_str(), vif_addr.str().c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->add_address(vif_addr);
    
    XLOG_INFO("Added new address to vif %s: %s",
	      mld6igmp_vif->name().c_str(), vif_addr.str().c_str());
    
    return (XORP_OK);
}

int
Mld6igmpNode::delete_vif_addr(const char *vif_name,
			      const IPvX& addr)
{
    if (vif_name == NULL) {
	XLOG_ERROR("Cannot delete address on vif with vif_name = NULL");
	return (XORP_ERROR);
    }
    
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot delete address on vif %s: no such vif",
		   vif_name);
	return (XORP_ERROR);
    }
    
    const VifAddr *tmp_vif_addr = mld6igmp_vif->find_address(addr);
    if (tmp_vif_addr == NULL) {
	XLOG_ERROR("Cannot delete address on vif %s: "
		   "invalid address %s",
		   mld6igmp_vif->name().c_str(), addr.str().c_str());
	return (XORP_ERROR);
    }
    
    VifAddr vif_addr = *tmp_vif_addr;	// Get a copy
    if (mld6igmp_vif->delete_address(addr) != XORP_OK) {
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Deleted address on vif %s: %s",
	      mld6igmp_vif->name().c_str(), vif_addr.str().c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::start_all_vifs:
 * @void: 
 * 
 * Start MLD6/IGMP on all enabled interfaces.
 * 
 * Return value: The number of virtual interfaces MLD6/IGMP was started on,
 * or %XORP_ERROR if error occured.
 **/
int
Mld6igmpNode::start_all_vifs(void)
{
    int n = 0;
    vector<Mld6igmpVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (mld6igmp_vif->start() == XORP_OK)
	    n++;
    }
    
    return (n);
}

/**
 * Mld6igmpNode::stop_all_vifs:
 * @void: 
 * 
 * Stop MLD6/IGMP on all interfaces it was running on.
 * 
 * Return value: The number of virtual interfaces MLD6/IGMP was stopped on,
 * or %XORP_ERROR if error occured.
 **/
int
Mld6igmpNode::stop_all_vifs(void)
{
    int n = 0;
    vector<Mld6igmpVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (mld6igmp_vif->stop() == XORP_OK)
	    n++;
    }
    
    return (n);
}

/**
 * Mld6igmpNode::enable_all_vifs:
 * @void: 
 * 
 * Enable MLD6/IGMP on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::enable_all_vifs(void)
{
    vector<Mld6igmpVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	mld6igmp_vif->enable();
    }
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::disable_all_vifs:
 * @void: 
 * 
 * Disable MLD6/IGMP on all interfaces. All running interfaces are stopped
 * first.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::disable_all_vifs(void)
{
    vector<Mld6igmpVif *>::iterator iter;
    
    stop_all_vifs();
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	mld6igmp_vif->disable();
    }
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::delete_all_vifs:
 * @void: 
 * 
 * Delete all MLD6IGMP vifs.
 **/
void
Mld6igmpNode::delete_all_vifs(void)
{
    // XXX: here we must use proto_vifs().size() to end the iteration,
    // because the proto_vifs() array may be modified when a vif
    // is deleted.
    for (size_t i = 0; i < proto_vifs().size(); i++) {
	Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(i);
	if (mld6igmp_vif == NULL)
	    continue;
	delete mld6igmp_vif;
    }
    
    proto_vifs().clear();
}

/**
 * Mld6igmpNode::proto_recv:
 * @src_module_instance_name: The module instance name of the module-origin
 * of the message.
 * @src_module_id: The #x_module_id of the module-origin of the message.
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
Mld6igmpNode::proto_recv(const string&	, // src_module_instance_name,
			 x_module_id src_module_id,
			 uint16_t vif_index,
			 const IPvX& src, const IPvX& dst,
			 int ip_ttl, int ip_tos, bool router_alert_bool,
			 const uint8_t *rcvbuf, size_t rcvlen)
{
    Mld6igmpVif *mld6igmp_vif = NULL;
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
    mld6igmp_vif = vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    if (! mld6igmp_vif->is_up())
	return (XORP_ERROR);
    
    // Copy the data to the receiving #buffer_t
    BUFFER_RESET(_buffer_recv);
    BUFFER_PUT_DATA(rcvbuf, _buffer_recv, rcvlen);
    
    // Process the data by the vif
    ret_value = mld6igmp_vif->mld6igmp_recv(src, dst, ip_ttl, ip_tos,
					    router_alert_bool,
					    _buffer_recv);
    
    return (ret_value);
    
 buflen_error:
    XLOG_ASSERT(false);
    return (XORP_ERROR);
    
    UNUSED(src_module_id);
}

/**
 * Mld6igmpNode::mld6igmp_send:
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
 * Send a MLD6 or IGMP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::mld6igmp_send(uint16_t vif_index,
			    const IPvX& src, const IPvX& dst,
			    int ip_ttl, int ip_tos, bool router_alert_bool,
			    buffer_t *buffer)
{
    if (! is_up())
	return (XORP_ERROR);
    
    // TODO: the target name of the MFEA must be configurable.
    if (proto_send(x_module_name(family(), X_MODULE_MFEA),
		   X_MODULE_MFEA,
		   vif_index, src, dst,
		   ip_ttl, ip_tos, router_alert_bool,
		   BUFFER_DATA_HEAD(buffer),
		   BUFFER_DATA_SIZE(buffer)) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::add_protocol:
 * @module_instance_name: The module instance name of the protocol to add.
 * @module_id: The #x_module_id of the protocol to add.
 * @vif_index: The vif index of the interface to add the protocol to.
 * 
 * Add a protocol to the list of entries that would be notified if there
 * is membership change on a particular interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::add_protocol(const string& module_instance_name,
			   x_module_id module_id,
			   uint16_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot add protocol instance %s on vif_index %d: "
		   "no such vif",
		   module_instance_name.c_str(), vif_index);
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->add_protocol(module_id, module_instance_name) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::delete_protocol:
 * @module_instance_name: The module instance name of the protocol to delete.
 * @module_id: The #x_module_id of the protocol to delete.
 * @vif_index: The vif index of the interface to delete the protocol from.
 * 
 * Delete a protocol from the list of entries that would be notified if there
 * is membership change on a particular interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::delete_protocol(const string& module_instance_name,
			      x_module_id module_id,
			      uint16_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot delete protocol instance %s on vif_index %d: "
		   "no such vif",
		   module_instance_name.c_str(), vif_index);
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->delete_protocol(module_id, module_instance_name) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::join_prune_notify_routing:
 * @module_instance_name: The module instance name of the protocol to notify.
 * @module_id: The #x_module_id of the protocol to notify.
 * @vif_index: The vif index of the interface with membership change.
 * @source: The source address of the (S,G) or (*,G) entry that has changed.
 * In case of group-specific membership, it is IPvX::ZERO().
 * @group: The group address of the (S,G) or (*,G) entry that has changed.
 * @action_jp: The membership change type #action_jp_t:
 * either %ACTION_JOIN or %ACTION_PRUNE.
 * 
 * Notify the protocol instance with name @module_instance_name that there is
 * multicast membership change on interface with vif index of @vif_index.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::join_prune_notify_routing(const string& module_instance_name,
					x_module_id module_id,
					uint16_t vif_index,
					const IPvX& source,
					const IPvX& group,
					action_jp_t action_jp)
{
    //
    // Send add/delete membership to the registered protocol instance.
    //
    switch (action_jp) {
    case ACTION_JOIN:
	send_add_membership(module_instance_name, module_id,
			    vif_index, source, group);
	break;
    case ACTION_PRUNE:
	send_delete_membership(module_instance_name, module_id, 
			       vif_index, source, group);
	break;
    default:
	XLOG_ASSERT(false);
	break;
    }
    
    return (XORP_OK);
}
