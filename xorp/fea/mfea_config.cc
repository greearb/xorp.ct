// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



//
// TODO: a temporary solution for various MFEA configuration
//

#include "mfea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mfea_node.hh"
#include "mfea_vif.hh"

/**
 * Add a configured vif.
 * 
 * @param vif the vif with the information to add.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::add_config_vif(const Vif& vif, string& error_msg)
{
    //
    // Perform all the micro-operations that are required to add a vif.
    //
    if (ProtoNode<MfeaVif>::add_config_vif(vif.name(), vif.vif_index(),
					   error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    list<VifAddr>::const_iterator vif_addr_iter;
    for (vif_addr_iter = vif.addr_list().begin();
	 vif_addr_iter != vif.addr_list().end();
	 ++vif_addr_iter) {
	const VifAddr& vif_addr = *vif_addr_iter;
	if (ProtoNode<MfeaVif>::add_config_vif_addr(vif.name(),
						    vif_addr.addr(),
						    vif_addr.subnet_addr(),
						    vif_addr.broadcast_addr(),
						    vif_addr.peer_addr(),
						    error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }
    if (ProtoNode<MfeaVif>::set_config_pif_index(vif.name(),
						 vif.pif_index(),
						 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    if (ProtoNode<MfeaVif>::set_config_vif_flags(vif.name(),
						 vif.is_pim_register(),
						 vif.is_p2p(),
						 vif.is_loopback(),
						 vif.is_multicast_capable(),
						 vif.is_broadcast_capable(),
						 vif.is_underlying_vif_up(),
						 vif.mtu(),
						 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * Complete the set of vif configuration changes.
 * 
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::set_config_all_vifs_done(string& error_msg)
{
    map<string, Vif>::iterator vif_iter;
    map<string, Vif>& configured_vifs = ProtoNode<MfeaVif>::configured_vifs();
    string dummy_error_msg;
    
    //
    // Add new vifs, and update existing ones
    //
    for (vif_iter = configured_vifs.begin();
	 vif_iter != configured_vifs.end();
	 ++vif_iter) {
	Vif* vif = &vif_iter->second;
	MfeaVif* node_vif = vif_find_by_name(vif->name());

	//
	// Add a new vif
	//
	if (node_vif == NULL) {
	    add_vif(*vif, dummy_error_msg);
	    continue;
	}

	//
	// Get the vif's old primary address and whether the vif is UP
	//
	bool old_vif_is_up = node_vif->is_up();
	IPvX old_primary_addr = IPvX::ZERO(family());
	if (node_vif->addr_ptr() != NULL)
	    old_primary_addr = *node_vif->addr_ptr();

	//
	// Update the vif flags
	//
	node_vif->set_p2p(vif->is_p2p());
	node_vif->set_loopback(vif->is_loopback());
	node_vif->set_multicast_capable(vif->is_multicast_capable());
	node_vif->set_broadcast_capable(vif->is_broadcast_capable());
	node_vif->set_underlying_vif_up(vif->is_underlying_vif_up());
	node_vif->set_mtu(vif->mtu());

	//
	// Add new vif addresses, and update existing ones
	//
	{
	    list<VifAddr>::const_iterator vif_addr_iter;
	    for (vif_addr_iter = vif->addr_list().begin();
		 vif_addr_iter != vif->addr_list().end();
		 ++vif_addr_iter) {
		const VifAddr& vif_addr = *vif_addr_iter;
		VifAddr* node_vif_addr = node_vif->find_address(vif_addr.addr());
		if (node_vif_addr == NULL) {
		    node_vif->add_address(vif_addr);
		    continue;
		}
		// Update the address
		if (*node_vif_addr != vif_addr) {
		    *node_vif_addr = vif_addr;
		}
	    }
	}

	//
	// Delete vif addresses that don't exist anymore.
	// If the vif's primary address is deleted, then first stop the vif.
	//
	{
	    list<IPvX> delete_addresses_list;
	    list<VifAddr>::const_iterator vif_addr_iter;
	    string dummy_error_msg;

	    for (vif_addr_iter = node_vif->addr_list().begin();
		 vif_addr_iter != node_vif->addr_list().end();
		 ++vif_addr_iter) {
		const VifAddr& vif_addr = *vif_addr_iter;
		if (vif->find_address(vif_addr.addr()) == NULL)
		    delete_addresses_list.push_back(vif_addr.addr());
	    }
	    // Delete the addresses
	    list<IPvX>::iterator ipvx_iter;
	    for (ipvx_iter = delete_addresses_list.begin();
		 ipvx_iter != delete_addresses_list.end();
		 ++ipvx_iter) {
		const IPvX& ipvx = *ipvx_iter;
		if (ipvx == old_primary_addr) {
		    // Stop the vif if the primary address is deleted
		    if (old_vif_is_up)
			node_vif->stop(dummy_error_msg, false, "primary addr deleted");
		}
		node_vif->delete_address(ipvx);
	    }
	}

	//
	// If the vif has no address then stop it.
	// If the vif's primary address was changed, then restart the vif.
	//
	do {
	    string dummy_error_msg;
	    if (node_vif->addr_ptr() == NULL) {
		node_vif->stop(dummy_error_msg, false, "null addr ptr");
		break;
	    }
	    if (old_primary_addr == *node_vif->addr_ptr())
		break;		// Nothing changed

	    // Conditionally restart the interface
	    node_vif->stop(dummy_error_msg, false, "del-addr, stop before possible restart");
	    if (old_vif_is_up)
		node_vif->start(dummy_error_msg, "restart after del-addr");
	    break;
	} while (false);

	// Notify updated...underlying vif enabled or similar might have changed.
	node_vif->notifyUpdated();
    }

    //
    // Remove vifs that don't exist anymore
    //
    for (uint32_t i = 0; i < maxvifs(); i++) {
	Vif* node_vif = vif_find_by_vif_index(i);
	if (node_vif == NULL)
	    continue;
	if (node_vif->is_pim_register())
	    continue;		// XXX: don't delete the PIM Register vif
	if (configured_vifs.find(node_vif->name()) == configured_vifs.end()) {
	    // Delete the interface
	    string vif_name = node_vif->name();
	    delete_vif(vif_name, dummy_error_msg);
	    continue;
	}
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}
