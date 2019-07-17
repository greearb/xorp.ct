// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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
// MFEA (Multicast Forwarding Engine Abstraction) implementation.
//

#include "mfea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/utils.hh"

#include "mrt/max_vifs.h"
#include "mrt/mifset.hh"
#include "mrt/multicast_defs.h"

#include "fea_node.hh"
#include "mfea_mrouter.hh"
#include "mfea_node.hh"
#include "mfea_kernel_messages.hh"
#include "mfea_vif.hh"


MfeaRouteStorage::MfeaRouteStorage(uint32_t d, const string module_name, const IPvX& _source,
				   const IPvX& _group, const string& _iif_name, const string& _oif_names) :
	distance(d), is_binary(false), module_instance_name(module_name), source(_source),
	group(_group), iif_name(_iif_name), oif_names(_oif_names) { }

MfeaRouteStorage::MfeaRouteStorage(uint32_t d, const string module_name, const IPvX& _source,
				   const IPvX& _group, uint32_t iif_vif_idx,
				   const Mifset& _oiflist, const Mifset& _oif_disable_wrongvif,
				   uint32_t max_vifs, const IPvX& _rp_addr) :
	distance(d), is_binary(true), module_instance_name(module_name), source(_source),
	group(_group), iif_vif_index(iif_vif_idx), oiflist(_oiflist),
	oiflist_disable_wrongvif(_oif_disable_wrongvif), max_vifs_oiflist(max_vifs),
	rp_addr(_rp_addr) { }

MfeaRouteStorage::MfeaRouteStorage(const MfeaRouteStorage& o) :
	distance(o.distance), is_binary(true), module_instance_name(o.module_instance_name),
	source(o.source), group(o.group), iif_name(o.iif_name),
	oif_names(o.oif_names), iif_vif_index(o.iif_vif_index),
	oiflist(o.oiflist), oiflist_disable_wrongvif(o.oiflist_disable_wrongvif),
	max_vifs_oiflist(o.max_vifs_oiflist), rp_addr(o.rp_addr) { }

MfeaRouteStorage& MfeaRouteStorage::operator=(const MfeaRouteStorage& o) {
    distance = o.distance;
    is_binary = o.is_binary;
    module_instance_name = o.module_instance_name;
    source = o.source;
    group = o.group;
    iif_name = o.iif_name;
    oif_names = o.oif_names;
    iif_vif_index = o.iif_vif_index;
    oiflist = o.oiflist;
    oiflist_disable_wrongvif = o. oiflist_disable_wrongvif;
    max_vifs_oiflist = o.max_vifs_oiflist;
    rp_addr = o.rp_addr;
    return *this;
}

MfeaRouteStorage::MfeaRouteStorage() : distance(0), is_binary(false), max_vifs_oiflist(0) { }

/**
 * MfeaNode::MfeaNode:
 * @fea_node: The corresponding FeaNode.
 * @family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @module_id: The module ID (must be %XORP_MODULE_MFEA).
 * @eventloop: The event loop.
 * 
 * MFEA node constructor.
 **/
MfeaNode::MfeaNode(FeaNode& fea_node, int family, xorp_module_id module_id,
		   EventLoop& eventloop)
    : ProtoNode<MfeaVif>(family, module_id, eventloop),
      IfConfigUpdateReporterBase(fea_node.ifconfig().ifconfig_update_replicator()),
      _fea_node(fea_node),
      _mfea_mrouter(*this, fea_node.fibconfig()),
      _mfea_dft(*this),
      _mfea_iftree("mfea-tree"),
      _mfea_iftree_update_replicator(_mfea_iftree),
      _is_log_trace(false)
{
    XLOG_ASSERT(module_id == XORP_MODULE_MFEA);
    
    if (module_id != XORP_MODULE_MFEA) {
	XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_MFEA' = %d)",
		   module_id, XORP_MODULE_MFEA);
    }
    
    //
    // Set the node status
    //
    ProtoNode<MfeaVif>::set_node_status(PROC_STARTUP);

    //
    // Set myself as an observer when the node status changes
    //
    set_observer(this);
}

/**
 * MfeaNode::~MfeaNode:
 * @: 
 * 
 * MFEA node destructor.
 * 
 **/
MfeaNode::~MfeaNode()
{
    // Remove myself from receiving FEA interface information
    remove_from_replicator();

    //
    // Unset myself as an observer when the node status changes
    //
    unset_observer(this);

    stop();

    ProtoNode<MfeaVif>::set_node_status(PROC_NULL);
    
    delete_all_vifs();
}

/**
 * Test if running in dummy mode.
 * 
 * @return true if running in dummy mode, otherwise false.
 */
bool
MfeaNode::is_dummy() const
{
    return (_fea_node.is_dummy());
}

