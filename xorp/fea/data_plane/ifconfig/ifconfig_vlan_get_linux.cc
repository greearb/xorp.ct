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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#ifdef HAVE_LINUX_IF_VLAN_H
#include <linux/if_vlan.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_vlan_get_linux.hh"


//
// Get VLAN information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is Linux-specific ioctl(2).
//

#ifdef HAVE_VLAN_LINUX

IfConfigVlanGetLinux::IfConfigVlanGetLinux(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigVlanGet(fea_data_plane_manager),
      _s4(-1)
{
}

IfConfigVlanGetLinux::~IfConfigVlanGetLinux()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Linux-specific ioctl(2) mechanism to get "
		   "information about VLAN network interfaces from the "
		   "underlying system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigVlanGetLinux::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (_s4 < 0) {
	_s4 = socket(AF_INET, SOCK_DGRAM, 0);
	if (_s4 < 0) {
	    error_msg = c_format("Could not initialize IPv4 ioctl() "
				 "socket: %s", strerror(errno));
	    XLOG_FATAL("%s", error_msg.c_str());
	}
    }

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigVlanGetLinux::stop(string& error_msg)
{
    int ret_value4 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	ret_value4 = comm_close(_s4);
	_s4 = -1;
	if (ret_value4 != XORP_OK) {
	    error_msg = c_format("Could not close IPv4 ioctl() socket: %s",
				 comm_get_last_error_str());
	}
    }

    if (ret_value4 != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigVlanGetLinux::pull_config(IfTree& iftree, bool& modified)
{
    return read_config(iftree, modified);
}

int
IfConfigVlanGetLinux::read_config(IfTree& iftree, bool& modified)
{
    IfTree::IfMap::iterator ii;
    string error_msg;
    bool mod_on_entry = modified;

    if (! _is_running) {
	error_msg = c_format("Cannot read VLAN interface intormation: "
			     "the IfConfigVlanGetLinux plugin is not running");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    XLOG_ASSERT(_s4 >= 0);

    //
    // Check all interfaces whether they are actually VLANs.
    // If an interface is found to be a VLAN, then its vif state is
    // copied as a vif child to its physical parent interface.
    // Note that we keep the original VLAN interface state in the IfTree
    // so the rest of the FEA is not surprised if that state suddenly is
    // deleted.
    //
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface* ifp = ii->second;
	// Ignore interfaces that are to be deleted
	if (ifp->is_marked(IfTreeItem::DELETED))
	    continue;

	// If the iface list was modified on entry (ie, we added/deleted an ifp somewhere
	// when reading iface config, then it's possible a parent has appeared and so we must
	// reprobe everything just in case.
	if (mod_on_entry) {
	    ifp->set_probed_vlan(false);
	}

	// If we've already probed this device for vlan-ness, then
	// no need to probe again I think.
	if (ifp->probed_vlan()) {
	    continue;
	}

	/** we'll have probed it when we return. */
	ifp->set_probed_vlan(true);

	struct vlan_ioctl_args vlanreq;

	// Test whether a VLAN interface
	memset(&vlanreq, 0, sizeof(vlanreq));
	strncpy(vlanreq.device1, ifp->ifname().c_str(),
		sizeof(vlanreq.device1) - 1);
	vlanreq.cmd = GET_VLAN_REALDEV_NAME_CMD;
	if (ioctl(_s4, SIOCGIFVLAN, &vlanreq) < 0) {
	    continue; // not a vlan
	}

	//
	// XXX: VLAN interface
	//

	// Get the parent device
	string parent_ifname = vlanreq.u.device2;
	if (parent_ifname.empty()) {
	    // BUG
	    XLOG_ERROR("Could not find parent ifname for iface: %s\n", ifp->ifname().c_str());
	    continue;
	}

	// Get the VLAN ID
	memset(&vlanreq, 0, sizeof(vlanreq));
	strncpy(vlanreq.device1, ifp->ifname().c_str(),
		sizeof(vlanreq.device1) - 1);
	vlanreq.cmd = GET_VLAN_VID_CMD;
	if (ioctl(_s4, SIOCGIFVLAN, &vlanreq) < 0) {
	    error_msg = c_format("Cannot get the VLAN ID for interface %s: %s",
				 ifp->ifname().c_str(), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    continue;
	}
	uint16_t vlan_id = vlanreq.u.VID;

	// NOTE:  It's possible for VLANs to be configured and not
	// have the parent device configured (or even visible at all, when linux namespace
	// code is completed.)  Since VLANs are 'real' devices in Linux, this might not really
	// matter if Xorp doesn't notice... --Ben
	IfTreeInterface* parent_ifp = iftree.find_interface(parent_ifname);
	IfTreeVif* parent_vifp = NULL;
	if ((parent_ifp == NULL) || parent_ifp->is_marked(IfTreeItem::DELETED)) {
	    // It could appear later, go set the vifp if we can find it
	}
	else {
	    // Find or add the VLAN vif
	    parent_vifp = parent_ifp->find_vif(ifp->ifname());
	    if (parent_vifp == NULL) {
		modified = true;
		parent_ifp->add_vif(ifp->ifname());
		parent_vifp = parent_ifp->find_vif(ifp->ifname());
	    }
	    XLOG_ASSERT(parent_vifp != NULL);
	}

	// Copy the vif state
	IfTreeVif* vifp = ifp->find_vif(ifp->ifname());
	if (vifp != NULL) {
	    if (parent_vifp) {
		modified = true; /* TODO: this could be a lie, but better safe than sorry until
				  * the copy-recursive method takes 'modified'.
				  */
		parent_vifp->copy_recursive_vif(*vifp);
	    }
	    else {
		// Well, lie and set the info in the vifp instead, since it's actually
		// a vlan.
		parent_vifp = vifp;
	    }
	}
	if (parent_vifp) {
	    // Set the VLAN vif info
	    if (!parent_vifp->is_vlan()) {
		parent_vifp->set_vlan(true);
		modified = true;
	    }
	    if (parent_vifp->vlan_id() != vlan_id) {
		parent_vifp->set_vlan_id(vlan_id);
		modified = true;
	    }
	}
    }

    return (XORP_OK);
}

#endif // HAVE_VLAN_LINUX
