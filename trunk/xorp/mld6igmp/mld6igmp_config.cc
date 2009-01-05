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

#ident "$XORP: xorp/mld6igmp/mld6igmp_config.cc,v 1.22 2008/10/02 21:57:43 bms Exp $"


//
// TODO: a temporary solution for various MLD6IGMP configuration
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mld6igmp_node.hh"
#include "mld6igmp_vif.hh"


int
Mld6igmpNode::set_config_all_vifs_done(string& error_msg)
{
    map<string, Vif>::iterator vif_iter;
    map<string, Vif>& configured_vifs = ProtoNode<Mld6igmpVif>::configured_vifs();
    string dummy_error_msg;

    //
    // Add new vifs and update existing ones
    //
    for (vif_iter = configured_vifs.begin();
	 vif_iter != configured_vifs.end();
	 ++vif_iter) {
	Vif* vif = &vif_iter->second;
	Vif* node_vif = vif_find_by_name(vif->name());

	if (vif->is_pim_register())
	    continue;	// XXX: don't add the PIM Register vifs

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
		      vif->mtu(),
		      dummy_error_msg);
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

	if (vif->is_pim_register())
	    continue;	// XXX: don't add the PIM Register vifs

	if (node_vif == NULL)
	    continue;
	
	for (vif_addr_iter = vif->addr_list().begin();
	     vif_addr_iter != vif->addr_list().end();
	     ++vif_addr_iter) {
	    const VifAddr& vif_addr = *vif_addr_iter;
	    add_vif_addr(vif->name(), vif_addr.addr(),
			 vif_addr.subnet_addr(),
			 vif_addr.broadcast_addr(),
			 vif_addr.peer_addr(),
			 dummy_error_msg);
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
		delete_vif_addr(vif->name(), ipvx, dummy_error_msg);
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

int
Mld6igmpNode::get_vif_proto_version(const string& vif_name, int& proto_version,
				    string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot get protocol version for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    proto_version = mld6igmp_vif->proto_version();
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_proto_version(const string& vif_name, int proto_version,
				    string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);

    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set protocol version for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->set_proto_version(proto_version) != XORP_OK) {
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
Mld6igmpNode::reset_vif_proto_version(const string& vif_name,
				      string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset protocol version for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->set_proto_version(mld6igmp_vif->proto_version_default());
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::get_vif_ip_router_alert_option_check(const string& vif_name,
						   bool& enabled,
						   string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot get 'IP Router Alert option check' "
			     "flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    enabled = mld6igmp_vif->ip_router_alert_option_check().get();
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_ip_router_alert_option_check(const string& vif_name,
						   bool enable,
						   string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);

    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set 'IP Router Alert option check' "
			     "flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->ip_router_alert_option_check().set(enable);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::reset_vif_ip_router_alert_option_check(const string& vif_name,
						     string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset 'IP Router Alert option check' "
			     "flag for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->ip_router_alert_option_check().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::get_vif_query_interval(const string& vif_name,
				     TimeVal& interval,
				     string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot get Query Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    interval = mld6igmp_vif->configured_query_interval().get();
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_query_interval(const string& vif_name,
				     const TimeVal& interval,
				     string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);

    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Query Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->configured_query_interval().set(interval);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::reset_vif_query_interval(const string& vif_name,
				       string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Query Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->configured_query_interval().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::get_vif_query_last_member_interval(const string& vif_name,
						 TimeVal& interval,
						 string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot get Last Member Query Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    interval = mld6igmp_vif->query_last_member_interval().get();
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_query_last_member_interval(const string& vif_name,
						 const TimeVal& interval,
						 string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);

    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Last Member Query Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->query_last_member_interval().set(interval);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::reset_vif_query_last_member_interval(const string& vif_name,
						   string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Last Member Query Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->query_last_member_interval().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::get_vif_query_response_interval(const string& vif_name,
					      TimeVal& interval,
					      string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot get Query Response Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    interval = mld6igmp_vif->query_response_interval().get();
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_query_response_interval(const string& vif_name,
					      const TimeVal& interval,
					      string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);

    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Query Response Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->query_response_interval().set(interval);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::reset_vif_query_response_interval(const string& vif_name,
						string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Query Response Interval for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->query_response_interval().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::get_vif_robust_count(const string& vif_name,
				   uint32_t& robust_count,
				   string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL) {
	error_msg = c_format("Cannot get Robustness Variable count for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    robust_count = mld6igmp_vif->configured_robust_count().get();
    
    return (XORP_OK);
}

int
Mld6igmpNode::set_vif_robust_count(const string& vif_name,
				   uint32_t robust_count,
				   string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);

    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot set Robustness Variable count for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->configured_robust_count().set(robust_count);
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::reset_vif_robust_count(const string& vif_name,
				     string& error_msg)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (mld6igmp_vif == NULL) {
	end_config(error_msg);
	error_msg = c_format("Cannot reset Robustness Variable count for vif %s: "
			     "no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->configured_robust_count().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}
