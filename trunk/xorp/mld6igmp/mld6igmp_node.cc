// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_node.cc,v 1.29 2004/06/09 11:22:13 pavlin Exp $"


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
 * @module_id: The module ID (must be %XORP_MODULE_MLD6IGMP).
 * @eventloop: The event loop.
 * 
 * MLD6IGMP node constructor.
 **/
Mld6igmpNode::Mld6igmpNode(int family, xorp_module_id module_id,
			   EventLoop& eventloop)
    : ProtoNode<Mld6igmpVif>(family, module_id, eventloop),
      _is_log_trace(true)		// XXX: default to print trace logs
{
    XLOG_ASSERT(module_id == XORP_MODULE_MLD6IGMP);
    if (module_id != XORP_MODULE_MLD6IGMP) {
	XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_MLD6IGMP' = %d)",
		   module_id, XORP_MODULE_MLD6IGMP);
    }
    
    _buffer_recv = BUFFER_MALLOC(BUF_SIZE_DEFAULT);

    //
    // Set the node status
    //
    ProtoNode<Mld6igmpVif>::set_node_status(PROC_STARTUP);

    //
    // Set myself as an observer when the node status changes
    //
    set_observer(this);
}

/**
 * Mld6igmpNode::~Mld6igmpNode:
 * @: 
 * 
 * MLD6IGMP node destructor.
 * 
 **/
Mld6igmpNode::~Mld6igmpNode()
{
    //
    // Unset myself as an observer when the node status changes
    //
    unset_observer(this);

    stop();

    ProtoNode<Mld6igmpVif>::set_node_status(PROC_NULL);

    delete_all_vifs();
    
    BUFFER_FREE(_buffer_recv);
}

/**
 * Mld6igmpNode::start:
 * @: 
 * 
 * Start the MLD or IGMP protocol.
 * TODO: This function should not start the protocol operation on the
 * interfaces. The interfaces must be activated separately.
 * After the startup operations are completed,
 * Mld6igmpNode::final_start() is called to complete the job.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
Mld6igmpNode::start()
{
    if (is_up() || is_pending_up())
	return (XORP_OK);

    if (ProtoNode<Mld6igmpVif>::pending_start() < 0)
	return (XORP_ERROR);

    //
    // Register with the MFEA
    //
    mfea_register_startup();

    //
    // Set the node status
    //
    ProtoNode<Mld6igmpVif>::set_node_status(PROC_STARTUP);
    update_status();

    return (XORP_OK);
}

/**
 * Mld6igmpNode::final_start:
 * @: 
 * 
 * Complete the start-up of the MLD/IGMP protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::final_start()
{
#if 0	// TODO: XXX: PAVPAVPAV
    if (! is_pending_up())
	return (XORP_ERROR);
#endif

    if (ProtoNode<Mld6igmpVif>::start() < 0) {
	ProtoNode<Mld6igmpVif>::stop();
	return (XORP_ERROR);
    }

    // Start the mld6igmp_vifs
    start_all_vifs();

    return (XORP_OK);
}

/**
 * Mld6igmpNode::stop:
 * @: 
 * 
 * Gracefully stop the MLD or IGMP protocol.
 * XXX: This function, unlike start(), will stop the protocol
 * operation on all interfaces.
 * XXX: After the cleanup is completed,
 * Mld6igmpNode::final_stop() is called to complete the job.
 * XXX: If this method is called one-after-another, the second one
 * will force calling immediately Mld6igmpNode::final_stop() to quickly
 * finish the job. TODO: is this a desired semantic?
 * XXX: This function, unlike start(), will stop the protocol
 * operation on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::stop()
{
    if (is_down())
	return (XORP_OK);

    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
    
    //
    // Perform misc. MLD6IGMP-specific stop operations
    //
    // XXX: nothing to do
    
    // Stop the vifs
    stop_all_vifs();
    
    if (ProtoNode<Mld6igmpVif>::pending_stop() < 0)
	return (XORP_ERROR);

    //
    // De-register with the MFEA
    //
    mfea_register_shutdown();

    //
    // Set the node status
    //
    ProtoNode<Mld6igmpVif>::set_node_status(PROC_SHUTDOWN);
    update_status();

    return (XORP_OK);
}

/**
 * Mld6igmpNode::final_stop:
 * @: 
 * 
 * Completely stop the MLD/IGMP protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::final_stop()
{
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);

    if (ProtoNode<Mld6igmpVif>::stop() < 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

/**
 * Mld6igmpNode::has_pending_down_units:
 * @reason_msg: return-by-reference string that contains human-readable
 * information about the status.
 * 
 * Test if there is an unit that is in PENDING_DOWN state.
 * 
 * Return value: True if there is an unit that is in PENDING_DOWN state,
 * otherwise false.
 **/
