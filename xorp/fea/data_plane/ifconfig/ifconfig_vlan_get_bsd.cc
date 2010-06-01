// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2010 XORP, Inc and Others
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
#ifdef HAVE_NET_IF_VLAN_VAR_H
#include <net/if_vlan_var.h>
#endif
#ifdef HAVE_NET_IF_VLANVAR_H
#include <net/if_vlanvar.h>
#endif
#ifdef HAVE_NET_VLAN_IF_VLAN_VAR_H
#include <net/vlan/if_vlan_var.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_vlan_get_bsd.hh"


//
// Get VLAN information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is BSD-specific ioctl(2).
//

#ifdef HAVE_VLAN_BSD

IfConfigVlanGetBsd::IfConfigVlanGetBsd(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigVlanGet(fea_data_plane_manager),
      _s4(-1)
{
}

IfConfigVlanGetBsd::~IfConfigVlanGetBsd()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the BSD-specific ioctl(2) mechanism to get "
		   "information about VLAN network interfaces from the "
		   "underlying system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigVlanGetBsd::start(string& error_msg)
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
IfConfigVlanGetBsd::stop(string& error_msg)
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
IfConfigVlanGetBsd::pull_config(IfTree& iftree, bool& modified)
{
    return read_config(iftree, modified);
}

int
IfConfigVlanGetBsd::read_config(IfTree& iftree, bool& modified)
{
    IfTree::IfMap::iterator ii;
    string error_msg;

    if (! _is_running) {
	error_msg = c_format("Cannot read VLAN interface intormation: "
			     "the IfConfigVlanGetBsd plugin is not running");
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

	//
	// XXX: VLAN interface
	//

	// Get the VLAN information
	uint16_t vlan_id = vlanreq.vlr_tag;
	string parent_ifname = vlanreq.vlr_parent;

	if (parent_ifname.empty())
	    continue;

	IfTreeInterface* parent_ifp = iftree.find_interface(parent_ifname);
	if ((parent_ifp == NULL) || parent_ifp->is_marked(IfTreeItem::DELETED))
	    continue;

	// Find or add the VLAN vif
	IfTreeVif* parent_vifp = parent_ifp->find_vif(ifp->ifname());
	if (parent_vifp == NULL) {
	    modified = true;
	    parent_ifp->add_vif(ifp->ifname());
	    parent_vifp = parent_ifp->find_vif(ifp->ifname());
	}
	XLOG_ASSERT(parent_vifp != NULL);

	// Copy the vif state
	IfTreeVif* vifp = ifp->find_vif(ifp->ifname());
	if (vifp != NULL) {
	    modified = true; /* TODO: this could be a lie, but better safe than sorry until
			      * the copy-recursive method takes 'modified'.
			      */
	    parent_vifp->copy_recursive_vif(*vifp);
	}

	// Set the VLAN vif info
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

    return (XORP_OK);
}

#endif // HAVE_VLAN_BSD
