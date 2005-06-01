// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_config.cc,v 1.5 2005/03/25 02:53:54 pavlin Exp $"


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
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (mld6igmp_vif->set_proto_version(proto_version) < 0) {
	end_config(error_msg);
        error_msg = c_format("Cannot set protocol version for vif %s: "
			     "invalid protocol version %d",
			     vif_name.c_str(), proto_version);
	XLOG_ERROR(error_msg.c_str());
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
	XLOG_ERROR(error_msg.c_str());
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
	XLOG_ERROR(error_msg.c_str());
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
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    mld6igmp_vif->ip_router_alert_option_check().reset();
    
    if (end_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}
