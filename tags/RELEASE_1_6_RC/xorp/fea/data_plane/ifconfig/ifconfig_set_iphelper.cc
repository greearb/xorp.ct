// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_iphelper.cc,v 1.19 2008/10/02 21:57:08 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/win_io.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_set_iphelper.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//

#ifdef HOST_OS_WINDOWS

IfConfigSetIPHelper::IfConfigSetIPHelper(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigSet(fea_data_plane_manager)
{
}

IfConfigSetIPHelper::~IfConfigSetIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper API mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetIPHelper::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetIPHelper::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigSetIPHelper::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);
}

bool
IfConfigSetIPHelper::is_unreachable_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);
}

int
IfConfigSetIPHelper::config_begin(string& error_msg)
{
    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_end(string& error_msg)
{
    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_interface_begin(const IfTreeInterface* pulled_ifp,
					    IfTreeInterface& config_iface,
					    string& error_msg)
{
    if (pulled_ifp == NULL) {
	// Nothing to do: the interface has been deleted from the system
	return (XORP_OK);
    }

    //
    // Set the MTU
    //
    if (config_iface.mtu() != pulled_ifp->mtu()) {
	error_msg = c_format("Cannot set the MTU to %u on "
			     "interface %s: method not supported",
			     config_iface.mtu(),
			     config_iface.ifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Set the MAC address
    //
    if (config_iface.mac() != pulled_ifp->mac()) {
	error_msg = c_format("Cannot set the MAC address to %s "
			     "on interface %s: method not supported",
			     config_iface.mac().str().c_str(),
			     config_iface.ifname().c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_interface_end(const IfTreeInterface* pulled_ifp,
					  const IfTreeInterface& config_iface,
					  string& error_msg)
{
    if (pulled_ifp == NULL) {
	// Nothing to do: the interface has been deleted from the system
	return (XORP_OK);
    }

    //
    // Set the interface status
    //
    if (config_iface.enabled() != pulled_ifp->enabled()) {
	if (set_interface_status(config_iface.ifname(),
				 config_iface.pif_index(),
				 config_iface.interface_flags(),
				 config_iface.enabled(),
				 error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_vif_begin(const IfTreeInterface* pulled_ifp,
				      const IfTreeVif* pulled_vifp,
				      const IfTreeInterface& config_iface,
				      const IfTreeVif& config_vif,
				      string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(error_msg);

    if (pulled_vifp == NULL) {
	// Nothing to do: the vif has been deleted from the system
	return (XORP_OK);
    }

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_vif_end(const IfTreeInterface* pulled_ifp,
				    const IfTreeVif* pulled_vifp,
				    const IfTreeInterface& config_iface,
				    const IfTreeVif& config_vif,
				    string& error_msg)
{
    UNUSED(pulled_ifp);

    if (pulled_vifp == NULL) {
	// Nothing to do: the vif has been deleted from the system
	return (XORP_OK);
    }

    //
    // XXX: If the interface name and vif name are different, then
    // they might have different status: the interface can be UP, while
    // the vif can be DOWN.
    //
    if (config_iface.ifname() != config_vif.vifname()) {
	//
	// Set the vif status
	//
	if (config_vif.enabled() != pulled_vifp->enabled()) {
	    //
	    // XXX: The interface and vif status setting mechanism is
	    // equivalent for this platform.
	    //
	    if (set_interface_status(config_vif.vifname(),
				     config_vif.pif_index(),
				     config_vif.vif_flags(),
				     config_vif.enabled(),
				     error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_add_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr4* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr4& config_addr,
					string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    //
    // Test whether a new address
    //
    do {
	if (pulled_addrp == NULL)
	    break;
	if (pulled_addrp->addr() != config_addr.addr())
	    break;
	if (pulled_addrp->broadcast() != config_addr.broadcast())
	    break;
	if (pulled_addrp->broadcast()
	    && (pulled_addrp->bcast() != config_addr.bcast())) {
	    break;
	}
	if (pulled_addrp->point_to_point() != config_addr.point_to_point())
	    break;
	if (pulled_addrp->point_to_point()
	    && (pulled_addrp->endpoint() != config_addr.endpoint())) {
	    break;
	}
	if (pulled_addrp->prefix_len() != config_addr.prefix_len())
	    break;

	// XXX: Same address, therefore ignore it
	return (XORP_OK);
    } while (false);

    //
    // Delete the old address if necessary
    //
    if (pulled_addrp != NULL) {
	if (delete_addr(config_iface.ifname(), config_vif.vifname(),
			config_vif.pif_index(), config_addr.addr(),
			config_addr.prefix_len(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    //
    // Add the address
    //
    if (add_addr(config_iface.ifname(), config_vif.vifname(),
		 config_vif.pif_index(), config_addr.addr(),
		 config_addr.prefix_len(),
		 config_addr.broadcast(), config_addr.bcast(),
		 config_addr.point_to_point(), config_addr.endpoint(),
		 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_delete_address(const IfTreeInterface* pulled_ifp,
					   const IfTreeVif* pulled_vifp,
					   const IfTreeAddr4* pulled_addrp,
					   const IfTreeInterface& config_iface,
					   const IfTreeVif& config_vif,
					   const IfTreeAddr4& config_addr,
					   string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    //
    // Delete the address
    //
    if (delete_addr(config_iface.ifname(), config_vif.vifname(),
		    config_vif.pif_index(), config_addr.addr(),
		    config_addr.prefix_len(), error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_add_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr6* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr6& config_addr,
					string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    //
    // Test whether a new address
    //
    do {
	if (pulled_addrp == NULL)
	    break;
	if (pulled_addrp->addr() != config_addr.addr())
	    break;
	if (pulled_addrp->point_to_point() != config_addr.point_to_point())
	    break;
	if (pulled_addrp->point_to_point()
	    && (pulled_addrp->endpoint() != config_addr.endpoint())) {
	    break;
	}
	if (pulled_addrp->prefix_len() != config_addr.prefix_len())
	    break;

	// XXX: Same address, therefore ignore it
	return (XORP_OK);
    } while (false);

    //
    // Delete the old address if necessary
    //
    if (pulled_addrp != NULL) {
	if (delete_addr(config_iface.ifname(), config_vif.vifname(),
			config_vif.pif_index(), config_addr.addr(),
			config_addr.prefix_len(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    //
    // Add the address
    //
    if (add_addr(config_iface.ifname(), config_vif.vifname(),
		 config_vif.pif_index(), config_addr.addr(),
		 config_addr.prefix_len(),
		 config_addr.point_to_point(), config_addr.endpoint(),
		 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_delete_address(const IfTreeInterface* pulled_ifp,
					   const IfTreeVif* pulled_vifp,
					   const IfTreeAddr6* pulled_addrp,
					   const IfTreeInterface& config_iface,
					   const IfTreeVif& config_vif,
					   const IfTreeAddr6& config_addr,
					   string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    //
    // Delete the address
    //
    if (delete_addr(config_iface.ifname(), config_vif.vifname(),
		    config_vif.pif_index(), config_addr.addr(),
		    config_addr.prefix_len(), error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::set_interface_status(const string& ifname,
					  uint32_t if_index,
					  uint32_t interface_flags,
					  bool is_enabled,
					  string& error_msg)
{
    // TODO: implement/test it!

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(interface_flags);
    UNUSED(is_enabled);
    UNUSED(error_msg);
#if 0
    MIB_IFROW ifrow;
    DWORD result;

    UNUSED(interface_flags);

    memset(&ifrow, 0, sizeof(ifrow));
    ifrow.dwIndex = if_index;

    result = GetIfEntry(&ifrow);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot obtain existing MIB_IFROW for "
			     "interface %s: error %d\n",
			     ifname.c_str(),
			     (int)result);
	return (XORP_ERROR);
    }
    if (is_enabled)
	ifrow.dwAdminStatus = MIB_IF_ADMIN_STATUS_UP;
    else
	ifrow.dwAdminStatus = MIB_IF_ADMIN_STATUS_DOWN;
    result = SetIfEntry(&ifrow);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot set administrative status of "
			     "interface %s: error %d\n",
			     ifname.c_str(),
			     (int)result);
	return (XORP_ERROR);
    }
#endif // 0

    return (XORP_OK);
}

int
IfConfigSetIPHelper::add_addr(const string& ifname, const string& vifname,
			      uint32_t if_index, const IPv4& addr,
			      uint32_t prefix_len, bool is_broadcast,
			      const IPv4& broadcast_addr,
			      bool is_point_to_point,
			      const IPv4& endpoint_addr,
			      string& error_msg)
{
    PMIB_IPADDRTABLE pAddrTable;
    DWORD       result, tries;
    ULONG       dwSize;
    IPAddr	ipaddr;
    IPMask	ipmask;

    UNUSED(is_broadcast);
    UNUSED(broadcast_addr);
    UNUSED(is_point_to_point);
    UNUSED(endpoint_addr);

    addr.copy_out((uint8_t*)&ipaddr);
    IPv4 prefix_addr = IPv4::make_prefix(prefix_len);
    prefix_addr.copy_out((uint8_t*)&ipmask);

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(*pAddrTable) + (30 * sizeof(MIB_IPADDRROW));
    do {
        pAddrTable = (PMIB_IPADDRTABLE) ((tries == 0) ? malloc(dwSize) :
					 realloc(pAddrTable, dwSize));
        if (pAddrTable == NULL)
            break;
	result = GetIpAddrTable(pAddrTable, &dwSize, FALSE);
        if (pAddrTable == NULL)
            break;
    } while ((++tries < 3) || (result == ERROR_INSUFFICIENT_BUFFER));

    if (result != NO_ERROR) {
        XLOG_ERROR("GetIpAddrTable(): %s", win_strerror(result));
        if (pAddrTable != NULL)
            free(pAddrTable);
        return (XORP_OK);
    }

    for (unsigned int i = 0; i < pAddrTable->dwNumEntries; i++) {
	if (pAddrTable->table[i].dwAddr == ipaddr) {
	    XLOG_WARNING("IP address %s already exists on interface "
			 "with index %lu",
			 addr.str().c_str(),
			 pAddrTable->table[i].dwIndex);
            return (XORP_OK);
	}
    }

    ULONG	ntectx = 0, nteinst = 0;

    result = AddIPAddress(ipaddr, ipmask, if_index, &ntectx, &nteinst);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot add address '%s' "
			     "on interface '%s' vif '%s': error %d",
			     addr.str().c_str(), ifname.c_str(),
			     vifname.c_str(), (int)result);
	return (XORP_OK);
    }

    //
    // We can't delete IP addresses using IP Helper unless we cache the
    // returned NTE context. This means we can only delete addresses
    // which were created during the lifetime of the FEA.
    //
    // Also, these entries have to be keyed by the address we added,
    // so we can figure out which ntectx value to use when deleting it.
    //
    _nte_map.insert(make_pair(make_pair(if_index, (IPAddr)addr.addr()),
			      ntectx));

    return (XORP_OK);
}

int
IfConfigSetIPHelper::delete_addr(const string& ifname, const string& vifname,
				 uint32_t if_index, const IPv4& addr,
				 uint32_t prefix_len, string& error_msg)
{
    map<pair<uint32_t, IPAddr>, ULONG>::iterator ii;

    UNUSED(prefix_len);

    ii = _nte_map.find(make_pair(if_index, (IPAddr)addr.addr()));
    if (ii == _nte_map.end()) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': "
			     "address not found in internal table",
			     addr.str().c_str(), ifname.c_str(),
			     vifname.c_str());
	    return (XORP_ERROR);
    }

    ULONG ntectx = ii->second;
    DWORD result = DeleteIPAddress(ntectx);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': error %d",
			     addr.str().c_str(), ifname.c_str(),
			     vifname.c_str(), (int)result);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIPHelper::add_addr(const string& ifname, const string& vifname,
			      uint32_t if_index, const IPv6& addr,
			      uint32_t prefix_len, bool is_point_to_point,
			      const IPv6& endpoint_addr, string& error_msg)
{
#ifndef HAVE_IPV6
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);
    UNUSED(is_point_to_point);
    UNUSED(endpoint_addr);

    error_msg = "IPv6 is not supported";

    return (XORP_ERROR);

#else // HAVE_IPV6

    //
    // No mechanism to add the address
    //
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);
    UNUSED(is_point_to_point);
    UNUSED(endpoint_addr);

    error_msg = c_format("No mechanism to add an IPv6 address "
			 "on an interface");
    return (XORP_ERROR);
#endif // HAVE_IPV6
}

int
IfConfigSetIPHelper::delete_addr(const string& ifname, const string& vifname,
				 uint32_t if_index, const IPv6& addr,
				 uint32_t prefix_len, string& error_msg)
{
#ifndef HAVE_IPV6
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = "IPv6 is not supported";

    return (XORP_ERROR);

#else // HAVE_IPV6

    //
    // No mechanism to delete the address
    //
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = c_format("No mechanism to delete an IPv6 address "
			 "on an interface");

    return (XORP_ERROR);
#endif
}

#endif // HOST_OS_WINDOWS
