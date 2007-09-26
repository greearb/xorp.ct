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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_vlan_set_bsd.cc,v 1.4 2007/09/25 23:00:30 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
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

#include "ifconfig_vlan_set_bsd.hh"


//
// Set VLAN information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is BSD-specific ioctl(2).
//

#ifdef HAVE_VLAN_BSD

IfConfigVlanSetBsd::IfConfigVlanSetBsd(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigVlanSet(fea_data_plane_manager),
      _s4(-1)
{
}

IfConfigVlanSetBsd::~IfConfigVlanSetBsd()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the BSD-specific ioctl(2) mechanism to set "
		   "information about VLAN network interfaces into the "
		   "underlying system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigVlanSetBsd::start(string& error_msg)
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
IfConfigVlanSetBsd::stop(string& error_msg)
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
IfConfigVlanSetBsd::config_vlan(const IfTreeInterface* pulled_ifp,
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
	set_vif_obsoleted(true);

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
	set_vif_obsoleted(true);
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
    set_vif_obsoleted(true);

    return (XORP_OK);
}

int
IfConfigVlanSetBsd::add_vlan(const string& parent_ifname,
			     const string& vlan_name,
			     uint16_t vlan_id,
			     string& error_msg)
{
    struct ifreq ifreq;
    struct vlanreq vlanreq;

    //
    // Create the VLAN
    //
    memset(&ifreq, 0, sizeof(ifreq));
    strlcpy(ifreq.ifr_name, vlan_name.c_str(), sizeof(ifreq.ifr_name));
    if (ioctl(_s4, SIOCIFCREATE, &ifreq) < 0) {
	error_msg = c_format("Cannot create VLAN interface %s: %s",
			     vlan_name.c_str(), strerror(errno));
	return (XORP_ERROR);
    }
    if (strncmp(vlan_name.c_str(), ifreq.ifr_name, sizeof(ifreq.ifr_name))
	!= 0) {
	// XXX: The created name didn't match
	error_msg = c_format("Cannot create VLAN interface %s: "
			     "the created name (%s) doesn't match",
			     vlan_name.c_str(), ifreq.ifr_name);
	string dummy_error_msg;
	delete_vlan(parent_ifname, string(ifreq.ifr_name), dummy_error_msg);
	return (XORP_ERROR);
    }

    //
    // Configure the VLAN
    //
    memset(&ifreq, 0, sizeof(ifreq));
    strlcpy(ifreq.ifr_name, vlan_name.c_str(), sizeof(ifreq.ifr_name));
    memset(&vlanreq, 0, sizeof(vlanreq));
    vlanreq.vlr_tag = vlan_id;
    strlcpy(vlanreq.vlr_parent, parent_ifname.c_str(),
	    sizeof(vlanreq.vlr_parent));
    ifreq.ifr_data = (caddr_t)(&vlanreq);
    if (ioctl(_s4, SIOCSETVLAN, (caddr_t)&ifreq) < 0) {
	error_msg = c_format("Cannot configure VLAN interface %s "
			     "(parent = %s VLAN ID = %u): %s",
			     vlan_name.c_str(), parent_ifname.c_str(),
			     vlan_id, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigVlanSetBsd::delete_vlan(const string& parent_ifname,
				const string& vlan_name,
				string& error_msg)
{
    struct ifreq ifreq;

    UNUSED(parent_ifname);

    //
    // Delete the VLAN
    //
    memset(&ifreq, 0, sizeof(ifreq));
    strlcpy(ifreq.ifr_name, vlan_name.c_str(), sizeof(ifreq.ifr_name));
    if (ioctl(_s4, SIOCIFDESTROY, &ifreq) < 0) {
	error_msg = c_format("Cannot destroy VLAN interface %s: %s",
			     vlan_name.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_VLAN_BSD
