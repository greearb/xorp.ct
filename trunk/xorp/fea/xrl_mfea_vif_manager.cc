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

#ident "$XORP: xorp/fea/xrl_mfea_vif_manager.cc,v 1.21 2003/10/28 19:36:27 pavlin Exp $"

#include "mfea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "mfea_node.hh"
#include "mfea_vif.hh"
#include "xrl_mfea_node.hh"
#include "xrl_mfea_vif_manager.hh"

XrlMfeaVifManager::XrlMfeaVifManager(MfeaNode& mfea_node,
				     EventLoop& eventloop,
				     XrlRouter* xrl_router)
    : _mfea_node(mfea_node),
      _eventloop(eventloop),
      _xrl_router(*xrl_router),
      _ifmgr_client(xrl_router)
{
    _no_fea = false;
    _state = INITIALIZING;
    _interfaces_remaining = 0;
    _vifs_remaining = 0;
    _addrs_remaining = 0;

    _fea_target_name = xorp_module_name(family(), XORP_MODULE_FEA);
    
    enable();	// XXX: by default the XrlMfeaVifManager is always enabled
}

XrlMfeaVifManager::~XrlMfeaVifManager()
{
    stop();
    
    // Remove all Vif entries
    map<string, Vif*>::iterator iter;
    for (iter = _vifs_by_name.begin(); iter != _vifs_by_name.end(); ++iter) {
	delete iter->second;
    }
}

int
XrlMfeaVifManager::family() const
{
    return (_mfea_node.family());
}

