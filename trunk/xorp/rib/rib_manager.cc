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

#ident "$XORP: xorp/rib/rib_manager.cc,v 1.22 2003/09/27 22:32:46 mjh Exp $"

#include "rib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"
#include "libxipc/xrl_error.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "rib_manager.hh"

RibManager::RibManager(EventLoop& eventloop, XrlStdRouter& xrl_std_router)
    : _status_code(PROC_NOT_READY), 
      _status_reason("Initializing"), 
      _eventloop(eventloop),
      _xrl_router(xrl_std_router),
      _register_server(&_xrl_router),
      _urib4(UNICAST, *this, _eventloop),
      _mrib4(MULTICAST, *this, _eventloop),
      _urib6(UNICAST, *this, _eventloop),
      _mrib6(MULTICAST, *this, _eventloop),
      _vif_manager(_xrl_router, _eventloop, this),
      _xrl_rib_target(&_xrl_router, _urib4, _mrib4, _urib6, _mrib6, 
		      _vif_manager, this)
{
    _urib4.initialize_export(&_urib4_clients_list);
    _urib4.initialize_register(&_register_server);
    if (_urib4.add_igp_table("connected", "", "") != XORP_OK) {
	XLOG_ERROR("Could not add igp table \"connected\" for urib4");
	return;
    }

    _mrib4.initialize_export(&_mrib4_clients_list);
    _mrib4.initialize_register(&_register_server);
    if (_mrib4.add_igp_table("connected", "", "") != XORP_OK) {
	XLOG_ERROR("Could not add igp table \"connected\" for mrib4");
	return;
    }

    _urib6.initialize_export(&_urib6_clients_list);
    _urib6.initialize_register(&_register_server);
    if (_urib6.add_igp_table("connected", "", "") != XORP_OK) {
	XLOG_ERROR("Could not add igp table \"connected\" for urib6");
	return;
    }

    _mrib6.initialize_export(&_mrib6_clients_list);
    _mrib6.initialize_register(&_register_server);
    if (_mrib6.add_igp_table("connected", "", "") != XORP_OK) {
	XLOG_ERROR("Could not add igp table \"connected\" for mrib6");
	return;
    }
    PeriodicTimerCallback cb = callback(this, &RibManager::status_updater);
    _status_update_timer = _eventloop.new_periodic(1000, cb);
}

RibManager::~RibManager()
{
    stop();
    
    delete_pointers_list(_urib4_clients_list);
    delete_pointers_list(_mrib4_clients_list);
    delete_pointers_list(_urib6_clients_list);
    delete_pointers_list(_mrib6_clients_list);
}

/**
 * RibManager::start:
 * @: 
 * 
 * Start operation.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
RibManager::start()
{
    if (ProtoState::start() != XORP_OK)
	return (XORP_ERROR);
    
    _vif_manager.start();
    
    return (XORP_OK);
}

/**
 * RibManager::stop:
 * @: 
 * 
 * Gracefully stop the RIB.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
RibManager::stop()
{
    if (! is_up())
	return (XORP_ERROR);
    
    _vif_manager.stop();
    
    ProtoState::stop();
 
    _status_code = PROC_SHUTDOWN;
    _status_reason = "Shutting down";
    status_updater();
    return (XORP_OK);
}

bool
RibManager::status_updater()
{
    ProcessStatus s = PROC_READY;
    string reason;

    /*
     *Check the VifManager's status
     */
    VifManager::State vif_mgr_state = _vif_manager.state();
    switch (vif_mgr_state) {
    case VifManager::INITIALIZING:
	s = PROC_NOT_READY;
	reason = "VifManager initializing";
	break;
    case VifManager::READY:
	break;
    case VifManager::FAILED:
	//VifManager failed: set process state to failed.
	//XXX Should we exit here, or wait to be restarted?
	_status_code = PROC_FAILED;
	_status_reason = "VifManager Failed";
	return false;
    }

    /*
     *Check the unicast RIBs can still talk to the FEA
     */
    RibClient *fea_client;
    fea_client = find_rib_client("fea", AF_INET, true, false);
    if (fea_client != NULL) {
	if (fea_client->failed()) {
	    _status_code = PROC_FAILED;
	    _status_reason = "Fatal error talking to FEA (v4, unicast)";
	    return false;
	}
    }
    fea_client = find_rib_client("fea", AF_INET6, true, false);
    if (fea_client != NULL) {
	if (fea_client->failed()) {
	    _status_code = PROC_FAILED;
	    _status_reason = "Fatal error talking to FEA (v6, unicast)";
	    return false;
	}
    }
    
    //SHUTDOWN state is persistent unless there's a failure.
    if (_status_code != PROC_SHUTDOWN) {
	_status_code = s;
	_status_reason = "Ready";
    }
    return true;
}

