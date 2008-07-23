// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/io/io_link_dummy.cc,v 1.2 2008/01/04 03:16:14 pavlin Exp $"

//
// I/O link raw communication support.
//
// The mechanism is Dummy.
//

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/mac.hh"

#include "fea/iftree.hh"

#include "io_link_dummy.hh"


IoLinkDummy::IoLinkDummy(FeaDataPlaneManager& fea_data_plane_manager,
			 const IfTree& iftree, const string& if_name,
			 const string& vif_name, uint16_t ether_type,
			 const string& filter_program)
    : IoLink(fea_data_plane_manager, iftree, if_name, vif_name, ether_type,
	     filter_program)
{
}

IoLinkDummy::~IoLinkDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy I/O Link raw communication mechanism: %s",
		   error_msg.c_str());
    }
}

int
IoLinkDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IoLinkDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IoLinkDummy::join_multicast_group(const Mac& group, string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name(), vif_name());
    if (vifp == NULL) {
	error_msg = c_format("Joining multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }

#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
	error_msg = c_format("Cannot join group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     cstring(group),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1

    // Add the group to the set of joined groups
    IoLinkComm::JoinedMulticastGroup joined_group(group);
    _joined_groups_table.insert(joined_group);

    return (XORP_OK);
}

int
IoLinkDummy::leave_multicast_group(const Mac& group, string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name(), vif_name());
    if (vifp == NULL) {
	error_msg = c_format("Leaving multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }

#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     cstring(group),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1

    // Remove the group from the set of joined groups
    set<IoLinkComm::JoinedMulticastGroup>::iterator iter;
    IoLinkComm::JoinedMulticastGroup joined_group(group);
    iter = find(_joined_groups_table.begin(), _joined_groups_table.end(),
		joined_group);
    if (iter == _joined_groups_table.end()) {
	error_msg = c_format("Multicast group %s is not joined on "
			     "interface %s vif %s",
			     group.str().c_str(), if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }
    _joined_groups_table.erase(iter);

    return (XORP_OK);
}

int
IoLinkDummy::send_packet(const Mac& src_address,
			const Mac& dst_address,
			uint16_t ether_type,
			const vector<uint8_t>& payload,
			string& error_msg)
{
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;

    //
    // Test whether the interface/vif is enabled
    //
    ifp = iftree().find_interface(if_name());
    if (ifp == NULL) {
	error_msg = c_format("No interface %s", if_name().c_str());
	return (XORP_ERROR);
    }
    vifp = ifp->find_vif(vif_name());
    if (vifp == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     if_name().c_str(), vif_name().c_str());
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
    UNUSED(ether_type);
    UNUSED(payload);

    return (XORP_OK);
}
