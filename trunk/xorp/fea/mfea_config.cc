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

#ident "$XORP: xorp/fea/mfea_config.cc,v 1.3 2003/06/01 02:49:55 pavlin Exp $"


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
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::add_config_vif(const Vif& vif, string& reason)
{
    //
    // Perform all the micro-operations that are required to add a vif.
    //
    if (add_config_vif(vif.name(), vif.vif_index(), reason) < 0)
	return (XORP_ERROR);
    
    list<VifAddr>::const_iterator vif_addr_iter;
    for (vif_addr_iter = vif.addr_list().begin();
	 vif_addr_iter != vif.addr_list().end();
	 ++vif_addr_iter) {
	const VifAddr& vif_addr = *vif_addr_iter;
	if (add_config_vif_addr(vif.name(),
				vif_addr.addr(),
				vif_addr.subnet_addr(),
				vif_addr.broadcast_addr(),
				vif_addr.peer_addr(),
				reason) < 0) {
	    return (XORP_ERROR);
	}
    }
    if (set_config_vif_flags(vif.name(),
			     vif.is_pim_register(),
			     vif.is_p2p(),
			     vif.is_loopback(),
			     vif.is_multicast_capable(),
			     vif.is_broadcast_capable(),
			     vif.is_underlying_vif_up(),
			     reason) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * Add a configured vif.
 * 
 * @param vif_name the name of the vif to add.
 * @param vif_index the vif index of the vif to add.
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::add_config_vif(const string& vif_name,
			 uint16_t vif_index,
			 string& reason)
{
    if (ProtoNode<MfeaVif>::add_config_vif(vif_name, vif_index, reason) < 0)
	return (XORP_ERROR);
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _proto_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
        const string& dst_module_instance_name = (*iter).first;
        xorp_module_id dst_module_id = (*iter).second;
        send_add_config_vif(dst_module_instance_name, dst_module_id,
			    vif_name, vif_index);
    }
    
    return (XORP_OK);
}

/**
 * Delete a configured vif.
 * 
 * @param vif_name the name of the vif to delete.
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::delete_config_vif(const string& vif_name,
			    string& reason)
{
    if (ProtoNode<MfeaVif>::delete_config_vif(vif_name, reason) < 0)
	return (XORP_ERROR);
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _proto_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
        const string& dst_module_instance_name = (*iter).first;
        xorp_module_id dst_module_id = (*iter).second;
        send_delete_config_vif(dst_module_instance_name, dst_module_id,
			       vif_name);
    }
    
    return (XORP_OK);
}

/**
 * Add an address to a configured vif.
 * 
 * @param vif_name the name of the vif.
 * @param addr the address to add.
 * @param subnet the subnet address to add.
 * @param broadcast the broadcast address to add.
 * @param peer the peer address to add.
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::add_config_vif_addr(const string& vif_name,
			      const IPvX& addr,
			      const IPvXNet& subnet,
			      const IPvX& broadcast,
			      const IPvX& peer,
			      string& reason)
{
    if (ProtoNode<MfeaVif>::add_config_vif_addr(vif_name, addr, subnet,
						broadcast, peer, reason) < 0)
	return (XORP_ERROR);
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _proto_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
        const string& dst_module_instance_name = (*iter).first;
        xorp_module_id dst_module_id = (*iter).second;
        send_add_config_vif_addr(dst_module_instance_name, dst_module_id,
				 vif_name, addr, subnet, broadcast, peer);
    }
    
    return (XORP_OK);
}
    
/**
 * Delete an address from a configured vif.
 * 
 * @param vif_name the name of the vif.
 * @param addr the address to delete.
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::delete_config_vif_addr(const string& vif_name,
				 const IPvX& addr,
				 string& reason)
{
    if (ProtoNode<MfeaVif>::delete_config_vif_addr(vif_name, addr, reason) < 0)
	return (XORP_ERROR);
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _proto_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
        const string& dst_module_instance_name = (*iter).first;
        xorp_module_id dst_module_id = (*iter).second;
        send_delete_config_vif_addr(dst_module_instance_name, dst_module_id,
				    vif_name, addr);
    }
    
    return (XORP_OK);
}

/**
 * Set the vif flags to a configured vif.
 * 
 * @param vif_name the name of the vif.
 * @param is_pim_register true if the vif is a PIM Register interface.
 * @param is_p2p true if the vif is point-to-point interface.
 * @param is_loopback true if the vif is a loopback interface.
 * @param is_multicast true if the vif is multicast capable.
 * @param is_broadcast true if the vif is broadcast capable.
 * @param is_up true if the underlying vif is UP.
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::set_config_vif_flags(const string& vif_name,
			       bool is_pim_register,
			       bool is_p2p,
			       bool is_loopback,
			       bool is_multicast,
			       bool is_broadcast,
			       bool is_up,
			       string& reason)
{
    if (ProtoNode<MfeaVif>::set_config_vif_flags(vif_name, is_pim_register,
						 is_p2p, is_loopback,
						 is_multicast, is_broadcast,
						 is_up, reason) < 0)
	return (XORP_ERROR);
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _proto_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
        const string& dst_module_instance_name = (*iter).first;
        xorp_module_id dst_module_id = (*iter).second;
        send_set_config_vif_flags(dst_module_instance_name, dst_module_id,
				  vif_name, is_pim_register, is_p2p,
				  is_loopback, is_multicast, is_broadcast,
				  is_up);
    }
    
    return (XORP_OK);
}

/**
 * Complete the set of vif configuration changes.
 * 
 * @param reason return-by-reference string that contains human-readable
 * string with information about the reason for failure (if any).
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaNode::set_config_all_vifs_done(string& reason)
{
    map<string, Vif>::iterator vif_iter;
    string err;
    map<string, Vif>& configured_vifs = ProtoNode<MfeaVif>::configured_vifs();
    
    //
    // Remove vifs that don't exist anymore
    //
    for (uint16_t i = 0; i < maxvifs(); i++) {
	Vif* node_vif = vif_find_by_vif_index(i);
	if (node_vif == NULL)
	    continue;
	if (node_vif->is_pim_register())
	    continue;		// XXX: don't delete the PIM Register vif
	if (configured_vifs.find(node_vif->name()) == configured_vifs.end()) {
	    // Delete the interface
	    string vif_name = node_vif->name();
	    delete_vif(vif_name, err);
	    continue;
	}
    }
    
    //
    // Add new vifs, and update existing ones
    //
    for (vif_iter = configured_vifs.begin();
	 vif_iter != configured_vifs.end();
	 ++vif_iter) {
	Vif* vif = &vif_iter->second;
	Vif* node_vif = vif_find_by_name(vif->name());
	
	//
	// Add a new vif
	//
	if (node_vif == NULL) {
	    add_vif(*vif, err);
	    continue;
	}
	
	//
	// Update the vif flags
	//
	node_vif->set_p2p(vif->is_p2p());
	node_vif->set_loopback(vif->is_loopback());
	node_vif->set_multicast_capable(vif->is_multicast_capable());
	node_vif->set_broadcast_capable(vif->is_broadcast_capable());
	node_vif->set_underlying_vif_up(vif->is_underlying_vif_up());
	
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
		if (vif->find_address(vif_addr.addr()) == NULL)
		    delete_addresses_list.push_back(vif_addr.addr());
	    }
	    // Delete the addresses
	    list<IPvX>::iterator ipvx_iter;
	    for (ipvx_iter = delete_addresses_list.begin();
		 ipvx_iter != delete_addresses_list.end();
		 ++ipvx_iter) {
		const IPvX& ipvx = *ipvx_iter;
		node_vif->delete_address(ipvx);
	    }
	}
	
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
    }
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _proto_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
        const string& dst_module_instance_name = (*iter).first;
        xorp_module_id dst_module_id = (*iter).second;
        send_set_config_all_vifs_done(dst_module_instance_name, dst_module_id);
    }
    
    if (end_config(reason) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
MfeaNode::set_mrib_table_default_metric_preference(uint32_t metric_preference)
{
    mrib_table_default_metric_preference().set(metric_preference);
    
    return (XORP_OK);
}

int
MfeaNode::reset_mrib_table_default_metric_preference()
{
    mrib_table_default_metric_preference().reset();
    
    return (XORP_OK);
}

int
MfeaNode::set_mrib_table_default_metric(uint32_t metric)
{
    mrib_table_default_metric().set(metric);
    
    return (XORP_OK);
}

int
MfeaNode::reset_mrib_table_default_metric()
{
    mrib_table_default_metric().reset();
    
    return (XORP_OK);
}
