// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
// // Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_set_iphelper.cc,v 1.6 2007/06/07 01:28:38 pavlin Exp $"

#include "fea/fea_module.h"

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

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_set.hh"

#include "fibconfig_entry_set_iphelper.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is the IP helper API for
// Windows (IPHLPAPI.DLL).
//

#ifdef HOST_OS_WINDOWS

FibConfigEntrySetIPHelper::FibConfigEntrySetIPHelper(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntrySet(fea_data_plane_manager)
{
}

FibConfigEntrySetIPHelper::~FibConfigEntrySetIPHelper()
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
FibConfigEntrySetIPHelper::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntrySetIPHelper::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
FibConfigEntrySetIPHelper::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FibConfigEntrySetIPHelper::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

bool
FibConfigEntrySetIPHelper::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FibConfigEntrySetIPHelper::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

bool
FibConfigEntrySetIPHelper::add_entry(const FteX& fte)
{
    MIB_IPFORWARDROW	ipfwdrow;
    int			family = fte.net().af();

    debug_msg("add_entry (network = %s nexthop = %s)\n",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! fibconfig().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! fibconfig().have_ipv6())
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

    const IfTree& iftree = fibconfig().iftree();
    const IfTreeInterface* ifp = iftree.find_interface(fte.ifname());
    XLOG_ASSERT(ifp != NULL);

    ipfwdrow.dwForwardDest = tmpdest;
    ipfwdrow.dwForwardMask = tmpmask;
    ipfwdrow.dwForwardNextHop = tmpnexthop;
    ipfwdrow.dwForwardIfIndex = ifp->pif_index();
    ipfwdrow.dwForwardProto = PROTO_IP_NETMGMT;
    ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_OTHER;
    //
    // The following fields *must* be filled out if RRAS is running.
    // XXX: Should obtain interface metric and use that; we use a hard
    // coded metric of 20 because other values do not work.
    //
    ipfwdrow.dwForwardAge = INFINITE;
    ipfwdrow.dwForwardMetric1 = 20;
#if 0
    // Filling out the fields in this way doesn't work;
    // seems we must set a valid metric for the first metric field
    ipfwdrow.dwForwardMetric1 = ipfwdrow.dwForwardMetric2 =
	ipfwdrow.dwForwardMetric3 = ipfwdrow.dwForwardMetric4 =
	ipfwdrow.dwForwardMetric5 = MIB_IPROUTE_METRIC_UNUSED;
#endif
#if 0 // XXX: Not enough arguments in debug_msg for this debugging message:
    XLOG_INFO("ipfwdrow: %08lx/%08lx pol %lu nh %08lx index %lu t %lu p %lu a %lu as %lu metrics %lu %lu %lu %lu %lu", ipfwdrow.dwForwardDest, ipfwdrow.dwForwardMask, ipfwdrow.dwForwardPolicy, ipfwdrow.dwForwardNextHop, ipfwdrow.dwForwardIfIndex, ipfwdrow.dwForwardType, ipfwdrow.dwForwardProto, ipfwdrow.dwForwardAge, ipfwdrow.dwForwardNextHopAS, ipfwdrow.dwForwardMetric1, ipfwdrow.dwForwardMetric2, ipfwdrow.dwForwardMetric3, ipfwdrow.dwForwardMetric4, ipfwdrow.dwForwardMetric5);
#endif

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
FibConfigEntrySetIPHelper::delete_entry(const FteX& fte)
{
    MIB_IPFORWARDROW	ipfwdrow;
    int			family = fte.net().af();

    debug_msg("delete_entry (network = %s nexthop = %s)\n",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! fibconfig().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! fibconfig().have_ipv6())
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
    // Fully fill out the MIB_IPFORWARDROW members in order.
    //

    memset(&ipfwdrow, 0, sizeof(ipfwdrow));

    fte.net().masked_addr().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
						&ipfwdrow.dwForwardDest));
    fte.net().netmask().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
					    &ipfwdrow.dwForwardMask));

    ipfwdrow.dwForwardPolicy = 0;

    fte.nexthop().get_ipv4().copy_out(reinterpret_cast<uint8_t*>(
					    &ipfwdrow.dwForwardNextHop));

    //
    // If the RIB did not tell us which interface this route was
    // previously added on, there is a programming error.
    //
    XLOG_ASSERT(!fte.ifname().empty());

    //
    // Look up the interface in the live IfTree.
    // The FIB needs to know the physical IfIndex for removal.
    // If the FEA does not know about this interface, there is a
    // programming error.
    //
    const IfTree& iftree = fibconfig().iftree();
    const IfTreeInterface* ifp = iftree.find_interface(fte.ifname());
    XLOG_ASSERT(ifp != NULL);
    ipfwdrow.dwForwardIfIndex = ifp->pif_index();

    //
    // The FIB fills out the dwForwardType field strictly according
    // to RFC 1354. Because we don't specify a specific ForwardType
    // on creation, specify the non-specific manifest constant
    // MIB_IPROUTE_TYPE_OTHER when removing the route.
    //
    ipfwdrow.dwForwardType = MIB_IPROUTE_TYPE_OTHER;
    ipfwdrow.dwForwardProto = PROTO_IP_NETMGMT;

    //
    // It seems to be OK to ignore the age field when deleting an
    // PROTO_IP_NETMGMT route.
    //
    ipfwdrow.dwForwardAge = 0;
    ipfwdrow.dwForwardNextHopAS = 0;

    //
    // Don't specify any of the metric fields.
    //
    // We are supposed to specify the dwForwardMetric1 field for
    // PROTO_IP_NETMGMT and leave the others as MIB_IPROUTE_METRIC_UNUSED.
    //
    // Currently we don't do this, as to do so would require maintaining
    // multipath state here. We don't specify it for FIB entry creation-
    // so Windows will pick the metric from the interface's default metric.
    //
    ipfwdrow.dwForwardMetric1 = ipfwdrow.dwForwardMetric2 =
	ipfwdrow.dwForwardMetric3 = ipfwdrow.dwForwardMetric4 =
	ipfwdrow.dwForwardMetric5 = MIB_IPROUTE_METRIC_UNUSED;

    // XXX: debug_msg() does not support >=12 arguments at this time
    debug_msg("DeleteIpForwardEntry(%p) : %08lx %08lx %lu %08lx %08lx %lu "
	      "%lu %lu %lu %lu\n",
	      &ipfwdrow,
	      ipfwdrow.dwForwardDest,
	      ipfwdrow.dwForwardMask,
	      ipfwdrow.dwForwardPolicy,
	      ipfwdrow.dwForwardNextHop,
	      ipfwdrow.dwForwardIfIndex,
	      ipfwdrow.dwForwardType,		// format string break above
	      ipfwdrow.dwForwardProto,
	      ipfwdrow.dwForwardAge,
	      ipfwdrow.dwForwardNextHopAS,
	      ipfwdrow.dwForwardMetric1);
    
    DWORD result = DeleteIpForwardEntry(&ipfwdrow);
    if (result != NO_ERROR) {
	XLOG_ERROR("DeleteIpForwardEntry() failed, error: %s\n",
		   win_strerror(result));
	return false;
    }

    return true;
}

#endif // HOST_OS_WINDOWS
