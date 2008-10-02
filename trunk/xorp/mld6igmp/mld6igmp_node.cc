// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_node.cc,v 1.59 2008/07/23 05:11:04 pavlin Exp $"


//
// Multicast Listener Discovery and Internet Group Management Protocol
// node implementation (common part).
// IGMPv1 and IGMPv2 (RFC 2236), IGMPv3 (RFC 3376),
// MLDv1 (RFC 2710), and MLDv2 (RFC 3810).
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

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
      _is_log_trace(false)
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
    if (! is_enabled())
	return (XORP_OK);

    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_STARTING)
	|| (ServiceBase::status() == SERVICE_RUNNING)) {
	return (XORP_OK);
    }
    if (ServiceBase::status() != SERVICE_READY) {
	return (XORP_ERROR);
    }

    if (ProtoNode<Mld6igmpVif>::pending_start() != XORP_OK)
	return (XORP_ERROR);

    //
    // Register with the FEA and MFEA
    //
    fea_register_startup();
    mfea_register_startup();

    //
    // Set the node status
    //
    ProtoNode<Mld6igmpVif>::set_node_status(PROC_STARTUP);

    //
    // Update the node status
    //
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

    if (ProtoNode<Mld6igmpVif>::start() != XORP_OK) {
	ProtoNode<Mld6igmpVif>::stop();
	return (XORP_ERROR);
    }

    // Start the mld6igmp_vifs
    start_all_vifs();

    XLOG_INFO("Protocol started");

    return (XORP_OK);
}

/**
 * Mld6igmpNode::stop:
 * @: 
 * 
 * Gracefully stop the MLD or IGMP protocol.
 * XXX: After the cleanup is completed,
 * Mld6igmpNode::final_stop() is called to complete the job.
 * XXX: This function, unlike start(), will stop the protocol
 * operation on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::stop()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_SHUTDOWN)
	|| (ServiceBase::status() == SERVICE_SHUTTING_DOWN)
	|| (ServiceBase::status() == SERVICE_FAILED)) {
	return (XORP_OK);
    }
    if ((ServiceBase::status() != SERVICE_RUNNING)
	&& (ServiceBase::status() != SERVICE_STARTING)
	&& (ServiceBase::status() != SERVICE_PAUSING)
	&& (ServiceBase::status() != SERVICE_PAUSED)
	&& (ServiceBase::status() != SERVICE_RESUMING)) {
	return (XORP_ERROR);
    }

    if (ProtoNode<Mld6igmpVif>::pending_stop() != XORP_OK)
	return (XORP_ERROR);

    //
    // Perform misc. MLD6IGMP-specific stop operations
    //
    // XXX: nothing to do
    
    // Stop the vifs
    stop_all_vifs();
    
    //
    // Set the node status
    //
    ProtoNode<Mld6igmpVif>::set_node_status(PROC_SHUTDOWN);

    //
    // Update the node status
    //
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

    if (ProtoNode<Mld6igmpVif>::stop() != XORP_OK)
	return (XORP_ERROR);

    XLOG_INFO("Protocol stopped");

    return (XORP_OK);
}

/**
 * Enable the node operation.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
void
Mld6igmpNode::enable()
{
    ProtoUnit::enable();

    XLOG_INFO("Protocol enabled");
}

/**
 * Disable the node operation.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
void
Mld6igmpNode::disable()
{
    stop();
    ProtoUnit::disable();

    XLOG_INFO("Protocol disabled");
}

/**
 * Get the IP protocol number.
 *
 * @return the IP protocol number.
 */
uint8_t
Mld6igmpNode::ip_protocol_number() const
{
    if (proto_is_igmp())
	return (IPPROTO_IGMP);

    if (proto_is_mld6())
	return (IPPROTO_ICMPV6);

    XLOG_UNREACHABLE();
    return (0);
}

void
Mld6igmpNode::status_change(ServiceBase*  service,
			    ServiceStatus old_status,
			    ServiceStatus new_status)
{
    if (service == this) {
	if ((old_status == SERVICE_STARTING)
	    && (new_status == SERVICE_RUNNING)) {
	    // The startup process has completed
	    if (final_start() != XORP_OK) {
		XLOG_ERROR("Cannot complete the startup process; "
			   "current state is %s",
			   ProtoNode<Mld6igmpVif>::state_str().c_str());
		return;
	    }
	    ProtoNode<Mld6igmpVif>::set_node_status(PROC_READY);
	    return;
	}

	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    // The shutdown process has completed
	    final_stop();
	    // Set the node status
	    ProtoNode<Mld6igmpVif>::set_node_status(PROC_DONE);
	    return;
	}

	//
	// TODO: check if there was an error
	//
	return;
    }

    if (service == ifmgr_mirror_service_base()) {
	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    decr_shutdown_requests_n();			// XXX: for the ifmgr
	}
    }
}

