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

#ident "$XORP: xorp/fea/mfea_node.cc,v 1.12 2003/08/05 05:37:29 pavlin Exp $"


//
// MFEA (Multicast Forwarding Engine Abstraction) implementation.
//


#include "mfea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mrt/max_vifs.h"
#include "mrt/mifset.hh"
#include "mrt/multicast_defs.h"

#include "mfea_mrouter.hh"
#include "mfea_node.hh"
#include "mfea_proto_comm.hh"
#include "mfea_kernel_messages.hh"
#include "mfea_vif.hh"

// TODO: XXX: PAVPAVPAV: TEMPORARY HERE!!
#include "fticonfig.hh"

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
 * MfeaNode::MfeaNode:
 * @family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @module_id: The module ID (must be %XORP_MODULE_MFEA).
 * @eventloop: The event loop.
 * @ftic the @ref FtiConfig entry to use to obtain the routing
 * table (NOTE (TODO: XXX: PAVPAVPAV: this parameter is only temporary here,
 * and will be removed in the near future).
 * 
 * MFEA node constructor.
 **/
MfeaNode::MfeaNode(int family, xorp_module_id module_id,
		   EventLoop& eventloop, FtiConfig& ftic)
    : ProtoNode<MfeaVif>(family, module_id, eventloop),
    _mfea_mrouter(*this),
    _mrib_table(family),
    _mrib_table_default_metric_preference(MRIB_TABLE_DEFAULT_METRIC_PREFERENCE),
    _mrib_table_default_metric(MRIB_TABLE_DEFAULT_METRIC),
    _mfea_dft(*this),
    _is_log_trace(true),		// XXX: default to print trace logs
    _ftic(ftic)
{
    XLOG_ASSERT(module_id == XORP_MODULE_MFEA);
    
    if (module_id != XORP_MODULE_MFEA) {
	XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_MFEA' = %d)",
		   module_id, XORP_MODULE_MFEA);
    }
    
    for (size_t i = 0; i < _proto_comms.size(); i++)
	_proto_comms[i] = NULL;
    
    // Set the node status.
    // XXX: note that we don't really need to wait for FEA, hence we are READY.
    ProtoNode<MfeaVif>::set_node_status(PROC_READY);
}

/**
 * MfeaNode::~MfeaNode:
 * @void: 
 * 
 * MFEA node destructor.
 * 
 **/
MfeaNode::~MfeaNode(void)
{
    stop();
    
    delete_all_vifs();
    
    // Delete the ProtoComm entries
    for (size_t i = 0; i < _proto_comms.size(); i++) {
	if (_proto_comms[i] != NULL)
	    delete _proto_comms[i];
	_proto_comms[i] = NULL;
    }
}

