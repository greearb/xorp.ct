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
// MFEA virtual interfaces implementation.
//

#include "mfea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mrt/multicast_defs.h"

#include "mfea_node.hh"
#include "mfea_osdep.hh"
#include "mfea_vif.hh"

map<string, VifPermInfo> perm_info;

/**
 * MfeaVif::MfeaVif:
 * @mfea_node: The MFEA node this interface belongs to.
 * @vif: The generic Vif interface that contains various information.
 * 
 * MFEA vif constructor.
 **/
MfeaVif::MfeaVif(MfeaNode& mfea_node, const Vif& vif)
    : ProtoUnit(mfea_node.family(), mfea_node.module_id()),
      Vif(vif),
      _mfea_node(mfea_node),
      _min_ttl_threshold(MINTTL),
      _max_rate_limit(0),		// XXX: unlimited rate limit
      _registered_ip_protocol(0)
{
    // Check our wants-to-be-running list
    map<string, VifPermInfo>::iterator i = perm_info.find(name());
    if (i != perm_info.end()) {
	wants_to_be_started = i->second.should_start;
    }
    else {
	wants_to_be_started = false;
    }
}

/**
 * MfeaVif::MfeaVif:
 * @mfea_node: The MFEA node this interface belongs to.
 * @mfea_vif: The origin MfeaVif interface that contains the initialization
 * information.
 * 
 * MFEA vif copy constructor.
 **/
MfeaVif::MfeaVif(MfeaNode& mfea_node, const MfeaVif& mfea_vif)
    : ProtoUnit(mfea_node.family(), mfea_node.module_id()),
      Vif(mfea_vif),
      _mfea_node(mfea_node),
      _min_ttl_threshold(mfea_vif.min_ttl_threshold()),
      _max_rate_limit(mfea_vif.max_rate_limit()),
      _registered_ip_protocol(0)
{
}

/**
 * MfeaVif::~MfeaVif:
 * @: 
 * 
 * MFEA vif destructor.
 * 
 **/
MfeaVif::~MfeaVif()
{
    string error_msg;

    stop(error_msg, false, "destructor");
}

/**
 * MfeaVif::start:
 * @error_msg: The error message (if error).
 * 
 * Start MFEA on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaVif::start(string& error_msg, const char* dbg)
{
    XLOG_INFO("%s:  start called, is_enabled: %i  is-up: %i  is-pending-up: %i, dbg: %s\n",
	      name().c_str(), (int)(is_enabled()), (int)(is_up()), (int)(is_pending_up()),
	      dbg);

    map<string, VifPermInfo>::iterator i = perm_info.find(name());

    if (! is_enabled()) {
	if (i != perm_info.end()) {
	    if (i->second.should_enable) {
		enable("start, should_enable");
	    }
	}
    }

    if (! is_enabled())
	return XORP_OK;

    if (is_up() || is_pending_up())
	return (XORP_OK);

    // Add to our wants-to-be-running list
    if (i != perm_info.end()) {
	i->second.should_start = true;
    }
    else {
	VifPermInfo pi(name(), true, false);
	perm_info[name()] = pi;
    }

    if (! is_underlying_vif_up()) {
	// Start us later.
	wants_to_be_started = true;
	XLOG_WARNING("WARNING:  Delaying start of mfea-vif: %s because underlying vif is not up.",
		     name().c_str());
	return XORP_OK;
    }

    if (!(is_pim_register() || is_multicast_capable())) {
	wants_to_be_started = true;
	XLOG_WARNING("WARNING:  Delaying start of mfea-vif: %s because underlying vif is not multicast capable.",
		     name().c_str());
	return XORP_OK;
    }

    //
    // Install in the kernel only if the vif is of the appropriate type:
    // multicast-capable (loopback excluded), or PIM Register vif.
    //
    if (is_loopback()) {
	error_msg = "mfea-vif: Loopback interfaces cannot be used for multicast.";
	return (XORP_ERROR);
    }

    if (ProtoUnit::start() != XORP_OK) {
	error_msg = "internal error";
	return (XORP_ERROR);
    }
    
    if (mfea_node().add_multicast_vif(vif_index()) != XORP_OK) {
	error_msg = "cannot add the multicast vif to the kernel";
	return (XORP_ERROR);
    }

    XLOG_INFO("Interface started: %s%s",
	      this->str().c_str(), flags_string().c_str());

    wants_to_be_started = false; //it worked

    return (XORP_OK);
}

/** System detected some change.  */
void MfeaVif::notifyUpdated() {
    int perm_started = -1;
    if (!wants_to_be_started) {
	map<string, VifPermInfo>::iterator i = perm_info.find(name());
	if (i != perm_info.end()) {
	    perm_started = i->second.should_start;
	}
    }

    XLOG_INFO("notifyUpdated, vif: %s  wants-to-be-started: %i, perm-should-start: %i",
	      name().c_str(), (int)(wants_to_be_started), perm_started);

    if (wants_to_be_started || (perm_started == 1)) {
	string err_msg;
	int rv = start(err_msg, "notifyUpdated");
	if (rv == XORP_OK) {
	    XLOG_WARNING("notifyUpdated, successfully started mfea_vif: %s",
			 name().c_str());
	}
	else {
	    XLOG_WARNING("notifyUpdated, tried to start vif: %s, but failed: %s",
			 name().c_str(), err_msg.c_str());
	}
    }
}
	    
