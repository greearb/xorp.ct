// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
#include "libxorp/win_io.h"
#include "libxorp/ipvxnet.hh"

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif
#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif

#include "fea/fibconfig.hh"

#include "fibconfig_entry_get_iphelper.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//

#ifdef HOST_OS_WINDOWS

FibConfigEntryGetIPHelper::FibConfigEntryGetIPHelper(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntryGet(fea_data_plane_manager)
{
}

FibConfigEntryGetIPHelper::~FibConfigEntryGetIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryGetIPHelper::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntryGetIPHelper::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigEntryGetIPHelper::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

int
FibConfigEntryGetIPHelper::lookup_route_by_network4(const IPv4Net& dst,
						    Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

int
FibConfigEntryGetIPHelper::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

int
FibConfigEntryGetIPHelper::lookup_route_by_network6(const IPv6Net& dst,
						    Fte6& fte)
{ 
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

int
FibConfigEntryGetIPHelper::lookup_route_by_dest(const IPvX& dst, FteX& fte)
{
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);

    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return (XORP_ERROR);
    }

    switch (dst.af()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	return (XORP_ERROR);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    PMIB_IPFORWARDTABLE		pfwdtable = NULL;
    DWORD			result, tries;
    ULONG			dwSize;

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(MIB_IPFORWARDTABLE);

    do {
	pfwdtable = (PMIB_IPFORWARDTABLE) ((tries == 0) ?
		     malloc(dwSize) : realloc(pfwdtable, dwSize));
	if (pfwdtable == NULL)
	    break;
	result = GetIpForwardTable(pfwdtable, &dwSize, TRUE);
    } while ((++tries < 3) || (result == ERROR_INSUFFICIENT_BUFFER));

    if (result != NO_ERROR) {
	XLOG_ERROR("GetIpForwardTable(): %s\n", win_strerror(result));
	if (pfwdtable != NULL)
	    free(pfwdtable);
	return (XORP_ERROR);
    }

    IPv4	dest;
    IPv4	nexthop;
    IPv4	mask;
    IPv4Net	destnet;
    bool	found;

    for (uint32_t i = 0; i < pfwdtable->dwNumEntries; i++) {
	// XXX: Windows can have multiple routes to the same destination.
	// Here, we only return the first match.
	if (dst.get_ipv4().addr() == pfwdtable->table[i].dwForwardDest) {
	    dest.copy_in(reinterpret_cast<uint8_t*>(
			 &pfwdtable->table[i].dwForwardDest));
	    mask.copy_in(reinterpret_cast<uint8_t*>(
			 &pfwdtable->table[i].dwForwardMask));
	    destnet = IPv4Net(dest, mask.mask_len());
	    nexthop.copy_in(reinterpret_cast<uint8_t*>(
			    &pfwdtable->table[i].dwForwardNextHop));

	    uint32_t ifindex = static_cast<uint32_t>(
				pfwdtable->table[i].dwForwardIfIndex);
	    const IfTree& iftree = fibconfig().system_config_iftree();
	    const IfTreeVif* vifp = iftree.find_vif(ifindex);
	    XLOG_ASSERT(vifp != NULL);

	    //
	    // XXX: The old test for a XORP route was:
	    // pfwdtable->table[i].dwForwardType == PROTO_IP_NETMGMT
	    // For now, just pass true; we will deal with this better
	    // once RTMv2 is supported.
	    //
	    fte = FteX(destnet, nexthop, vifp->ifname(), vifp->vifname(),
		       0xffff, 0xffff, true);
	    found = true;
	    break;
	}
    }

    if (! found)
	return (XORP_ERROR);
    return (XORP_OK);
}

int
FibConfigEntryGetIPHelper::lookup_route_by_network(const IPvXNet& dst,
						   FteX& fte)
{
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);

    switch (dst.af()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	return (XORP_ERROR);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    PMIB_IPFORWARDTABLE		pfwdtable = NULL;
    DWORD			result, tries;
    ULONG			dwSize;

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(MIB_IPFORWARDTABLE);

    do {
	pfwdtable = (PMIB_IPFORWARDTABLE) ((tries == 0) ?
		     malloc(dwSize) : realloc(pfwdtable, dwSize));
	if (pfwdtable == NULL)
	    break;
	result = GetIpForwardTable(pfwdtable, &dwSize, TRUE);
    } while ((++tries < 3) || (result == ERROR_INSUFFICIENT_BUFFER));

    if (result != NO_ERROR) {
	XLOG_ERROR("GetIpForwardTable(): %s\n", win_strerror(result));
	if (pfwdtable != NULL)
	    free(pfwdtable);
	return (XORP_ERROR);
    }

    IPv4Net	destnet;
    IPv4	nexthop, mask, dest;
    bool	found;

    for (unsigned int i = 0; i < pfwdtable->dwNumEntries; i++) {
	// XXX: Windows can have multiple routes to the same destination.
	// Here, we only return the first match.
	if (dst.masked_addr().get_ipv4().addr() ==
	    pfwdtable->table[i].dwForwardDest) {
	    dest.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardDest));
	    mask.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardMask));
	    destnet = IPv4Net(dest, mask.mask_len());
	    nexthop.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardNextHop));

	    uint32_t ifindex = static_cast<uint32_t>(
				pfwdtable->table[i].dwForwardIfIndex);
	    const IfTree& iftree = fibconfig().system_config_iftree();
	    const IfTreeVif* vifp = iftree.find_vif(ifindex);
	    XLOG_ASSERT(vifp != NULL);

	    fte = FteX(destnet, nexthop, vifp->ifname(), vifp->vifname(),
		       0xffff, 0xffff, true);

	    found = true;
	    break;
	}
    }

    if (! found)
	return (XORP_ERROR);
    return (XORP_OK);
}

#endif // HOST_OS_WINDOWS