/**
 * MfeaNode::start:
 * @void: 
 * 
 * Start the MFEA.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
MfeaNode::start(void)
{
    if (ProtoNode<MfeaVif>::start() < 0)
	return (XORP_ERROR);
    
    // Set the node status.
    // XXX: note that we don't really need to wait for FEA, hence we are READY.
    ProtoNode<MfeaVif>::set_node_status(PROC_READY);
    
    // Start the MfeaMrouter
    _mfea_mrouter.start();
    
    // Start the ProtoComm entries
    for (size_t i = 0; i < _proto_comms.size(); i++) {
	if (_proto_comms[i] != NULL) {
	    _proto_comms[i]->start();
	}
    }
    
    // Start the mfea_vifs
    start_all_vifs();
    
    // Start the timer to read immediately the MRIB table
    mrib_table().clear();
    _mrib_table_read_timer =
	eventloop().new_oneoff_after(TimeVal(0,0),
				     callback(this, &MfeaNode::mrib_table_read_timer_timeout));
    
    return (XORP_OK);
}

/**
 * MfeaNode::stop:
 * @void: 
 * 
 * Stop the MFEA.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop(void)
{
    if (! is_up())
	return (XORP_ERROR);
    
    // Stop the vifs
    stop_all_vifs();
    
    // Stop the ProtoComm entries
    for (size_t i = 0; i < _proto_comms.size(); i++) {
	if (_proto_comms[i] != NULL)
	    _proto_comms[i]->stop();
    }
    
    // Clear the MRIB table
    _mrib_table_read_timer.unschedule();
    mrib_table().clear();
    
    if (ProtoNode<MfeaVif>::stop() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * MfeaNode::add_vif:
 * @vif: Vif information about new MfeaVif to install.
 * @err: The error message (if error).
 * 
 * Install a new MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_vif(const Vif& vif, string& err)
{
    //
    // Create a new MfeaVif
    //
    MfeaVif *mfea_vif = new MfeaVif(*this, vif);
    
    if (ProtoNode<MfeaVif>::add_vif(mfea_vif) != XORP_OK) {
	// Cannot add this new vif
	err = c_format("Cannot add vif %s: internal error",
		       vif.name().c_str());
	XLOG_ERROR(err.c_str());
	
	delete mfea_vif;
	return (XORP_ERROR);
    }
    
    XLOG_INFO("New vif: %s", mfea_vif->str().c_str());
    
    // XXX: add PIM Register vif (if needed)
    add_pim_register_vif();
    
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
    //
    // Test first whether we have already PIM Register vif
    //
    for (uint16_t i = 0; i < maxvifs(); i++) {
	MfeaVif *mfea_vif = vif_find_by_vif_index(i);
	if (mfea_vif == NULL)
	    continue;
	if (mfea_vif->is_pim_register())
	    return (XORP_OK);		// Found: OK
    }
    
    //
    // Create the PIM Register vif if there is a valid IP address
    //
    // TODO: check with Linux, Solaris, etc, if we can
    // use 127.0.0.2 or ::2 as a PIM Register vif address, and use that
    // address instead (otherwise we may always have to keep track
    // whether the underlying address has changed).
    //
    IPvX pim_register_vif_addr(IPvX::ZERO(family()));
    uint16_t pif_index = 0;
    for (uint16_t i = 0; i < maxvifs(); i++) {
	MfeaVif *mfea_vif = vif_find_by_vif_index(i);
	if (mfea_vif == NULL)
	    continue;
	if (! mfea_vif->is_underlying_vif_up())
	    continue;
	if (mfea_vif->addr_ptr() == NULL)
	    continue;
	if (mfea_vif->is_pim_register())
	    continue;
	if (mfea_vif->is_loopback())
	    continue;
	if (! mfea_vif->is_multicast_capable())
	    continue;
	// Found appropriate local address.
	pim_register_vif_addr = *(mfea_vif->addr_ptr());
	pif_index = mfea_vif->pif_index();
	break;
    }
    if (pim_register_vif_addr != IPvX::ZERO(family())) {
	// Add the Register vif
	uint16_t vif_index = find_unused_config_vif_index();
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	// TODO: XXX: the Register vif name is hardcoded here!
	MfeaVif register_vif(*this, Vif("register_vif"));
	register_vif.set_vif_index(vif_index);
	register_vif.set_pif_index(pif_index);
	register_vif.set_underlying_vif_up(true); // XXX: 'true' to allow creation
	register_vif.set_pim_register(true);
	register_vif.add_address(pim_register_vif_addr,
				 IPvXNet(pim_register_vif_addr,
					 pim_register_vif_addr.addr_bitlen()),
				 pim_register_vif_addr,
				 IPvX::ZERO(family()));
	string err;
	if (add_vif(register_vif, err) < 0) {
	    XLOG_ERROR("Cannot add Register vif: %s", err.c_str());
	    return (XORP_ERROR);
	}
	
	if (add_config_vif(register_vif, err) < 0) {
	    XLOG_ERROR("Cannot add Register vif to set of configured vifs: %s",
		       err.c_str());
	    return (XORP_ERROR);
	}
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_vif:
 * @vif_name: The name of the vif to delete.
 * @err: The error message (if error).
 * 
 * Delete an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_vif(const string& vif_name, string& err)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	err = c_format("Cannot delete vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    if (ProtoNode<MfeaVif>::delete_vif(mfea_vif) != XORP_OK) {
	err = c_format("Cannot delete vif %s: internal error",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	delete mfea_vif;
	return (XORP_ERROR);
    }
    
    delete mfea_vif;
    
    XLOG_INFO("Deleted vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::enable_vif:
 * @vif_name: The name of the vif to enable.
 * @err: The error message (if error).
 * 
 * Enable an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::enable_vif(const string& vif_name, string& err)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	err = c_format("Cannot enable vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    mfea_vif->enable();
    
    XLOG_INFO("Enabled vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::disable_vif:
 * @vif_name: The name of the vif to disable.
 * @err: The error message (if error).
 * 
 * Disable an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::disable_vif(const string& vif_name, string& err)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	err = c_format("Cannot disable vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    mfea_vif->disable();
    
    XLOG_INFO("Disabled vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::start_vif:
 * @vif_name: The name of the vif to start.
 * @err: The error message (if error).
 * 
 * Start an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::start_vif(const string& vif_name, string& err)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	err = c_format("Cannot start vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    if (mfea_vif->start() != XORP_OK) {
	err = c_format("Cannot start vif %s: internal error",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Started vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::stop_vif:
 * @vif_name: The name of the vif to stop.
 * @err: The error message (if error).
 * 
 * Stop an existing MFEA vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop_vif(const string& vif_name, string& err)
{
    MfeaVif *mfea_vif = vif_find_by_name(vif_name);
    if (mfea_vif == NULL) {
	err = c_format("Cannot stop vif %s: no such vif",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    if (mfea_vif->stop() != XORP_OK) {
	err = c_format("Cannot stop vif %s: internal error",
		       vif_name.c_str());
	XLOG_ERROR(err.c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Stopped vif: %s", vif_name.c_str());
    
    return (XORP_OK);
}

/**
 * MfeaNode::start_all_vifs:
 * @void: 
 * 
 * Start MFEA on all enabled interfaces.
 * 
 * Return value: The number of virtual interfaces MFEA was started on,
 * or %XORP_ERROR if error occured.
 **/
int
MfeaNode::start_all_vifs(void)
{
    int n = 0;
    vector<MfeaVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (mfea_vif->start() == XORP_OK)
	    n++;
    }
    
    return (n);
}

/**
 * MfeaNode::stop_all_vifs:
 * @void: 
 * 
 * Stop MFEA on all interfaces it was running on.
 * 
 * Return value: The number of virtual interfaces MFEA was stopped on,
 * or %XORP_ERROR if error occured.
 **/