/**
 * Start operation.
 * 
 * Start the process of registering with the FEA, etc.
 * 
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaVifManager::start()
{
    enable();	// XXX: by default the XrlMfeaVifManager is always enabled
    
    if (ProtoState::start() < 0)
	return (XORP_ERROR);
    
    if (_no_fea) {
	_state = READY;
	return (XORP_OK);
    }
    
    register_if_spy();
    return (XORP_OK);
}

/**
 * Stop operation.
 * 
 * Gracefully stop operation.
 * 
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaVifManager::stop()
{
    if (is_down())
	return (XORP_OK);

    if (! is_up())
	return (XORP_ERROR);

    clean_out_old_state();
    
    ProtoState::stop();
    
    return (XORP_OK);
}

void
XrlMfeaVifManager::update_state()
{
    switch (_state) {
    case INITIALIZING:
	if ((_interfaces_remaining == 0)
	    && (_vifs_remaining == 0)
	    && (_addrs_remaining == 0)) {
	    _state = READY;
	}
	break;
    default:
	break;
    }
    
    // Time to set the vif state
    if (_state == READY) {
	if ((_interfaces_remaining == 0)
	    && (_vifs_remaining == 0)
	    && (_addrs_remaining == 0)) {
	    set_vif_state();
	}
    }
}

void
XrlMfeaVifManager::set_vif_state()
{
    map<string, Vif*>::const_iterator vif_iter;
    map<string, Vif>::iterator mfea_vif_iter;
    string error_msg;
    
    //
    // Remove vifs that don't exist anymore
    //
    for (mfea_vif_iter = _mfea_node.configured_vifs().begin();
	 mfea_vif_iter != _mfea_node.configured_vifs().end();
	 ++mfea_vif_iter) {
	Vif* node_vif = &mfea_vif_iter->second;
	if (node_vif->is_pim_register())
	    continue;		// XXX: don't delete the PIM Register vif
	if (_vifs_by_name.find(node_vif->name()) == _vifs_by_name.end()) {
	    // Delete the interface
	    string vif_name = node_vif->name();
	    if (_mfea_node.delete_config_vif(vif_name, error_msg) < 0) {
		XLOG_ERROR("Cannot delete vif %s from the set of configured "
			   "vifs: %s",
			   vif_name.c_str(), error_msg.c_str());
	    }
	    continue;
	}
    }
    
    //
    // Add new vifs, and update existing ones
    //
    for (vif_iter = _vifs_by_name.begin();
	 vif_iter != _vifs_by_name.end(); ++vif_iter) {
	Vif* vif = vif_iter->second;
	Vif* node_vif = NULL;
	
	mfea_vif_iter = _mfea_node.configured_vifs().find(vif->name());
	if (mfea_vif_iter != _mfea_node.configured_vifs().end()) {
	    node_vif = &(mfea_vif_iter->second);
	}
	
	//
	// Add a new vif
	//
	if (node_vif == NULL) {
	    uint16_t vif_index = _mfea_node.find_unused_config_vif_index();
	    XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	    vif->set_vif_index(vif_index);
	    if (_mfea_node.add_config_vif(*vif, error_msg) < 0) {
		XLOG_ERROR("Cannot add vif %s to the set of configured "
			   "vifs: %s",
			   vif->name().c_str(), error_msg.c_str());
	    }
	    continue;
	}

	//
	// Update the pif_index
	//
	_mfea_node.set_config_pif_index(vif->name(),
					vif->pif_index(),
					error_msg);
	
	//
	// Update the vif flags
	//
	_mfea_node.set_config_vif_flags(vif->name(),
					vif->is_pim_register(),
					vif->is_p2p(),
					vif->is_loopback(),
					vif->is_multicast_capable(),
					vif->is_broadcast_capable(),
					vif->is_underlying_vif_up(),
					error_msg);
	
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
		if (_mfea_node.delete_config_vif_addr(vif->name(), ipvx,
						      error_msg)
		    < 0) {
		    XLOG_ERROR("Cannot delete address %s from vif %s from "
			       "the set of configured vifs: %s",
			       cstring(ipvx), vif->name().c_str(),
			       error_msg.c_str());
		}
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
		    if (_mfea_node.add_config_vif_addr(
			vif->name(),
			vif_addr.addr(),
			vif_addr.subnet_addr(),
			vif_addr.broadcast_addr(),
			vif_addr.peer_addr(),
			error_msg) < 0) {
			XLOG_ERROR("Cannot add address %s to vif %s from "
				   "the set of configured vifs: %s",
				   cstring(vif_addr), vif->name().c_str(),
				   error_msg.c_str());
		    }
		    continue;
		}
		// Update the address
		if (*node_vif_addr != vif_addr) {
		    *node_vif_addr = vif_addr;
		    {
			if (_mfea_node.delete_config_vif_addr(vif->name(),
							      vif_addr.addr(),
							      error_msg) < 0) {
			    XLOG_ERROR("Cannot delete address %s from vif %s "
				       "from the set of configured vifs: %s",
				       cstring(vif_addr.addr()),
				       vif->name().c_str(),
				       error_msg.c_str());
			}
			if (_mfea_node.add_config_vif_addr(
			    vif->name(),
			    vif_addr.addr(),
			    vif_addr.subnet_addr(),
			    vif_addr.broadcast_addr(),
			    vif_addr.peer_addr(),
			    error_msg) < 0) {
			    XLOG_ERROR("Cannot add address %s to vif %s from "
				       "the set of configured vifs: %s",
				       cstring(vif_addr), vif->name().c_str(),
				       error_msg.c_str());
			}
		    }
		}
	    }
	}
    }
    
    // Done
    _mfea_node.set_config_all_vifs_done(error_msg);
}

void
XrlMfeaVifManager::clean_out_old_state()
{
    if (_no_fea)
	return;
    
    //
    // We try to unregister first, to cause the FEA to remove any
    // registrations left over from previous incarnations of the RIB.
    //
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(this, &XrlMfeaVifManager::xrl_result_unregister_system_interfaces_client);
    _ifmgr_client.send_unregister_system_interfaces_client(
	_fea_target_name.c_str(),
	_xrl_router.name(), cb);
}

void
XrlMfeaVifManager::xrl_result_unregister_system_interfaces_client(const XrlError& e)
{
    UNUSED(e);
    
    //
    // XXX: We really don't care here if the request succeeded or failed.
    // It is normal to fail with COMMAND_FAILED if there was no state
    // left behind from a previous incarnation. Any other errors would
    // also show up in register_if_spy, so we'll let that deal with
    // them.
    //
    // register_if_spy();
}

void
XrlMfeaVifManager::register_if_spy()
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(this, &XrlMfeaVifManager::xrl_result_register_system_interfaces_client);
    _ifmgr_client.send_register_system_interfaces_client(_fea_target_name.c_str(),
							 _xrl_router.name(), cb);
}

void
XrlMfeaVifManager::xrl_result_register_system_interfaces_client(const XrlError& e)
{
    if (_no_fea) {
	_state = READY;
	return;
    }
    
    if (e == XrlError::OKAY()) {
	//
	// The registration was successful. Now we need to query the
	// entries that are already there. First, find out the set of
	// all interfaces.
	//
	XorpCallback2<void, const XrlError&, const XrlAtomList*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_interface_names);
	_ifmgr_client.send_get_system_interface_names(_fea_target_name.c_str(),
						      cb);
	return;
    }
    
    //
    // If the resolve failed, it could be that we got going too quickly
    // for the FEA. Retry every two seconds.
    //
    if (e == XrlError::RESOLVE_FAILED()) {
	debug_msg("Register Interface Spy: RESOLVE_FAILED\n");
	OneoffTimerCallback cb;
	cb = callback(this, &XrlMfeaVifManager::register_if_spy);
	_register_retry_timer = _eventloop.new_oneoff_after(TimeVal(2, 0), cb);
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Register Interface Spy: Permanent Error");
}

void
XrlMfeaVifManager::xrl_result_get_system_interface_names(
    const XrlError& e,
    const XrlAtomList* alist)
{
    if (e == XrlError::OKAY()) {
	for (size_t i = 0; i < alist->size(); i++) {
	    //
	    // Spin through the list of interfaces, and fire off
	    // requests in parallel for all the Vifs on each interface.
	    //
	    XrlAtom atom = alist->get(i);
	    string ifname = atom.text();
	    debug_msg("got interface name: %s\n", ifname.c_str());
	    
	    XorpCallback2<void, const XrlError&, const XrlAtomList*>::RefPtr cb;
	    cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_vif_names, ifname);
	    _ifmgr_client.send_get_system_vif_names(_fea_target_name.c_str(),
						 ifname, cb);
	    _interfaces_remaining++;
	}
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Get Interface Names: Permanent Error");
}

void
XrlMfeaVifManager::xrl_result_get_system_vif_names(
    const XrlError& e,
    const XrlAtomList* alist,
    string ifname)
{
    if (_interfaces_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_vif_names for interface %s",
		     ifname.c_str());
	return;
    }
    
    _interfaces_remaining--;
    
    if (e == XrlError::OKAY()) {
	// Spin through all the Vifs on this interface, and create them.
	for (size_t i = 0; i < alist->size(); i++) {
	    XrlAtom atom = alist->get(i);
	    string vifname = atom.text();
	    vif_created(ifname, vifname);
	}
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the interface went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Get VIF Names: Permanent Error");
}

void
XrlMfeaVifManager::xrl_result_get_system_vif_pif_index(const XrlError& e,
						    const uint32_t* pif_index,
						    string ifname,
						    string vifname)
{
    if (_vifs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_vif_pif_index for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _vifs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("setting pif_index for interface %s vif %s\n",
		  ifname.c_str(), vifname.c_str());
	vif->set_pif_index(*pif_index);
	
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get pif_index for vif %s: Permanent Error",
	       vifname.c_str());
}

void
XrlMfeaVifManager::xrl_result_get_system_vif_flags(const XrlError& e,
						   const bool* enabled,
						   const bool* broadcast,
						   const bool* loopback,
						   const bool* point_to_point,
						   const bool* multicast,
						   string ifname,
						   string vifname)
{
    if (_vifs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_vif_flags for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _vifs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("setting flags for interface %s vif %s\n",
		  ifname.c_str(), vifname.c_str());
	vif->set_underlying_vif_up(*enabled);
	vif->set_broadcast_capable(*broadcast);
	vif->set_loopback(*loopback);
	vif->set_p2p(*point_to_point);
	vif->set_multicast_capable(*multicast);
	
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get flags for vif %s: Permanent Error",
	       vifname.c_str());
}

void
XrlMfeaVifManager::xrl_result_get_system_vif_addresses4(
    const XrlError& e,
    const XrlAtomList* alist,
    string ifname,
    string vifname)
{
    // If unexpected address family response, then silently ignore it
    if (family() != AF_INET)
	return;
    
    if (_vifs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_vif_addresses4 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _vifs_remaining--;
    
    if (e == XrlError::OKAY()) {
	for (size_t i = 0; i < alist->size(); i++) {
	    XrlAtom atom = alist->get(i);
	    IPv4 addr = atom.ipv4();
	    vifaddr4_created(ifname, vifname, addr);
	}
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Get VIF addresses: Permanent Error");
}

void
XrlMfeaVifManager::xrl_result_get_system_vif_addresses6(
    const XrlError& e,
    const XrlAtomList* alist,
    string ifname,
    string vifname)
{
    // If unexpected address family response, then silently ignore it
#ifdef HAVE_IPV6
    if (family() != AF_INET6)
	return;
#else
    return;
#endif
    
    if (_vifs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_vif_addresses6 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _vifs_remaining--;
    
    if (e == XrlError::OKAY()) {
	for (size_t i = 0; i < alist->size(); i++) {
	    XrlAtom atom = alist->get(i);
	    IPv6 addr = atom.ipv6();
	    vifaddr6_created(ifname, vifname, addr);
	}
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Get VIF addresses: Permanent Error");
}

void
XrlMfeaVifManager::interface_update(const string& ifname,
				    const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	// doesn't directly affect vifs - we'll get a vif_update for
	// any new vifs
	break;
    case IF_EVENT_DELETED:
	interface_deleted(ifname);
	break;
    case IF_EVENT_CHANGED:
	// doesn't directly affect vifs
	break;
    default:
	XLOG_WARNING("interface_update invalid event: %u", event);
	break;
    }
}

void
XrlMfeaVifManager::vif_update(const string& ifname, const string& vifname,
			      const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	vif_created(ifname, vifname);
	break;
    case IF_EVENT_DELETED:
	vif_deleted(ifname, vifname);
	break;
    case IF_EVENT_CHANGED:
	// doesn't directly affect us
	break;
    default:
	XLOG_WARNING("vif_update invalid event: %u", event);
	break;
    }
}

void
XrlMfeaVifManager::vifaddr4_update(const string& ifname,
				   const string& vifname,
				   const IPv4& addr,
				   const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	vifaddr4_created(ifname, vifname, addr);
	break;
    case IF_EVENT_DELETED:
	vifaddr4_deleted(ifname, vifname, addr);
	break;
    case IF_EVENT_CHANGED:
	// XXX: force query address-related info
	vifaddr4_created(ifname, vifname, addr);
	break;
    default:
	XLOG_WARNING("vifaddr4_update invalid event: %u", event);
	break;
    }
}

void
XrlMfeaVifManager::vifaddr6_update(const string& ifname,
				   const string& vifname,
				   const IPv6& addr,
				   const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	vifaddr6_created(ifname, vifname, addr);
	break;
    case IF_EVENT_DELETED:
	vifaddr6_deleted(ifname, vifname, addr);
	break;
    case IF_EVENT_CHANGED:
	// XXX: force query address-related info
	vifaddr6_created(ifname, vifname, addr);
	break;
    default:
	XLOG_WARNING("vifaddr6_update invalid event: %u", event);
	break;
    }
}

void
XrlMfeaVifManager::interface_deleted(const string& ifname)
{
    debug_msg("interface_deleted %s\n", ifname.c_str());
    
    // Reomve all vifs for the same interface name
    multimap<string, Vif*>::iterator iter;
    iter = _vifs_by_interface.find(ifname);
    for (; iter != _vifs_by_interface.end(); ++iter) {
	if (iter->first != ifname)
	    break;
	vif_deleted(ifname, iter->second->name());
    }
}

void
XrlMfeaVifManager::vif_deleted(const string& ifname, const string& vifname)
{
    debug_msg("vif_deleted %s %s\n", ifname.c_str(), vifname.c_str());
    
    map<string, Vif*>::iterator vi = _vifs_by_name.find(vifname);
    if (vi == _vifs_by_name.end()) {
	XLOG_ERROR("vif_deleted: vif %s not found", vifname.c_str());
	return;
    }
    
    //
    // Remove the vif from the local maps
    //
    Vif* vif = vi->second;
    _vifs_by_name.erase(vi);
    multimap<string, Vif*>::iterator ii = _vifs_by_interface.find(ifname);
    XLOG_ASSERT(ii != _vifs_by_interface.end());
    while (ii != _vifs_by_interface.end() && ii->first == ifname) {
	if (ii->second == vif) {
	    _vifs_by_interface.erase(ii);
	    break;
	}
	++ii;
	XLOG_ASSERT(ii != _vifs_by_interface.end());
    }
    delete vif;
    
    update_state();
}

void
XrlMfeaVifManager::vif_created(const string& ifname, const string& vifname)
{
    debug_msg("vif_created: %s %s\n", ifname.c_str(), vifname.c_str());
    
    if (_vifs_by_name.find(vifname) != _vifs_by_name.end()) {
	XLOG_ERROR("vif_created: vif %s already exists", vifname.c_str());
	return;
    }
    
    //
    // Create a new vif
    //
    Vif* vif = new Vif(vifname, ifname);
    _vifs_by_name[vifname] = vif;
    _vifs_by_interface.insert(pair<string, Vif*>(ifname, vif));
    
    //
    // Fire off requests in parallel to get the pif_index, flags and all the
    // addresses on each Vif.
    //
    {
	// Get the pif_index
	XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_vif_pif_index,
		      ifname, vifname);
	_ifmgr_client.send_get_system_vif_pif_index(_fea_target_name.c_str(),
						    ifname, vifname, cb);
	_vifs_remaining++;
    }
    
    {
	// Get the vif flags
	XorpCallback6<void, const XrlError&, const bool*, const bool*,
	    const bool*, const bool*, const bool*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_vif_flags,
		      ifname, vifname);
	_ifmgr_client.send_get_system_vif_flags(_fea_target_name.c_str(),
						ifname, vifname, cb);
	_vifs_remaining++;
    }
    
    XorpCallback2<void, const XrlError&, const XrlAtomList*>::RefPtr cb;
    switch (family()) {
    case AF_INET:
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_vif_addresses4,
		      ifname, vifname);
	_ifmgr_client.send_get_system_vif_addresses4(_fea_target_name.c_str(),
						     ifname,
						     vifname,
						     cb);
	_vifs_remaining++;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_vif_addresses6,
		      ifname, vifname);
	_ifmgr_client.send_get_system_vif_addresses6(_fea_target_name.c_str(),
						     ifname,
						     vifname,
						     cb);
	_vifs_remaining++;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    
    update_state();
}

void
XrlMfeaVifManager::vifaddr4_created(const string& ifname,
				    const string& vifname,
				    const IPv4& addr)
{
    debug_msg("vifaddr4_created for interface %s vif %s: %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr4_created on unknown vif: %s", vifname.c_str());
	return;
    }
    
    {
	// Get the address flags
	XorpCallback6<void, const XrlError&, const bool*, const bool*,
	    const bool*, const bool*, const bool*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_address_flags4,
		      ifname, vifname, addr);
	_ifmgr_client.send_get_system_address_flags4(_fea_target_name.c_str(),
						     ifname, vifname, addr, cb);
	_addrs_remaining++;
    }
    
    {
	// Get the prefix length
	XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_prefix4,
		      ifname, vifname, addr);
	_ifmgr_client.send_get_system_prefix4(_fea_target_name.c_str(), ifname,
					      vifname, addr, cb);
	_addrs_remaining++;
    }
}

void
XrlMfeaVifManager::vifaddr6_created(const string& ifname,
				    const string& vifname,
				    const IPv6& addr)
{
    debug_msg("vifaddr6_created for interface %s vif %s: %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr6_created on unknown vif: %s", vifname.c_str());
	return;
    }
    
    {
	// Get the address flags
	XorpCallback5<void, const XrlError&, const bool*,
	    const bool*, const bool*, const bool*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_address_flags6,
		      ifname, vifname, addr);
	_ifmgr_client.send_get_system_address_flags6(_fea_target_name.c_str(),
						     ifname, vifname, addr, cb);
	_addrs_remaining++;
    }
    
    {
	// Get the prefix length
	XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
	cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_prefix6,
		      ifname, vifname, addr);
	_ifmgr_client.send_get_system_prefix6(_fea_target_name.c_str(), ifname,
					      vifname, addr, cb);
	_addrs_remaining++;
    }
}

void
XrlMfeaVifManager::xrl_result_get_system_address_flags4(const XrlError& e,
							const bool* enabled,
							const bool* broadcast,
							const bool* loopback,
							const bool* point_to_point,
							const bool* multicast,
							string ifname,
							string vifname,
							IPv4 addr)
{
    // If unexpected address family response, then silently ignore it
    if (family() != AF_INET)
	return;
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_address_flags4 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	
	if (*broadcast) {
	    // Get the broadcast address
	    XorpCallback2<void, const XrlError&, const IPv4*>::RefPtr cb;
	    cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_broadcast4,
			  ifname, vifname, addr);
	    _ifmgr_client.send_get_system_broadcast4(_fea_target_name.c_str(),
						     ifname, vifname, addr, cb);
	    _addrs_remaining++;
	}
	
	if (*point_to_point) {
	    // Get the endpoint address
	    XorpCallback2<void, const XrlError&, const IPv4*>::RefPtr cb;
	    cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_endpoint4,
			  ifname, vifname, addr);
	    _ifmgr_client.send_get_system_endpoint4(_fea_target_name.c_str(),
						    ifname, vifname, addr, cb);
	    _addrs_remaining++;
	}
	
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get flags for address %s: Permanent Error",
	       addr.str().c_str());
    
    UNUSED(enabled);
    UNUSED(loopback);
    UNUSED(multicast);
}

void
XrlMfeaVifManager::xrl_result_get_system_address_flags6(const XrlError& e,
							const bool* enabled,
							const bool* loopback,
							const bool* point_to_point,
							const bool* multicast,
							string ifname,
							string vifname,
							IPv6 addr)
{
    // If unexpected address family response, then silently ignore it
#ifdef HAVE_IPV6
    if (family() != AF_INET6)
	return;
#else
    return;
#endif
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_address_flags6 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	
	{
	    // Get the broadcast address
	    // XXX: IPv6 doesn't have broadcast addresses, hence we don't do it
	}
	
	if (*point_to_point) {
	    // Get the endpoint address
	    XorpCallback2<void, const XrlError&, const IPv6*>::RefPtr cb;
	    cb = callback(this, &XrlMfeaVifManager::xrl_result_get_system_endpoint6,
			  ifname, vifname, addr);
	    _ifmgr_client.send_get_system_endpoint6(_fea_target_name.c_str(),
						    ifname, vifname, addr, cb);
	    _addrs_remaining++;
	}
	
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get flags for address %s: Permanent Error",
	       addr.str().c_str());
    
    UNUSED(enabled);
    UNUSED(loopback);
    UNUSED(multicast);
}

void
XrlMfeaVifManager::xrl_result_get_system_prefix4(const XrlError& e,
						 const uint32_t* prefix_len,
						 string ifname,
						 string vifname,
						 IPv4 addr)
{
    // If unexpected address family response, then silently ignore it
    if (family() != AF_INET)
	return;
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_prefix4 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s prefix_len %d to interface %s vif %s\n",
		  addr.str().c_str(), *prefix_len,
		  ifname.c_str(), vifname.c_str());
	VifAddr* vif_addr = vif->find_address(IPvX(addr));
	if (vif_addr == NULL) {
	    vif->add_address(IPvX(addr));
	    vif_addr = vif->find_address(IPvX(addr));
	}
	XLOG_ASSERT(vif_addr != NULL);
	vif_addr->set_subnet_addr(IPvXNet(IPvX(addr), *prefix_len));
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get prefix_len for address %s: Permanent Error",
	       addr.str().c_str());
}

void
XrlMfeaVifManager::xrl_result_get_system_prefix6(const XrlError& e,
						 const uint32_t* prefix_len,
						 string ifname,
						 string vifname,
						 IPv6 addr)
{
    // If unexpected address family response, then silently ignore it
#ifdef HAVE_IPV6
    if (family() != AF_INET6)
	return;
#else
    return;
#endif
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_prefix6 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s prefix_len %d to interface %s vif %s\n",
		  addr.str().c_str(), *prefix_len,
		  ifname.c_str(), vifname.c_str());
	VifAddr* vif_addr = vif->find_address(IPvX(addr));
	if (vif_addr == NULL) {
	    vif->add_address(IPvX(addr));
	    vif_addr = vif->find_address(IPvX(addr));
	}
	XLOG_ASSERT(vif_addr != NULL);
	vif_addr->set_subnet_addr(IPvXNet(IPvX(addr), *prefix_len));
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get prefix_len for address %s",
	       addr.str().c_str());
}

void
XrlMfeaVifManager::xrl_result_get_system_broadcast4(const XrlError& e,
						    const IPv4* broadcast,
						    string ifname,
						    string vifname,
						    IPv4 addr)
{
    // If unexpected address family response, then silently ignore it
    if (family() != AF_INET)
	return;
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_broadcast4 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s broadcast %s to interface %s vif %s\n",
		  addr.str().c_str(), broadcast->str().c_str(),
		  ifname.c_str(), vifname.c_str());
	VifAddr* vif_addr = vif->find_address(IPvX(addr));
	if (vif_addr == NULL) {
	    vif->add_address(IPvX(addr));
	    vif_addr = vif->find_address(IPvX(addr));
	}
	XLOG_ASSERT(vif_addr != NULL);
	vif_addr->set_broadcast_addr(IPvX(*broadcast));
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get broadcast addr for address %s: Permanent Error",
	       addr.str().c_str());
}

void
XrlMfeaVifManager::xrl_result_get_system_endpoint4(const XrlError& e,
						   const IPv4* endpoint,
						   string ifname,
						   string vifname,
						   IPv4 addr)
{
    // If unexpected address family response, then silently ignore it
    if (family() != AF_INET)
	return;
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_endpoint4 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s endpoint %s to interface %s vif %s\n",
		  addr.str().c_str(), endpoint->str().c_str(),
		  ifname.c_str(), vifname.c_str());
	VifAddr* vif_addr = vif->find_address(IPvX(addr));
	if (vif_addr == NULL) {
	    vif->add_address(IPvX(addr));
	    vif_addr = vif->find_address(IPvX(addr));
	}
	XLOG_ASSERT(vif_addr != NULL);
	vif_addr->set_peer_addr(IPvX(*endpoint));
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get endpoint addr for address %s: Permanent Error",
	       addr.str().c_str());
}

void
XrlMfeaVifManager::xrl_result_get_system_endpoint6(const XrlError& e,
						   const IPv6* endpoint,
						   string ifname,
						   string vifname,
						   IPv6 addr)
{
    // If unexpected address family response, then silently ignore it
#ifdef HAVE_IPV6
    if (family() != AF_INET6)
	return;
#else
    return;
#endif
    
    if (_addrs_remaining == 0) {
	// Unexpected response
	XLOG_WARNING("Received unexpected XRL response for "
		     "get_system_endpoint6 for interface %s, vif %s",
		     ifname.c_str(), vifname.c_str());
	return;
    }
    
    _addrs_remaining--;
    
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    update_state();
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s endpoint %s to interface %s vif %s\n",
		  addr.str().c_str(), endpoint->str().c_str(),
		  ifname.c_str(), vifname.c_str());
	VifAddr* vif_addr = vif->find_address(IPvX(addr));
	if (vif_addr == NULL) {
	    vif->add_address(IPvX(addr));
	    vif_addr = vif->find_address(IPvX(addr));
	}
	XLOG_ASSERT(vif_addr != NULL);
	vif_addr->set_peer_addr(IPvX(*endpoint));
	update_state();
	return;
    }
    
    if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away
	update_state();
	return;
    }
    
    // Permanent error
    _state = FAILED;
    XLOG_ERROR("Failed to get endpoint addr for address %s: Permanent Error",
	       addr.str().c_str());
}

void
XrlMfeaVifManager::vifaddr4_deleted(const string& ifname,
				    const string& vifname,
				    const IPv4& addr)
{
    debug_msg("vifaddr4_deleted for interface %s vif %s: %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr4_deleted on unknown vif: %s", vifname.c_str());
	return;
    }
    Vif* vif = _vifs_by_name[vifname];
    vif->delete_address(addr);
    
    update_state();
}

void
XrlMfeaVifManager::vifaddr6_deleted(const string& ifname,
				    const string& vifname,
				    const IPv6& addr)
{
    debug_msg("vifaddr6_deleted for interface %s vif %s: %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr6_deleted on unknown vif: %s", vifname.c_str());
	return;
    }
    Vif* vif = _vifs_by_name[vifname];
    vif->delete_address(addr);
    
    update_state();
}
