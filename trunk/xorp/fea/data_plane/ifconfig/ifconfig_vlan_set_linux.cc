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

#ident "$XORP$"

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

#include "ifconfig_vlan_set_linux.hh"


//
// Set VLAN information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is Linux-specific ioctl(2).
//

#ifdef HAVE_VLAN_LINUX

IfConfigVlanSetLinux::IfConfigVlanSetLinux(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigVlanSet(fea_data_plane_manager),
      _s4(-1)
{
}

IfConfigVlanSetLinux::~IfConfigVlanSetLinux()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Linux-specific ioctl(2) mechanism to set "
		   "information about VLAN network interfaces into the "
		   "underlying system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigVlanSetLinux::start(string& error_msg)
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
IfConfigVlanSetLinux::stop(string& error_msg)
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
IfConfigVlanSetLinux::config_vlan(const IfTreeInterface* pulled_ifp,
				  const IfTreeVif* pulled_vifp,
				  const IfTreeInterface& config_iface,
				  const IfTreeVif& config_vif,
				  string& error_msg)
{
    UNUSED(pulled_ifp);

    //
    // Delete the VLAN if marked for deletion
    //
    if (config_vif.state() == IfTreeItem::DELETED) {
	if (delete_vlan(config_iface.ifname(), config_vif.vifname(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	set_vif_deleted(true);

	return (XORP_OK);
    }

    //
    // Test whether the VLAN already exists
    //
    if ((pulled_vifp != NULL)
	&& (pulled_vifp->is_vlan())
	&& (pulled_vifp->vlan_id() == config_vif.vlan_id())) {
	return (XORP_OK);		// XXX: nothing changed
    }

    //
    // Delete the old VLAN if necessary
    //
    if (pulled_vifp != NULL) {
	if (delete_vlan(config_iface.ifname(), config_vif.vifname(), error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to delete VLAN %s on "
				 "interface %s: %s",
				 config_vif.vifname().c_str(),
				 config_iface.ifname().c_str(),
				 error_msg.c_str());
	    return (XORP_ERROR);
	}
	set_vif_deleted(true);
    }

    //
    // Add the VLAN
    //
    if (add_vlan(config_iface.ifname(), config_vif.vifname(),
		 config_vif.vlan_id(), error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to configure VLAN %s to "
			     "interface %s: %s",
			     config_vif.vifname().c_str(),
			     config_iface.ifname().c_str(),
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigVlanSetLinux::add_vlan(const string& parent_ifname,
			       const string& vlan_name,
			       uint16_t vlan_id,
			       string& error_msg)
{
    struct vlan_ioctl_args vlanreq;
    char expected_vlan_name;

    //
    // Test whether the VLAN name matches
    //
    expected_vlan_name = c_format("vlan%u", vlan_id);
    if (vlan_name != expected_vlan_name) {
	error_msg = c_format("Cannot create VLAN interface %s: "
			     "expected VLAN name is %s",
			     vlan_name.c_str(), expected_vlan_name.c_str());
	return (XORP_ERROR);
    }

    //
    // Set the VLAN interface naming
    //
    memset(&vlanreq, 0, sizeof(vlanreq));
    vlanreq.u.name_type = VLAN_NAME_TYPE_PLUS_VID_NO_PAD;	// vlan10
    vlanreq.cmd = SET_VLAN_NAME_TYPE_CMD;
    if (ioctl(sock, SIOCSIFVLAN, &vlanreq) < 0) {
	error_msg = c_format("Cannot set the VLAN interface name type"
			     "to %s: %s",
			     "VLAN_NAME_TYPE_PLUS_VID_NO_PAD",
			     strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Create the VLAN
    //
    memset(&vlanreq, 0, sizeof(vlanreq));
    strncpy(vlanreq.device1, parent_ifname.c_str(),
	    sizeof(vlanreq.device1) - 1);
    vlanreq.u.VID = vlan_id;
    vlanreq.cmd = ADD_VLAN_CMD;
    if (ioctl(sock, SIOCSIFVLAN, &vlanreq) < 0) {
	error_msg = c_format("Cannot create VLAN interface %s "
			     "(parent = %s VLAN ID = %u): %s",
			     vlan_name.c_str(), parent_ifname.c_str(),
			     vlan_id, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigVlanSetLinux::delete_vlan(const string& parent_ifname,
				  const string& vlan_name,
				  string& error_msg)
{
    struct vlan_ioctl_args vlanreq;

    UNUSED(parent_ifname);

    //
    // Delete the VLAN
    //
    memset(&vlanreq, 0, sizeof(vlanreq));
    strncpy(vlanreq.device1, vlan_name.c_str(), sizeof(vlanreq.device1) - 1);
    vlanreq.cmd = DEL_VLAN_CMD;
    if (ioctl(sock, SIOCSIFVLAN, &vlanreq) < 0) {
	error_msg = c_format("Cannot destroy VLAN interface %s: %s",
			     vlan_name.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_VLAN_LINUX