/**
 * MfeaVif::stop:
 * @error_msg: The error message (if error).
 * 
 * Stop MFEA on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaVif::stop(string& error_msg, bool stay_down, const char* dbg)
{
    int ret_value = XORP_OK;
    wants_to_be_started = false;

    if (stay_down) {
	// Remove from our wants-to-be-running list
	map<string, VifPermInfo>::iterator i = perm_info.find(name());
	if (i != perm_info.end()) {
	    i->second.should_start = false;
	}
    }

    XLOG_INFO("%s:  stop called, stay_down: %i dbg: %s\n",
	      name().c_str(), (int)(stay_down), dbg);

    if (is_down())
	return (XORP_OK);

    if (! (is_up() || is_pending_up() || is_pending_down())) {
	error_msg = "the vif state is not UP or PENDING_UP or PENDING_DOWN";
	return (XORP_ERROR);
    }

    if (ProtoUnit::pending_stop() != XORP_OK) {
	error_msg = "internal error";
	ret_value = XORP_ERROR;
    }

    if (ProtoUnit::stop() != XORP_OK) {
	error_msg = "internal error";
	ret_value = XORP_ERROR;
    }

    if (mfea_node().delete_multicast_vif(vif_index()) != XORP_OK) {
	XLOG_ERROR("Cannot delete multicast vif %s with the kernel",
		   name().c_str());
	ret_value = XORP_ERROR;
    }

    XLOG_INFO("Interface stopped %s%s",
	      this->str().c_str(), flags_string().c_str());

    //
    // Inform the node that the vif has completed the shutdown
    //
    mfea_node().vif_shutdown_completed(name());

    return (ret_value);
}

/**
 * Enable MFEA on a single virtual interface.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
void
MfeaVif::enable(const char* dbg)
{
    XLOG_INFO("MfeaVif: Interface enable %s%s, dbg: %s",
	      this->str().c_str(), flags_string().c_str(), dbg);
    ProtoUnit::enable();
}

/**
 * Disable MFEA on a single virtual interface.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
void
MfeaVif::disable(const char* dbg)
{
    string error_msg;

    stop(error_msg, true, "MfeaVif::disable");
    ProtoUnit::disable();

    XLOG_INFO("Interface disabled %s%s, dbg: %s",
	      this->str().c_str(), flags_string().c_str(), dbg);
}

int
MfeaVif::register_protocol(const string&	module_instance_name,
			   uint8_t		ip_protocol,
			   string&		error_msg)
{
    if (! _registered_module_instance_name.empty()) {
	error_msg = c_format("Cannot register %s on vif %s: "
			     "%s already registered",
			     module_instance_name.c_str(),
			     name().c_str(),
			     _registered_module_instance_name.c_str());
	return (XORP_ERROR);
    }

    _registered_module_instance_name = module_instance_name;
    _registered_ip_protocol = ip_protocol;

    return (XORP_OK);
}

int
MfeaVif::unregister_protocol(const string&	module_instance_name,
			     string&		error_msg)
{
    if (module_instance_name != _registered_module_instance_name) {
	error_msg = c_format("Cannot unregister %s on vif %s: "
			     "%s was registered previously",
			     module_instance_name.c_str(),
			     name().c_str(),
			     (_registered_module_instance_name.size())?
			     _registered_module_instance_name.c_str() : "NULL");
	return (XORP_ERROR);
    }

    _registered_module_instance_name = "";
    _registered_ip_protocol = 0;

    return (XORP_OK);
}

// TODO: temporary here. Should go to the Vif class after the Vif
// class starts using the Proto class
string
MfeaVif::flags_string() const
{
    string flags;
    
    if (is_up())
	flags += " UP";
    if (is_down())
	flags += " DOWN";
    if (is_pending_up())
	flags += " PENDING_UP";
    if (is_pending_down())
	flags += " PENDING_DOWN";
    if (is_ipv4())
	flags += " IPv4";
    if (is_ipv6())
	flags += " IPv6";
    if (is_enabled())
	flags += " ENABLED";
    if (is_disabled())
	flags += " DISABLED";
    
    return (flags);
}