/**
 * MfeaNode::start:
 * @: 
 * 
 * Start the MFEA.
 * TODO: This function should not start the operation on the
 * interfaces. The interfaces must be activated separately.
 * After the startup operations are completed,
 * MfeaNode::final_start() is called to complete the job.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
MfeaNode::start()
{
    if (! is_enabled())
	return (XORP_OK);

    // Add myself to receive FEA interface information
    add_to_replicator();

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

    if (ProtoNode<MfeaVif>::pending_start() != XORP_OK)
	return (XORP_ERROR);

    //
    // Set the node status
    //
    ProtoNode<MfeaVif>::set_node_status(PROC_STARTUP);

    // XXX: needed to update the status state properly
    incr_startup_requests_n();

    // Start the MfeaMrouter
    _mfea_mrouter.start();
    
    // XXX: needed to update the status state properly
    decr_startup_requests_n();

    return (XORP_OK);
}

/**
 * MfeaNode::final_start:
 * @: 
 * 
 * Completely start the node operation.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::final_start()
{
#if 0	// TODO: XXX: PAVPAVPAV
    if (! is_pending_up())
	return (XORP_ERROR);
#endif

    if (ProtoNode<MfeaVif>::start() != XORP_OK) {
	ProtoNode<MfeaVif>::stop();
	return (XORP_ERROR);
    }

    // Start the mfea_vifs
    start_all_vifs();

    XLOG_INFO("MFEA started");

    return (XORP_OK);
}

/**
 * MfeaNode::stop:
 * @: 
 * 
 * Gracefully stop the MFEA.
 * XXX: After the cleanup is completed,
 * MfeaNode::final_stop() is called to complete the job.
 * XXX: This function, unlike start(), will stop the MFEA
 * operation on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop()
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

    if (ProtoNode<MfeaVif>::pending_stop() != XORP_OK)
	return (XORP_ERROR);

    //
    // Perform misc. MFEA-specific stop operations
    //
    
    // XXX: needed to update the status state properly
    incr_shutdown_requests_n();

    // Stop the vifs
    stop_all_vifs();
    
    // Stop the MfeaMrouter
    _mfea_mrouter.stop();

    //
    // Set the node status
    //
    ProtoNode<MfeaVif>::set_node_status(PROC_SHUTDOWN);

    //
    // Update the node status
    //
    update_status();

    // XXX: needed to update the status state properly
    decr_shutdown_requests_n();

    return (XORP_OK);
}

/**
 * MfeaNode::final_stop:
 * @: 
 * 
 * Completely stop the MFEA operation.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::final_stop()
{
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);

    if (ProtoNode<MfeaVif>::stop() != XORP_OK)
	return (XORP_ERROR);

    XLOG_INFO("MFEA stopped");

    return (XORP_OK);
}

/**
 * Enable the node operation.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
void
MfeaNode::enable()
{
    ProtoUnit::enable();

    XLOG_INFO("MFEA enabled");
}

/**
 * Disable the node operation.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
void
MfeaNode::disable()
{
    stop();
    ProtoUnit::disable();

    XLOG_INFO("MFEA disabled");
}

void
MfeaNode::status_change(ServiceBase*  service,
			ServiceStatus old_status,
			ServiceStatus new_status)
{
    if (service == this) {
	// My own status has changed
	if ((old_status == SERVICE_STARTING)
	    && (new_status == SERVICE_RUNNING)) {
	    // The startup process has completed
	    if (final_start() != XORP_OK) {
		XLOG_ERROR("Cannot complete the startup process; "
			   "current state is %s",
			   ProtoNode<MfeaVif>::state_str().c_str());
		return;
	    }
	    ProtoNode<MfeaVif>::set_node_status(PROC_READY);
	    return;
	}

	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    // The shutdown process has completed
	    final_stop();
	    // Set the node status
	    ProtoNode<MfeaVif>::set_node_status(PROC_DONE);
	    return;
	}

	//
	// TODO: check if there was an error
	//
	return;
    }
}

void
MfeaNode::interface_update(const string&	ifname,
			   const Update&	update)
{
    IfTreeInterface* mfea_ifp;
    const Vif* node_vif = NULL;
    bool is_up;
    string error_msg;

    switch (update) {
    case CREATED:
	// Update the MFEA iftree
	_mfea_iftree.add_interface(ifname);

	// XXX: The MFEA vif creation is handled by vif_update(),
	// so we only update the status.
	break;					// FALLTHROUGH

    case DELETED:
	// Update the MFEA iftree
	XLOG_WARNING("interface_update:  Delete: %s\n",
		     ifname.c_str());
	// First, make sure our protocols are un-registered on this interface
	// and all it's vifs.
	unregister_protocols_for_iface(ifname);

	_mfea_iftree.remove_interface(ifname);
	_mfea_iftree_update_replicator.interface_update(ifname, update);

	// XXX: Ignore any errors, in case the MFEA vif was deleted by
	// vif_update()
	ProtoNode<MfeaVif>::delete_config_vif(ifname, error_msg);
	return;

    case CHANGED:
	break;					// FALLTHROUGH
    }

    //
    // Find the state from the FEA tree
    //
    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for interface not in the FEA tree: %s",
		     ifname.c_str());
	return;
    }

    //
    // Update the MFEA iftree
    //
    mfea_ifp = _mfea_iftree.find_interface(ifname);
    if (mfea_ifp == NULL) {
	XLOG_WARNING("Got update for interface not in the MFEA tree: %s",
		     ifname.c_str());
	return;
    }
    mfea_ifp->copy_state(*ifp, false);
    _mfea_iftree_update_replicator.interface_update(ifname, update);

    //
    // Update the vif flags (only if the vif already exists)
    //
    node_vif = configured_vif_find_by_name(ifname);
    if (node_vif == NULL)
	return;		// No vif to update

    // XXX: For any flag updates we need to consider the IfTreeVif as well
    const IfTreeVif* vifp = ifp->find_vif(node_vif->name());
    if (vifp == NULL)
	return;		// No IfTreeVif to consider

    is_up = ifp->enabled();
    is_up &= vifp->enabled();
    ProtoNode<MfeaVif>::set_config_vif_flags(ifname,
					     false,	// is_pim_register
					     node_vif->is_p2p(),
					     node_vif->is_loopback(),
					     node_vif->is_multicast_capable(),
					     node_vif->is_broadcast_capable(),
					     is_up,
					     ifp->mtu(),
					     error_msg);
}

void
MfeaNode::vif_update(const string&	ifname,
		     const string&	vifname,
		     const Update&	update)
{
    IfTreeInterface* mfea_ifp;
    IfTreeVif* mfea_vifp = NULL;
    Vif* node_vif = NULL;
    bool is_up;
    uint32_t vif_index = Vif::VIF_INDEX_INVALID;
    string error_msg;


    switch (update) {
    case CREATED:
	//XLOG_INFO("MfeaNode: vif_updated:  Created: %s/%s\n",
	//	  ifname.c_str(), vifname.c_str());

	// Update the MFEA iftree
	mfea_ifp = _mfea_iftree.find_interface(ifname);
	if (mfea_ifp == NULL) {
	    XLOG_WARNING("Got update for vif on interface not in the MFEA tree: %s/%s",
			 ifname.c_str(), vifname.c_str());
	    return;
	}
	mfea_ifp->add_vif(vifname);
	mfea_vifp = mfea_ifp->find_vif(vifname);
	XLOG_ASSERT(mfea_vifp != NULL);

	node_vif = configured_vif_find_by_name(ifname);
	if (node_vif == NULL) {
	    //XLOG_INFO("MfeaNode: vif_updated:  Created, node_vif was NULL: %s/%s\n",
	    //	      ifname.c_str(), vifname.c_str());
	    vif_index = find_unused_config_vif_index();
	    XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	    if (ProtoNode<MfeaVif>::add_config_vif(vifname, vif_index,
						   error_msg)
		!= XORP_OK) {
		XLOG_ERROR("Cannot add vif %s to the set of configured vifs: %s",
			   vifname.c_str(), error_msg.c_str());
		return;
	    }
	}
	else {
	    //XLOG_INFO("MfeaNode: vif_updated:  Created, node_vif exists: %s/%s\n%s",
	    //	      ifname.c_str(), vifname.c_str(), node_vif->str().c_str());
	    // Deal with invalid vif-index (maybe device was configured but not
	    //  actually present)
	    if (node_vif->vif_index() == Vif::VIF_INDEX_INVALID) {
		vif_index = find_unused_config_vif_index();
		XLOG_INFO("Assigning new vif_index: %i to vif: %s/%s\n",
			  vif_index, ifname.c_str(), vifname.c_str());
		XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
		node_vif->set_vif_index(vif_index);
	    }
	    vif_index = node_vif->vif_index();
	}
	break;

    case DELETED:
	// Update the MFEA iftree
	//XLOG_INFO("vif_updated:  vif_updated: Delete: %s/%s\n",
	//  ifname.c_str(), vifname.c_str());

	// Unregister protocols on this VIF
	unregister_protocols_for_vif(ifname, vifname);

	mfea_ifp = _mfea_iftree.find_interface(ifname);
	if (mfea_ifp != NULL) {
	    mfea_ifp->remove_vif(vifname);
	}
	_mfea_iftree_update_replicator.vif_update(ifname, vifname, update);

	if (ProtoNode<MfeaVif>::delete_config_vif(vifname, error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Cannot delete vif %s from the set of configured "
		       "vifs: %s",
		       vifname.c_str(), error_msg.c_str());
	}
	return;

    case CHANGED:
	//XLOG_INFO("MfeaNode: vif_updated: Changed vif: %s/%s\n",
	//	  ifname.c_str(), vifname.c_str());

	mfea_vifp = _mfea_iftree.find_vif(ifname, vifname);
	if (mfea_vifp == NULL) {
	    XLOG_WARNING("Got update for vif not in the MFEA tree: %s/%s",
			 ifname.c_str(), vifname.c_str());
	    return;
	}
	vif_index = mfea_vifp->vif_index();
	break; // FALLTHROUGH
    }//switch

    //
    // Find the state from the FEA tree
    //
    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for vif on interface not in the FEA tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    const IfTreeVif* vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	XLOG_WARNING("Got update for vif not in the FEA tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    //
    // Update the MFEA iftree
    //
    XLOG_ASSERT(mfea_vifp != NULL);
    if (vif_index == Vif::VIF_INDEX_INVALID) {
	XLOG_ERROR("Got update (%i) for vif, vif_index is invalid, vif: %s/%s:  %s  vifp: %s\n",
		   update, ifname.c_str(), vifname.c_str(), mfea_vifp->str().c_str(),
		   vifp->str().c_str());
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
    }
    mfea_vifp->copy_state(*vifp);
    mfea_vifp->set_vif_index(vif_index);
    _mfea_iftree_update_replicator.vif_update(ifname, vifname, update);

    //
    // Find the MFEA vif to update
    //
    node_vif = configured_vif_find_by_name(ifname);
    if (node_vif == NULL) {
	XLOG_WARNING("Got update for vif that is not in the MFEA tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    //
    // Update the pif_index
    //
    ProtoNode<MfeaVif>::set_config_pif_index(vifname, vifp->pif_index(),
					     error_msg);

    //
    // Update the vif flags
    //
    is_up = ifp->enabled();
    is_up &= vifp->enabled();
    ProtoNode<MfeaVif>::set_config_vif_flags(vifname,
					     false,	// is_pim_register
					     vifp->point_to_point(),
					     vifp->loopback(),
					     vifp->multicast(),
					     vifp->broadcast(),
					     is_up,
					     ifp->mtu(),
					     error_msg);

    // See if it wants to start?
    MfeaVif *mfea_vif = vif_find_by_name(vifname);
    if (mfea_vif) {
	mfea_vif->notifyUpdated();
    }
}

void
MfeaNode::vifaddr4_update(const string&	ifname,
			  const string&	vifname,
			  const IPv4&	addr,
			  const Update&	update)
{
    IfTreeVif* mfea_vifp;
    IfTreeAddr4* mfea_ap;
    const Vif* node_vif = NULL;
    const IPvX addrx(addr);
    string error_msg;

    if (! is_ipv4())
	return;

    switch (update) {
    case CREATED:
	// Update the MFEA iftree
	mfea_vifp = _mfea_iftree.find_vif(ifname, vifname);

	if (mfea_vifp == NULL) {
	    XLOG_WARNING("Got update for address on interface not in the FEA tree: "
			 "%s/%s/%s",
			 ifname.c_str(), vifname.c_str(), addr.str().c_str());
	    return;
	}
	mfea_vifp->add_addr(addr);

	break; // FALLTHROUGH

    case DELETED:
	// Update the MFEA iftree
	mfea_vifp = _mfea_iftree.find_vif(ifname, vifname);
	if (mfea_vifp != NULL) {
	    mfea_vifp->remove_addr(addr);
	}
	_mfea_iftree_update_replicator.vifaddr4_update(ifname, vifname, addr,
						       update);

	if (ProtoNode<MfeaVif>::delete_config_vif_addr(vifname, addrx,
						       error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Cannot delete address %s from vif %s from the set of "
		       "configured vifs: %s",
		       addr.str().c_str(), vifname.c_str(), error_msg.c_str());
	}
	return;

    case CHANGED:
	break; // FALLTHROUGH
    }

    //
    // Find the state from the FEA tree
    //
    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for address on interface not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }
    const IfTreeVif* vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	XLOG_WARNING("Got update for address on vif not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }
    const IfTreeAddr4* ap = vifp->find_addr(addr);
    if (ap == NULL) {
	XLOG_WARNING("Got update for address not in the FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    //
    // Update the MFEA iftree
    //
    mfea_ap = _mfea_iftree.find_addr(ifname, vifname, addr);
    if (mfea_ap == NULL) {
	XLOG_WARNING("Got update for address for vif that is not in the MFEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
    }
    mfea_ap->copy_state(*ap);
    _mfea_iftree_update_replicator.vifaddr4_update(ifname, vifname, addr,
						   update);

    //
    // Find the MFEA vif to update
    //
    node_vif = configured_vif_find_by_name(ifname);
    if (node_vif == NULL) {
	XLOG_WARNING("Got update for address for vif that is not in the MFEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    // Calculate various addresses
    IPvXNet subnet_addr(addrx, ap->prefix_len());
    IPvX broadcast_addr(IPvX::ZERO(family()));
    IPvX peer_addr(IPvX::ZERO(family()));
    if (ap->broadcast())
	broadcast_addr = IPvX(ap->bcast());
    if (ap->point_to_point())
	peer_addr = IPvX(ap->endpoint());

    const VifAddr* node_vif_addr = node_vif->find_address(addrx);
    if (node_vif_addr == NULL) {
	// First create the MFEA vif address
	if (ProtoNode<MfeaVif>::add_config_vif_addr(vifname, addrx,
						    subnet_addr,
						    broadcast_addr,
						    peer_addr, error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Cannot add address %s to vif %s from the set of "
		       "configured vifs: %s",
		       addr.str().c_str(), vifname.c_str(), error_msg.c_str());
	    return;
	}
	node_vif_addr = node_vif->find_address(addrx);
    }

    //
    // Update the address.
    //
    // First we delete it then add it back with the new values.
    // If there are no changes, we return immediately.
    //
    if ((addrx == node_vif_addr->addr())
	&& (subnet_addr == node_vif_addr->subnet_addr()
	    && (broadcast_addr == node_vif_addr->broadcast_addr())
	    && (peer_addr == node_vif_addr->peer_addr()))) {
	return;			// Nothing changed
    }

    if (ProtoNode<MfeaVif>::delete_config_vif_addr(vifname, addrx, error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Cannot delete address %s from vif %s from the set of "
		   "configured vifs: %s",
		   addr.str().c_str(), vifname.c_str(), error_msg.c_str());
    }
    if (ProtoNode<MfeaVif>::add_config_vif_addr(vifname, addrx, subnet_addr,
						broadcast_addr, peer_addr,
						error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Cannot add address %s to vif %s from the set of "
		   "configured vifs: %s",
		   addr.str().c_str(), vifname.c_str(), error_msg.c_str());
    }

    MfeaVif *mfea_vif = vif_find_by_name(vifname);
    if (mfea_vif) {
	mfea_vif->notifyUpdated();
    }
}

#ifdef HAVE_IPV6
void
MfeaNode::vifaddr6_update(const string&	ifname,
			  const string&	vifname,
			  const IPv6&	addr,
			  const Update&	update)
{
    IfTreeVif* mfea_vifp;
    IfTreeAddr6* mfea_ap;
    const Vif* node_vif = NULL;
    const IPvX addrx(addr);
    string error_msg;

    if (! is_ipv6())
	return;

    switch (update) {
    case CREATED:
	// Update the MFEA iftree
	mfea_vifp = _mfea_iftree.find_vif(ifname, vifname);
	if (mfea_vifp == NULL) {
	    XLOG_WARNING("Got update for address on interface not in the FEA tree: "
			 "%s/%s/%s",
			 ifname.c_str(), vifname.c_str(), addr.str().c_str());
	    return;
	}
	mfea_vifp->add_addr(addr);

	break;					// FALLTHROUGH

    case DELETED:
	// Update the MFEA iftree
	mfea_vifp = _mfea_iftree.find_vif(ifname, vifname);
	if (mfea_vifp != NULL) {
	    mfea_vifp->remove_addr(addr);
	}
	_mfea_iftree_update_replicator.vifaddr6_update(ifname, vifname, addr,
						       update);

	if (ProtoNode<MfeaVif>::delete_config_vif_addr(vifname, addrx,
						       error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Cannot delete address %s from vif %s from the set of "
		       "configured vifs: %s",
		       addr.str().c_str(), vifname.c_str(), error_msg.c_str());
	}
	return;

    case CHANGED:
	break;					// FALLTHROUGH
    }

    //
    // Find the state from the FEA tree
    //
    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for address on interface not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }
    const IfTreeVif* vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	XLOG_WARNING("Got update for address on vif not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }
    const IfTreeAddr6* ap = vifp->find_addr(addr);
    if (ap == NULL) {
	XLOG_WARNING("Got update for address not in the FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    //
    // Update the MFEA iftree
    //
    mfea_ap = _mfea_iftree.find_addr(ifname, vifname, addr);
    if (mfea_ap == NULL) {
	XLOG_WARNING("Got update for address for vif that is not in the MFEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }
    mfea_ap->copy_state(*ap);
    _mfea_iftree_update_replicator.vifaddr6_update(ifname, vifname, addr,
						   update);

    //
    // Find the MFEA vif to update
    //
    node_vif = configured_vif_find_by_name(ifname);
    if (node_vif == NULL) {
	XLOG_WARNING("Got update for address for vif that is not in the MFEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    // Calculate various addresses
    IPvXNet subnet_addr(addrx, ap->prefix_len());
    IPvX broadcast_addr(IPvX::ZERO(family()));
    IPvX peer_addr(IPvX::ZERO(family()));
    if (ap->point_to_point())
	peer_addr = IPvX(ap->endpoint());

    const VifAddr* node_vif_addr = node_vif->find_address(addrx);
    if (node_vif_addr == NULL) {
	// First create the MFEA vif address
	if (ProtoNode<MfeaVif>::add_config_vif_addr(vifname, addrx,
						    subnet_addr,
						    broadcast_addr, peer_addr,
						    error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Cannot add address %s to vif %s from the set of "
		       "configured vifs: %s",
		       addr.str().c_str(), vifname.c_str(), error_msg.c_str());
	    return;
	}
	node_vif_addr = node_vif->find_address(addrx);
    }

    //
    // Update the address.
    //
    // First we delete it then add it back with the new values.
    // If there are no changes, we return immediately.
    //
    if ((addrx == node_vif_addr->addr())
	&& (subnet_addr == node_vif_addr->subnet_addr()
	    && (broadcast_addr == node_vif_addr->broadcast_addr())
	    && (peer_addr == node_vif_addr->peer_addr()))) {
	return;			// Nothing changed
    }

    if (ProtoNode<MfeaVif>::delete_config_vif_addr(vifname, addrx, error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Cannot delete address %s from vif %s from the set of "
		   "configured vifs: %s",
		   addr.str().c_str(), vifname.c_str(), error_msg.c_str());
    }
    if (ProtoNode<MfeaVif>::add_config_vif_addr(vifname, addrx, subnet_addr,
						broadcast_addr, peer_addr,
						error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Cannot add address %s to vif %s from the set of "
		   "configured vifs: %s",
		   addr.str().c_str(), vifname.c_str(), error_msg.c_str());
    }

    MfeaVif *mfea_vif = vif_find_by_name(vifname);
    if (mfea_vif) {
	mfea_vif->notifyUpdated();
    }
}
#endif

void
MfeaNode::updates_completed()
{
    string error_msg;

    // Update the MFEA iftree
    _mfea_iftree.finalize_state();
    _mfea_iftree_update_replicator.updates_completed();

    set_config_all_vifs_done(error_msg);
}

/**
 * MfeaNode::add_vif:
 * @vif: Vif information about the new MfeaVif to install.
 * @error_msg: The error message (if error).
 * 
 * Install a new MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_vif(const Vif& vif, string& error_msg)
{
    //
    // Create a new MfeaVif
    //
    MfeaVif *mfea_vif = new MfeaVif(*this, vif);
    
    if (ProtoNode<MfeaVif>::add_vif(mfea_vif) != XORP_OK) {
	// Cannot add this new vif
	error_msg = c_format("Cannot add vif %s: internal error",
			     vif.name().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	
	delete mfea_vif;
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Interface added: %s", mfea_vif->str().c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::add_pim_register_vif:
 * 
 * Install a new MFEA PIM Register vif (if needed).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_pim_register_vif()
{
    string error_msg;

    //
    // Test first whether we have already PIM Register vif
    //
    for (uint32_t i = 0; i < maxvifs(); i++) {
	MfeaVif *mfea_vif = vif_find_by_vif_index(i);
	if (mfea_vif == NULL)
	    continue;
	if (mfea_vif->is_pim_register())
	    return (XORP_OK);		// Found: OK
    }
    
    //
    // Create the PIM Register vif if there is a valid IP address
    // on an interface that is already up and running.
    //
    // TODO: check with Linux, Solaris, etc, if we can
    // use 127.0.0.2 or ::2 as a PIM Register vif address, and use that
    // address instead (otherwise we may always have to keep track
    // whether the underlying address has changed).
    //
    bool mfea_vif_found = false;
    MfeaVif *mfea_vif = NULL;
    for (uint32_t i = 0; i < maxvifs(); i++) {
	mfea_vif = vif_find_by_vif_index(i);
	if (mfea_vif == NULL)
	    continue;
	if (! mfea_vif->is_underlying_vif_up())
	    continue;
	if (! mfea_vif->is_up())
	    continue;
	if (mfea_vif->addr_ptr() == NULL)
	    continue;
	if (mfea_vif->is_pim_register())
	    continue;
	if (mfea_vif->is_loopback())
	    continue;
	if (! mfea_vif->is_multicast_capable())
	    continue;
	// Found appropriate vif
	mfea_vif_found = true;
	break;
    }
    if (mfea_vif_found) {
	// Add the Register vif
	uint32_t vif_index = find_unused_config_vif_index();
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	// TODO: XXX: the Register vif name is hardcoded here!
	MfeaVif register_vif(*this, Vif("register_vif"));
	register_vif.set_vif_index(vif_index);
	//
	// XXX: A hack to make it appear that the PIM Register vif
	// has a valid physical interface index.
	// This is needed to pass a requirement inside the KAME-derived
	// IPv6 BSD kernel.
	//
	register_vif.set_pif_index(mfea_vif->pif_index());
	register_vif.set_underlying_vif_up(true); // XXX: 'true' to allow creation
	register_vif.set_pim_register(true);
	register_vif.set_mtu(mfea_vif->mtu());
	// Add all addresses, but ignore subnets, broadcast and p2p addresses
	list<VifAddr>::const_iterator vif_addr_iter;
	for (vif_addr_iter = mfea_vif->addr_list().begin();
	     vif_addr_iter != mfea_vif->addr_list().end();
	     ++vif_addr_iter) {
	    const VifAddr& vif_addr = *vif_addr_iter;
	    const IPvX& ipvx = vif_addr.addr();
	    register_vif.add_address(ipvx, IPvXNet(ipvx, ipvx.addr_bitlen()),
				     ipvx, IPvX::ZERO(family()));
	}
	if (add_vif(register_vif, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot add Register vif: %s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	
	if (add_config_vif(register_vif, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot add Register vif to set of configured vifs: %s",
		       error_msg.c_str());
	    return (XORP_ERROR);
	}

	//
	// Update the MFEA iftree
	//
	IfTreeInterface* mfea_ifp;
	IfTreeVif* mfea_vifp;
	_mfea_iftree.add_interface(register_vif.name());
	mfea_ifp = _mfea_iftree.find_interface(register_vif.name());
	XLOG_ASSERT(mfea_ifp != NULL);
	mfea_ifp->set_pif_index(0);		// XXX: invalid pif_index
	mfea_ifp->set_enabled(register_vif.is_underlying_vif_up());
	mfea_ifp->set_mtu(register_vif.mtu());
	_mfea_iftree_update_replicator.interface_update(mfea_ifp->ifname(),
							CREATED);

	mfea_ifp->add_vif(register_vif.name());
	mfea_vifp = mfea_ifp->find_vif(register_vif.name());
	XLOG_ASSERT(mfea_vifp != NULL);
	mfea_vifp->set_pif_index(0);		// XXX: invalid pif_index
	mfea_vifp->set_vif_index(register_vif.vif_index());
	mfea_vifp->set_enabled(register_vif.is_underlying_vif_up());
	mfea_vifp->set_pim_register(register_vif.is_pim_register());
	_mfea_iftree_update_replicator.vif_update(mfea_ifp->ifname(),
						  mfea_vifp->vifname(),
						  CREATED);

	for (vif_addr_iter = register_vif.addr_list().begin();
	     vif_addr_iter != register_vif.addr_list().end();
	     ++vif_addr_iter) {
	    const VifAddr& vif_addr = *vif_addr_iter;
	    const IPvX& ipvx = vif_addr.addr();
	    if (ipvx.is_ipv4()) {
		IPv4 ipv4 = ipvx.get_ipv4();
		mfea_vifp->add_addr(ipv4);
		IfTreeAddr4* ap = mfea_vifp->find_addr(ipv4);
		XLOG_ASSERT(ap != NULL);
		ap->set_enabled(register_vif.is_underlying_vif_up());
		ap->set_prefix_len(ipv4.addr_bitlen());
		_mfea_iftree_update_replicator.vifaddr4_update(mfea_ifp->ifname(),
							       mfea_vifp->vifname(),
							       ap->addr(),
							       CREATED);
	    }

#ifdef HAVE_IPV6
	    if (ipvx.is_ipv6()) {
		IPv6 ipv6 = ipvx.get_ipv6();
		mfea_vifp->add_addr(ipv6);
		IfTreeAddr6* ap = mfea_vifp->find_addr(ipv6);
		XLOG_ASSERT(ap != NULL);
		ap->set_enabled(register_vif.is_underlying_vif_up());
		ap->set_prefix_len(ipv6.addr_bitlen());
		_mfea_iftree_update_replicator.vifaddr6_update(mfea_ifp->ifname(),
							       mfea_vifp->vifname(),
							       ap->addr(),
							       CREATED);
	    }
#endif
	}

	_mfea_iftree_update_replicator.updates_completed();
    }

    // Done
    set_config_all_vifs_done(error_msg);

    return XORP_OK;
}

/**
 * MfeaNode::delete_vif:
 * @vif_name: The name of the vif to delete.
 * @error_msg: The error message (if error).
 * 
 * Delete an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_vif(const string& vif_name, string& error_msg)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	error_msg = c_format("Cannot delete vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (ProtoNode<MfeaVif>::delete_vif(mfea_vif) != XORP_OK) {
	error_msg = c_format("Cannot delete vif %s: internal error",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	delete mfea_vif;
	return (XORP_ERROR);
    }
    
    delete mfea_vif;
    
    XLOG_INFO("Interface deleted: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::enable_vif:
 * @vif_name: The name of the vif to enable.
 * @error_msg: The error message (if error).
 * 
 * Enable an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::enable_vif(const string& vif_name, string& error_msg)
{
    // Check our wants-to-be-running list
    map<string, VifPermInfo>::iterator i = perm_info.find(vif_name);
    if (i != perm_info.end()) {
	i->second.should_enable = true;
    }
    else {
	VifPermInfo pv(vif_name, false, true);
	perm_info[vif_name] = pv;
    }

    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	error_msg = c_format("MfeaNode:  Cannot enable vif %s: no such vif",
			     vif_name.c_str());
	XLOG_INFO("%s", error_msg.c_str());
	return XORP_OK; // will start later
    }
    
    mfea_vif->enable("MfeaNote::enable_vif");

    return XORP_OK;
}

/**
 * MfeaNode::disable_vif:
 * @vif_name: The name of the vif to disable.
 * @error_msg: The error message (if error).
 * 
 * Disable an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::disable_vif(const string& vif_name, string& error_msg)
{
    // Check our wants-to-be-running list
    map<string, VifPermInfo>::iterator i = perm_info.find(vif_name);
    if (i != perm_info.end()) {
	i->second.should_enable = false;
    }

    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	error_msg = c_format("Cannot disable vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
        // If it's gone, it's as disabled as it can be.
	//return (XORP_ERROR);
        return XORP_OK;
    }
    
    mfea_vif->disable("MfeaNode::disable_vif");
    
    return (XORP_OK);
}

/**
 * MfeaNode::start_vif:
 * @vif_name: The name of the vif to start.
 * @error_msg: The error message (if error).
 * 
 * Start an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::start_vif(const string& vif_name, string& error_msg)
{
    // Check our wants-to-be-running list
    map<string, VifPermInfo>::iterator i = perm_info.find(vif_name);
    if (i != perm_info.end()) {
	i->second.should_start = true;
    }
    else {
	VifPermInfo pv(vif_name, true, false);
	perm_info[vif_name] = pv;
    }

    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	error_msg = c_format("MfeaNode: Cannot start vif %s: no such vif",
			     vif_name.c_str());
	XLOG_INFO("%s", error_msg.c_str());
	return XORP_OK; // will start later
    }
    
    if (mfea_vif->start(error_msg, "MfeaNode::start_vif") != XORP_OK) {
	error_msg = c_format("MfeaNode: Cannot start vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return XORP_ERROR;
    }
    
    // XXX: add PIM Register vif (if needed)
    add_pim_register_vif();

    return XORP_OK;
}

/**
 * MfeaNode::stop_vif:
 * @vif_name: The name of the vif to stop.
 * @error_msg: The error message (if error).
 * 
 * Stop an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop_vif(const string& vif_name, string& error_msg)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	error_msg = c_format("Cannot stop vif %s: no such vif  (will continue)",
			     vif_name.c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	// If it doesn't exist, it's as stopped as it's going to get.  Returning
	// error will cause entire commit to fail.
	//return (XORP_ERROR);
	return XORP_OK;
    }
    
    if (mfea_vif->stop(error_msg, true, "MfeaNode::stop_vif") != XORP_OK) {
	error_msg = c_format("Cannot stop vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::start_all_vifs:
 * @: 
 * 
 * Start MFEA on all enabled interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::start_all_vifs()
{
    vector<MfeaVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (start_vif(mfea_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * MfeaNode::stop_all_vifs:
 * @: 
 * 
 * Stop MFEA on all interfaces it was running on.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop_all_vifs()
{
    vector<MfeaVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (stop_vif(mfea_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * MfeaNode::enable_all_vifs:
 * @: 
 * 
 * Enable MFEA on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::enable_all_vifs()
{
    vector<MfeaVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (enable_vif(mfea_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * MfeaNode::disable_all_vifs:
 * @: 
 * 
 * Disable MFEA on all interfaces. All running interfaces are stopped first.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::disable_all_vifs()
{
    vector<MfeaVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (disable_vif(mfea_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * MfeaNode::delete_all_vifs:
 * @: 
 * 
 * Delete all MFEA vifs.
 **/