int
RibManager::new_vif(const string& vifname, const Vif& vif, string& err) 
{
    if (_urib4.new_vif(vifname, vif) != XORP_OK) {
	err = (c_format("Failed to add vif \"%s\" to unicast IPv4 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib4.new_vif(vifname, vif) != XORP_OK) {
	err = (c_format("Failed to add vif \"%s\" to multicast IPv4 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }

    if (_urib6.new_vif(vifname, vif) != XORP_OK) {
	err = (c_format("Failed to add vif \"%s\" to unicast IPv6 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib6.new_vif(vifname, vif) != XORP_OK) {
	err = (c_format("Failed to add vif \"%s\" to multicast IPv6 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }
    return XORP_OK;
}

int
RibManager::delete_vif(const string& vifname, string& err) 
{
    if (_urib4.delete_vif(vifname) != XORP_OK) {
	err = (c_format("Failed to delete vif \"%s\" from unicast IPv4 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib4.delete_vif(vifname) != XORP_OK) {
	err = (c_format("Failed to delete vif \"%s\" from multicast IPv4 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }

    if (_urib6.delete_vif(vifname) != XORP_OK) {
	err = (c_format("Failed to delete vif \"%s\" from unicast IPv6 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib6.delete_vif(vifname) != XORP_OK) {
	err = (c_format("Failed to delete vif \"%s\" from multicast IPv6 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }
    return XORP_OK;
}

int
RibManager::add_vif_address(const string& vifname,
			    const IPv4& addr,
			    const IPv4Net& subnet,
			    string& err)
{
    if (_urib4.add_vif_address(vifname, addr, subnet) != XORP_OK) {
	err = "Failed to add IPv4 Vif address to unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib4.add_vif_address(vifname, addr, subnet) != XORP_OK) {
	err = "Failed to add IPv4 Vif address to multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
RibManager::delete_vif_address(const string& vifname,
			       const IPv4& addr,
			       string& err)
{
    if (_urib4.delete_vif_address(vifname, addr) != XORP_OK) {
	err = "Failed to delete IPv4 Vif address from unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib4.delete_vif_address(vifname, addr) != XORP_OK) {
	err = "Failed to delete IPv4 Vif address from multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
RibManager::add_vif_address(const string& vifname,
			    const IPv6& addr,
			    const IPv6Net& subnet,
			    string& err)
{
    if (_urib6.add_vif_address(vifname, addr, subnet) != XORP_OK) {
	err = "Failed to add IPv6 Vif address to unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib6.add_vif_address(vifname, addr, subnet) != XORP_OK) {
	err = "Failed to add IPv6 Vif address to multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
RibManager::delete_vif_address(const string& vifname,
			       const IPv6& addr,
			       string& err)
{
    if (_urib6.delete_vif_address(vifname, addr) != XORP_OK) {
	err = "Failed to delete IPv6 Vif address from unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib6.delete_vif_address(vifname, addr) != XORP_OK) {
	err = "Failed to delete IPv6 Vif address from multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

//
// Select the appropriate list of RIB clients
//
list<RibClient *> *
RibManager::select_rib_clients_list(int family, bool unicast, bool multicast)
{
    if (! (unicast ^ multicast))
	return (NULL);	// Only one of the flags must be set
    
    //
    // Select the appropriate list
    //
    switch (family) {
    case AF_INET:
	if (unicast)
	    return (&_urib4_clients_list);
	if (multicast)
	    return (&_mrib4_clients_list);
	break;
    case AF_INET6:
	if (unicast)
	    return (&_urib6_clients_list);
	if (multicast)
	    return (&_mrib6_clients_list);
	break;
    default:
	XLOG_FATAL("Invalid address family: %d", family);
	break;
    }
    
    //
    // Nothing found
    //
    return (NULL);
}

//
// Find a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
RibClient *
RibManager::find_rib_client(const string& target_name, int family,
			    bool unicast, bool multicast)
{
    list<RibClient *> *rib_clients_list;
    
    rib_clients_list = select_rib_clients_list(family, unicast, multicast);
    if (rib_clients_list == NULL)
	return (NULL);
    
    //
    // Search for a RIB client with the same target name
    //
    list<RibClient *>::iterator iter;
    for (iter = rib_clients_list->begin();
	 iter != rib_clients_list->end();
	 ++iter) {
	RibClient *rib_client = *iter;
	if (rib_client->target_name() == target_name)
	    return (rib_client);
    }
    
    //
    // Nothing found
    //
    return (NULL);
}

//
// Add a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::add_rib_client(const string& target_name, int family,
			   bool unicast, bool multicast)
{
    list<RibClient *> *rib_clients_list;
    
    //
    // Check if this RIB client has been added already
    //
    if (find_rib_client(target_name, family, unicast, multicast) != NULL)
	return (XORP_OK);
    
    //
    // Find the list of RIB clients to add the new client to.
    //
    rib_clients_list = select_rib_clients_list(family, unicast, multicast);
    if (rib_clients_list == NULL)
	return (XORP_ERROR);
    
    //
    // Create a new RibClient, and add it to the list
    //
    RibClient *rib_client = new RibClient(_xrl_router, target_name);
    rib_clients_list->push_back(rib_client);
    
    return (XORP_OK);
}

//
// Delete a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::delete_rib_client(const string& target_name, int family,
			      bool unicast, bool multicast)
{
    RibClient *rib_client;
    list<RibClient *> *rib_clients_list;
    
    //
    // Find the RIB client
    //
    rib_client = find_rib_client(target_name, family, unicast, multicast);
    if (rib_client == NULL)
	return (XORP_ERROR);

    //
    // Find the list of RIB clients
    //
    rib_clients_list = select_rib_clients_list(family, unicast, multicast);
    if (rib_clients_list == NULL)
	return (XORP_ERROR);
    
    list<RibClient *>::iterator iter;
    iter = find(rib_clients_list->begin(),
		rib_clients_list->end(),
		rib_client);
    XLOG_ASSERT(iter != rib_clients_list->end());
    rib_clients_list->erase(iter);
    
    delete rib_client;
    
    return (XORP_OK);
}

//
// Enable a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::enable_rib_client(const string& target_name, int family,
			      bool unicast, bool multicast)
{
    RibClient *rib_client;
    
    //
    // Find the RIB client
    //
    rib_client = find_rib_client(target_name, family, unicast, multicast);
    if (rib_client == NULL)
	return (XORP_ERROR);
    
    rib_client->set_enabled(true);
    
    return (XORP_OK);
}

//
// Disable a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::disable_rib_client(const string& target_name, int family,
			       bool unicast, bool multicast)
{
    RibClient *rib_client;
    
    //
    // Find the RIB client
    //
    rib_client = find_rib_client(target_name, family, unicast, multicast);
    if (rib_client == NULL)
	return (XORP_ERROR);
    
    rib_client->set_enabled(false);
    
    return (XORP_OK);
}

//
// Don't try to communicate with the FEA.
//
// Note that this method will be obsoleted in the future, and will
// be replaced with cleaner interface.
//
int
RibManager::no_fea()
{
    // TODO: FEA target name hardcoded
    disable_rib_client("fea", AF_INET, true, false);
    disable_rib_client("fea", AF_INET6, true, false);
    
    _vif_manager.no_fea();
    
    return (XORP_OK);
}

void
RibManager::make_errors_fatal()
{
    _urib4.set_errors_are_fatal();
    _urib6.set_errors_are_fatal();
    _mrib4.set_errors_are_fatal();
    _mrib6.set_errors_are_fatal();
}

void 
RibManager::register_interest_in_target(const string& tgt_class)
{
    if (_targets_of_interest.find(tgt_class) 
	== _targets_of_interest.end()) {
	_targets_of_interest.insert(tgt_class);
	XrlFinderEventNotifierV0p1Client finder(&_xrl_router);
	XrlFinderEventNotifierV0p1Client::RegisterClassEventInterestCB cb =
	    callback(this, &RibManager::register_interest_in_target_done);
	finder.send_register_class_event_interest("finder",
						  _xrl_router.instance_name(),
						  tgt_class, cb);
    } else {
	return;
    }
}

void 
RibManager::register_interest_in_target_done(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to register interest in an XRL target\n");
    }
}


void
RibManager::target_death(const string& tgt_class, const string& tgt_instance)
{
    if (tgt_class == "fea") {
	//No cleanup - we just exit.
	XLOG_ERROR("FEA died, so RIB is exiting too\n");
	exit(0);
    }

    //inform the RIBs in case this was a routing protocol that died.
    _urib4.target_death(tgt_class, tgt_instance);
    _urib6.target_death(tgt_class, tgt_instance);
    _mrib4.target_death(tgt_class, tgt_instance);
    _mrib6.target_death(tgt_class, tgt_instance);

    //remove any RibClients that died.
    list<RibClient *>::iterator i;

    for (i=_urib4_clients_list.begin(); i!= _urib4_clients_list.end(); i++) {
	if (((*i)->target_name() == tgt_instance)
	    || ((*i)->target_name() == tgt_class)) {
	    delete (*i);
	    _urib4_clients_list.erase(i);
	    break;
	}
    }

    for (i=_mrib4_clients_list.begin(); i!= _mrib4_clients_list.end(); i++) {
	if (((*i)->target_name() == tgt_instance)
	    || ((*i)->target_name() == tgt_class)) {
	    delete (*i);
	    _mrib4_clients_list.erase(i);
	    break;
	}
    }

    for (i=_urib6_clients_list.begin(); i!= _urib6_clients_list.end(); i++) {
	if (((*i)->target_name() == tgt_instance)
	    || ((*i)->target_name() == tgt_class)) {
	    delete (*i);
	    _urib6_clients_list.erase(i);
	    break;
	}
    }

    for (i=_mrib6_clients_list.begin(); i!= _mrib6_clients_list.end(); i++) {
	if (((*i)->target_name() == tgt_instance)
	    || ((*i)->target_name() == tgt_class)) {
	    delete (*i);
	    _mrib6_clients_list.erase(i);
	    break;
	}
    }
}
