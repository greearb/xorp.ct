// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
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
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#ifdef HAVE_LINUX_IF_VLAN_H
#include <linux/if_vlan.h>
#endif
#ifdef HAVE_NET_IF_VLAN_VAR_H
#include <net/if_vlan_var.h>
#endif
#ifdef HAVE_NET_IF_VLANVAR_H
#include <net/if_vlanvar.h>
#endif


#include "fea/ifconfig.hh"
#include "ifconfig_vlan_set_linux.hh"


//
// Set VLAN information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is Linux-specific ioctl(2).
//

IfConfigVlanSetLinux::IfConfigVlanSetLinux(FeaDataPlaneManager& fea_data_plane_manager,
					   bool is_dummy)
    : IfConfigVlanSet(fea_data_plane_manager),
      _is_dummy(is_dummy),
      _s4(-1)
{
}

IfConfigVlanSetLinux::~IfConfigVlanSetLinux()
{
#if defined(HAVE_VLAN_LINUX) or defined(HAVE_VLAN_BSD)
    if (! _is_dummy) {
	string error_msg;

	if (stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot stop the Linux-specific ioctl(2) mechanism to set "
		       "information about VLAN network interfaces into the "
		       "underlying system: %s",
		       error_msg.c_str());
	}
    }
#endif
}

int
IfConfigVlanSetLinux::start(string& error_msg)
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
#else
    UNUSED(error_msg);
#endif
    _is_running = true;

    return (XORP_OK);
}