int
MfeaNode::stop_all_vifs(void)
{
    int n = 0;
    vector<MfeaVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	if (mfea_vif->stop() == XORP_OK)
	    n++;
    }
    
    return (n);
}

/**
 * MfeaNode::enable_all_vifs:
 * @void: 
 * 
 * Enable MFEA on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::enable_all_vifs(void)
{
    vector<MfeaVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	mfea_vif->enable();
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::disable_all_vifs:
 * @void: 
 * 
 * Disable MFEA on all interfaces. All running interfaces are stopped first.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::disable_all_vifs(void)
{
    vector<MfeaVif *>::iterator iter;

    stop_all_vifs();
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	mfea_vif->disable();
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_all_vifs:
 * @void: 
 * 
 * Delete all MFEA vifs.
 **/
void
MfeaNode::delete_all_vifs(void)
{
    vector<MfeaVif *>::iterator iter;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	MfeaVif *mfea_vif = (*iter);
	if (mfea_vif == NULL)
	    continue;
	
	string vif_name = mfea_vif->name();
	
	string err;
	if (delete_vif(vif_name, err) != XORP_OK) {
	    err = c_format("Cannot delete vif %s: internal error",
			   vif_name.c_str());
	    XLOG_ERROR(err.c_str());
	}
    }
}

