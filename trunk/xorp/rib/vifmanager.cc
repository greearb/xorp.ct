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

#ident "$XORP: xorp/rib/vifmanager.cc,v 1.9 2003/05/14 10:32:25 pavlin Exp $"

#include "rib_module.h"
#include "config.h"
#include "libxorp/xlog.h"

#include "rib_manager.hh"
#include "vifmanager.hh"

VifManager::VifManager(XrlRouter& xrl_router, EventLoop& eventloop,
		       RibManager *rib_manager)
    : _xrl_router(xrl_router),
      _eventloop(eventloop),
      _rib_manager(rib_manager),
      _ifmgr_client(&xrl_router)
{
    _no_fea = false;
    _state = INITIALIZING;
    _register_retry_counter = 0;
    _interfaces_remaining = 0;
    _vifs_remaining = 0;
    _addrs_remaining = 0;
    
    enable();		// XXX: by default the VifManager is always enabled
}

VifManager::~VifManager() 
{
    stop();
    
    map <string, Vif*>::iterator i;
    for (i = _vifs_by_name.begin(); i != _vifs_by_name.end(); ++i) {
	delete i->second;
    }
}

/**
 * Start operation.
 * 
 * Start the process of registering with the FEA, etc.
 * 
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
VifManager::start()
{
    enable();		// XXX: by default the VifManager is always enabled
    
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
 * Gracefully stop the VifManager.
 * 
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
VifManager::stop()
{
    if (! is_up())
        return (XORP_ERROR);
    
    clean_out_old_state();
    
    ProtoState::stop();
    
    return (XORP_OK);
}

void
VifManager::clean_out_old_state() 
{
    if (_no_fea)
	return;
    
    // we call unregister_client first, to cause the FEA to remove any
    // registrations left over from previous incarnations of the RIB
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(this, &VifManager::clean_out_old_state_done);
    _ifmgr_client.send_unregister_client("fea", _xrl_router.name(), cb);
}

void
VifManager::clean_out_old_state_done(const XrlError& e) 
{
    UNUSED(e);
    // We really don't care here if the request succeeded or failed.
    // It's normal to fail with COMMAND_FAILED if there was no state
    // left behind from a previous incarnation.  Any other errors would
    // also show up in register_if_spy, so we'll let that deal with
    // them.
    // register_if_spy();
}

void
VifManager::register_if_spy() 
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(this, &VifManager::register_if_spy_done);
    _ifmgr_client.send_register_client("fea", _xrl_router.name(), cb);
}

void
VifManager::register_if_spy_done(const XrlError& e) 
{
    if (e == XrlError::OKAY()) {
	// The registration was successful.  Now we need to query the
	// entries that are already there.  First, find out the set of
	// configured interfaces.
	XorpCallback2<void, const XrlError&, const XrlAtomList*>::RefPtr cb;
	cb = callback(this, &VifManager::interface_names_done);
	_ifmgr_client.send_get_configured_interface_names("fea", cb);
	return;
    }

    if (_no_fea) {
	_state = READY;
	return;
    }

    //
    // If the resolve failed, it could be that we got going too quickly
    // for the FEA.  Retry every two seconds.  If after ten seconds we
    // still can't register, give up.  It's a higher level issue as to
    // whether failing to register is a fatal error.
    //
    if (e == XrlError::RESOLVE_FAILED() && (_register_retry_counter < 5)) {
	debug_msg("Register Interface Spy: RESOLVE_FAILED\n");
	_register_retry_counter++;
	OneoffTimerCallback cb;
	cb = callback(this, &VifManager::register_if_spy);
	_register_retry_timer = _eventloop.new_oneoff_after_ms(2000, cb);
	return;
    }
    _state = FAILED;
    XLOG_ERROR("Register Interface Spy: Permanent Error");
}

void
VifManager::interface_names_done(const XrlError& e, const XrlAtomList* alist) 
{
    printf("interface_names_done\n");
    if (e == XrlError::OKAY()) {
	printf("OK\n");
	for (size_t i = 0; i < alist->size(); i++) {
	    // Spin through the list of interfaces, and fire off
	    // requests in parallel for all the Vifs on each interface
	    XrlAtom atom = alist->get(i);
	    string ifname = atom.text();
	    XorpCallback2<void, const XrlError&,
		const XrlAtomList*>::RefPtr cb;
	    printf("got interface name: %s\n", ifname.c_str());
	    cb = callback(this, &VifManager::vif_names_done, ifname);
	    _ifmgr_client.send_get_configured_vif_names("fea", ifname, cb);
	    _interfaces_remaining++;
	}
	return;
    }
    _state = FAILED;
    XLOG_ERROR("Get Interface Names: Permanent Error");
}

void
VifManager::vif_names_done(const XrlError& e, const XrlAtomList* alist,
			   string ifname) 
{
    printf("vif_names_done\n");
    UNUSED(ifname);
    if (e==XrlError::OKAY()) {
	for (size_t i = 0; i < alist->size(); i++) {
	    // Spin through all the Vifs on this interface, and fire
	    // off requests in parallel for all the addresses on each
	    // Vif.
	    XrlAtom atom = alist->get(i);
	    string vifname = atom.text();
	    vif_created(ifname, vifname);
	    XorpCallback2<void, const XrlError&,
		const XrlAtomList*>::RefPtr cb;
	    cb = callback(this, &VifManager::get_all_vifaddr4_done,
			  ifname, vifname);
	    _ifmgr_client.send_get_configured_vif_addresses4("fea", ifname,
							   vifname, cb);
	    _vifs_remaining++;
	}
	_interfaces_remaining--;
    } else if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the interface went away.
	_interfaces_remaining--;
    } else {
	_state = FAILED;
	XLOG_ERROR("Get VIF Names: Permanent Error");
	return;
    }
    if (_interfaces_remaining == 0
	&& _vifs_remaining == 0
	&& _addrs_remaining == 0) {
	_state = READY;
    }
}

void
VifManager::get_all_vifaddr4_done(const XrlError& e, const XrlAtomList* alist,
				  string ifname, string vifname) 
{
    if (e == XrlError::OKAY()) {
	for (size_t i = 0; i < alist->size(); i++) {
	    XrlAtom atom = alist->get(i);
	    IPv4 addr = atom.ipv4();
	    vifaddr4_created(ifname, vifname, addr);
	}
	_vifs_remaining--;
    } else if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away?
	_vifs_remaining--;
    } else {
	_state = FAILED;
	XLOG_ERROR("Get VIF Names: Permanent Error");
	return;
    }

    if (_interfaces_remaining == 0
	&& _vifs_remaining == 0
	&& _addrs_remaining == 0) {
	_state = READY;
    }
}

void
VifManager::interface_update(const string& ifname,
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
    }
}

void
VifManager::vif_update(const string& ifname,
		       const string& vifname, const uint32_t& event) 
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
    }
}

void
VifManager::vifaddr4_update(const string& ifname,
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
	// shouldn't happen
	abort();
	break;
    }
}

void
VifManager::vifaddr6_update(const string& ifname,
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
	// shouldn't happen
	abort();
	break;
    }
}

void
VifManager::interface_deleted(const string& ifname) 
{
    multimap<string, Vif*>::iterator i;
    i = _vifs_by_interface.find(ifname);
    if (i != _vifs_by_interface.end()) {
	vif_deleted(ifname, i->second->name());
    }
}

void
VifManager::vif_deleted(const string& ifname, const string& vifname) 
{
    Vif *vif;
    printf("vif_deleted %s %s\n", ifname.c_str(), vifname.c_str());
    map<string, Vif*>::iterator vi = _vifs_by_name.find(vifname);
    if (vi != _vifs_by_name.end()) {
	vif = vi->second;
	_vifs_by_name.erase(vi);
	multimap<string, Vif*>::iterator ii = _vifs_by_interface.find(ifname);
	assert(ii != _vifs_by_interface.end());
	while (ii != _vifs_by_interface.end() &&
	       ii->first == ifname) {
	    if (ii->second == vif) {
		_vifs_by_interface.erase(ii);
		break;
	    }
	    ii++;
	    assert(ii != _vifs_by_interface.end());
	}

	// now process the deletion...
	string err;
	_rib_manager->delete_vif(vifname, err);
	delete vif;
    } else {
	XLOG_ERROR("vif_deleted: vif %s not found", vifname.c_str());
    }
}

void
VifManager::vif_created(const string& ifname, const string& vifname) 
{
    printf("vif_created: %s\n", vifname.c_str());
    if (_vifs_by_name.find(vifname) != _vifs_by_name.end()) {
	XLOG_ERROR("vif_created: vif %s already exists", vifname.c_str());
	return;
    }
    Vif *vif = new Vif(vifname, ifname);
    _vifs_by_name[vifname] = vif;
    _vifs_by_interface.insert(pair<string, Vif*>(ifname, vif));

    // now process the addition
    string err;
    _rib_manager->new_vif(vifname, *vif, err);
}

void
VifManager::vifaddr4_created(const string& ifname, const string& vifname,
			     const IPv4& addr) 
{
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr4_created on unknown vif: %s", vifname.c_str());
	return;
    }
    XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
    cb = callback(this, &VifManager::vifaddr4_done, ifname, vifname, addr);
    _ifmgr_client.send_get_prefix4("fea", ifname, vifname, addr, cb);
    _addrs_remaining++;
}

void
VifManager::vifaddr4_done(const XrlError& e, const uint32_t* prefix_len,
			  string ifname, string vifname,
			  IPv4 addr) 
{
    UNUSED(ifname);

    if (e == XrlError::OKAY()) {

	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	printf("adding address %s prefix_len %d to vif %s\n",
	       addr.str().c_str(), *prefix_len, vifname.c_str());
	vif->add_address(addr, IPvXNet(addr, *prefix_len),
			 IPvX("0.0.0.0"), IPvX("0.0.0.0"));
	string err;
	if (_rib_manager->add_vif_addr(vifname, addr, 
				       IPv4Net(addr, *prefix_len),
				       err) != XORP_OK) {
	    XLOG_ERROR(c_format("failed to add address %s/%d to vif %s",
				addr.str().c_str(), *prefix_len,
				vifname.c_str()).c_str());
	}
    } else if (e != XrlError::COMMAND_FAILED()) {
	_addrs_remaining--;
    } else {
	XLOG_ERROR("Failed to get prefix_len for address %s",
		   addr.str().c_str());
	_addrs_remaining--;
    }

    if (_interfaces_remaining == 0
	&& _vifs_remaining == 0
	&& _addrs_remaining == 0
	&& _state == INITIALIZING) {
	_state = READY;
    }
}


void
VifManager::vifaddr4_deleted(const string& ifname, const string& vifname,
			     const IPv4& addr) 
{
    UNUSED(ifname);
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr4_deleted on unknown vif: %s", vifname.c_str());
	return;
    }
     Vif *vif = _vifs_by_name[vifname];
     vif->delete_address(addr);

     string err;
     if (_rib_manager->delete_vif_addr(vifname, addr, err) != XORP_OK) {
	 XLOG_ERROR(c_format("failed to delete address %s from vif %s",
			     addr.str().c_str(), vifname.c_str()).c_str());
     }
}

void
VifManager::vifaddr6_created(const string& ifname, const string& vifname,
			     const IPv6& addr) 
{
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr6_created on unknown vif: %s", vifname.c_str());
	return;
    }
    XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
    cb = callback(this, &VifManager::vifaddr6_done, ifname, vifname, addr);
    _ifmgr_client.send_get_prefix6("fea", ifname, vifname, addr, cb);
    _addrs_remaining++;
}

void
VifManager::vifaddr6_done(const XrlError& e, const uint32_t* prefix_len,
			  string ifname, string vifname,
			  IPv6 addr) 
{
    UNUSED(ifname);

    if (e == XrlError::OKAY()) {

	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    return;
	}
	Vif *vif = _vifs_by_name[vifname];
	printf("adding address %s prefix_len %d to vif %s\n",
	       addr.str().c_str(), *prefix_len, vifname.c_str());
	vif->add_address(addr, IPvXNet(addr, *prefix_len),
			 IPvX("0.0.0.0"), IPvX("0.0.0.0"));
	string err;
	if (_rib_manager->add_vif_addr(vifname, addr, 
				       IPv6Net(addr,*prefix_len),
				       err) != XORP_OK) {
	    XLOG_ERROR("failed to add address %s/%d to vif %s",
		       addr.str().c_str(), *prefix_len,
		       vifname.c_str());
	}
    } else if (e != XrlError::COMMAND_FAILED()) {
	_addrs_remaining--;
    } else {
	XLOG_ERROR("Failed to get prefix_len for address %s",
		   addr.str().c_str());
	_addrs_remaining--;
    }

    if (_interfaces_remaining == 0
	&& _vifs_remaining == 0
	&& _addrs_remaining == 0
	&& _state == INITIALIZING) {
	_state = READY;
    }
}


void
VifManager::vifaddr6_deleted(const string& ifname, const string& vifname,
			     const IPv6& addr) 
{
    UNUSED(ifname);
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr6_deleted on unknown vif: %s", vifname.c_str());
	return;
    }
     Vif *vif = _vifs_by_name[vifname];
     vif->delete_address(addr);

     string err;
     if (_rib_manager->delete_vif_addr(vifname, addr, err) != XORP_OK) {
	 XLOG_ERROR("failed to delete address %s from vif %s",
		    addr.str().c_str(), vifname.c_str());
     }
}

