// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
// // Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/fticonfig_entry_set_iphelper.cc,v 1.2 2005/08/18 15:45:44 bms Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif

#ifndef MIB_IPROUTE_TYPE_OTHER
#define MIB_IPROUTE_TYPE_OTHER		1
#endif
#ifndef MIB_IPROUTE_TYPE_INVALID
#define MIB_IPROUTE_TYPE_INVALID	2
#endif
#ifndef MIB_IPROUTE_TYPE_DIRECT
#define MIB_IPROUTE_TYPE_DIRECT		3
#endif
#ifndef MIB_IPROUTE_TYPE_INDIRECT
#define MIB_IPROUTE_TYPE_INDIRECT	4
#endif

#ifndef PROTO_IP_NETMGMT
#define PROTO_IP_NETMGMT		3
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is the IP helper API for
// Windows (IPHLPAPI.DLL).
//


FtiConfigEntrySetIPHelper::FtiConfigEntrySetIPHelper(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic)
{
#ifdef HOST_OS_WINDOWS
    register_ftic_primary();
#endif
}

FtiConfigEntrySetIPHelper::~FtiConfigEntrySetIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigEntrySetIPHelper::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
    UNUSED(error_msg);
}

int
FtiConfigEntrySetIPHelper::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
    UNUSED(error_msg);
}

bool
FtiConfigEntrySetIPHelper::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetIPHelper::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetIPHelper::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetIPHelper::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

#ifndef HOST_OS_WINDOWS
bool
FtiConfigEntrySetIPHelper::add_entry(const FteX& )
{
    return false;
}

bool
FtiConfigEntrySetIPHelper::delete_entry(const FteX& )
{
    return false;
}

#else // HOST_OS_WINDOWS
bool
FtiConfigEntrySetIPHelper::add_entry(const FteX& fte)
{
    MIB_IPFORWARDROW	ipfwdrow;
    int			family = fte.net().af();

    debug_msg("add_entry (network = %s nexthop = %s)\n",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes

    switch (family) {
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

    memset(&ipfwdrow, 0, sizeof(ipfwdrow));

    fte.net().masked_addr().get_ipv4().copy_out((uint8_t*)
						&ipfwdrow.dwForwardDest);
    if (!fte.is_host_route()) {
	fte.net().netmask().get_ipv4().copy_out((uint8_t*)
						&ipfwdrow.dwForwardMask);
    }
    fte.nexthop().get_ipv4().copy_out((uint8_t*)&ipfwdrow.dwForwardNextHop);

    ipfwdrow.dwForwardIfIndex = (DWORD)if_nametoindex(fte.vifname().c_str());
    ipfwdrow.dwForwardProto = PROTO_IP_NETMGMT;

#if 0
    // We need to tell Windows if the route is a connected route.
    if (fte.is_connected_route())
	ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_DIRECT;
    else
	ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
#endif

    // XXX: Instead of setting to 0, try the 'other' (not specified by
    // RFC 1354) forward type, otherwise we need to treat connected routes
    // differently.
    ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_OTHER;

#if 0
    debug_msg("About to call CreateIpForwardEntry() with the following:\n");
    debug_msg("%08lx %08lx %lu %08lx %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
	      ipfwdrow.dwForwardDest,
	      ipfwdrow.dwForwardMask,
	      ipfwdrow.dwForwardPolicy,
	      ipfwdrow.dwForwardNextHop,
	      ipfwdrow.dwForwardIfIndex,
	      ipfwdrow.dwForwardType,
	      ipfwdrow.dwForwardProto,
	      ipfwdrow.dwForwardAge,
	      ipfwdrow.dwForwardNextHopAS,
	      ipfwdrow.dwForwardMetric1,
	      ipfwdrow.dwForwardMetric2,
	      ipfwdrow.dwForwardMetric3,
	      ipfwdrow.dwForwardMetric4,
	      ipfwdrow.dwForwardMetric5);
#endif

    DWORD result = CreateIpForwardEntry(&ipfwdrow);
    if (result != NO_ERROR) {
	XLOG_ERROR("CreateIpForwardEntry() failed, error: %lu\n", result);
	return false;
    }

    return true;
}

bool
FtiConfigEntrySetIPHelper::delete_entry(const FteX& fte)
{
    MIB_IPFORWARDROW	ipfwdrow;
    int			family = fte.net().af();

    debug_msg("delete_entry (network = %s nexthop = %s)\n",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes

    switch (family) {
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

    DWORD result;

    memset(&ipfwdrow, 0, sizeof(ipfwdrow));

#if 1
    //
    // XXX: This is a hack. The upper half of the FEA hasn't passed us
    // the actual next hop, which is a bug, because Windows' FIB requires
    // that *all* information is specified for a delete operation.
    //
    struct in_addr mydest;
    fte.net().masked_addr().get_ipv4().copy_out(mydest);
    result = GetBestRoute((DWORD)mydest.s_addr, 0, &ipfwdrow);
    if (result != NO_ERROR) {
	XLOG_ERROR("DeleteIpForwardEntry() failed, error: %lu\n", result);
	return false;
    }
#else
    fte.net().masked_addr().get_ipv4().copy_out(
	(uint8_t*)&(ipfwdrow.dwForwardDest));
    fte.net().netmask().get_ipv4().copy_out((uint8_t*)&ipfwdrow.dwForwardMask);

    fte.nexthop().get_ipv4().copy_out((uint8_t*)&ipfwdrow.dwForwardNextHop);

    ipfwdrow.dwForwardIfIndex = (DWORD)if_nametoindex(fte.vifname().c_str());
    ipfwdrow.dwForwardProto = PROTO_IP_NETMGMT;

    debug_msg("vifname = %s, ifindex = %lu\n", fte.vifname().c_str(),
	      ipfwdrow.dwForwardIfIndex);

#if 0
    // We need to tell Windows if the route is a connected route.
    if (fte.is_connected_route())
	ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_DIRECT;
    else
	ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
#endif

#endif

#if 0
    debug_msg("About to call DeleteIpForwardEntry() with the following:\n");
    debug_msg("%08lx %08lx %lu %08lx %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
	      ipfwdrow.dwForwardDest,
	      ipfwdrow.dwForwardMask,
	      ipfwdrow.dwForwardPolicy,
	      ipfwdrow.dwForwardNextHop,
	      ipfwdrow.dwForwardIfIndex,
	      ipfwdrow.dwForwardType,
	      ipfwdrow.dwForwardProto,
	      ipfwdrow.dwForwardAge,
	      ipfwdrow.dwForwardNextHopAS,
	      ipfwdrow.dwForwardMetric1,
	      ipfwdrow.dwForwardMetric2,
	      ipfwdrow.dwForwardMetric3,
	      ipfwdrow.dwForwardMetric4,
	      ipfwdrow.dwForwardMetric5);
#endif

    result = DeleteIpForwardEntry(&ipfwdrow);
    if (result != NO_ERROR) {
	XLOG_ERROR("DeleteIpForwardEntry() failed, error: %lu\n", result);
	return false;
    }

    return true;
}
#endif // HOST_OS_WINDOWS