/**
 * MfeaNode::add_protocol:
 * @module_instance_name: The module instance name of the protocol to add.
 * @module_id: The #xorp_module_id of the protocol to add.
 * 
 * A method used by a protocol instance to register with this #MfeaNode.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_protocol(const string& module_instance_name,
		       xorp_module_id module_id)
{
    ProtoComm *proto_comm;
    int ipproto;
    size_t i;
    
    // Add the state
    if (_proto_register.add_protocol(module_instance_name, module_id) < 0) {
	XLOG_ERROR("Cannot add protocol instance %s with module_id = %d",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);	// Already added
    }
    
    // Test if we have already the appropriate ProtoComm
    if (proto_comm_find_by_module_id(module_id) != NULL)
	return (XORP_OK);
    
    //
    // Get the IP protocol number (IPPROTO_*)
    //
    ipproto = -1;
    switch (module_id) {
    case XORP_MODULE_MLD6IGMP:
	switch (family()) {
	case AF_INET:
	    ipproto = IPPROTO_IGMP;
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    ipproto = IPPROTO_ICMPV6;
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    _proto_register.delete_protocol(module_instance_name, module_id);
	    return (XORP_ERROR);
	}
	break;
    case XORP_MODULE_PIMSM:
    case XORP_MODULE_PIMDM:
	ipproto = IPPROTO_PIM;
	break;
    default:
	XLOG_UNREACHABLE();
	_proto_register.delete_protocol(module_instance_name, module_id);
	return (XORP_ERROR);
    }
    
    proto_comm = new ProtoComm(*this, ipproto, module_id);
    
    // Add the new entry
    for (i = 0; i < _proto_comms.size(); i++) {
	if (_proto_comms[i] == NULL)
	    break;
    }
    if (i < _proto_comms.size()) {
	_proto_comms[i] = proto_comm;
    } else {
	_proto_comms.push_back(proto_comm);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_protocol:
 * @module_instance_name: The module instance name of the protocol to delete.
 * @module_id: The #xorp_module_id of the protocol to delete.
 * 
 * A method used by a protocol instance to deregister with this #MfeaNode.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_protocol(const string& module_instance_name,
			  xorp_module_id module_id)
{
    // Delete MRIB registration
    if (_mrib_messages_register.is_registered(module_instance_name,
					      module_id)) {
	delete_allow_mrib_messages(module_instance_name, module_id);
    }
    
    // Delete kernel signal registration
    if (_kernel_signal_messages_register.is_registered(module_instance_name,
						       module_id)) {
	delete_allow_kernel_signal_messages(module_instance_name, module_id);
    }
    
    // Delete the state
    if (_proto_register.delete_protocol(module_instance_name, module_id) < 0) {
	XLOG_ERROR("Cannot delete protocol instance %s with module_id = %d",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);	// Probably not added before
    }
    
    if (! _proto_register.is_registered(module_id)) {
	//
	// The last registered protocol instance
	//
	ProtoComm *proto_comm = proto_comm_find_by_module_id(module_id);
	
	if (proto_comm == NULL)
	    return (XORP_ERROR);
	
	// Remove the pointer storage for this ProtoComm entry
	for (size_t i = 0; i < _proto_comms.size(); i++) {
	    if (_proto_comms[i] == proto_comm) {
		_proto_comms[i] = NULL;
		break;
	    }
	}
	if (_proto_comms[_proto_comms.size() - 1] == NULL) {
	    // Remove the last entry, if not used anymore
	    _proto_comms.pop_back();
	}
	
	delete proto_comm;
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::start_protocol:
 * @module_id: The #xorp_module_id of the protocol to start.
 * 
 * Start operation for protocol with #xorp_module_id of @module_id.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::start_protocol(xorp_module_id module_id)
{
    ProtoComm *proto_comm = proto_comm_find_by_module_id(module_id);
    
    if (proto_comm == NULL)
	return (XORP_ERROR);
    
    if (proto_comm->is_up())
	return (XORP_OK);		// Already running
    
    if (proto_comm->start() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * MfeaNode::stop_protocol:
 * @module_id: The #xorp_module_id of the protocol to stop.
 * 
 * Stop operation for protocol with #xorp_module_id of @module_id.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop_protocol(xorp_module_id module_id)
{
    ProtoComm *proto_comm = proto_comm_find_by_module_id(module_id);
    
    if (proto_comm == NULL)
	return (XORP_ERROR);
    
    if (proto_comm->stop() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * MfeaNode::start_protocol_vif:
 * @module_instance_name: The module instance name of the protocol to start
 * on vif with vif_index of @vif_index.
 * @module_id: The #xorp_module_id of the protocol to start on vif with
 * vif_index of @vif_index.
 * @vif_index: The index of the vif the protocol start to apply to.
 * 
 * Start a protocol on an interface with vif_index of @vif_index.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::start_protocol_vif(const string& module_instance_name,
			     xorp_module_id module_id,
			     uint16_t vif_index)
{
    MfeaVif *mfea_vif = vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	XLOG_ERROR("Cannot start protocol instance %s on vif_index %d: "
		   "no such vif",
		   module_instance_name.c_str(), vif_index);
	return (XORP_ERROR);
    }
    
    if (mfea_vif->start_protocol(module_instance_name, module_id) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * MfeaNode::stop_protocol_vif:
 * @module_instance_name: The module instance name of the protocol to stop
 * on vif with vif_index of @vif_index.
 * @module_id: The #xorp_module_id of the protocol to stop on vif with
 * vif_index of @vif_index.
 * @vif_index: The index of the vif the protocol stop to apply to.
 * 
 * Stop a protocol on an interface with vif_index of @vif_index.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::stop_protocol_vif(const string& module_instance_name,
			    xorp_module_id module_id,
			    uint16_t vif_index)
{
    MfeaVif *mfea_vif = vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	XLOG_ERROR("Cannot stop protocol instance %s on vif_index %d: "
		   "no such vif",
		   module_instance_name.c_str(), vif_index);
	return (XORP_ERROR);
    }
    
    if (mfea_vif->stop_protocol(module_instance_name, module_id) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * MfeaNode::add_allow_kernel_signal_messages:
 * @module_instance_name: The module instance name of the protocol to add.
 * @module_id: The #xorp_module_id of the protocol to add to receive kernel
 * signal messages.
 * 
 * Add a protocol to the set of protocols that are interested in
 * receiving kernel signal messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_allow_kernel_signal_messages(const string& module_instance_name,
					   xorp_module_id module_id)
{
    // Add the state
    if (_kernel_signal_messages_register.add_protocol(module_instance_name,
						      module_id)
	< 0) {
	XLOG_ERROR("Cannot add protocol instance %s with module_id = %d "
		   "to receive kernel signal messages",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);	// Already added
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_allow_kernel_signal_messages:
 * @module_instance_name: The module instance name of the protocol to delete.
 * @module_id: The #xorp_module_id of the protocol to delete from receiving
 * kernel signal messages.
 * 
 * Delete a protocol from the set of protocols that are interested in
 * receiving kernel signal messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_allow_kernel_signal_messages(const string& module_instance_name,
					      xorp_module_id module_id)
{
    // Delete the state
    if (_kernel_signal_messages_register.delete_protocol(module_instance_name,
							 module_id)
	< 0) {
	XLOG_ERROR("Cannot delete protocol instance %s with module_id = %d "
		   "from receiving kernel signal messages",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);	// Probably not added before
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::add_allow_mrib_messages:
 * @module_instance_name: The module instance name of the protocol to add to
 * receive MRIB messages.
 * @module_id: The #xorp_module_id of the protocol to add to receive
 * MRIB messages.
 * 
 * Add a protocol to the set of protocols that are interested in
 * receiving MRIB messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_allow_mrib_messages(const string& module_instance_name,
				  xorp_module_id module_id)
{
    // Add the state
    if (_mrib_messages_register.add_protocol(module_instance_name, module_id)
	< 0) {
	XLOG_ERROR("Cannot add protocol instance %s with module_id = %d "
		   "to receive MRIB messages",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);	// Already added
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::delete_allow_mrib_messages:
 * @module_instance_name: The module instance name of the protocol to delete
 * from receiving MRIB messages.
 * @module_id: The #xorp_module_id of the protocol to delete from receiving
 * MRIB messages.
 * 
 * Delete a protocol from the set of protocols that are interested in
 * receiving MRIB messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_allow_mrib_messages(const string& module_instance_name,
				     xorp_module_id module_id)
{
    // Delete the state
    if (_mrib_messages_register.delete_protocol(module_instance_name,
						module_id)
	< 0) {
	XLOG_ERROR("Cannot delete protocol instance %s with module_id = %d "
		   "from receiving MRIB messages",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);	// Probably not added before
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::send_add_mrib:
 * @mrib: The #Mrib entry to add.
 * 
 * Add a #Mrib entry to all user-level protocols that are interested in
 * receiving MRIB messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::send_add_mrib(const Mrib& mrib)
{
    ProtoRegister& pr = _mrib_messages_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
	const string& dst_module_instance_name = (*iter).first;
	xorp_module_id dst_module_id = (*iter).second;
	send_add_mrib(dst_module_instance_name, dst_module_id, mrib);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::send_delete_mrib:
 * @mrib: The #Mrib entry to delete.
 * 
 * Delete a #Mrib entry from all user-level protocols that are interested in
 * receiving MRIB messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::send_delete_mrib(const Mrib& mrib)
{
    ProtoRegister& pr = _mrib_messages_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
	const string& dst_module_instance_name = (*iter).first;
	xorp_module_id dst_module_id = (*iter).second;
	send_delete_mrib(dst_module_instance_name, dst_module_id, mrib);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::send_set_mrib_done:
 * @: 
 * 
 * Complete a transaction of add/delete #Mrib entries with all user-level
 * protocols that are interested in receiving MRIB messages.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::send_set_mrib_done()
{
    ProtoRegister& pr = _mrib_messages_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    
    //
    // Send the message to all upper-layer protocols that expect it.
    //
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
	const string& dst_module_instance_name = (*iter).first;
	xorp_module_id dst_module_id = (*iter).second;
	send_set_mrib_done(dst_module_instance_name, dst_module_id);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::proto_recv:
 * @src_module_instance_name: The module instance name of the module-origin
 * of the message.
 * @src_module_id: The #xorp_module_id of the module-origin of the message.
 * @vif_index: The vif index of the interface to use to send this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @router_alert_bool: If true, set the Router Alert IP option for the IP
 * packet of the outgoung message.
 * @rcvbuf: The data buffer with the message to send.
 * @rcvlen: The data length in @rcvbuf.
 * 
 * Receive a protocol message from a user-level process, and send it
 * through the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::proto_recv(const string&	, // src_module_instance_name,
		     xorp_module_id src_module_id,
		     uint16_t vif_index,
		     const IPvX& src, const IPvX& dst,
		     int ip_ttl, int ip_tos, bool router_alert_bool,
		     const uint8_t *rcvbuf, size_t rcvlen)
{
    ProtoComm *proto_comm;
    
    if (! is_up())
	return (XORP_ERROR);
    
    // TODO: for now @src_module_id that comes by the
    // upper-layer protocol is used to find-out the ProtoComm entry.
    proto_comm = proto_comm_find_by_module_id(src_module_id);
    if (proto_comm == NULL)
	return (XORP_ERROR);
    
    if (proto_comm->proto_socket_write(vif_index,
				       src, dst,
				       ip_ttl,
				       ip_tos,
				       router_alert_bool,
				       rcvbuf,
				       rcvlen) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

// The function to process incoming messages from the kernel
int
MfeaNode::proto_comm_recv(xorp_module_id dst_module_id,
			  uint16_t vif_index,
			  const IPvX& src, const IPvX& dst,
			  int ip_ttl, int ip_tos, bool router_alert_bool,
			  const uint8_t *rcvbuf, size_t rcvlen)
{
    XLOG_TRACE(false & is_log_trace(),	// XXX: unconditionally disabled
	       "RX packet for dst_module_name %s: "
	       "vif_index = %d src = %s dst = %s ttl = %d tos = %#x "
	       "router_alert = %d rcvbuf = %p rcvlen = %u",
	       xorp_module_name(family(), dst_module_id), vif_index,
	       cstring(src), cstring(dst), ip_ttl, ip_tos, router_alert_bool,
	       rcvbuf, (uint32_t)rcvlen);
    
    //
    // Test if we should accept or drop the message
    //
    MfeaVif *mfea_vif = vif_find_by_vif_index(vif_index);
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    ProtoRegister& pr = mfea_vif->proto_register();
    if (! pr.is_registered(dst_module_id))
	return (XORP_ERROR);	// The message is not expected
    
    if (! is_up())
	return (XORP_ERROR);
    
    //
    // Send the message to all interested protocol instances
    //

    list<string>::const_iterator iter;
    for (iter = pr.module_instance_name_list(dst_module_id).begin();
	 iter != pr.module_instance_name_list(dst_module_id).end();
	 ++iter) {
	const string& dst_module_instance_name = *iter;
	proto_send(dst_module_instance_name,
		   dst_module_id,
		   vif_index,
		   src, dst,
		   ip_ttl,
		   ip_tos,
		   router_alert_bool,
		   rcvbuf,
		   rcvlen);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::signal_message_recv:
 * @src_module_instance_name: Unused.
 * @src_module_id: The #xorp_module_id module ID of the associated #ProtoComm
 * entry. XXX: in the future it may become irrelevant.
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
			      xorp_module_id src_module_id,
			      int message_type,
			      uint16_t vif_index,
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
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
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
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
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
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	    break;
	}
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
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
    //
    // Send the signal to all upper-layer protocols that expect it.
    //
    ProtoRegister& pr = _kernel_signal_messages_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
	const string& dst_module_instance_name = (*iter).first;
	xorp_module_id dst_module_id = (*iter).second;
	signal_message_send(dst_module_instance_name,
			    dst_module_id,
			    message_type,
			    vif_index,
			    src, dst,
			    rcvbuf,
			    rcvlen);
    }
    
    return (XORP_OK);
    
    UNUSED(src_module_id);
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
    ProtoRegister& pr = _kernel_signal_messages_register;
    const list<pair<string, xorp_module_id> >& module_list = pr.all_module_instance_name_list();
    
    list<pair<string, xorp_module_id> >::const_iterator iter;
    for (iter = module_list.begin(); iter != module_list.end(); ++iter) {
	const string& dst_module_instance_name = (*iter).first;
	xorp_module_id dst_module_id = (*iter).second;
	
	dataflow_signal_send(dst_module_instance_name,
			     dst_module_id,
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

/**
 * MfeaNode::join_multicast_group:
 * @module_instance_name: The module instance name of the protocol to join the
 * multicast group.
 * @module_id: The #xorp_module_id of the protocol to join the multicast
 * group.
 * @vif_index: The vif index of the interface to join.
 * @group: The multicast group to join.
 * 
 * Join a multicast group.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::join_multicast_group(const string& module_instance_name,
			       xorp_module_id module_id,
			       uint16_t vif_index,
			       const IPvX& group)
{
    ProtoComm *proto_comm = proto_comm_find_by_module_id(module_id);
    MfeaVif *mfea_vif = vif_find_by_vif_index(vif_index);
    
    if ((proto_comm == NULL) || mfea_vif == NULL)
	return (XORP_ERROR);
    
    bool has_group = mfea_vif->has_multicast_group(group);
    
    // Add the state for the group
    if (mfea_vif->add_multicast_group(module_instance_name,
				      module_id,
				      group) < 0) {
	return (XORP_ERROR);
    }
    
    if (! has_group) {
	if (proto_comm->join_multicast_group(vif_index, group) < 0) {
	    mfea_vif->delete_multicast_group(module_instance_name,
					     module_id,
					     group);
	    return (XORP_ERROR);
	}
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::leave_multicast_group:
 * @module_instance_name: The module instance name of the protocol to leave the
 * multicast group.
 * @module_id: The #xorp_module_id of the protocol to leave the multicast
 * group.
 * @vif_index: The vif index of the interface to leave.
 * @group: The multicast group to leave.
 * 
 * Leave a multicast group.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::leave_multicast_group(const string& module_instance_name,
				xorp_module_id module_id,
				uint16_t vif_index,
				const IPvX& group)
{
    ProtoComm *proto_comm = proto_comm_find_by_module_id(module_id);
    MfeaVif *mfea_vif = vif_find_by_vif_index(vif_index);
    
    if ((proto_comm == NULL) || mfea_vif == NULL)
	return (XORP_ERROR);
    
    // Delete the state for the group
    if (mfea_vif->delete_multicast_group(module_instance_name,
					 module_id,
					 group) < 0) {
	return (XORP_ERROR);
    }
    
    if (! mfea_vif->has_multicast_group(group)) {
	if (proto_comm->leave_multicast_group(vif_index, group) < 0) {
	    return (XORP_ERROR);
	}
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::add_mfc:
 * @module_instance_name: The module instance name of the protocol that adds
 * the MFC.
 * @source: The source address.
 * @group: The group address.
 * @iif_vif_index: The vif index of the incoming interface.
 * @oiflist: The bitset with the outgoing interfaces.
 * @oiflist_disable_wrongvif: The bitset with the outgoing interfaces to
 * disable the WRONGVIF signal.
 * @max_vifs_oiflist: The number of vifs covered by @oiflist
 * or @oiflist_disable_wrongvif.
 * @rp_addr: The RP address.
 * 
 * Add Multicast Forwarding Cache (MFC) to the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::add_mfc(const string& , // module_instance_name,
		  const IPvX& source, const IPvX& group,
		  uint16_t iif_vif_index, const Mifset& oiflist,
		  const Mifset& oiflist_disable_wrongvif,
		  uint32_t max_vifs_oiflist,
		  const IPvX& rp_addr)
{
    uint8_t oifs_ttl[MAX_VIFS];
    uint8_t oifs_flags[MAX_VIFS];
    
    if (max_vifs_oiflist > MAX_VIFS)
	return (XORP_ERROR);
    
    // Check the iif
    if (iif_vif_index == Vif::VIF_INDEX_INVALID)
	return (XORP_ERROR);
    if (iif_vif_index >= max_vifs_oiflist)
	return (XORP_ERROR);
    
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
	    if (family() == AF_INET) {
#if defined(MRT_MFC_FLAGS_DISABLE_WRONGVIF) && defined(ENABLE_ADVANCED_MCAST_API)
		oifs_flags[i] |= MRT_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
	    }
#ifdef HAVE_IPV6
	    if (family() == AF_INET6) {
#if defined(MRT6_MFC_FLAGS_DISABLE_WRONGVIF) && defined(ENABLE_ADVANCED_MCAST_API)
		oifs_flags[i] |= MRT6_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
	    }
#endif // HAVE_IPV6
	}
    }
    
    if (_mfea_mrouter.add_mfc(source, group, iif_vif_index, oifs_ttl,
			      oifs_flags, rp_addr)
	< 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
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
MfeaNode::delete_mfc(const string& , // module_instance_name,
		     const IPvX& source, const IPvX& group)
{
    if (_mfea_mrouter.delete_mfc(source, group) < 0) {
	return (XORP_ERROR);
    }
    
    //
    // XXX: Remove all corresponding dataflow entries
    //
    mfea_dft().delete_entry(source, group);
    
    return (XORP_OK);
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
			       bool is_leq_upcall)
{
    // XXX: flags is_geq_upcall and is_leq_upcall are mutually exclusive
    if (! (is_geq_upcall ^ is_leq_upcall))
	return (XORP_ERROR);		// Invalid arguments
    if (! (is_threshold_in_packets || is_threshold_in_bytes))
	return (XORP_ERROR);		// Invalid arguments
    
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
					is_leq_upcall)
	    < 0) {
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
			     is_leq_upcall)
	< 0) {
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
				  bool is_leq_upcall)
{
    // XXX: flags is_geq_upcall and is_leq_upcall are mutually exclusive
    if (! (is_geq_upcall ^ is_leq_upcall))
	return (XORP_ERROR);		// Invalid arguments
    if (! (is_threshold_in_packets || is_threshold_in_bytes))
	return (XORP_ERROR);		// Invalid arguments
    
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
					   is_leq_upcall)
	    < 0) {
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
				is_leq_upcall)
	< 0) {
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
 * 
 * Delete all dataflow monitor entries for a given @source and @group address.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaNode::delete_all_dataflow_monitor(const string& , // module_instance_name,
				      const IPvX& source, const IPvX& group)
{
    //
    // If the kernel supports bandwidth-related upcalls, use it
    //
    if (_mfea_mrouter.mrt_api_mrt_mfc_bw_upcall()) {
	if (_mfea_mrouter.delete_all_bw_upcall(source, group)
	    < 0) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    
    //
    // The kernel doesn't support bandwidth-related upcalls, hence use
    // a work-around mechanism (periodic quering).
    //
    if (mfea_dft().delete_entry(source, group)
	< 0) {
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
MfeaNode::add_multicast_vif(uint16_t vif_index)
{
    if (_mfea_mrouter.add_multicast_vif(vif_index) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
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
MfeaNode::delete_multicast_vif(uint16_t vif_index)
{
    if (_mfea_mrouter.delete_multicast_vif(vif_index) < 0) {
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
    if (_mfea_mrouter.get_sg_count(source, group, sg_count) < 0) {
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
MfeaNode::get_vif_count(uint16_t vif_index, VifCount& vif_count)
{
    if (_mfea_mrouter.get_vif_count(vif_index, vif_count) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaNode::proto_comm_find_by_module_id:
 * @module_id: The #xorp_module_id to search for.
 * 
 * Return the #ProtoComm entry that corresponds to @module_id.
 * 
 * Return value: The corresponding #ProtoComm entry if found, otherwise NULL.
 **/
