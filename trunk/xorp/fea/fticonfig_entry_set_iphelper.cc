// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
// // Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/fticonfig_entry_set_iphelper.cc,v 1.5 2005/12/22 12:18:20 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/win_io.h"

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif
#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"

#include "iftree.hh"


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
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntrySetIPHelper::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
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

    bool is_existing;

    IPAddr tmpdest;
    IPAddr tmpnexthop;
    IPAddr tmpmask;
    DWORD result;

    fte.net().masked_addr().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
						&tmpdest));
    fte.nexthop().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(&tmpnexthop));
    if (!fte.is_host_route()) {
    	fte.net().netmask().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
						&tmpmask));
    } else {
	tmpmask = 0xFFFFFFFF;
    }

    // Check for an already existing specific route to this destination.
    result = GetBestRoute(tmpdest, 0, &ipfwdrow);
    if (result == NO_ERROR &&
	ipfwdrow.dwForwardDest == tmpdest &&
	ipfwdrow.dwForwardMask == tmpmask &&
	ipfwdrow.dwForwardNextHop == tmpnexthop) {
	is_existing = true;
    } else {
    	memset(&ipfwdrow, 0, sizeof(ipfwdrow));
    }

    IfTree& it = ftic().iftree();
    IfTree::IfMap::const_iterator ii = it.get_if(fte.ifname());
    XLOG_ASSERT(ii != it.ifs().end());

    ipfwdrow.dwForwardDest = tmpdest;
    ipfwdrow.dwForwardMask = tmpmask;
    ipfwdrow.dwForwardNextHop = tmpnexthop;
    ipfwdrow.dwForwardIfIndex = ii->second.pif_index();
    ipfwdrow.dwForwardProto = PROTO_IP_NETMGMT;
    ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_OTHER;

    if (is_existing) {
        result = SetIpForwardEntry(&ipfwdrow);
    } else {
        result = CreateIpForwardEntry(&ipfwdrow);
    }

    if (result != NO_ERROR) {
	XLOG_ERROR("%sIpForwardEntry() failed, error: %s\n",
		   is_existing ? "Set" : "Create", win_strerror(result));
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

    //
    // Windows needs to know the next-hop. We don't always know it.
    // Try to figure it out via GetBestRoute().
    //
    DWORD result;
    IPAddr tmpdest;
    IPAddr tmpmask;
    IPAddr tmpnexthop;

    memset(&ipfwdrow, 0, sizeof(ipfwdrow));

    fte.net().masked_addr().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
						&tmpdest));
    fte.net().netmask().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
					    &tmpmask));
    fte.nexthop().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(&tmpnexthop));

    result = GetBestRoute(tmpdest, 0, &ipfwdrow);
    if (result == NO_ERROR &&
	ipfwdrow.dwForwardDest == tmpdest &&
	ipfwdrow.dwForwardMask == tmpmask &&
	ipfwdrow.dwForwardNextHop == tmpnexthop) {
	// Don't change any fields
    } else {
	ipfwdrow.dwForwardDest = tmpdest;
	ipfwdrow.dwForwardMask = tmpmask;
	ipfwdrow.dwForwardNextHop = tmpnexthop;
        IfTree& it = ftic().iftree();
        IfTree::IfMap::const_iterator ii = it.get_if(fte.ifname());
        XLOG_ASSERT(ii != it.ifs().end());
        ipfwdrow.dwForwardIfIndex = ii->second.pif_index();
        ipfwdrow.dwForwardProto = PROTO_IP_NETMGMT;
    }

    result = DeleteIpForwardEntry(&ipfwdrow);
    if (result != NO_ERROR) {
	XLOG_ERROR("DeleteIpForwardEntry() failed, error: %s\n",
		   win_strerror(result));
	return false;
    }

    return true;
}
#endif // HOST_OS_WINDOWS
