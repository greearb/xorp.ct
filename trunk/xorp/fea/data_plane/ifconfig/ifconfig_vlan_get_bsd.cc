// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_vlan_get_bsd.cc,v 1.3 2007/09/15 05:10:22 pavlin Exp $"

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
	if (ret_value4 < 0) {
	    error_msg = c_format("Could not close IPv4 ioctl() socket: %s",
				 strerror(errno));
	}
    }

    if (ret_value4 < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigVlanGetBsd::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

int
IfConfigVlanGetBsd::read_config(IfTree& iftree)
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
	IfTreeInterface* ifp = &ii->second;
	// Ignore interfaces that are to be deleted
	if (ifp->is_marked(IfTreeItem::DELETED))
	    continue;

	struct ifreq ifreq;
	struct vlanreq vlanreq;

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
	    parent_ifp->add_vif(ifp->ifname());
	    parent_vifp = parent_ifp->find_vif(ifp->ifname());
	}
	XLOG_ASSERT(parent_vifp != NULL);

	// Copy the vif state
	IfTreeVif* vifp = ifp->find_vif(ifp->ifname());
	if (vifp != NULL) {
	    parent_vifp->copy_state(*vifp);
	    parent_vifp->ipv4addrs() = vifp->ipv4addrs();
	    parent_vifp->ipv6addrs() = vifp->ipv6addrs();
	}

	// Set the VLAN vif info
	parent_vifp->set_vlan(true);
	parent_vifp->set_vlan_id(vlan_id);
    }

    return (XORP_OK);
}

#endif // HAVE_VLAN_BSD