ProtoComm *
MfeaNode::proto_comm_find_by_module_id(xorp_module_id module_id) const
{
    for (size_t i = 0; i < _proto_comms.size(); i++) {
	if (_proto_comms[i] != NULL) {
	    if (_proto_comms[i]->module_id() == module_id)
		return (_proto_comms[i]);
	}
    }
    
    return (NULL);
}

/**
 * MfeaNode::proto_comm_find_by_ipproto:
 * @ipproto: The IP protocol number to search for.
 * 
 * Return the #ProtoComm entry that corresponds to @ipproto IP protocol number.
 * 
 * Return value: The corresponding #ProtoComm entry if found, otherwise NULL.
 **/
ProtoComm *
MfeaNode::proto_comm_find_by_ipproto(int ipproto) const
{
    for (size_t i = 0; i < _proto_comms.size(); i++) {
	if (_proto_comms[i] != NULL) {
	    if (_proto_comms[i]->ipproto() == ipproto)
		return (_proto_comms[i]);
	}
    }
    
    return (NULL);
}

/**
 * MfeaNode::get_mrib_table:
 * @return_mrib_table: A pointer to the routing table array composed
 * of #Mrib elements.
 * 
 * Return value: The number of entries in @return_mrib_table, or %XORP_ERROR
 * if there was an error.
 **/