int
IfConfigVlanSetLinux::stop(string& error_msg)
{
    if (_is_dummy)
	_is_running = false;

    if (! _is_running)
	return (XORP_OK);

#if defined(HAVE_VLAN_LINUX) or defined(HAVE_VLAN_BSD)
    int ret_value4 = XORP_OK;
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
#else
    UNUSED(error_msg);
#endif

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigVlanSetLinux::config_add_vlan(const IfTreeInterface* system_ifp,
				      const IfTreeInterface& config_if,
				      string& error_msg)
{
    if (_is_dummy)
	return XORP_OK;

#if defined(HAVE_VLAN_LINUX) or defined(HAVE_VLAN_BSD)
    //
    // Test whether the VLAN already exists
    //
    if ((system_ifp != NULL)
	&& (system_ifp->parent_ifname() == config_if.parent_ifname())
	&& (system_ifp->iface_type() == config_if.iface_type())
	&& (system_ifp->vid() == config_if.vid())) {
	return (XORP_OK);		// XXX: nothing changed
    }

    //
    // Delete the old VLAN if necessary
    //
    if (system_ifp != NULL) {
	if (system_ifp->is_vlan()) {
	    if (delete_vlan(config_if.ifname(), error_msg) != XORP_OK) {
		error_msg = c_format("Failed to delete VLAN %s, reason: %s",
				     config_if.ifname().c_str(),
				     error_msg.c_str());
		return (XORP_ERROR);
	    }
	}
	// TODO:  Support mac-vlans, etc??
    }

    //
    // Add the VLAN
    //
    if (config_if.is_vlan()) {
	int vid = atoi(config_if.vid().c_str());
	if (vid < 0 || vid >= 4095) {
	    error_msg = c_format("ERROR:  VLAN-ID: %s is out of range (0-4094).\n",
				 config_if.vid().c_str());
	    return XORP_ERROR;
	}

	if (add_vlan(config_if.parent_ifname(), config_if.ifname(),
		     vid, error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to add VLAN %i to interface %s: %s",
				 vid,
				 config_if.parent_ifname().c_str(),
				 error_msg.c_str());
	    return XORP_ERROR;
	}
	return XORP_OK;
    }
    // TODO:  else, support mac-vlans, etc.
    error_msg = c_format("Unknown virtual device type: %s\n",
			 config_if.iface_type().c_str());
    return XORP_ERROR;
#else
    UNUSED(system_ifp);
    UNUSED(config_if);
    UNUSED(error_msg);
    return XORP_OK;
#endif
}

int
IfConfigVlanSetLinux::config_delete_vlan(const IfTreeInterface& config_iface,
					 string& error_msg)
{

    //
    // Delete the VLAN
    //
    return delete_vlan(config_iface.ifname(), error_msg);
}

int
IfConfigVlanSetLinux::add_vlan(const string& parent_ifname,
			       const string& vlan_name,
			       uint16_t vlan_id,
			       string& error_msg)
{
    if (_is_dummy)
	return XORP_OK;

#ifdef HAVE_VLAN_BSD
    struct ifreq ifreq;

    //
    // Create the VLAN
    //
    do {
	int result;
	bool is_same_name = false;

	// This seems to create the VLAN and set the VID all at once.
	// The old way of using ioctls to create the VLAN was failing
	// due to failure to set the VID (EBUSY).
	string cmd(c_format("ifconfig %s.%hu create",
			    parent_ifname.c_str(), vlan_id));
	result = system(cmd.c_str());
	XLOG_WARNING("Tried to create vlan with command: %s, result: %i",
		     cmd.c_str(), result);
	cmd = c_format("%s.%hu", parent_ifname.c_str(), vlan_id);
	if (strcmp(vlan_name.c_str(), cmd.c_str()) == 0) {
	    is_same_name = true;
	}

	if ((result >= 0) && is_same_name) {
	    break;		// Success
	}

#ifndef SIOCSIFNAME
	//
	// No support for interface renaming, therefore return an error
	//
	if (result < 0) {
	    error_msg = c_format("Cannot create VLAN interface %s: %s errno: %i",
				 vlan_name.c_str(), strerror(errno), errno);
	    return (XORP_ERROR);
	}
	// XXX: The created name didn't match
	XLOG_ASSERT(is_same_name == false);
	error_msg = c_format("Cannot create VLAN interface %s: "
			     "the created name (%s) doesn't match",
			     vlan_name.c_str(), ifreq.ifr_name);
	string dummy_error_msg;
	delete_vlan(vlan_name, dummy_error_msg);
	return (XORP_ERROR);

#else // SIOCSIFNAME
	//
	// Create the VLAN interface with a temporary name
	//
	string tmp_vlan_name = c_format("vlan%u", vlan_id);

	memset(&ifreq, 0, sizeof(ifreq));
	strlcpy(ifreq.ifr_name, tmp_vlan_name.c_str(), sizeof(ifreq.ifr_name));
	if (ioctl(_s4, SIOCIFCREATE, &ifreq) < 0) {
	    error_msg = c_format("Cannot create VLAN interface %s: %s",
				 tmp_vlan_name.c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	//
	// XXX: We don't care if the created name doesn't match the
	// intended name, because this is just a temporary one.
	//
	tmp_vlan_name = string(ifreq.ifr_name);

	//
	// Rename the VLAN interface
	//
	char new_vlan_name[sizeof(ifreq.ifr_name)];
	memset(&ifreq, 0, sizeof(ifreq));
	strlcpy(ifreq.ifr_name, tmp_vlan_name.c_str(), sizeof(ifreq.ifr_name));
	strlcpy(new_vlan_name, vlan_name.c_str(), sizeof(new_vlan_name));
	ifreq.ifr_data = new_vlan_name;
	if (ioctl(_s4, SIOCSIFNAME, &ifreq) < 0) {
	    error_msg = c_format("Cannot rename VLAN interface %s to %s: %s",
				 tmp_vlan_name.c_str(), new_vlan_name,
				 strerror(errno));
	    string dummy_error_msg;
	    delete_vlan(tmp_vlan_name.c_str(),
			dummy_error_msg);
	    return (XORP_ERROR);
	}
#endif // SIOCSIFNAME

	break;
    } while (false);

#elif defined(HAVE_VLAN_LINUX)

    /* Linux */
    struct vlan_ioctl_args vlanreq;

    //
    // Set the VLAN interface naming
    //
    memset(&vlanreq, 0, sizeof(vlanreq));
    vlanreq.u.name_type = VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD; // eth0.5
    vlanreq.cmd = SET_VLAN_NAME_TYPE_CMD;
    if (ioctl(_s4, SIOCSIFVLAN, &vlanreq) < 0) {
	error_msg = c_format("Cannot set the VLAN interface name type"
			     "to VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Create the VLAN
    //
    memset(&vlanreq, 0, sizeof(vlanreq));
    strlcpy(vlanreq.device1, parent_ifname.c_str(), sizeof(vlanreq.device1));
    vlanreq.u.VID = vlan_id;
    vlanreq.cmd = ADD_VLAN_CMD;
    errno = 0;
    if (ioctl(_s4, SIOCSIFVLAN, &vlanreq) < 0) {
	if (errno != EEXIST) {
	    error_msg = c_format("Cannot create VLAN interface %s "
				 "(parent = %s VLAN ID = %u): %s",
				 vlan_name.c_str(), parent_ifname.c_str(),
				 vlan_id, strerror(errno));
	    return (XORP_ERROR);
	}
    }

    //
    // Rename the VLAN interface if necessary
    //
    string tmp_vlan_name = c_format("%s.%u", parent_ifname.c_str(), vlan_id);

    if (vlan_name != tmp_vlan_name) {
#ifndef SIOCSIFNAME
	//
	// No support for interface renaming, therefore return an error
	//
	error_msg = c_format("Cannot create VLAN interface %s "
			     "(parent = %s VLAN ID = %u): "
			     "no support for interface renaming",
			     vlan_name.c_str(), parent_ifname.c_str(),
			     vlan_id);
	return (XORP_ERROR);

#else // SIOCSIFNAME
	//
	// Rename the VLAN interface
	//
	struct ifreq ifreq;
	char new_vlan_name[sizeof(ifreq.ifr_newname)];

	memset(&ifreq, 0, sizeof(ifreq));
	strlcpy(ifreq.ifr_name, tmp_vlan_name.c_str(), sizeof(ifreq.ifr_name));
	strlcpy(new_vlan_name, vlan_name.c_str(), sizeof(new_vlan_name));
	strlcpy(ifreq.ifr_newname, new_vlan_name, sizeof(ifreq.ifr_newname));
	if (ioctl(_s4, SIOCSIFNAME, &ifreq) < 0) {
	    error_msg = c_format("Cannot rename VLAN interface %s to %s: %s",
				 tmp_vlan_name.c_str(), new_vlan_name,
				 strerror(errno));
	    string dummy_error_msg;
	    delete_vlan(tmp_vlan_name, dummy_error_msg);
	    return (XORP_ERROR);
	}
#endif // SIOCSIFNAME
    }
#else
    /* Un-supported VLAN platform (Windows??) */
    UNUSED(parent_ifname);
    UNUSED(vlan_name);
    UNUSED(vlan_id);
    UNUSED(error_msg);
#endif

    return (XORP_OK);
}

int
IfConfigVlanSetLinux::delete_vlan(const string& vlan_name,
				  string& error_msg)
{
    if (_is_dummy)
	return XORP_OK;

    //
    // Delete the VLAN
    //

#ifdef HAVE_VLAN_BSD
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(ifreq));
    strlcpy(ifreq.ifr_name, vlan_name.c_str(), sizeof(ifreq.ifr_name));
    if (ioctl(_s4, SIOCIFDESTROY, &ifreq) < 0) {
        error_msg = c_format("Cannot destroy BSD VLAN interface %s: %s",
                             vlan_name.c_str(), strerror(errno));
        return (XORP_ERROR);
    }

#elif defined(HAVE_VLAN_LINUX)

    struct vlan_ioctl_args vlanreq;

    memset(&vlanreq, 0, sizeof(vlanreq));
    strlcpy(vlanreq.device1, vlan_name.c_str(), sizeof(vlanreq.device1));
    vlanreq.cmd = DEL_VLAN_CMD;
    if (ioctl(_s4, SIOCSIFVLAN, &vlanreq) < 0) {
	error_msg = c_format("Cannot destroy Linux VLAN interface %s: %s",
			     vlan_name.c_str(), strerror(errno));
	return (XORP_ERROR);
    }
#else
    /* Un-supported VLAN platform...Windows?? */
    UNUSED(vlan_name);
    UNUSED(error_msg);
#endif

    return (XORP_OK);
}