void
Mld6igmpNode::tree_complete()
{
    decr_startup_requests_n();				// XXX: for the ifmgr

    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

void
Mld6igmpNode::updates_made()
{
    map<string, Vif>::iterator mld6igmp_vif_iter;
    string error_msg;

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();

    //
    // Add new vifs and update existing ones
    //
    IfMgrIfTree::IfMap::const_iterator ifmgr_iface_iter;
    for (ifmgr_iface_iter = _iftree.interfaces().begin();
	 ifmgr_iface_iter != _iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;

	IfMgrIfAtom::VifMap::const_iterator ifmgr_vif_iter;
	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();
	    Vif* node_vif = NULL;
	
	    mld6igmp_vif_iter = configured_vifs().find(ifmgr_vif_name);
	    if (mld6igmp_vif_iter != configured_vifs().end()) {
		node_vif = &(mld6igmp_vif_iter->second);
	    }

	    //
	    // Add a new vif
	    //
	    if (node_vif == NULL) {
		uint32_t vif_index = ifmgr_vif.vif_index();
		XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
		if (add_config_vif(ifmgr_vif_name, vif_index, error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot add vif %s to the set of configured "
			       "vifs: %s",
			       ifmgr_vif_name.c_str(), error_msg.c_str());
		    continue;
		}
		mld6igmp_vif_iter = configured_vifs().find(ifmgr_vif_name);
		XLOG_ASSERT(mld6igmp_vif_iter != configured_vifs().end());
		node_vif = &(mld6igmp_vif_iter->second);
		// FALLTHROUGH
	    }

	    //
	    // Update the pif_index
	    //
	    set_config_pif_index(ifmgr_vif_name,
				 ifmgr_vif.pif_index(),
				 error_msg);
	
	    //
	    // Update the vif flags
	    //
	    bool is_up = ifmgr_iface.enabled();
	    is_up &= (! ifmgr_iface.no_carrier());
	    is_up &= ifmgr_vif.enabled();
	    set_config_vif_flags(ifmgr_vif_name,
				 ifmgr_vif.pim_register(),
				 ifmgr_vif.p2p_capable(),
				 ifmgr_vif.loopback(),
				 ifmgr_vif.multicast_capable(),
				 ifmgr_vif.broadcast_capable(),
				 is_up,
				 ifmgr_iface.mtu(),
				 error_msg);
	
	}
    }

    //
    // Add new vif addresses, update existing ones, and remove old addresses
    //
    for (ifmgr_iface_iter = _iftree.interfaces().begin();
	 ifmgr_iface_iter != _iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;
	const string& ifmgr_iface_name = ifmgr_iface.name();
	IfMgrIfAtom::VifMap::const_iterator ifmgr_vif_iter;

	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();
	    Vif* node_vif = NULL;

	    //
	    // Add new vif addresses and update existing ones
	    //
	    mld6igmp_vif_iter = configured_vifs().find(ifmgr_vif_name);
	    if (mld6igmp_vif_iter != configured_vifs().end()) {
		node_vif = &(mld6igmp_vif_iter->second);
	    }

	    if (is_ipv4()) {
		IfMgrVifAtom::IPv4Map::const_iterator a4_iter;

		for (a4_iter = ifmgr_vif.ipv4addrs().begin();
		     a4_iter != ifmgr_vif.ipv4addrs().end();
		     ++a4_iter) {
		    const IfMgrIPv4Atom& a4 = a4_iter->second;
		    VifAddr* node_vif_addr = node_vif->find_address(IPvX(a4.addr()));
		    IPvX addr(a4.addr());
		    IPvXNet subnet_addr(addr, a4.prefix_len());
		    IPvX broadcast_addr(IPvX::ZERO(family()));
		    IPvX peer_addr(IPvX::ZERO(family()));
		    if (a4.has_broadcast())
			broadcast_addr = IPvX(a4.broadcast_addr());
		    if (a4.has_endpoint())
			peer_addr = IPvX(a4.endpoint_addr());

		    if (node_vif_addr == NULL) {
			if (add_config_vif_addr(
				ifmgr_vif_name,
				addr,
				subnet_addr,
				broadcast_addr,
				peer_addr,
				error_msg)
			    != XORP_OK) {
			    XLOG_ERROR("Cannot add address %s to vif %s from "
				       "the set of configured vifs: %s",
				       cstring(addr), ifmgr_vif_name.c_str(),
				       error_msg.c_str());
			}
			continue;
		    }
		    if ((addr == node_vif_addr->addr())
			&& (subnet_addr == node_vif_addr->subnet_addr())
			&& (broadcast_addr == node_vif_addr->broadcast_addr())
			&& (peer_addr == node_vif_addr->peer_addr())) {
			continue;	// Nothing changed
		    }

		    // Update the address
		    if (delete_config_vif_addr(ifmgr_vif_name,
					       addr,
					       error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s from vif %s "
				   "from the set of configured vifs: %s",
				   cstring(addr),
				   ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		    if (add_config_vif_addr(
			    ifmgr_vif_name,
			    addr,
			    subnet_addr,
			    broadcast_addr,
			    peer_addr,
			    error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot add address %s to vif %s from "
				   "the set of configured vifs: %s",
				   cstring(addr), ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}
	    }

	    if (is_ipv6()) {
		IfMgrVifAtom::IPv6Map::const_iterator a6_iter;

		for (a6_iter = ifmgr_vif.ipv6addrs().begin();
		     a6_iter != ifmgr_vif.ipv6addrs().end();
		     ++a6_iter) {
		    const IfMgrIPv6Atom& a6 = a6_iter->second;
		    VifAddr* node_vif_addr = node_vif->find_address(IPvX(a6.addr()));
		    IPvX addr(a6.addr());
		    IPvXNet subnet_addr(addr, a6.prefix_len());
		    IPvX broadcast_addr(IPvX::ZERO(family()));
		    IPvX peer_addr(IPvX::ZERO(family()));
		    if (a6.has_endpoint())
			peer_addr = IPvX(a6.endpoint_addr());

		    if (node_vif_addr == NULL) {
			if (add_config_vif_addr(
				ifmgr_vif_name,
				addr,
				subnet_addr,
				broadcast_addr,
				peer_addr,
				error_msg)
			    != XORP_OK) {
			    XLOG_ERROR("Cannot add address %s to vif %s from "
				       "the set of configured vifs: %s",
				       cstring(addr), ifmgr_vif_name.c_str(),
				       error_msg.c_str());
			}
			continue;
		    }
		    if ((addr == node_vif_addr->addr())
			&& (subnet_addr == node_vif_addr->subnet_addr())
			&& (peer_addr == node_vif_addr->peer_addr())) {
			continue;	// Nothing changed
		    }

		    // Update the address
		    if (delete_config_vif_addr(ifmgr_vif_name,
					       addr,
					       error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s from vif %s "
				   "from the set of configured vifs: %s",
				   cstring(addr),
				   ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		    if (add_config_vif_addr(
			    ifmgr_vif_name,
			    addr,
			    subnet_addr,
			    broadcast_addr,
			    peer_addr,
			    error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot add address %s to vif %s from "
				   "the set of configured vifs: %s",
				   cstring(addr), ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}
	    }

	    //
	    // Delete vif addresses that don't exist anymore
	    //
	    {
		list<IPvX> delete_addresses_list;
		list<VifAddr>::const_iterator vif_addr_iter;
		for (vif_addr_iter = node_vif->addr_list().begin();
		     vif_addr_iter != node_vif->addr_list().end();
		     ++vif_addr_iter) {
		    const VifAddr& vif_addr = *vif_addr_iter;
		    if (vif_addr.addr().is_ipv4()
			&& (_iftree.find_addr(ifmgr_iface_name,
					      ifmgr_vif_name,
					      vif_addr.addr().get_ipv4()))
			    == NULL) {
			    delete_addresses_list.push_back(vif_addr.addr());
		    }
		    if (vif_addr.addr().is_ipv6()
			&& (_iftree.find_addr(ifmgr_iface_name,
					      ifmgr_vif_name,
					      vif_addr.addr().get_ipv6()))
			    == NULL) {
			    delete_addresses_list.push_back(vif_addr.addr());
		    }
		}

		// Delete the addresses
		list<IPvX>::iterator ipvx_iter;
		for (ipvx_iter = delete_addresses_list.begin();
		     ipvx_iter != delete_addresses_list.end();
		     ++ipvx_iter) {
		    const IPvX& ipvx = *ipvx_iter;
		    if (delete_config_vif_addr(ifmgr_vif_name, ipvx, error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s from vif %s from "
				   "the set of configured vifs: %s",
				   cstring(ipvx), ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}
	    }
	}
    }

    //
    // Remove vifs that don't exist anymore
    //
    list<string> delete_vifs_list;
    for (mld6igmp_vif_iter = configured_vifs().begin();
	 mld6igmp_vif_iter != configured_vifs().end();
	 ++mld6igmp_vif_iter) {
	Vif* node_vif = &mld6igmp_vif_iter->second;
#if 0
	if (node_vif->is_pim_register())
	    continue;		// XXX: don't delete the PIM Register vif
#endif
	if (_iftree.find_vif(node_vif->name(), node_vif->name()) == NULL) {
	    // Add the vif to the list of old interfaces
	    delete_vifs_list.push_back(node_vif->name());
	}
    }
    // Delete the old vifs
    list<string>::iterator vif_name_iter;
    for (vif_name_iter = delete_vifs_list.begin();
	 vif_name_iter != delete_vifs_list.end();
	 ++vif_name_iter) {
	const string& vif_name = *vif_name_iter;
	if (delete_config_vif(vif_name, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot delete vif %s from the set of configured "
		       "vifs: %s",
		       vif_name.c_str(), error_msg.c_str());
	}
    }
    
    // Done
    set_config_all_vifs_done(error_msg);
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
	XLOG_ERROR("%s", error_msg.c_str());
	
	delete mld6igmp_vif;
	return (XORP_ERROR);
    }

    //
    // Update and check the primary address
    //
    do {
	if (mld6igmp_vif->update_primary_address(error_msg) == XORP_OK)
	    break;
	if (mld6igmp_vif->addr_ptr() == NULL) {
	    // XXX: don't print an error if the vif has no addresses
	    break;
	}
	if (mld6igmp_vif->is_loopback() || mld6igmp_vif->is_pim_register()) {
	    // XXX: don't print an error if this is a loopback or register_vif
	    break;
	}
	XLOG_ERROR("Error updating primary address for vif %s: %s",
		   mld6igmp_vif->name().c_str(), error_msg.c_str());
	return (XORP_ERROR);
    } while (false);

    XLOG_INFO("Interface added: %s", mld6igmp_vif->str().c_str());
    
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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (ProtoNode<Mld6igmpVif>::delete_vif(mld6igmp_vif) != XORP_OK) {
	error_msg = c_format("Cannot delete vif %s: internal error",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	delete mld6igmp_vif;
	return (XORP_ERROR);
    }
    
    delete mld6igmp_vif;
    
    XLOG_INFO("Interface deleted: %s", vif_name.c_str());
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_flags(const string& vif_name,
			    bool is_pim_register, bool is_p2p,
			    bool is_loopback, bool is_multicast,
			    bool is_broadcast, bool is_up, uint32_t mtu,
			    string& error_msg)
{
    bool is_changed = false;
    
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot set flags vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
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
    if (mld6igmp_vif->mtu() != mtu) {
	mld6igmp_vif->set_mtu(mtu);
	is_changed = true;
    }
    
    if (is_changed)
	XLOG_INFO("Interface flags changed: %s", mld6igmp_vif->str().c_str());
    
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
	XLOG_ERROR("%s", error_msg.c_str());
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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if ((addr.af() != family())
	|| (subnet_addr.af() != family())
	|| (broadcast_addr.af() != family())
	|| (peer_addr.af() != family())) {
	error_msg = c_format("Cannot add address on vif %s: "
			     "invalid address family: %s ",
			     vif_name.c_str(), vif_addr.str().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
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

    //
    // Update and check the primary address
    //
    do {
	if (mld6igmp_vif->update_primary_address(error_msg) == XORP_OK)
	    break;
	if (! (mld6igmp_vif->is_up() || mld6igmp_vif->is_pending_up())) {
	    // XXX: print an error only if the interface is UP or PENDING_UP
	    break;
	}
	if (mld6igmp_vif->is_loopback() || mld6igmp_vif->is_pim_register()) {
	    // XXX: don't print an error if this is a loopback or register_vif
	    break;
	}
	XLOG_ERROR("Error updating primary address for vif %s: %s",
		   mld6igmp_vif->name().c_str(), error_msg.c_str());
	return (XORP_ERROR);
    } while (false);

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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr *tmp_vif_addr = mld6igmp_vif->find_address(addr);
    if (tmp_vif_addr == NULL) {
	error_msg = c_format("Cannot delete address on vif %s: "
			     "invalid address %s",
			     vif_name.c_str(), addr.str().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    VifAddr vif_addr = *tmp_vif_addr;	// Get a copy

    //
    // Get the vif's old primary address and whether the vif is UP
    //
    bool old_vif_is_up = mld6igmp_vif->is_up() || mld6igmp_vif->is_pending_up();
    IPvX old_primary_addr = mld6igmp_vif->primary_addr();
    
    //
    // If an interface's primary address is deleted, first stop the vif.
    //
    if (old_vif_is_up) {
	if (mld6igmp_vif->primary_addr() == addr) {
	    string dummy_error_msg;
	    mld6igmp_vif->stop(dummy_error_msg);
	}
    }

    if (mld6igmp_vif->delete_address(addr) != XORP_OK) {
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Deleted address on interface %s: %s",
	      mld6igmp_vif->name().c_str(), vif_addr.str().c_str());
    
    //
    // Update and check the primary address.
    // If the vif has no primary address, then stop it.
    // If the vif's primary address was changed, then restart the vif.
    //
    do {
	string dummy_error_msg;

	if (mld6igmp_vif->update_primary_address(error_msg) != XORP_OK) {
	    XLOG_ERROR("Error updating primary address for vif %s: %s",
		       mld6igmp_vif->name().c_str(), error_msg.c_str());
	}
	if (mld6igmp_vif->primary_addr().is_zero()) {
	    mld6igmp_vif->stop(dummy_error_msg);
	    break;
	}
	if (old_primary_addr == mld6igmp_vif->primary_addr())
	    break;		// Nothing changed

	// Conditionally restart the interface
	mld6igmp_vif->stop(dummy_error_msg);
	if (old_vif_is_up)
	    mld6igmp_vif->start(dummy_error_msg);
	break;
    } while (false);

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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->enable();
    
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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->disable();
    
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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->start(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot start vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
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
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->stop(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot stop vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * Mld6igmpNode::start_all_vifs:
 * @: 
 * 
 * Start MLD/IGMP on all enabled interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::start_all_vifs()
{
    vector<Mld6igmpVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (start_vif(mld6igmp_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * Mld6igmpNode::stop_all_vifs:
 * @: 
 * 
 * Stop MLD/IGMP on all interfaces it was running on.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpNode::stop_all_vifs()
{
    vector<Mld6igmpVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (stop_vif(mld6igmp_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
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
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (enable_vif(mld6igmp_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
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
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = (*iter);
	if (mld6igmp_vif == NULL)
	    continue;
	if (disable_vif(mld6igmp_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
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
	string error_msg;
	if (delete_vif(vif_name, error_msg) != XORP_OK) {
	    error_msg = c_format("Cannot delete vif %s: internal error",
				 vif_name.c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	}
    }
}

/**
 * A method called when a vif has completed its shutdown.
 * 
 * @param vif_name the name of the vif that has completed its shutdown.
 */
void
Mld6igmpNode::vif_shutdown_completed(const string& vif_name)
{
    vector<Mld6igmpVif *>::iterator iter;

    //
    // If all vifs have completed the shutdown, then de-register with
    // the MFEA.
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	Mld6igmpVif *mld6igmp_vif = *iter;
	if (mld6igmp_vif == NULL)
	    continue;
	if (! mld6igmp_vif->is_down())
	    return;
    }

    //
    // De-register with the FEA and MFEA
    //
    if (ServiceBase::status() == SERVICE_SHUTTING_DOWN) {
	mfea_register_shutdown();
	fea_register_shutdown();
    }

    UNUSED(vif_name);
}

int
Mld6igmpNode::proto_recv(const string& if_name,
			 const string& vif_name,
			 const IPvX& src_address,
			 const IPvX& dst_address,
			 uint8_t ip_protocol,
			 int32_t ip_ttl,
			 int32_t ip_tos,
			 bool ip_router_alert,
			 bool ip_internet_control,
			 const vector<uint8_t>& payload,
			 string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = NULL;
    int ret_value = XORP_ERROR;
    
    debug_msg("Received message on %s/%s from %s to %s: "
	      "ip_ttl = %d ip_tos = %#x ip_router_alert = %d "
	      "ip_internet_control = %d rcvlen = %u\n",
	      if_name.c_str(), vif_name.c_str(),
	      cstring(src_address), cstring(dst_address),
	      ip_ttl, ip_tos, ip_router_alert, ip_internet_control,
	      XORP_UINT_CAST(payload.size()));

    UNUSED(if_name);
    //
    // XXX: We registered to receive only one protocol, hence we ignore
    // the ip_protocol value.
    //
    UNUSED(ip_protocol);

    //
    // Check whether the node is up.
    //
    if (! is_up()) {
	error_msg = c_format("MLD/IGMP node is not UP");
	return (XORP_ERROR);
    }
    
    //
    // Find the vif for that packet
    //
    mld6igmp_vif = vif_find_by_name(vif_name);
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot find vif with vif_name = %s",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    // Copy the data to the receiving #buffer_t
    BUFFER_RESET(_buffer_recv);
    BUFFER_PUT_DATA(&payload[0], _buffer_recv, payload.size());
    
    // Process the data by the vif
    ret_value = mld6igmp_vif->mld6igmp_recv(src_address, dst_address,
					    ip_ttl, ip_tos,
					    ip_router_alert,
					    ip_internet_control,
					    _buffer_recv,
					    error_msg);
    
    return (ret_value);
    
 buflen_error:
    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

int
Mld6igmpNode::mld6igmp_send(const string& if_name,
			    const string& vif_name,
			    const IPvX& src_address,
			    const IPvX& dst_address,
			    uint8_t ip_protocol,
			    int32_t ip_ttl,
			    int32_t ip_tos,
			    bool ip_router_alert,
			    bool ip_internet_control,
			    buffer_t *buffer,
			    string& error_msg)
{
    if (! is_up()) {
	error_msg = c_format("MLD/IGMP node is not UP");
	return (XORP_ERROR);
    }
    
    if (proto_send(if_name, vif_name, src_address, dst_address,
		   ip_protocol, ip_ttl, ip_tos, ip_router_alert,
		   ip_internet_control,
		   BUFFER_DATA_HEAD(buffer),
		   BUFFER_DATA_SIZE(buffer),
		   error_msg)
	!= XORP_OK) {
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
			   uint32_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot add protocol instance %s on vif_index %d: "
		   "no such vif",
		   module_instance_name.c_str(), vif_index);
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->add_protocol(module_id, module_instance_name) != XORP_OK)
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
			      uint32_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot delete protocol instance %s on vif_index %d: "
		   "no such vif",
		   module_instance_name.c_str(), vif_index);
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->delete_protocol(module_id, module_instance_name)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
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
					uint32_t vif_index,
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

/**
 * Mld6igmpNode::is_directly_connected:
 * @mld6igmp_vif: The virtual interface to test against.
 * @ipaddr_test: The address to test.
 * 
 * Note that the virtual interface the address is directly connected to
 * must be UP.
 * 
 * Return value: True if @ipaddr_test is directly connected to @mld6igmp_vif,
 * otherwise false.
 **/
bool
Mld6igmpNode::is_directly_connected(const Mld6igmpVif& mld6igmp_vif,
				    const IPvX& ipaddr_test) const
{
    if (! mld6igmp_vif.is_up())
	return (false);

#if 0	// TODO: not implemented yet
    //
    // Test the alternative subnets
    //
    list<IPvXNet>::const_iterator iter;
    for (iter = mld6igmp_vif.alternative_subnet_list().begin();
	 iter != mld6igmp_vif.alternative_subnet_list().end();
	 ++iter) {
	const IPvXNet& ipvxnet = *iter;
	if (ipvxnet.contains(ipaddr_test))
	    return true;
    }
#endif

    //
    // Test the same subnet addresses, or the P2P addresses
    //
    return (mld6igmp_vif.is_same_subnet(ipaddr_test)
	    || mld6igmp_vif.is_same_p2p(ipaddr_test));
}