int
MfeaNode::get_mrib_table(Mrib **return_mrib_table)
{
    Mrib	*mrib_table;
    int		mrib_table_n = 0;
    
    *return_mrib_table = NULL;
    
    switch (family()) {
    case AF_INET:
    {
	list<Fte4> fte_list4;
	list<Fte4>::iterator iter;
	
	if (_ftic.get_table4(fte_list4) != true)
	    return (XORP_ERROR);
	mrib_table = new Mrib[fte_list4.size()](family());
	for (iter = fte_list4.begin(); iter != fte_list4.end(); ++iter) {
	    Fte4& fte = *iter;
	    Mrib& mrib = mrib_table[mrib_table_n++];
	    MfeaVif *mfea_vif = NULL;
	    mrib.set_dest_prefix(IPvXNet(fte.net()));
	    mrib.set_metric_preference(mrib_table_default_metric_preference().get());
	    mrib.set_metric(mrib_table_default_metric().get());
	    mrib.set_next_hop_router_addr(IPvX(fte.gateway()));
	    
	    // Get the vif index
	    do {
		mfea_vif = vif_find_same_subnet_or_p2p(mrib.next_hop_router_addr());
		if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up())
		    break;
		
		// Try again in case it is a LAN address.
		mfea_vif = vif_find_same_subnet(mrib.dest_prefix());
		if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up())
		    break;
		
		// Try again in case it is a P2P entry
		if (mrib.dest_prefix().prefix_len()
		    == IPvX::addr_bitlen(family())) {
		    mfea_vif = vif_find_same_subnet_or_p2p(mrib.dest_prefix().masked_addr());
		}
		if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up())
		    break;
	    } while (false);
	    if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up()) {
		mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	    }
	    if (mrib.next_hop_vif_index() >= maxvifs()) {
		XLOG_WARNING("get_mrib_table() error: "
			     "cannot find interface toward next-hop "
			     "router %s for destination %s",
			     cstring(mrib.next_hop_router_addr()),
			     cstring(mrib.dest_prefix()));
	    }
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	list<Fte6> fte_list6;
	list<Fte6>::iterator iter;
	
	if (_ftic.get_table6(fte_list6) != true)
	    return (XORP_ERROR);
	mrib_table = new Mrib[fte_list6.size()](family());
	for (iter = fte_list6.begin(); iter != fte_list6.end(); ++iter) {
	    Fte6& fte = *iter;
	    Mrib& mrib = mrib_table[mrib_table_n++];
	    MfeaVif *mfea_vif = NULL;
	    mrib.set_dest_prefix(IPvXNet(fte.net()));
	    mrib.set_metric_preference(mrib_table_default_metric_preference().get());
	    mrib.set_metric(mrib_table_default_metric().get());
	    mrib.set_next_hop_router_addr(IPvX(fte.gateway()));
	    
	    // Get the vif index
	    do {
		mfea_vif = vif_find_same_subnet_or_p2p(mrib.next_hop_router_addr());
		if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up())
		    break;
		
		// Try again in case it is a LAN address.
		mfea_vif = vif_find_same_subnet(mrib.dest_prefix());
		if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up())
		    break;
		
		// Try again in case it is a P2P entry
		if (mrib.dest_prefix().prefix_len()
		    == IPvX::addr_bitlen(family())) {
		    mfea_vif = vif_find_same_subnet_or_p2p(mrib.dest_prefix().masked_addr());
		}
		if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up())
		    break;
	    } while (false);
	    if ((mfea_vif != NULL) && mfea_vif->is_underlying_vif_up()) {
		mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	    }
	    if (mrib.next_hop_vif_index() >= maxvifs()) {
		XLOG_WARNING("get_mrib_table() error: "
			     "cannot find interface toward next-hop "
			     "router %s for destination %s",
			     cstring(mrib.next_hop_router_addr()),
			     cstring(mrib.dest_prefix()));
	    }
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    
    *return_mrib_table = mrib_table;
    return (mrib_table_n);
}