bool
Mld6igmpNode::has_pending_down_units(string& reason_msg)
{
    vector<Mld6igmpVif *>::iterator iter;
    
    //
    // Test the interfaces
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	// TODO: XXX: PAVPAVPAV: vif pending-down state
	// is not used/implemented yet
	if (mld6igmp_vif->is_pending_down()) {
	    reason_msg = c_format("Vif %s is in state %s",
				  mld6igmp_vif->name().c_str(),
				  mld6igmp_vif->state_str().c_str());
	    return (true);
	}
    }
    
    //
    // TODO: XXX: PAVPAVPAV: test other units that may be waiting
    // in PENDING_DOWN state.
    //
    
    reason_msg = "No pending-down units";
    return (false);
}

void
Mld6igmpNode::status_change(ServiceBase*  service,
			    ServiceStatus old_status,
			    ServiceStatus new_status)
{
    XLOG_ASSERT(this == service);

    if ((old_status == STARTING) && (new_status == RUNNING)) {
	// The startup process has completed
	if (final_start() < 0) {
	    XLOG_ERROR("Cannot complete the startup process; "
		       "current state is %s",
		       ProtoNode<Mld6igmpVif>::state_str().c_str());
	    return;
	}
	ProtoNode<Mld6igmpVif>::set_node_status(PROC_READY);
	return;
    }

    if ((old_status == SHUTTING_DOWN) && (new_status == SHUTDOWN)) {
	// The shutdown process has completed
	final_stop();
	// Set the node status
	ProtoNode<Mld6igmpVif>::set_node_status(PROC_DONE);
	return;
    }

    //
    // TODO: check if there was an error
    //
}

