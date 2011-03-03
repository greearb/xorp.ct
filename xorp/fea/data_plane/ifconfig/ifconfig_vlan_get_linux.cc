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

IfConfigVlanGetLinux::IfConfigVlanGetLinux(FeaDataPlaneManager& fea_data_plane_manager,
					   bool is_dummy)
    : IfConfigVlanGet(fea_data_plane_manager),
      _is_dummy(is_dummy),
      _s4(-1)
{
}

IfConfigVlanGetLinux::~IfConfigVlanGetLinux()
{
    if (_is_dummy) {
	return;
    }
#if defined(HAVE_VLAN_LINUX) or defined(HAVE_VLAN_BSD)
    else {
	string error_msg;

	if (stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot stop the ioctl(2) mechanism to get "
		       "information about VLAN network interfaces from the "
		       "underlying system: %s",
		       error_msg.c_str());
	}
    }
#endif
}

int
IfConfigVlanGetLinux::start(string& error_msg)
{
    if (_is_dummy)
	_is_running = true;

    if (_is_running)
	return (XORP_OK);

#if defined(HAVE_VLAN_LINUX) or defined(HAVE_VLAN_BSD)

    if (_s4 < 0) {
	_s4 = socket(AF_INET, SOCK_DGRAM, 0);
	if (_s4 < 0) {
	    error_msg = c_format("Could not initialize IPv4 ioctl() "
				 "socket: %s", strerror(errno));
	    XLOG_FATAL("%s", error_msg.c_str());
	}
    }
#endif

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigVlanGetLinux::stop(string& error_msg)
{
    int ret_value4 = XORP_OK;

    if (_is_dummy)
	_is_running = false;

    if (! _is_running)
	return (XORP_OK);

#if defined(HAVE_VLAN_LINUX) or defined(HAVE_VLAN_BSD)
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
#endif

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
    if (_is_dummy)
	return XORP_OK;

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

	uint16_t vlan_id;
	string parent_ifname;

#ifdef HAVE_VLAN_BSD
	struct ifreq ifreq;
	struct vlanreq vlanreq;

	// Test whether a VLAN interface
	memset(&ifreq, 0, sizeof(ifreq));
	memset(&vlanreq, 0, sizeof(vlanreq));
	strncpy(ifreq.ifr_name, ifp->ifname().c_str(),
		sizeof(ifreq.ifr_name) - 1);
	ifreq.ifr_data = reinterpret_cast<caddr_t>(&vlanreq);
	if (ioctl(_s4, SIOCGETVLAN, (caddr_t)&ifreq) < 0)
	    continue;		// XXX: Most likely not a VLAN interface

	// Get the VLAN information
	vlan_id = vlanreq.vlr_tag;
	parent_ifname = vlanreq.vlr_parent;

	if (parent_ifname.empty())
	    continue;

#elif defined(HAVE_VLAN_LINUX)
	struct vlan_ioctl_args vlanreq;

	// Test whether a VLAN interface
	memset(&vlanreq, 0, sizeof(vlanreq));
	strncpy(vlanreq.device1, ifp->ifname().c_str(),
		sizeof(vlanreq.device1) - 1);
	vlanreq.cmd = GET_VLAN_REALDEV_NAME_CMD;
	if (ioctl(_s4, SIOCGIFVLAN, &vlanreq) < 0) {
	    continue; // not a vlan
	}

	// Get the parent device
	parent_ifname = vlanreq.u.device2;
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
	vlan_id = vlanreq.u.VID;
#endif

	IfTreeVif* vifp = ifp->find_vif(ifp->ifname());
	if (vifp == NULL) {
	    ifp->add_vif(ifp->ifname());
	    modified = true;
	}

	if (ifp->parent_ifname() != parent_ifname) {
	    modified = true;
	    ifp->set_parent_ifname(parent_ifname);
	}
	// TODO:  Use a #define or similar for 'VLAN'
	string vl("VLAN");
	if (ifp->iface_type() != vl) {
	    modified = true;
	    ifp->set_iface_type(vl);
	}
	string vid = c_format("%hu", vlan_id);
	if (ifp->vid() != vid) {
	    modified = true;
	    ifp->set_vid(vid);
	}
    }

    return XORP_OK;
}