void
MfeaNode::delete_all_vifs()
{
    list<string> vif_names;
    vector<MfeaVif *>::iterator iter;

    //
    // Create the list of all vif names to delete
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif != NULL) {
	    string vif_name = mfea_vif->name();
	    vif_names.push_back(mfea_vif->name());
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
MfeaNode::vif_shutdown_completed(const string& vif_name)
{
    vector<MfeaVif *>::iterator iter;

    UNUSED(vif_name);

    //
    // If all vifs have completed the shutdown, then de-register with
    // the MFEA.
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = *iter;
	if (mfea_vif == NULL)
	    continue;
	if (! mfea_vif->is_down())
	    return;
    }
}

int
MfeaNode::register_protocol(const string&	module_instance_name,
			    const string&	if_name,
			    const string&	vif_name,
			    uint8_t		ip_protocol,
			    string&		error_msg)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);

    if (mfea_vif == NULL) {
	error_msg = c_format("Cannot register module %s on interface %s "
			     "vif %s: no such vif",
			     module_instance_name.c_str(),
			     if_name.c_str(),
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    if (mfea_vif->register_protocol(module_instance_name, ip_protocol,
				    error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    //
    // Insert into the global state for registered module instance names
    // and IP protocols.
    //
    if ((ip_protocol == IPPROTO_PIM)
	&& (_registered_ip_protocols.find(ip_protocol)
	    == _registered_ip_protocols.end())) {

	// If necessary, start PIM processing
	if (mfea_mrouter().start_pim(error_msg) != XORP_OK) {
	    string dummy_error_msg;
	    mfea_vif->unregister_protocol(module_instance_name,
					  dummy_error_msg);
	    error_msg = c_format("Cannot start PIM processing: %s",
				 error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
    _registered_module_instance_names.insert(module_instance_name);
    _registered_ip_protocols.insert(ip_protocol);

    return (XORP_OK);
}

void MfeaNode::unregister_protocols_for_iface(const string& if_name) {
    IfTreeInterface* mfea_ifp = _mfea_iftree.find_interface(if_name);
    if (mfea_ifp) {
	// This is more paranoid than it currently needs to be.  It seems
	// that there cannot be more than one vif per iface, and vif-name is
	// probably always ifname.  But, it shouldn't hurt to make it look
	// for additional vifs in case support is someday added.
	//
	// Gather up list of strings and process them while NOT iterating
	// over the lists in case something in the unregister logic
	// changes the lists.
	// --Ben
	list<string> vifs;
	list<string> module_names;
	IfTreeInterface::VifMap::const_iterator i;
	for (i = mfea_ifp->vifs().begin(); i != mfea_ifp->vifs().end(); i++) {
	    vifs.push_back(i->first); // name
	    MfeaVif *mfea_vif = vif_find_by_name(i->first);
	    if (mfea_vif) {
		module_names.push_back(mfea_vif->registered_module_instance_name());
	    }

	    // Delete the mcast vif from the kernel.
	    delete_multicast_vif(mfea_vif->vif_index());
	}

	// Now, for all vifs/modules unregister_protocol.
	string err_msg;
	list<string>::iterator si;
	for (si = vifs.begin(); si != vifs.end(); si++) {
	    list<string>::iterator mi;
	    for (mi = module_names.begin(); mi != module_names.end(); mi++) {
		unregister_protocol(*mi, if_name, *si, err_msg);
	    }
	}
    }
}


void MfeaNode::unregister_protocols_for_vif(const string& ifname, const string& vifname) {
    MfeaVif *mfea_vif = vif_find_by_name(vifname);
    if (mfea_vif) {
	string mn(mfea_vif->registered_module_instance_name());
	string err_msg;

	// Delete the mcast vif from the kernel.
	delete_multicast_vif(mfea_vif->vif_index());

	unregister_protocol(mn, ifname, vifname, err_msg);
    }
}

int
MfeaNode::unregister_protocol(const string&	module_instance_name,
			      const string&	if_name,
			      const string&	vif_name,
			      string&		error_msg)
{

    XLOG_WARNING("unregister_protocol: module: %s  iface: %s/%s\n",
		 module_instance_name.c_str(), if_name.c_str(), vif_name.c_str());

    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	error_msg = c_format("Cannot unregister module %s on interface %s "
			     "vif %s: no such vif (will continue)",
			     module_instance_name.c_str(),
			     if_name.c_str(),
			     vif_name.c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	return XORP_OK;
    }

    uint8_t ip_protocol = mfea_vif->registered_ip_protocol();
    if (mfea_vif->unregister_protocol(module_instance_name, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    //
    // If necessary, remove from the global state for registered module
    // instance names and IP protocols.
    //
    bool name_found = false;
    bool ip_protocol_found = false;
    vector<MfeaVif *>::iterator iter;

    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (mfea_vif->registered_module_instance_name()
	    == module_instance_name) {
	    name_found = true;
	}
	if (mfea_vif->registered_ip_protocol() == ip_protocol) {
	    ip_protocol_found = true;
	}
	if (name_found && ip_protocol_found)
	    break;
    }
    
    if (! name_found)
	_registered_module_instance_names.erase(module_instance_name);

    if (! ip_protocol_found) {
	_registered_ip_protocols.erase(ip_protocol);

	// If necessary, stop PIM processing
	if (ip_protocol == IPPROTO_PIM) {
	    if (mfea_mrouter().stop_pim(error_msg) != XORP_OK) {
		error_msg = c_format("Cannot stop PIM processing: %s",
				     error_msg.c_str());
		// XXX: don't return error, but just print the error message
		XLOG_ERROR("%s", error_msg.c_str());
	    }
	}
    }

    return (XORP_OK);
}

/**
 * MfeaNode::signal_message_recv:
 * @src_module_instance_name: Unused.
 * @message_type: The message type of the kernel signal
 * (%IGMPMSG_* or %MRT6MSG_*)
 * @vif_index: The vif index of the related interface (message-specific).
 * @src: The source address in the message.
 * @dst: The destination address in the message.
 * @rcvbuf: The data buffer with the additional information in the message.
 * @rcvlen: The data length in @rcvbuf.
 * 
 * Process NOCACHE, WRONGVIF/WRONGMIF, WHOLEPKT, BW_UPCALL signals from the
 * kernel. The signal is sent to all user-level protocols that expect it.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::signal_message_recv(const string&	, // src_module_instance_name,
			      int message_type,
			      uint32_t vif_index,
			      const IPvX& src, const IPvX& dst,
			      const uint8_t *rcvbuf, size_t rcvlen)
{
    XLOG_TRACE(is_log_trace(),
	       "RX kernel signal: "
	       "message_type = %d vif_index = %d src = %s dst = %s",
	       message_type, vif_index,
	       cstring(src), cstring(dst));

    if (! is_up())
	return (XORP_ERROR);
    
    //
    // If it is a bandwidth upcall message, parse it now
    //
    if (message_type == MFEA_KERNEL_MESSAGE_BW_UPCALL) {
	//
	// XXX: do we need to check if the kernel supports the
	// bandwidth-upcall mechanism?
	//
	
	//
	// Do the job
	//
	switch (family()) {
	case AF_INET:
	{
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	    size_t from = 0;
	    struct bw_upcall bw_upcall;
	    IPvX src(family()), dst(family());
	    bool is_threshold_in_packets, is_threshold_in_bytes;
	    bool is_geq_upcall, is_leq_upcall;
	    
	    while (rcvlen >= sizeof(bw_upcall)) {
		memcpy(&bw_upcall, rcvbuf + from, sizeof(bw_upcall));
		rcvlen -= sizeof(bw_upcall);
		from += sizeof(bw_upcall);
		
		src.copy_in(bw_upcall.bu_src);
		dst.copy_in(bw_upcall.bu_dst);
		is_threshold_in_packets
		    = bw_upcall.bu_flags & BW_UPCALL_UNIT_PACKETS;
		is_threshold_in_bytes
		    = bw_upcall.bu_flags & BW_UPCALL_UNIT_BYTES;
		is_geq_upcall = bw_upcall.bu_flags & BW_UPCALL_GEQ;
		is_leq_upcall = bw_upcall.bu_flags & BW_UPCALL_LEQ;
		signal_dataflow_message_recv(
		    src,
		    dst,
		    TimeVal(bw_upcall.bu_threshold.b_time),
		    TimeVal(bw_upcall.bu_measured.b_time),
		    bw_upcall.bu_threshold.b_packets,
		    bw_upcall.bu_threshold.b_bytes,
		    bw_upcall.bu_measured.b_packets,
		    bw_upcall.bu_measured.b_bytes,
		    is_threshold_in_packets,
		    is_threshold_in_bytes,
		    is_geq_upcall,
		    is_leq_upcall);
	    }
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
	}
	break;
	
#ifdef HAVE_IPV6
	case AF_INET6:
	{
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("signal_message_recv() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	    size_t from = 0;
	    struct bw6_upcall bw_upcall;
	    IPvX src(family()), dst(family());
	    bool is_threshold_in_packets, is_threshold_in_bytes;
	    bool is_geq_upcall, is_leq_upcall;
	    
	    while (rcvlen >= sizeof(bw_upcall)) {
		memcpy(&bw_upcall, rcvbuf + from, sizeof(bw_upcall));
		rcvlen -= sizeof(bw_upcall);
		from += sizeof(bw_upcall);
		
		src.copy_in(bw_upcall.bu6_src);
		dst.copy_in(bw_upcall.bu6_dsr);
		is_threshold_in_packets
		    = bw_upcall.bu6_flags & BW_UPCALL_UNIT_PACKETS;
		is_threshold_in_bytes
		    = bw_upcall.bu6_flags & BW_UPCALL_UNIT_BYTES;
		is_geq_upcall = bw_upcall.bu6_flags & BW_UPCALL_GEQ;
		is_leq_upcall = bw_upcall.bu6_flags & BW_UPCALL_LEQ;
		signal_dataflow_message_recv(
		    src,
		    dst,
		    TimeVal(bw_upcall.bu6_threshold.b_time),
		    TimeVal(bw_upcall.bu6_measured.b_time),
		    bw_upcall.bu6_threshold.b_packets,
		    bw_upcall.bu6_threshold.b_bytes,
		    bw_upcall.bu6_measured.b_packets,
		    bw_upcall.bu6_measured.b_bytes,
		    is_threshold_in_packets,
		    is_threshold_in_bytes,
		    is_geq_upcall,
		    is_leq_upcall);
	    }
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
#endif // HAVE_IPV6_MULTICAST_ROUTING
	}
	break;
#endif // HAVE_IPV6
	
	default:
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    
    //
    // Test if we should accept or drop the message
    //
    MfeaVif *mfea_vif = vif_find_by_vif_index(vif_index);
    if (mfea_vif == NULL) {
	XLOG_ERROR("signal_message_recv, can't find mfea_vif, vif_index: %i\n",
		   vif_index);
	return (XORP_ERROR);
    }
    
    //
    // Send the signal to all upper-layer protocols that expect it.
    //
    set<string>::const_iterator iter;
    for (iter = _registered_module_instance_names.begin();
	 iter != _registered_module_instance_names.end();
	 ++iter) {
	const string& dst_module_instance_name = (*iter);
	signal_message_send(dst_module_instance_name,
			    message_type,
			    vif_index,
			    src, dst,
			    rcvbuf,
			    rcvlen);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::signal_dataflow_message_recv:
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @measured_interval: The dataflow measured interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @measured_packets: The number of packets measured within
 * the @measured_interval.
 * @measured_bytes: The number of bytes measured within
 * the @measured_interval.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * 
 * Process a dataflow upcall from the kernel or from the MFEA internal
 * bandwidth-estimation mechanism (i.e., periodic reading of the kernel
 * multicast forwarding statistics).
 * The signal is sent to all user-level protocols that expect it.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::signal_dataflow_message_recv(const IPvX& source, const IPvX& group,
				       const TimeVal& threshold_interval,
				       const TimeVal& measured_interval,
				       uint32_t threshold_packets,
				       uint32_t threshold_bytes,
				       uint32_t measured_packets,
				       uint32_t measured_bytes,
				       bool is_threshold_in_packets,
				       bool is_threshold_in_bytes,
				       bool is_geq_upcall,
				       bool is_leq_upcall)
{
    XLOG_TRACE(is_log_trace(),
	       "RX dataflow message: "
	       "src = %s dst = %s",
	       cstring(source), cstring(group));
    
    if (! is_up())
	return (XORP_ERROR);
    
    //
    // Send the signal to all upper-layer protocols that expect it.
    //
    set<string>::const_iterator iter;
    for (iter = _registered_module_instance_names.begin();
	 iter != _registered_module_instance_names.end();
	 ++iter) {
	const string& dst_module_instance_name = (*iter);
	dataflow_signal_send(dst_module_instance_name,
			     source,
			     group,
			     threshold_interval.sec(),
			     threshold_interval.usec(),
			     measured_interval.sec(),
			     measured_interval.usec(),
			     threshold_packets,
			     threshold_bytes,
			     measured_packets,
			     measured_bytes,
			     is_threshold_in_packets,
			     is_threshold_in_bytes,
			     is_geq_upcall,
			     is_leq_upcall);
    }
    
    return (XORP_OK);
}

int MfeaNode::add_mfc_str(const string& module_instance_name,
			  const IPvX& source,
			  const IPvX& group,
			  const string& iif_name,
			  const string& oif_names,
			  uint32_t distance,
			  string& error_msg, bool check_stored_routes) {
    int rv;
    uint32_t iif_vif_index;

    XLOG_INFO("MFEA add_mfc_str, module: %s  source: %s  group: %s check-stored-routes: %i distance: %u",
	      module_instance_name.c_str(), source.str().c_str(),
	      group.str().c_str(), (int)(check_stored_routes), distance);

    if (distance >= MAX_MFEA_DISTANCE) {
	error_msg += c_format("distance is above maximimum: %u >= %u\n",
			      distance, MAX_MFEA_DISTANCE);
	return XORP_ERROR;
    }

    if (check_stored_routes) {
	MfeaRouteStorage mrs(distance, module_instance_name, source,
			     group, iif_name, oif_names);
	routes[mrs.distance][mrs.getHash()] = mrs;

	// If any lower distance routes are already in place, do nothing and
	// return.
	if (mrs.distance > 0) {
	    for (unsigned int i = mrs.distance - 1; i > 0; i--) {
		map<string, MfeaRouteStorage>::const_iterator iter = routes[i].find(mrs.getHash());
		if (iter != routes[i].end()) {
		    XLOG_INFO("Lower-distance mfea route exists, distance: %i source: %s  group: %s",
			      i, source.str().c_str(), group.str().c_str());
		    return XORP_OK;
		}
	    }
	}

	// If any higher distance routes are already in place, remove them from kernel.
	for (unsigned int i = mrs.distance + 1; i<MAX_MFEA_DISTANCE; i++) {
	    map<string, MfeaRouteStorage>::const_iterator iter = routes[i].find(mrs.getHash());
	    if (iter != routes[i].end()) {
		XLOG_INFO("Removing higher-distance mfea route, distance: %i source: %s  group: %s",
			  i, source.str().c_str(), group.str().c_str());
		delete_mfc(iter->second.module_instance_name, iter->second.source,
			   iter->second.group, error_msg, false);
		break;
	    }
	}
    }

    // Convert strings to mfc binary stuff and pass to
    // add_mfc()
    MfeaVif *mfea_vif = vif_find_by_name(iif_name);
    if (!mfea_vif) {
	error_msg = "Could not find iif-interface: " + iif_name;
	return XORP_ERROR;
    }
    iif_vif_index = mfea_vif->vif_index();

    Mifset oiflist;
    Mifset oiflist_disable_wrongvif;
    istringstream iss(oif_names);
    string t;
    while (!(iss.eof() || iss.fail())) {
	iss >> t;
	if (t.size() == 0)
	    break;
	mfea_vif = vif_find_by_name(t);
	if (!mfea_vif) {
	    error_msg = "Could not find oif-interface: " + t;
	    return XORP_ERROR;
	}
	uint32_t vi = mfea_vif->vif_index();
	if (vi < MAX_VIFS) {
	    oiflist.set(vi);
	}
	else {
	    error_msg = c_format("Vif %s has invalid vif_index: %u",
				 mfea_vif->ifname().c_str(), vi);
	    return XORP_ERROR;
	}
    }

    IPvX rp_addr;

    rv = add_mfc(module_instance_name, source, group,
		 iif_vif_index, oiflist,
		 oiflist_disable_wrongvif,
		 MAX_VIFS, rp_addr, distance, error_msg, false);
    if (rv != XORP_OK) {
	error_msg = "call to add_mfc failed";
    }
    return rv;
}

/**
 * MfeaNode::add_mfc:
 * @module_instance_name: The module instance name of the protocol that adds
 *       the MFC.
 * @source: The source address.
 * @group: The group address.
 * @iif_vif_index: The vif index of the incoming interface.
 * @oiflist: The bitset with the outgoing interfaces.
 * @oiflist_disable_wrongvif: The bitset with the outgoing interfaces to
 * disable the WRONGVIF signal.
 * @max_vifs_oiflist: The number of vifs covered by @oiflist
 *        or @oiflist_disable_wrongvif.
 * @rp_addr: The RP address.
 * 
 * Add Multicast Forwarding Cache (MFC) to the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_mfc(const string& module_instance_name,
		  const IPvX& source, const IPvX& group,
		  uint32_t iif_vif_index, const Mifset& oiflist,
		  const Mifset& oiflist_disable_wrongvif,
		  uint32_t max_vifs_oiflist,
		  const IPvX& rp_addr, uint32_t distance,
		  string& error_msg,
		  bool check_stored_routes)
{
    uint8_t oifs_ttl[MAX_VIFS];
    uint8_t oifs_flags[MAX_VIFS];

    XLOG_INFO("MFEA add_mfc, module: %s  source: %s  group: %s check-stored-routes: %i distance: %u",
	      module_instance_name.c_str(), source.str().c_str(),
	      group.str().c_str(), (int)(check_stored_routes), distance);

    if (distance >= MAX_MFEA_DISTANCE) {
	error_msg += c_format("distance is above maximimum: %u >= %u\n",
			      distance, MAX_MFEA_DISTANCE);
	return XORP_ERROR;
    }

    if (max_vifs_oiflist > MAX_VIFS) {
	error_msg += c_format("max-vifs-oiflist: %u > MAX_VIFS: %u\n",
			      max_vifs_oiflist, MAX_VIFS);
	return XORP_ERROR;
    }
    
    // Check the iif
    if (iif_vif_index == Vif::VIF_INDEX_INVALID) {
	error_msg += "iif_vif_index is VIF_INDEX_INVALID\n";
	return XORP_ERROR;
    }

    if (iif_vif_index >= max_vifs_oiflist) {
	error_msg += c_format("iif_vif_index: %u >= max-vifs-oiflist: %u\n",
			      iif_vif_index, max_vifs_oiflist);
	return XORP_ERROR;
    }

    if (check_stored_routes) {
	MfeaRouteStorage mrs(distance, module_instance_name, source,
			     group, iif_vif_index, oiflist,
			     oiflist_disable_wrongvif, max_vifs_oiflist,
			     rp_addr);
	routes[mrs.distance][mrs.getHash()] = mrs;

	// If any lower distance routes are already in place, do nothing and
	// return.
	if (mrs.distance > 0) {
	    for (unsigned int i = mrs.distance - 1; i > 0; i--) {
		map<string, MfeaRouteStorage>::const_iterator iter = routes[i].find(mrs.getHash());
		if (iter != routes[i].end()) {
		    XLOG_INFO("Lower-distance mfea route exists, distance: %i source: %s  group: %s",
			      i, source.str().c_str(), group.str().c_str());
		    return XORP_OK;
		}
	    }
	}

	// If any higher distance routes are already in place, remove them from kernel.
	for (unsigned int i = mrs.distance + 1; i<MAX_MFEA_DISTANCE; i++) {
	    map<string, MfeaRouteStorage>::const_iterator iter = routes[i].find(mrs.getHash());
	    if (iter != routes[i].end()) {
		XLOG_INFO("Removing higher-distance mfea route, distance: %i source: %s  group: %s",
			  i, source.str().c_str(), group.str().c_str());
		delete_mfc(iter->second.module_instance_name, iter->second.source,
			   iter->second.group, error_msg, false);
		break;
	    }
	}
    }
    
    //
    // Reset the initial values
    //
    for (size_t i = 0; i < MAX_VIFS; i++) {
	oifs_ttl[i] = 0;
	oifs_flags[i] = 0;
    }
    
    //
    // Set the minimum required TTL for each outgoing interface,
    // and the optional flags.
    //
    // TODO: XXX: PAVPAVPAV: the TTL should be configurable per vif.
    for (size_t i = 0; i < max_vifs_oiflist; i++) {
	// Set the TTL
	if (oiflist.test(i))
	    oifs_ttl[i] = MINTTL;
	else
	    oifs_ttl[i] = 0;
	
	// Set the flags
	oifs_flags[i] = 0;
	
	if (oiflist_disable_wrongvif.test(i)) {
	    switch (family()) {
	    case AF_INET:
#if defined(MRT_MFC_FLAGS_DISABLE_WRONGVIF) && defined(ENABLE_ADVANCED_MULTICAST_API)
		oifs_flags[i] |= MRT_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
	    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
		string em = "add_mfc() failed: IPv6 multicast routing not supported\n";
		error_msg += em;
		XLOG_ERROR(em.c_str());
		return XORP_ERROR;
#else
#if defined(MRT6_MFC_FLAGS_DISABLE_WRONGVIF) && defined(ENABLE_ADVANCED_MULTICAST_API)
		oifs_flags[i] |= MRT6_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
#endif // HAVE_IPV6_MULTICAST_ROUTING
	    }
	    break;
#endif // HAVE_IPV6
	    
	    default:
		XLOG_UNREACHABLE();
		return (XORP_ERROR);
	    }
	}
    }
    
    if (_mfea_mrouter.add_mfc(source, group, iif_vif_index, oifs_ttl,
			      oifs_flags, rp_addr)
	!= XORP_OK) {
	error_msg += "mfea_mrouter add_mfc failed.\n";
	return XORP_ERROR;
    }
    
    return XORP_OK;
}

/**
 * MfeaNode::delete_mfc:
 * @module_instance_name: The module instance name of the protocol that deletes
 * the MFC.
 * @source: The source address.
 * @group: The group address.
 * 
 * Delete Multicast Forwarding Cache (MFC) from the kernel.
 * XXX: All corresponding dataflow entries are also removed.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_mfc(const string& module_instance_name,
		     const IPvX& source, const IPvX& group,
		     string& error_msg, bool check_stored_routes)
{
    bool do_rem_route = true;
    int rv;
    string hash = source.str() + ":" + group.str();

    XLOG_INFO("delete-mfc, module: %s  source: %s  group: %s  check-stored-routes: %i\n",
	      module_instance_name.c_str(), source.str().c_str(),
	      group.str().c_str(), (int)(check_stored_routes));

    if (check_stored_routes) {
	// find existing route by instance-name.
	for (unsigned int i = 0; i<MAX_MFEA_DISTANCE; i++) {
	    map<string, MfeaRouteStorage>::iterator iter = routes[i].find(hash);
	    if (iter != routes[i].end()) {
		// Found something to delete..see if it belongs to us?
		if (iter->second.module_instance_name == module_instance_name) {
		    // Yep, that's us.
		    routes[i].erase(hash);
		    goto check_remove_route;
		}
		else {
		    // was some other protocol's route.  Don't mess with this,
		    // and don't remove anything from the kernel.
		    do_rem_route = false;
		}
	    }
	}
    }

  check_remove_route:

    if (! do_rem_route) {
	return XORP_OK;
    }

    rv = _mfea_mrouter.delete_mfc(source, group);
    
    // Remove all corresponding dataflow entries
    mfea_dft().delete_entry(source, group);

    if (check_stored_routes) {
	// Now, see if we have any higher-distance routes to add.
	for (unsigned int i = 0; i<MAX_MFEA_DISTANCE; i++) {
	    map<string, MfeaRouteStorage>::iterator iter = routes[i].find(hash);
	    if (iter != routes[i].end()) {
		if (iter->second.isBinary()) {
		    rv = add_mfc(iter->second.module_instance_name,
				 iter->second.source, iter->second.group,
				 iter->second.iif_vif_index,
				 iter->second.oiflist, iter->second.oiflist_disable_wrongvif,
				 iter->second.max_vifs_oiflist, iter->second.rp_addr,
				 iter->second.distance, error_msg, false);
		}
		else {
		    rv = add_mfc_str(iter->second.module_instance_name,
				     iter->second.source, iter->second.group,
				     iter->second.iif_name, iter->second.oif_names,
				     iter->second.distance, error_msg, false);
		}
		break;
	    }
	}
    }

    return rv;
}

/**
 * MfeaNode::add_dataflow_monitor:
 * @module_instance_name: The module instance name of the protocol that adds
 * the dataflow monitor entry.
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * @error_msg: The error message (if error).
 * 
 * Add a dataflow monitor entry.
 * Note: either @is_threshold_in_packets or @is_threshold_in_bytes (or both)
 * must be true.
 * Note: either @is_geq_upcall or @is_leq_upcall (but not both) must be true.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_dataflow_monitor(const string& ,	// module_instance_name,
			       const IPvX& source, const IPvX& group,
			       const TimeVal& threshold_interval,
			       uint32_t threshold_packets,
			       uint32_t threshold_bytes,
			       bool is_threshold_in_packets,
			       bool is_threshold_in_bytes,
			       bool is_geq_upcall,
			       bool is_leq_upcall,
			       string& error_msg)
{
    // XXX: flags is_geq_upcall and is_leq_upcall are mutually exclusive
    if (! (is_geq_upcall ^ is_leq_upcall)) {
	error_msg = c_format("Cannot add dataflow monitor for (%s, %s): "
			     "the GEQ and LEQ flags are mutually exclusive "
			     "(GEQ = %s; LEQ = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_geq_upcall),
			     bool_c_str(is_leq_upcall));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    // XXX: at least one of the threshold flags must be set
    if (! (is_threshold_in_packets || is_threshold_in_bytes)) {
	error_msg = c_format("Cannot add dataflow monitor for (%s, %s): "
			     "invalid threshold flags "
			     "(is_threshold_in_packets = %s; "
			     "is_threshold_in_bytes = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_threshold_in_packets),
			     bool_c_str(is_threshold_in_bytes));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    
    //
    // If the kernel supports bandwidth-related upcalls, use it
    //
    if (_mfea_mrouter.mrt_api_mrt_mfc_bw_upcall()) {
	if (_mfea_mrouter.add_bw_upcall(source, group,
					threshold_interval,
					threshold_packets,
					threshold_bytes,
					is_threshold_in_packets,
					is_threshold_in_bytes,
					is_geq_upcall,
					is_leq_upcall,
					error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    
    //
    // The kernel doesn't support bandwidth-related upcalls, hence use
    // a work-around mechanism (periodic quering).
    //
    if (mfea_dft().add_entry(source, group,
			     threshold_interval,
			     threshold_packets,
			     threshold_bytes,
			     is_threshold_in_packets,
			     is_threshold_in_bytes,
			     is_geq_upcall,
			     is_leq_upcall,
			     error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_dataflow_monitor:
 * @module_instance_name: The module instance name of the protocol that deletes
 * the dataflow monitor entry.
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * @error_msg: The error message (if error).
 * 
 * Delete a dataflow monitor entry.
 * Note: either @is_threshold_in_packets or @is_threshold_in_bytes (or both)
 * must be true.
 * Note: either @is_geq_upcall or @is_leq_upcall (but not both) must be true.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_dataflow_monitor(const string& , // module_instance_name,
				  const IPvX& source, const IPvX& group,
				  const TimeVal& threshold_interval,
				  uint32_t threshold_packets,
				  uint32_t threshold_bytes,
				  bool is_threshold_in_packets,
				  bool is_threshold_in_bytes,
				  bool is_geq_upcall,
				  bool is_leq_upcall,
				  string& error_msg)
{
    // XXX: flags is_geq_upcall and is_leq_upcall are mutually exclusive
    if (! (is_geq_upcall ^ is_leq_upcall)) {
	error_msg = c_format("Cannot delete dataflow monitor for (%s, %s): "
			     "the GEQ and LEQ flags are mutually exclusive "
			     "(GEQ = %s; LEQ = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_geq_upcall),
			     bool_c_str(is_leq_upcall));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    // XXX: at least one of the threshold flags must be set
    if (! (is_threshold_in_packets || is_threshold_in_bytes)) {
	error_msg = c_format("Cannot delete dataflow monitor for (%s, %s): "
			     "invalid threshold flags "
			     "(is_threshold_in_packets = %s; "
			     "is_threshold_in_bytes = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_threshold_in_packets),
			     bool_c_str(is_threshold_in_bytes));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    
    //
    // If the kernel supports bandwidth-related upcalls, use it
    //
    if (_mfea_mrouter.mrt_api_mrt_mfc_bw_upcall()) {
	if (_mfea_mrouter.delete_bw_upcall(source, group,
					   threshold_interval,
					   threshold_packets,
					   threshold_bytes,
					   is_threshold_in_packets,
					   is_threshold_in_bytes,
					   is_geq_upcall,
					   is_leq_upcall,
					   error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    
    //
    // The kernel doesn't support bandwidth-related upcalls, hence use
    // a work-around mechanism (periodic quering).
    //
    if (mfea_dft().delete_entry(source, group,
				threshold_interval,
				threshold_packets,
				threshold_bytes,
				is_threshold_in_packets,
				is_threshold_in_bytes,
				is_geq_upcall,
				is_leq_upcall,
				error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_all_dataflow_monitor:
 * @module_instance_name: The module instance name of the protocol that deletes
 * the dataflow monitor entry.
 * @source: The source address.
 * @group: The group address.
 * @error_msg: The error message (if error).
 * 
 * Delete all dataflow monitor entries for a given @source and @group address.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_all_dataflow_monitor(const string& , // module_instance_name,
				      const IPvX& source, const IPvX& group,
				      string& error_msg)
{
    //
    // If the kernel supports bandwidth-related upcalls, use it
    //
    if (_mfea_mrouter.mrt_api_mrt_mfc_bw_upcall()) {
	if (_mfea_mrouter.delete_all_bw_upcall(source, group, error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    
    //
    // The kernel doesn't support bandwidth-related upcalls, hence use
    // a work-around mechanism (periodic quering).
    //
    if (mfea_dft().delete_entry(source, group) != XORP_OK) {
	error_msg = c_format("Cannot delete dataflow monitor for (%s, %s): "
			     "no such entry",
			     cstring(source), cstring(group));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::add_multicast_vif:
 * @vif_index: The vif index of the interface to add.
 * 
 * Add a multicast vif to the kernel.
 * 
 * Return value: %XORP_OK on success, othewise %XORP_ERROR.
 **/
int
MfeaNode::add_multicast_vif(uint32_t vif_index, string& error_msg)
{
    if (_mfea_mrouter.add_multicast_vif(vif_index, error_msg) != XORP_OK) {
	return XORP_ERROR;
    }
    
    return XORP_OK;
}

/**
 * MfeaNode::delete_multicast_vif:
 * @vif_index: The vif index of the interface to delete.
 * 
 * Delete a multicast vif from the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_multicast_vif(uint32_t vif_index)
{
    if (_mfea_mrouter.delete_multicast_vif(vif_index) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::get_sg_count:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * @sg_count: A reference to a #SgCount class to place the result: the
 * number of packets and bytes forwarded by the particular MFC entry, and the
 * number of packets arrived on a wrong interface.
 * 
 * Get the number of packets and bytes forwarded by a particular
 * Multicast Forwarding Cache (MFC) entry in the kernel, and the number
 * of packets arrived on wrong interface for that entry.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::get_sg_count(const IPvX& source, const IPvX& group,
		       SgCount& sg_count)
{
    if (_mfea_mrouter.get_sg_count(source, group, sg_count) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::get_vif_count:
 * @vif_index: The vif index of the virtual multicast interface whose
 * statistics we need.
 * @vif_count: A reference to a #VifCount class to store the result.
 * 
 * Get the number of packets and bytes received on, or forwarded on
 * a particular multicast interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::get_vif_count(uint32_t vif_index, VifCount& vif_count)
{
    if (_mfea_mrouter.get_vif_count(vif_index, vif_count) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}
