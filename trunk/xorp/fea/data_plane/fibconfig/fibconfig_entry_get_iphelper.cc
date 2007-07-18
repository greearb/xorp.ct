// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_iphelper.cc,v 1.7 2007/07/11 22:18:05 pavlin Exp $"

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
#include "fea/fibconfig_entry_get.hh"

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

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetIPHelper::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

/**
 * Lookup a route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetIPHelper::lookup_route_by_network4(const IPv4Net& dst,
						    Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetIPHelper::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

/**
 * Lookup route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetIPHelper::lookup_route_by_network6(const IPv6Net& dst,
						    Fte6& fte)
{ 
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetIPHelper::lookup_route_by_dest(const IPvX& dst, FteX& fte)
{
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return false;
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return false;
    }

    switch (dst.af()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	return false;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    PMIB_IPFORWARDTABLE		pfwdtable;
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
	return false;
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
	    const IfTree& iftree = fibconfig().iftree();
	    const IfTreeInterface* ifp = iftree.find_interface(ifindex);
	    XLOG_ASSERT(ifp != NULL);

	    // XXX: The old test for a XORP route was:
	    // pfwdtable->table[i].dwForwardType == PROTO_IP_NETMGMT
	    // For now, just pass true; we will deal with this better
	    // once RTMv2 is supported.
	    //
	    fte = FteX(destnet, nexthop, ifp->ifname(), ifp->ifname(),
		       0xffff, 0xffff, true);
	    found = true;
	    break;
	}
    }

    return (found);
}

/**
 * Lookup route by network.
 * XXX: The tests here may not be correct
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetIPHelper::lookup_route_by_network(const IPvXNet& dst,
						   FteX& fte)
{
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return false;
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    switch (dst.af()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	return false;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    PMIB_IPFORWARDTABLE		pfwdtable;
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
	return false;
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
	    const IfTree& iftree = fibconfig().iftree();
	    const IfTreeInterface* ifp = iftree.find_interface(ifindex);
	    XLOG_ASSERT(ifp != NULL);

	    fte = FteX(destnet, nexthop, ifp->ifname(), ifp->ifname(),
		       0xffff, 0xffff, true);

	    found = true;
	    break;
	}
    }

    return (found);
}

#endif // HOST_OS_WINDOWS
