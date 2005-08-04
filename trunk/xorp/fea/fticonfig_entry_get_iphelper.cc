// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

#if 0	// XXX: Missing from MinGW
#include <routprot.h>
#else
#define PROTO_IP_NETMGMT 3
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_get.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is IP Helper.
//


FtiConfigEntryGetIPHelper::FtiConfigEntryGetIPHelper(FtiConfig& ftic)
    : FtiConfigEntryGet(ftic)
{
#ifdef HOST_OS_WINDOWS
    register_ftic_primary();
#endif
}

FtiConfigEntryGetIPHelper::~FtiConfigEntryGetIPHelper()
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
FtiConfigEntryGetIPHelper::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
    UNUSED(error_msg);
}

int
FtiConfigEntryGetIPHelper::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
    UNUSED(error_msg);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false
 */
bool
FtiConfigEntryGetIPHelper::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
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
FtiConfigEntryGetIPHelper::lookup_route_by_network4(const IPv4Net& dst,
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
 * @return true on success, otherwise false;
 */
bool
FtiConfigEntryGetIPHelper::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
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
FtiConfigEntryGetIPHelper::lookup_route_by_network6(const IPv6Net& dst,
						  Fte6& fte)
{ 
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

#ifndef HOST_OS_WINDOWS
bool
FtiConfigEntryGetIPHelper::lookup_route_by_dest(const IPvX& , FteX& )
{
    return false;
}

bool
FtiConfigEntryGetIPHelper::lookup_route_by_network(const IPvXNet& , FteX& )
{
    return false;
}

#else // HOST_OS_WINDOWS

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetIPHelper::lookup_route_by_dest(const IPvX& dst, FteX& fte)
{
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! ftic().have_ipv6())
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
	DWORD dwMsgLen;
	LPSTR pMsg;

	dwMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|
				  FORMAT_MESSAGE_ALLOCATE_BUFFER|
				  FORMAT_MESSAGE_IGNORE_INSERTS|
				  FORMAT_MESSAGE_MAX_WIDTH_MASK,
				  NULL, result,
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPSTR)&pMsg, 0, 0);
	XLOG_ERROR("GetIpForwardTable(): %s\n", pMsg);
	LocalFree(pMsg);

	if (pfwdtable != NULL)
	    free(pfwdtable);

	return false;
    }

    IPv4Net	destnet;
    IPv4	dest;
    IPv4	nexthop;
    IPv4	mask;
    bool	found;
    char	cifname[IFNAMSIZ];

    for (uint32_t i = 0; i < pfwdtable->dwNumEntries; i++) {
	// XXX: Windows can have multiple routes to the same destination.
	// Here, we only return the first match.
	if (dst.get_ipv4().addr() == pfwdtable->table[i].dwForwardDest) {

	    dest.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardDest));
	    mask.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardMask));
	    destnet = IPv4Net(dest, mask.mask_len());
	    nexthop.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardNextHop));

	    // Map interface index back to a name.
	    if (if_indextoname(pfwdtable->table[i].dwForwardIfIndex,
			       cifname) == NULL) {
		XLOG_FATAL("if_indextoname() failed.\n");
	    }

	    string ifname(cifname);

	    fte = FteX(destnet, nexthop, ifname, ifname, 0xffff, 0xffff,
		       (pfwdtable->table[i].dwForwardType == PROTO_IP_NETMGMT));

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
FtiConfigEntryGetIPHelper::lookup_route_by_network(const IPvXNet& dst, FteX& fte)
{
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! ftic().have_ipv6())
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
	DWORD dwMsgLen;
	LPSTR pMsg;

	dwMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|
				  FORMAT_MESSAGE_ALLOCATE_BUFFER|
				  FORMAT_MESSAGE_IGNORE_INSERTS|
				  FORMAT_MESSAGE_MAX_WIDTH_MASK,
				  NULL, result,
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPSTR)&pMsg, 0, 0);
	XLOG_ERROR("GetIpForwardTable(): %s\n", pMsg);
	LocalFree(pMsg);

	if (pfwdtable != NULL)
	    free(pfwdtable);

	return false;
    }

    IPv4Net	destnet;
    IPv4	nexthop, mask, dest;
    bool	found;
    char	cifname[IFNAMSIZ];

    for (unsigned int i = 0; i < pfwdtable->dwNumEntries; i++) {
	// XXX: Windows can have multiple routes to the same destination.
	// Here, we only return the first match.
	if (dst.masked_addr().get_ipv4().addr() == pfwdtable->table[i].dwForwardDest) {

	    dest.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardDest));
	    mask.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardMask));
	    destnet = IPv4Net(dest, mask.mask_len());
	    nexthop.copy_in((uint8_t*)&(pfwdtable->table[i].dwForwardNextHop));

	    // Map interface index back to a name.
	    if (if_indextoname(pfwdtable->table[i].dwForwardIfIndex,
			       cifname) == NULL) {
		XLOG_FATAL("if_indextoname() failed.\n");
	    }
	    string ifname(cifname);

	    fte = FteX(destnet, nexthop, ifname, ifname, 0xffff, 0xffff,
		       (pfwdtable->table[i].dwForwardType == PROTO_IP_NETMGMT));

	    found = true;
	    break;
	}
    }

    return (found);
}
#endif // HOST_OS_WINDOWS