/**
 * Mld6igmpNode::add_vif:
 * @vif: Information about the new Mld6igmpVif to install.
 * @error_msg: The error message (if error).
 * 
 * Install a new MLD/IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::add_vif(const Vif& vif, string& error_msg)
{
    //
    // Create a new Mld6igmpVif
    //
    Mld6igmpVif *mld6igmp_vif = new Mld6igmpVif(*this, vif);
    
    if (ProtoNode<Mld6igmpVif>::add_vif(mld6igmp_vif) != XORP_OK) {
	// Cannot add this new vif
	error_msg = c_format("Cannot add vif %s: internal error",
			     vif.name().c_str());
	XLOG_ERROR(error_msg.c_str());
	
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
 * @error_msg: The error message (if error).
 * 
 * Install a new MLD/IGMP vif. If the vif exists, nothing is installed.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::add_vif(const string& vif_name, uint32_t vif_index,
		      string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(vif_index);
    
    if ((mld6igmp_vif != NULL) && (mld6igmp_vif->name() == vif_name)) {
	return (XORP_OK);		// Already have this vif
    }
    
    //
    // Create a new Vif
    //
    Vif vif(vif_name);
    vif.set_vif_index(vif_index);
    if (add_vif(vif, error_msg) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::delete_vif:
 * @vif_name: The name of the vif to delete.
 * @error_msg: The error message (if error).
 * 
 * Delete an existing MLD/IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::delete_vif(const string& vif_name, string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot delete vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (ProtoNode<Mld6igmpVif>::delete_vif(mld6igmp_vif) != XORP_OK) {
	error_msg = c_format("Cannot delete vif %s: internal error",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	delete mld6igmp_vif;
	return (XORP_ERROR);
    }
    
    delete mld6igmp_vif;
    
    XLOG_INFO("Deleted vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_flags(const string& vif_name,
			    bool is_pim_register, bool is_p2p,
			    bool is_loopback, bool is_multicast,
			    bool is_broadcast, bool is_up,
			    string& error_msg)
{
    bool is_changed = false;
    
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot set flags vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
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
Mld6igmpNode::add_vif_addr(const string& vif_name,
			   const IPvX& addr,
			   const IPvXNet& subnet_addr,
			   const IPvX& broadcast_addr,
			   const IPvX& peer_addr,
			   string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot add address on vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr vif_addr(addr, subnet_addr, broadcast_addr, peer_addr);
    
    //
    // Check the arguments
    //
    if (! addr.is_unicast()) {
	error_msg = c_format("Cannot add address on vif %s: "
			     "invalid unicast address: %s",
			     vif_name.c_str(), addr.str().c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    if ((addr.af() != family())
	|| (subnet_addr.af() != family())
	|| (broadcast_addr.af() != family())
	|| (peer_addr.af() != family())) {
	error_msg = c_format("Cannot add address on vif %s: "
			     "invalid address family: %s ",
			     vif_name.c_str(), vif_addr.str().c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    VifAddr* node_vif_addr = mld6igmp_vif->find_address(addr);

    if ((node_vif_addr != NULL) && (*node_vif_addr == vif_addr))
	return (XORP_OK);		// Already have this address

    //
    // TODO: If an interface changes its primary IP address, then
    // we should do something about it.
    //
    // However, by adding or updating an existing address we cannot
    // change a valid primary address, hence we do nothing here.
    //
    
    if (node_vif_addr != NULL) {
	// Update the address
	XLOG_INFO("Updated existing address on vif %s: old is %s new is %s",
		  mld6igmp_vif->name().c_str(), node_vif_addr->str().c_str(),
		  vif_addr.str().c_str());
	*node_vif_addr = vif_addr;
    } else {
	// Add a new address
	mld6igmp_vif->add_address(vif_addr);
	
	XLOG_INFO("Added new address to vif %s: %s",
		  mld6igmp_vif->name().c_str(), vif_addr.str().c_str());
    }
    
    return (XORP_OK);
}

int
Mld6igmpNode::delete_vif_addr(const string& vif_name,
			      const IPvX& addr,
			      string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot delete address on vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr *tmp_vif_addr = mld6igmp_vif->find_address(addr);
    if (tmp_vif_addr == NULL) {
	error_msg = c_format("Cannot delete address on vif %s: "
			     "invalid address %s",
			     vif_name.c_str(), addr.str().c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // TODO: If an interface changes its primary IP address, then
    // we should do something about it.
    //
    if (mld6igmp_vif->is_up()) {
	if (mld6igmp_vif->primary_addr() == addr) {
	    // TODO: do something. Maybe stop the vif now?
	}
    }

    VifAddr vif_addr = *tmp_vif_addr;	// Get a copy
    if (mld6igmp_vif->delete_address(addr) != XORP_OK) {
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Deleted address on vif %s: %s",
	      mld6igmp_vif->name().c_str(), vif_addr.str().c_str());
    
    //
    // Update the primary address
    // If the vif has no more primary address, then stop it.
    //
    mld6igmp_vif->update_primary_address(error_msg);
    if (mld6igmp_vif->is_up()) {
	// Check the primary address
	if (mld6igmp_vif->primary_addr() == IPvX::ZERO(family())) {
	    XLOG_ERROR("Cannot update primary address: %s",
		       error_msg.c_str());
	    mld6igmp_vif->stop(error_msg);
	}
    }

    return (XORP_OK);
}

/**
 * Mld6igmpNode::enable_vif:
 * @vif_name: The name of the vif to enable.
 * @error_msg: The error message (if error).
 * 
 * Enable an existing MLD6IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::enable_vif(const string& vif_name, string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot enable vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->enable();
    
    XLOG_INFO("Enabled vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::disable_vif:
 * @vif_name: The name of the vif to disable.
 * @error_msg: The error message (if error).
 * 
 * Disable an existing MLD6IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::disable_vif(const string& vif_name, string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot disable vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->disable();
    
    XLOG_INFO("Disabled vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::start_vif:
 * @vif_name: The name of the vif to start.
 * @error_msg: The error message (if error).
 * 
 * Start an existing MLD6IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::start_vif(const string& vif_name, string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot start vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->start(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot start vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Started vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::stop_vif:
 * @vif_name: The name of the vif to stop.
 * @error_msg: The error message (if error).
 * 
 * Stop an existing MLD6IGMP vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::stop_vif(const string& vif_name, string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot stop vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->stop(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot stop vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Stopped vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::start_all_vifs:
 * @: 
 * 
 * Start MLD/IGMP on all enabled interfaces.
 * 
 * Return value: The number of virtual interfaces MLD/IGMP was started on,
 * or %XORP_ERROR if error occured.
 **/
