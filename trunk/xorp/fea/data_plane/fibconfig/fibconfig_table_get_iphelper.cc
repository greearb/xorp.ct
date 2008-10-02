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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_get_iphelper.cc,v 1.15 2008/07/23 05:10:21 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/win_io.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif
#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif
#ifdef HAVE_IPRTRMIB_H
#include <iprtrmib.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_get.hh"

#include "fibconfig_table_get_iphelper.hh"

#ifndef MIB_IPROUTE_TYPE_DIRECT
#define MIB_IPROUTE_TYPE_DIRECT		(3)
#endif


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//

#ifdef HOST_OS_WINDOWS

FibConfigTableGetIPHelper::FibConfigTableGetIPHelper(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableGet(fea_data_plane_manager)
{
}

FibConfigTableGetIPHelper::~FibConfigTableGetIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper API mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableGetIPHelper::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigTableGetIPHelper::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigTableGetIPHelper::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET, ftex_list) != XORP_OK)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(ftex.get_fte4());
    }
    
    return (XORP_OK);
}

int
FibConfigTableGetIPHelper::get_table6(list<Fte6>& fte_list)
{
    UNUSED(fte_list);
    return (XORP_ERROR);
}

int
FibConfigTableGetIPHelper::get_table(int family, list<FteX>& fte_list)
{
    // Check that the family is supported
    switch(family) {
    case AF_INET:
	if (! fea_data_plane_manager().have_ipv4())
	    return (XORP_ERROR);
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

    PMIB_IPFORWARDTABLE	pFwdTable;
    DWORD		result, tries;
    ULONG		dwSize;

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(MIB_IPFORWARDTABLE);

    do {
	pFwdTable = (PMIB_IPFORWARDTABLE) ((tries == 0) ? malloc(dwSize) :
				   realloc(pFwdTable, dwSize));
	if (pFwdTable == NULL)
	    break;
	result = GetIpForwardTable(pFwdTable, &dwSize, TRUE);
    } while ((++tries < 3) || (result == ERROR_INSUFFICIENT_BUFFER));

    if (result != NO_ERROR) {
	XLOG_ERROR("GetIpForwardTable(): %s\n", win_strerror(result));
	if (pFwdTable != NULL)
	    free(pFwdTable);
	return (XORP_ERROR);
    }

    IPv4 dst_addr;
    IPv4 dst_mask;
    IPv4 nexthop_addr;
    bool xorp_route;

    for (unsigned int i = 0; i < pFwdTable->dwNumEntries; i++) {
	PMIB_IPFORWARDROW pFwdRow = &pFwdTable->table[i];

	dst_addr.copy_in(reinterpret_cast<uint8_t*>(&pFwdRow->dwForwardDest));
	dst_mask.copy_in(reinterpret_cast<uint8_t*>(&pFwdRow->dwForwardMask));
	nexthop_addr.copy_in(reinterpret_cast<uint8_t*>(
			     &pFwdRow->dwForwardNextHop));

	// XXX: No easy way of telling XORP routes apart from manually
	// added statics in NT.
	if (pFwdRow->dwForwardProto == PROTO_IP_NETMGMT)
	    xorp_route = true;

	uint32_t ifindex = static_cast<uint32_t>(pFwdRow->dwForwardIfIndex);
	const IfTree& iftree = fibconfig().merged_config_iftree();
	const IfTreeVif* vifp = iftree.find_vif(ifindex);

	string if_name;
	string vif_name;
	if (vifp != NULL) {
	    if_name = vifp->ifname();
	    vif_name = vifp->vifname();
	} else {
	    if_name = "unknown";
	    vif_name = "unknown";
	    XLOG_WARNING("Route via unknown interface index %u",
			 XORP_UINT_CAST(ifindex));
	}

	// TODO: define default routing metric and admin distance.
	Fte4 fte4 = Fte4(IPv4Net(dst_addr, dst_mask.mask_len()), nexthop_addr,
			 if_name, vif_name, 0xffff, 0xffff, xorp_route);

	// If the next hop is the final destination, then consider
	// the route to be a 'connected' route.
	if (pFwdRow->dwForwardType == MIB_IPROUTE_TYPE_DIRECT)
	    fte4.mark_connected_route();

	debug_msg("adding entry %s\n", fte4.str().c_str());

	fte_list.push_back(fte4);
    }

    free(pFwdTable);

    return (XORP_OK);
}

#endif // HOST_OS_WINDOWS
