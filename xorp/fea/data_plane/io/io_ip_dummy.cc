// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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
// I/O IP raw communication support.
//
// The mechanism is Dummy (for testing purpose).
//

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"
#include "libxorp/utils.hh"

#include "fea/iftree.hh"

#include "io_ip_dummy.hh"


//
// Local constants definitions
//
#ifndef MINTTL
#define MINTTL		1
#endif
#ifndef IPDEFTTL
#define IPDEFTTL	64
#endif
#ifndef MAXTTL
#define MAXTTL		255
#endif


IoIpDummy::IoIpDummy(FeaDataPlaneManager& fea_data_plane_manager,
		     const IfTree& iftree, int family, uint8_t ip_protocol)
    : IoIp(fea_data_plane_manager, iftree, family, ip_protocol),
      _multicast_ttl(MINTTL),
      _multicast_loopback(true)
{
}

IoIpDummy::~IoIpDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy I/O IP raw communication mechanism: %s",
		   error_msg.c_str());
    }
}

int
IoIpDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IoIpDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IoIpDummy::set_multicast_ttl(int ttl, string& error_msg)
{
    UNUSED(error_msg);

    _multicast_ttl = ttl;

    return (XORP_OK);
}

int
IoIpDummy::enable_multicast_loopback(bool is_enabled, string& error_msg)
{
    UNUSED(error_msg);

    _multicast_loopback = is_enabled;

    return (XORP_OK);
}

int
IoIpDummy::set_default_multicast_interface(const string& if_name,
					   const string& vif_name,
					   string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Setting the default multicast interface failed:"
			     "interface %s vif %s not found",
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }

    _default_multicast_interface = if_name;
    _default_multicast_vif = vif_name;

    return (XORP_OK);
}

int
IoIpDummy::create_input_socket(const string& if_name,
				const string& vif_name,
				string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Creating input socket failed: "
			     "interface %s vif %s not found",
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

int
IoIpDummy::join_multicast_group(const string& if_name,
				const string& vif_name,
				const IPvX& group,
				string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Joining multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }

#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
	error_msg = c_format("Cannot join group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
#endif // 0/1

    // Add the group to the set of joined groups
    IoIpComm::JoinedMulticastGroup joined_group(if_name, vif_name, group);
    _joined_groups_table.insert(joined_group);

    return (XORP_OK);
}

int
IoIpDummy::leave_multicast_group(const string& if_name,
				 const string& vif_name,
				 const IPvX& group,
				 string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Leaving multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }

#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
#endif // 0/1

    // Remove the group from the set of joined groups
    set<IoIpComm::JoinedMulticastGroup>::iterator iter;
    IoIpComm::JoinedMulticastGroup joined_group(if_name, vif_name, group);
    iter = find(_joined_groups_table.begin(), _joined_groups_table.end(),
		joined_group);
    if (iter == _joined_groups_table.end()) {
	error_msg = c_format("Multicast group %s is not joined on "
			     "interface %s vif %s",
			     group.str().c_str(), if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    _joined_groups_table.erase(iter);

    return (XORP_OK);
}

int
IoIpDummy::send_packet(const string& if_name,
		       const string& vif_name,
		       const IPvX& src_address,
		       const IPvX& dst_address,
		       int32_t ip_ttl,
		       int32_t ip_tos,
		       bool ip_router_alert,
		       bool ip_internet_control,
		       const vector<uint8_t>& ext_headers_type,
		       const vector<vector<uint8_t> >& ext_headers_payload,
		       const vector<uint8_t>& payload,
		       string& error_msg)
{
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;

    XLOG_ASSERT(ext_headers_type.size() == ext_headers_payload.size());

    ifp = iftree().find_interface(if_name);
    if (ifp == NULL) {
	error_msg = c_format("No interface %s", if_name.c_str());
	return (XORP_ERROR);
    }
    vifp = ifp->find_vif(vif_name);
    if (vifp == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }
    if (! ifp->enabled()) {
	error_msg = c_format("Interface %s is down",
			     ifp->ifname().c_str());
	return (XORP_ERROR);
    }
    if (! vifp->enabled()) {
	error_msg = c_format("Interface %s vif %s is down",
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str());
	return (XORP_ERROR);
    }

    UNUSED(src_address);
    UNUSED(dst_address);
    UNUSED(ip_ttl);
    UNUSED(ip_tos);
    UNUSED(ip_router_alert);
    UNUSED(ip_internet_control);
    UNUSED(ext_headers_type);
    UNUSED(ext_headers_payload);
    UNUSED(payload);

    return (XORP_OK);
}