int
Mld6igmpNode::start_all_vifs()
{
    int n = 0;
    vector<Mld6igmpVif *>::iterator iter;
    string error_msg;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (! mld6igmp_vif->is_enabled())
	    continue;
	if (mld6igmp_vif->start(error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot start vif %s: %s",
		       mld6igmp_vif->name().c_str(), error_msg.c_str());
	} else {
	    n++;
	}
    }
    
    return (n);
}

/**
 * Mld6igmpNode::stop_all_vifs:
 * @: 
 * 
 * Stop MLD/IGMP on all interfaces it was running on.
 * 
 * Return value: The number of virtual interfaces MLD/IGMP was stopped on,
 * or %XORP_ERROR if error occured.
 **/
int
Mld6igmpNode::stop_all_vifs()
{
    int n = 0;
    vector<Mld6igmpVif *>::iterator iter;
    string error_msg;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (mld6igmp_vif->stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot stop vif %s: %s",
		       mld6igmp_vif->name().c_str(), error_msg.c_str());
	} else {
	    n++;
	}
    }
    
    return (n);
}

/**
 * Mld6igmpNode::enable_all_vifs:
 * @: 
 * 
 * Enable MLD/IGMP on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::enable_all_vifs()
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
 * @: 
 * 
 * Disable MLD/IGMP on all interfaces. All running interfaces are stopped
 * first.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::disable_all_vifs()
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
 * @: 
 * 
 * Delete all MLD/IGMP vifs.
 **/
void
Mld6igmpNode::delete_all_vifs()
{
    list<string> vif_names;
    vector<Mld6igmpVif *>::iterator iter;

    //
    // Create the list of all vif names to delete
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif != NULL) {
	    string vif_name = mld6igmp_vif->name();
	    vif_names.push_back(mld6igmp_vif->name());
	}
    }

    //
    // Delete all vifs
    //
    list<string>::iterator vif_names_iter;
    for (vif_names_iter = vif_names.begin();
	 vif_names_iter != vif_names.end();
	 ++vif_names_iter) {
	const string& vif_name = *vif_names_iter;
	string err;
	if (delete_vif(vif_name, err) != XORP_OK) {
	    err = c_format("Cannot delete vif %s: internal error",
			   vif_name.c_str());
	    XLOG_ERROR(err.c_str());
	}
    }
}

/**
 * Mld6igmpNode::proto_recv:
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
 * @router_alert_bool: If true, the Router Alert IP option for the IP
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
			 xorp_module_id src_module_id,
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
    // Check whether the node is up.
    //
    if (! is_up())
	return (XORP_ERROR);
    
    //
    // Find the vif for that packet
    //
    mld6igmp_vif = vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL)
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
    XLOG_UNREACHABLE();
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
 * @router_alert_bool: If true, set the Router Alert IP option for the IP
 * packet of the outgoung message.
 * @buffer: The #buffer_t data buffer with the message to send.
 * 
 * Send a MLD or IGMP message.
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
 * Mld6igmpNode::add_protocol:
 * @module_instance_name: The module instance name of the protocol to add.
 * @module_id: The #xorp_module_id of the protocol to add.
 * @vif_index: The vif index of the interface to add the protocol to.
 * 
 * Add a protocol to the list of entries that would be notified if there
 * is membership change on a particular interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::add_protocol(const string& module_instance_name,
			   xorp_module_id module_id,
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
 * @module_id: The #xorp_module_id of the protocol to delete.
 * @vif_index: The vif index of the interface to delete the protocol from.
 * 
 * Delete a protocol from the list of entries that would be notified if there
 * is membership change on a particular interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::delete_protocol(const string& module_instance_name,
			      xorp_module_id module_id,
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
 * @module_id: The #xorp_module_id of the protocol to notify.
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
					xorp_module_id module_id,
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
	XLOG_UNREACHABLE();
	break;
    }
    
    return (XORP_OK);
}
