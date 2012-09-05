// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "pim_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/random.h"
#include "libxorp/ipvx.hh"

#include "pim_node.hh"
#include "pim_vif.hh"


int
PimNode::set_config_all_vifs_done(string& error_msg)
{
    map<string, Vif>::iterator vif_iter;
    map<string, Vif>& configured_vifs = ProtoNode<PimVif>::configured_vifs();
    set<string> send_pim_hello_vifs;
    string dummy_error_msg;

    //
    // Add new vifs and update existing ones
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
	    add_vif(*vif, dummy_error_msg);
	    continue;
	}
	
	//
	// Update the vif flags
	//
	set_vif_flags(vif->name(), vif->is_pim_register(), vif->is_p2p(),
		      vif->is_loopback(), vif->is_multicast_capable(),
		      vif->is_broadcast_capable(), vif->is_underlying_vif_up(),
		      vif->mtu(), dummy_error_msg);
    }

    //
    // Add new vif addresses, update existing ones, and remove old addresses
    //
    for (vif_iter = configured_vifs.begin();
	 vif_iter != configured_vifs.end();
	 ++vif_iter) {
	Vif* vif = &vif_iter->second;
	Vif* node_vif = vif_find_by_name(vif->name());
	list<VifAddr>::const_iterator vif_addr_iter;

	if (node_vif == NULL)
	    continue;

	//
	// Add new vif addresses and update existing ones
	//
	for (vif_addr_iter = vif->addr_list().begin();
	     vif_addr_iter != vif->addr_list().end();
	     ++vif_addr_iter) {
	    const VifAddr& vif_addr = *vif_addr_iter;
	    bool should_send_pim_hello = false;
	    add_vif_addr(vif->name(), vif_addr.addr(),
			 vif_addr.subnet_addr(),
			 vif_addr.broadcast_addr(),
			 vif_addr.peer_addr(),
			 should_send_pim_hello,
			 dummy_error_msg);
	    if (should_send_pim_hello)
		send_pim_hello_vifs.insert(vif->name());
	}
	
	//
	// Delete vif addresses that don't exist anymore
	//
	{
	    list<IPvX> delete_addresses_list;
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
		bool should_send_pim_hello = false;
		delete_vif_addr(vif->name(), ipvx, should_send_pim_hello,
				dummy_error_msg);
		if (should_send_pim_hello)
		    send_pim_hello_vifs.insert(vif->name());
	    }
	}
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

    //
    // Spec:
    // "If an interface changes one of its secondary IP addresses,
    // a Hello message with an updated Address_List option and a
    // non-zero HoldTime should be sent immediately."
    //
    set<string>::iterator set_iter;
    for (set_iter = send_pim_hello_vifs.begin();
	 set_iter != send_pim_hello_vifs.end();
	 ++set_iter) {
	string vif_name = *set_iter;
	PimVif *pim_vif = vif_find_by_name(vif_name);
	if ((pim_vif != NULL) && pim_vif->is_up()) {
	    if (! pim_vif->is_pim_register())
		pim_vif->pim_hello_send(dummy_error_msg);
	}
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_proto_version(const string& vif_name, int& proto_version,
			       string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get protocol version for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    proto_version = pim_vif->proto_version();
    
    return (XORP_OK);
}

int
PimNode::set_vif_proto_version(const string& vif_name, int proto_version,
			       string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set protocol version for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->set_proto_version(proto_version) != XORP_OK) {
	end_config(error_msg);
	error_msg = c_format("Cannot set protocol version for vif %s: "
			     "invalid protocol version %d",
			     vif_name.c_str(), proto_version);
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_proto_version(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset protocol version for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->set_proto_version(pim_vif->proto_version_default());
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_hello_triggered_delay(const string& vif_name,
				       uint16_t& hello_triggered_delay,
				       string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Hello triggered delay for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    hello_triggered_delay = pim_vif->hello_triggered_delay().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_hello_triggered_delay(const string& vif_name,
				       uint16_t hello_triggered_delay,
				       string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Hello triggered delay for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->hello_triggered_delay().set(hello_triggered_delay);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_hello_triggered_delay(const string& vif_name,
					 string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Hello triggered delay for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->hello_triggered_delay().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}


int
PimNode::get_vif_hello_period(const string& vif_name, uint16_t& hello_period,
			      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Hello period for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    hello_period = pim_vif->hello_period().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_hello_period(const string& vif_name, uint16_t hello_period,
			      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Hello period for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->hello_period().set(hello_period);

    if (! pim_vif->is_pim_register()) {
	//
	// Send immediately a Hello message, and schedule the next one
	// at random in the interval [0, hello_period)
	//
	pim_vif->pim_hello_send(dummy_error_msg);
	pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_hello_period(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Hello period for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->hello_period().reset();
    
    if (! pim_vif->is_pim_register()) {
	//
	// Send immediately a Hello message, and schedule the next one
	// at random in the interval [0, hello_period)
	//
	pim_vif->pim_hello_send(dummy_error_msg);
	pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_hello_holdtime(const string& vif_name,
				uint16_t& hello_holdtime,
				string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Hello holdtime for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    hello_holdtime = pim_vif->hello_holdtime().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_hello_holdtime(const string& vif_name,
				uint16_t hello_holdtime,
				string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Hello holdtime for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->hello_holdtime().set(hello_holdtime);
    
    if (! pim_vif->is_pim_register()) {
	//
	// Send immediately a Hello message, and schedule the next one
	// at random in the interval [0, hello_period)
	//
	pim_vif->pim_hello_send(dummy_error_msg);
	pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_hello_holdtime(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Hello holdtime for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->hello_holdtime().reset();
    
    if (! pim_vif->is_pim_register()) {
	//
	// Send immediately a Hello message, and schedule the next one
	// at random in the interval [0, hello_period)
	//
	pim_vif->pim_hello_send(dummy_error_msg);
	pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_dr_priority(const string& vif_name, uint32_t& dr_priority,
			     string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get DR priority for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    dr_priority = pim_vif->dr_priority().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_dr_priority(const string& vif_name, uint32_t dr_priority,
			     string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set DR priority for vif %s: "
			     "no such vif, will continue.",
			     vif_name.c_str());
	XLOG_INFO("%s", error_msg.c_str());
	map<string, PVifPermInfo>::iterator i = perm_info.find(vif_name);
	if (i != perm_info.end()) {
	    i->second.dr_priority = dr_priority;
	    i->second.set_dr_priority = true;
	}
	else {
	    PVifPermInfo pi(vif_name, true, false);
	    pi.dr_priority = dr_priority;
	    pi.set_dr_priority = true;
	    perm_info[vif_name] = pi;
	}
	return XORP_OK;
    }
    
    pim_vif->dr_priority().set(dr_priority);
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
	
	// (Re)elect the DR
	pim_vif->pim_dr_elect();
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_dr_priority(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset DR priority for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_INFO("%s", error_msg.c_str());
	map<string, PVifPermInfo>::iterator i = perm_info.find(vif_name);
	if (i != perm_info.end()) {
	    i->second.dr_priority = 0;
	    i->second.set_dr_priority = false;
	}
	return XORP_ERROR;
    }
    
    pim_vif->dr_priority().reset();
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
	
	// (Re)elect the DR
	pim_vif->pim_dr_elect();
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_propagation_delay(const string& vif_name,
				   uint16_t& propagation_delay,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Propagation delay for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    propagation_delay = pim_vif->propagation_delay().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_propagation_delay(const string& vif_name,
				   uint16_t propagation_delay,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Propagation delay for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->propagation_delay().set(propagation_delay);
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_propagation_delay(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Propagation delay for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->propagation_delay().reset();
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_override_interval(const string& vif_name,
				   uint16_t& override_interval,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Override interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    override_interval = pim_vif->override_interval().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_override_interval(const string& vif_name,
				   uint16_t override_interval,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Override interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->override_interval().set(override_interval);
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_override_interval(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Override interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->override_interval().reset();
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_is_tracking_support_disabled(const string& vif_name,
					      bool& is_tracking_support_disabled,
					      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Tracking support disabled flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    is_tracking_support_disabled = pim_vif->is_tracking_support_disabled().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_is_tracking_support_disabled(const string& vif_name,
					      bool is_tracking_support_disabled,
					      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Tracking support disabled flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->is_tracking_support_disabled().set(is_tracking_support_disabled);
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_is_tracking_support_disabled(const string& vif_name,
						string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    string dummy_error_msg;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Tracking support disabled flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->is_tracking_support_disabled().reset();
    
    if (! pim_vif->is_pim_register()) {
	// Send immediately a Hello message with the new value
	pim_vif->pim_hello_send(dummy_error_msg);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_accept_nohello_neighbors(const string& vif_name,
					  bool& accept_nohello_neighbors,
					  string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Accept nohello neighbors flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    accept_nohello_neighbors = pim_vif->accept_nohello_neighbors().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_accept_nohello_neighbors(const string& vif_name,
					  bool accept_nohello_neighbors,
					  string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Accept nohello neighbors flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (accept_nohello_neighbors && (! pim_vif->is_p2p())) {
	XLOG_WARNING("Accepting no-Hello neighbors should not be enabled "
	    "on non-point-to-point interfaces");
    }
    
    pim_vif->accept_nohello_neighbors().set(accept_nohello_neighbors);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_accept_nohello_neighbors(const string& vif_name,
					    string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Accept nohello neighbors flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->accept_nohello_neighbors().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_vif_join_prune_period(const string& vif_name,
				   uint16_t& join_prune_period,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get Join/Prune period for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    join_prune_period = pim_vif->join_prune_period().get();
    
    return (XORP_OK);
}

int
PimNode::set_vif_join_prune_period(const string& vif_name,
				   uint16_t join_prune_period,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Join/Prune period for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->join_prune_period().set(join_prune_period);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_join_prune_period(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Join/Prune period for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->join_prune_period().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::get_switch_to_spt_threshold(bool& is_enabled,
				     uint32_t& interval_sec,
				     uint32_t& bytes,
				     string& error_msg)
{
    UNUSED(error_msg);
    
    is_enabled = is_switch_to_spt_enabled().get();
    interval_sec = switch_to_spt_threshold_interval_sec().get();
    bytes = switch_to_spt_threshold_bytes().get();
    
    return (XORP_OK);
}

int
PimNode::set_switch_to_spt_threshold(bool is_enabled,
				     uint32_t interval_sec,
				     uint32_t bytes,
				     string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if ((is_switch_to_spt_enabled().get() != is_enabled)
	|| (switch_to_spt_threshold_interval_sec().get() != interval_sec)
	|| (switch_to_spt_threshold_bytes().get() != bytes)) {
	is_switch_to_spt_enabled().set(is_enabled);
	switch_to_spt_threshold_interval_sec().set(interval_sec);
	switch_to_spt_threshold_bytes().set(bytes);
	
	// Add the task to update the SPT-switch threshold
	pim_mrt().add_task_spt_switch_threshold_changed();
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::reset_switch_to_spt_threshold(string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    do {
	bool is_enabled = is_switch_to_spt_enabled().get();
	uint32_t interval_sec = switch_to_spt_threshold_interval_sec().get();
	uint32_t bytes = switch_to_spt_threshold_bytes().get();
	
	// Reset the values
	is_switch_to_spt_enabled().reset();
	switch_to_spt_threshold_interval_sec().reset();
	switch_to_spt_threshold_bytes().reset();
	
	if ((is_switch_to_spt_enabled().get() != is_enabled)
	    || (switch_to_spt_threshold_interval_sec().get() != interval_sec)
	    || (switch_to_spt_threshold_bytes().get() != bytes)) {
	    
	    // Add the task to update the SPT-switch threshold
	    pim_mrt().add_task_spt_switch_threshold_changed();
	}
    } while (false);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::add_config_scope_zone_by_vif_name(const IPvXNet& scope_zone_id,
					   const string& vif_name,
					   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot add configure scope zone with vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_scope_zone_table().add_scope_zone(scope_zone_id, pim_vif->vif_index());
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::add_config_scope_zone_by_vif_addr(const IPvXNet& scope_zone_id,
					   const IPvX& vif_addr,
					   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_addr(vif_addr);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot add configure scope zone with vif address %s: "
			     "no such vif",
			     cstring(vif_addr));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_scope_zone_table().add_scope_zone(scope_zone_id, pim_vif->vif_index());
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::delete_config_scope_zone_by_vif_name(const IPvXNet& scope_zone_id,
					      const string& vif_name,
					      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure scope zone with vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_scope_zone_table().delete_scope_zone(scope_zone_id,
					     pim_vif->vif_index());
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNode::delete_config_scope_zone_by_vif_addr(const IPvXNet& scope_zone_id,
					      const IPvX& vif_addr,
					      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_addr(vif_addr);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure scope zone with vif address %s: "
			     "no such vif",
			     cstring(vif_addr));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    pim_scope_zone_table().delete_scope_zone(scope_zone_id, pim_vif->vif_index());
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Add myself as a Cand-BSR
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_bsr(const IPvXNet& scope_zone_id,
			     bool is_scope_zone,
			     const string& vif_name,
			     const IPvX& vif_addr,
			     uint8_t bsr_priority,
			     uint8_t hash_mask_len,
			     string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    IPvX my_cand_bsr_addr = vif_addr;
    uint16_t fragment_tag = xorp_random() % 0xffff;
    string local_error_msg = "";
    PimScopeZoneId zone_id(scope_zone_id, is_scope_zone);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot add configure BSR with vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (my_cand_bsr_addr == IPvX::ZERO(family())) {
	// Use the domain-wide address for the vif
	if (pim_vif->domain_wide_addr() == IPvX::ZERO(family())) {
	    end_config(error_msg);
	    error_msg = c_format("Cannot add configure BSR with vif %s: "
				 "the vif has no configured address",
				 vif_name.c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);	// The vif has no address yet
	}
	my_cand_bsr_addr = pim_vif->domain_wide_addr();
    } else {
	// Test that the specified address belongs to the vif
	if (! pim_vif->is_my_addr(my_cand_bsr_addr)) {
	    end_config(error_msg);
	    error_msg = c_format("Cannot add configure BSR with vif %s "
				 "and address %s: "
				 "the address does not belong to this vif",
				 vif_name.c_str(),
				 cstring(my_cand_bsr_addr));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);	// Invalid address
	}
    }

    // XXX: if hash_mask_len is 0, then set its value to default
    if (hash_mask_len == 0)
	hash_mask_len = PIM_BOOTSTRAP_HASH_MASK_LEN_DEFAULT(family());
    
    BsrZone new_bsr_zone(pim_bsr(), my_cand_bsr_addr, bsr_priority,
			 hash_mask_len, fragment_tag);
    new_bsr_zone.set_zone_id(zone_id);
    new_bsr_zone.set_i_am_candidate_bsr(true, pim_vif->vif_index(),
					my_cand_bsr_addr, bsr_priority);
    if (vif_addr != IPvX::ZERO(family()))
	new_bsr_zone.set_is_my_bsr_addr_explicit(true);
    
    if (pim_bsr().add_config_bsr_zone(new_bsr_zone, local_error_msg) == NULL) {
	string dummy_error_msg;
	end_config(dummy_error_msg);
	error_msg = c_format("Cannot add configure BSR with vif %s address %s "
			     "for zone %s: %s",
			     vif_name.c_str(),
			     cstring(my_cand_bsr_addr),
			     cstring(zone_id),
			     local_error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::delete_config_cand_bsr(const IPvXNet& scope_zone_id,
				bool is_scope_zone,
				string& error_msg)
{
    BsrZone *bsr_zone = NULL;
    bool is_up = false;
    PimScopeZoneId zone_id(scope_zone_id, is_scope_zone);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    //
    // Find the BSR zone
    //
    bsr_zone = pim_bsr().find_config_bsr_zone(zone_id);
    if (bsr_zone == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure BSR for zone %s: "
			     "zone not found",
			     cstring(zone_id));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Stop the BSR, delete the BSR zone, and restart the BSR if necessary
    //
    is_up = pim_bsr().is_up();
    pim_bsr().stop();
    
    if (bsr_zone->bsr_group_prefix_list().empty()) {
	// No Cand-RP, therefore delete the zone.
	pim_bsr().delete_config_bsr_zone(bsr_zone);
    } else {
	// There is Cand-RP configuration, therefore only reset the Cand-BSR
	// configuration.
	bsr_zone->set_i_am_candidate_bsr(false, Vif::VIF_INDEX_INVALID,
					 IPvX::ZERO(family()), 0);
    }
    
    if (is_up)
	pim_bsr().start();	// XXX: restart the BSR
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Add myself as a Cand-RP
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_rp(const IPvXNet& group_prefix,
			    bool is_scope_zone,
			    const string& vif_name,
			    const IPvX& vif_addr,
			    uint8_t rp_priority,
			    uint16_t rp_holdtime,
			    string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    IPvX my_cand_rp_addr = vif_addr;
    BsrZone *config_bsr_zone = NULL;
    BsrRp *bsr_rp = NULL;
    string local_error_msg = "";
    bool is_new_zone = false;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot add configure Cand-RP with vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    if (my_cand_rp_addr == IPvX::ZERO(family())) {
	// Use the domain-wide address for the vif
	if (pim_vif->domain_wide_addr() == IPvX::ZERO(family())) {
	    end_config(error_msg);
	    error_msg = c_format("Cannot add configure Cand-RP with vif %s: "
				 "the vif has no configured address",
				 vif_name.c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);	// The vif has no address yet
	}
	my_cand_rp_addr = pim_vif->domain_wide_addr();
    } else {
	// Test that the specified address belongs to the vif
	if (! pim_vif->is_my_addr(my_cand_rp_addr)) {
	    string error_msg;
	    end_config(error_msg);
	    error_msg = c_format("Cannot add configure Cand-RP with vif %s "
				 "and address %s: "
				 "the address does not belong to this vif",
				 vif_name.c_str(),
				 cstring(my_cand_rp_addr));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);	// Invalid address
	}
    }
    
    config_bsr_zone = pim_bsr().find_config_bsr_zone_by_prefix(group_prefix,
							       is_scope_zone);
    
    if (config_bsr_zone == NULL) {
	PimScopeZoneId zone_id(group_prefix, is_scope_zone);
	
	if (! is_scope_zone) {
	    zone_id = PimScopeZoneId(IPvXNet::ip_multicast_base_prefix(family()),
				     is_scope_zone);
	}
	BsrZone new_bsr_zone(pim_bsr(), zone_id);
	config_bsr_zone = pim_bsr().add_config_bsr_zone(new_bsr_zone,
							local_error_msg);
	if (config_bsr_zone == NULL) {
	    string dummy_error_msg;
	    end_config(dummy_error_msg);
	    error_msg = c_format("Cannot add configure Cand-RP for "
				 "zone group prefix %s (%s): %s",
				 cstring(group_prefix),
				 (is_scope_zone)? "scoped" : "non-scoped",
				 local_error_msg.c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	is_new_zone = true;
    }
    
    bsr_rp = config_bsr_zone->add_rp(group_prefix, is_scope_zone,
				     my_cand_rp_addr, rp_priority,
				     rp_holdtime, local_error_msg);
    if (bsr_rp == NULL) {
	string dummy_error_msg;
	end_config(dummy_error_msg);
	error_msg = c_format("Cannot add configure Cand-RP address %s for "
			     "zone group prefix %s (%s): %s",
			     cstring(my_cand_rp_addr),
			     cstring(group_prefix),
			     (is_scope_zone)? "scoped" : "non-scoped",
			     local_error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	if (is_new_zone)
	    pim_bsr().delete_config_bsr_zone(config_bsr_zone);
	return (XORP_ERROR);
    }
    bsr_rp->set_my_vif_index(pim_vif->vif_index());
    if (vif_addr != IPvX::ZERO(family()))
	bsr_rp->set_is_my_rp_addr_explicit(true);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Delete myself as a Cand-RP
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::delete_config_cand_rp(const IPvXNet& group_prefix,
			       bool is_scope_zone,
			       const string& vif_name,
			       const IPvX& vif_addr,
			       string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    IPvX my_cand_rp_addr = vif_addr;
    BsrZone *bsr_zone = NULL;
    BsrGroupPrefix *bsr_group_prefix = NULL;
    BsrRp *bsr_rp = NULL;
    bool is_up = false;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (pim_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure Cand-RP with vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    if (my_cand_rp_addr == IPvX::ZERO(family())) {
	// Use the domain-wide address for the vif
	if (pim_vif->domain_wide_addr() == IPvX::ZERO(family())) {
	    end_config(error_msg);
	    error_msg = c_format("Cannot delete configure Cand-RP with vif %s: "
				 "the vif has no configured address",
				 vif_name.c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);	// The vif has no address yet
	}
	my_cand_rp_addr = pim_vif->domain_wide_addr();
    } else {
	//
	// XXX: don't test that the specified address belongs to the vif
	// because the vif may have been reconfigured already and the
	// address may have been deleted.
	//
    }
    
    //
    // Find the BSR zone
    //
    bsr_zone = pim_bsr().find_config_bsr_zone_by_prefix(group_prefix,
							is_scope_zone);
    if (bsr_zone == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure Cand-RP for zone for "
			     "group prefix %s (%s): zone not found",
			     cstring(group_prefix),
			     (is_scope_zone)? "scoped" : "non-scoped");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Find the BSR group prefix
    //
    bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
    if (bsr_group_prefix == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure Cand-RP for zone for "
			     "group prefix %s (%s): prefix not found",
			     cstring(group_prefix),
			     (is_scope_zone)? "scoped" : "non-scoped");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Find the RP
    //
    bsr_rp = bsr_group_prefix->find_rp(my_cand_rp_addr);
    if (bsr_rp == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot delete configure Cand-RP for zone for "
			     "group prefix %s (%s) and RP %s: RP not found",
			     cstring(group_prefix),
			     (is_scope_zone)? "scoped" : "non-scoped",
			     cstring(my_cand_rp_addr));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Stop the BSR, delete the RP zone, and restart the BSR if necessary
    //
    is_up = pim_bsr().is_up();
    pim_bsr().stop();
    
    bsr_group_prefix->delete_rp(bsr_rp);
    bsr_rp = NULL;
    // Delete the BSR group prefix if not needed anymore
    if (bsr_group_prefix->rp_list().empty()) {
	bsr_zone->delete_bsr_group_prefix(bsr_group_prefix);
	bsr_group_prefix = NULL;
    }
    if (bsr_zone->bsr_group_prefix_list().empty()
	&& (! bsr_zone->i_am_candidate_bsr())) {
	// No Cand-RP, and no Cand-BSR, therefore delete the zone.
	pim_bsr().delete_config_bsr_zone(bsr_zone);
	bsr_zone = NULL;
    }
    
    if (is_up)
	pim_bsr().start();	// XXX: restart the BSR
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Add a statically configured RP
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
// XXX: we don't call end_config(), because config_static_rp_done() will
// do it when the RP configuration is completed.
//
int
PimNode::add_config_static_rp(const IPvXNet& group_prefix,
			      const IPvX& rp_addr,
			      uint8_t rp_priority,
			      uint8_t hash_mask_len,
			      string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (! group_prefix.is_multicast()) {
	// XXX: don't call end_config(error_msg);
	error_msg = c_format("Cannot add configure static RP with address %s "
			     "for group prefix %s: "
			     "not a multicast address",
			     cstring(rp_addr),
			     cstring(group_prefix));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (! rp_addr.is_unicast()) {
	// XXX: don't call end_config(error_msg);
	error_msg = c_format("Cannot add configure static RP with address %s: "
			     "not an unicast address",
			     cstring(rp_addr));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // XXX: if hash_mask_len is 0, then set its value to default
    if (hash_mask_len == 0)
	hash_mask_len = PIM_BOOTSTRAP_HASH_MASK_LEN_DEFAULT(family());
    
    if (rp_table().add_rp(rp_addr, rp_priority, group_prefix, hash_mask_len,
			  PimRp::RP_LEARNED_METHOD_STATIC)
	== NULL) {
	// XXX: don't call end_config(error_msg);
	error_msg = c_format("Cannot add configure static RP with address %s "
			     "and priority %d for group prefix %s",
			     cstring(rp_addr),
			     rp_priority,
			     cstring(group_prefix));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // XXX: config_static_rp_done() will complete the configuration setup
    
    return (XORP_OK);
}

//
// Delete a statically configured RP
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
// XXX: we don't call end_config(), because config_static_rp_done() will
// do it when the RP configuration is completed.
//
int
PimNode::delete_config_static_rp(const IPvXNet& group_prefix,
				 const IPvX& rp_addr,
				 string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (rp_table().delete_rp(rp_addr, group_prefix,
			     PimRp::RP_LEARNED_METHOD_STATIC)
	!= XORP_OK) {
	// XXX: don't call end_config(error_msg);
	error_msg = c_format("Cannot delete configure static RP with address %s "
			     "for group prefix %s",
			     cstring(rp_addr),
			     cstring(group_prefix));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // XXX: config_static_rp_done() will complete the configuration setup
    
    return (XORP_OK);
}

//
// Delete a statically configured RP for all group prefixes
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
// XXX: we don't call end_config(), because config_static_rp_done() will
// do it when the RP configuration is completed.
//
int
PimNode::delete_config_all_static_group_prefixes_rp(const IPvX& rp_addr,
						    string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (rp_table().delete_all_group_prefixes_rp(rp_addr,
						PimRp::RP_LEARNED_METHOD_STATIC)
	!= XORP_OK) {
	// XXX: don't call end_config(error_msg);
	error_msg = c_format("Cannot delete configure static RP with address %s",
			     cstring(rp_addr));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // XXX: config_static_rp_done() will complete the configuration setup
    
    return (XORP_OK);
}

//
// Delete all statically configured RPs
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
// XXX: we don't call end_config(), because config_static_rp_done() will
// do it when the RP configuration is completed.
//
int
PimNode::delete_config_all_static_rps(string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (rp_table().delete_all_rps(PimRp::RP_LEARNED_METHOD_STATIC)
	!= XORP_OK) {
	// XXX: don't call end_config(error_msg);
	error_msg = c_format("Cannot delete configure all static RPs");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // XXX: config_static_rp_done() will complete the configuration setup
    
    return (XORP_OK);
}

//
// Finish with the static RP configuration.
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::config_static_rp_done(string& error_msg)
{
    rp_table().apply_rp_changes();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Add an alternative subnet on a PIM vif.
//
// An alternative subnet is used to make incoming traffic with a non-local
// source address appear as it is coming from a local subnet.
// Note: add alternative subnets with extreme care, only if you know what
// you are really doing!
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_alternative_subnet(const string& vif_name,
				const IPvXNet& subnet,
				string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot add alternative subnet to vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->add_alternative_subnet(subnet);
    
    return (XORP_OK);
}

//
// Delete an alternative subnet on a PIM vif.
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::delete_alternative_subnet(const string& vif_name,
				   const IPvXNet& subnet,
				   string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot delete alternative subnet from vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->delete_alternative_subnet(subnet);
    
    return (XORP_OK);
}

//
// Remove all alternative subnets on a PIM vif.
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::remove_all_alternative_subnets(const string& vif_name,
					string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot remove all alternative subnets from vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->remove_all_alternative_subnets();
    
    return (XORP_OK);
}

//
// Add a J/P entry to the _test_jp_headers_list
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_test_jp_entry(const IPvX& source_addr, const IPvX& group_addr,
			   uint8_t group_mask_len,
			   mrt_entry_type_t mrt_entry_type,
			   action_jp_t action_jp, uint16_t holdtime,
			   bool is_new_group)
{
    int ret_value;
    
    if (_test_jp_headers_list.empty() || is_new_group)
	_test_jp_headers_list.push_back(PimJpHeader(this));

    PimJpHeader& pim_jp_header = _test_jp_headers_list.back();
    ret_value = pim_jp_header.jp_entry_add(source_addr, group_addr,
					   group_mask_len, mrt_entry_type,
					   action_jp, holdtime,
					   is_new_group);
    
    return (ret_value);
}

//
// Send the accumulated state in the _test_jp_headers_list
// XXX: the _test_jp_headers_list entries are reset by the sending method
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::send_test_jp_entry(const string& vif_name, const IPvX& nbr_addr,
			    string& error_msg)
{
    int ret_value = XORP_OK;
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);

    list<PimJpHeader>::iterator iter;
    for (iter = _test_jp_headers_list.begin();
	 iter != _test_jp_headers_list.end();
	 ++iter) {
	PimJpHeader& pim_jp_header = *iter;
	if (pim_jp_header.network_commit(pim_vif, nbr_addr, error_msg)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    break;
	}
    }
    _test_jp_headers_list.clear();
    
    return (ret_value);
}

//
// Send test Assert message on an interface.
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::send_test_assert(const string& vif_name,
			  const IPvX& source_addr,
			  const IPvX& group_addr,
			  bool rpt_bit,
			  uint32_t metric_preference,
			  uint32_t metric,
			  string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot send Test-Assert on vif %s: no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->pim_assert_send(source_addr,
				 group_addr,
				 rpt_bit,
				 metric_preference,
				 metric,
				 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::add_test_bsr_zone(const PimScopeZoneId& zone_id,
			   const IPvX& bsr_addr,
			   uint8_t bsr_priority,
			   uint8_t hash_mask_len,
			   uint16_t fragment_tag)
{
    if (pim_bsr().add_test_bsr_zone(zone_id, bsr_addr, bsr_priority,
				    hash_mask_len, fragment_tag)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::add_test_bsr_group_prefix(const PimScopeZoneId& zone_id,
				   const IPvXNet& group_prefix,
				   bool is_scope_zone,
				   uint8_t expected_rp_count)
{
    if (pim_bsr().add_test_bsr_group_prefix(zone_id, group_prefix,
					    is_scope_zone, expected_rp_count)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::add_test_bsr_rp(const PimScopeZoneId& zone_id,
			 const IPvXNet& group_prefix,
			 const IPvX& rp_addr,
			 uint8_t rp_priority,
			 uint16_t rp_holdtime)
{
    if (pim_bsr().add_test_bsr_rp(zone_id, group_prefix, rp_addr, rp_priority,
				  rp_holdtime)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::send_test_bootstrap(const string& vif_name, string& error_msg)
{
    if (pim_bsr().send_test_bootstrap(vif_name, error_msg) != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::send_test_bootstrap_by_dest(const string& vif_name,
				     const IPvX& dest_addr,
				     string& error_msg)
{
    if (pim_bsr().send_test_bootstrap_by_dest(vif_name, dest_addr,
					      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::send_test_cand_rp_adv()
{
    if (pim_bsr().send_test_cand_rp_adv() != XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}