/**
 * MfeaNode::mrib_table_read_timer_timeout:
 * 
 * Timeout handler to read the MRIB table.
 **/
void
MfeaNode::mrib_table_read_timer_timeout()
{
    MribTable new_mrib_table(family());
    MribTable::iterator iter;
    bool is_changed = false;
    
    if (maxvifs() == 0) {
	//
	// XXX: we don't have vifs yet, hence no point to get the MRIB table
	// Restart the timer after a much shorter interval.
	//
	TimeVal timeval(MRIB_TABLE_READ_PERIOD_SEC,
			MRIB_TABLE_READ_PERIOD_USEC);
	timeval = timeval/4 + TimeVal(1, 0);
	
	//
	// Start again the timer
	//
	_mrib_table_read_timer =
	    eventloop().new_oneoff_after(timeval,
					 callback(this,
						  &MfeaNode::mrib_table_read_timer_timeout));
	return;
    }
    
    //
    // Add all new entries to the temporary table
    //
    do {
	Mrib *new_mrib_array = NULL;
	int new_mrib_array_size = get_mrib_table(&new_mrib_array);
	
	if (new_mrib_array_size > 0) {
	    for (int i = 0; i < new_mrib_array_size; i++) {
		Mrib& new_mrib = new_mrib_array[i];
		new_mrib_table.insert(new_mrib);
	    }
	}
	delete[] new_mrib_array;
    } while (false);
    
    //
    // Delete the entries that have disappeared
    //
    for (iter = mrib_table().begin();
	 iter != mrib_table().end();
	 ++iter) {
	Mrib *mrib = *iter;
	if (mrib == NULL)
	    continue;
	Mrib *new_mrib = new_mrib_table.find_exact(mrib->dest_prefix());
	if ((new_mrib == NULL) || (*new_mrib != *mrib)) {
	    send_delete_mrib(*mrib);
	    is_changed = true;
	}
    }
    
    //
    // Add the entries that have appeared
    //
    for (iter = new_mrib_table.begin();
	 iter != new_mrib_table.end();
	 ++iter) {
	Mrib *new_mrib = *iter;
	if (new_mrib == NULL)
	    continue;
	Mrib *mrib = mrib_table().find_exact(new_mrib->dest_prefix());
	if ((mrib == NULL) || (*new_mrib != *mrib)) {
	    send_add_mrib(*new_mrib);
	    is_changed = true;
	}
    }

    //
    // Replace the table if changed
    //
    if (is_changed) {
	mrib_table().clear();
	
	for (iter = new_mrib_table.begin();
	     iter != new_mrib_table.end();
	     ++iter) {
	    Mrib *new_mrib = *iter;
	    if (new_mrib == NULL)
		continue;
	    mrib_table().insert(*new_mrib);
	}
	
	send_set_mrib_done();
    }
    
    //
    // Start again the timer
    //
    _mrib_table_read_timer =
	eventloop().new_oneoff_after(TimeVal(MRIB_TABLE_READ_PERIOD_SEC,
					     MRIB_TABLE_READ_PERIOD_USEC),
				     callback(this,
					      &MfeaNode::mrib_table_read_timer_timeout));
}
