// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif

// XXX: Missing from MinGW just now (iprtrmib.h definitions, and routprot.h)

#ifndef PROTO_IP_NETMGMT
#define PROTO_IP_NETMGMT	3
#endif

#ifndef MIB_IPROUTE_TYPE_DIRECT
#define MIB_IPROUTE_TYPE_DIRECT	3
#endif

#include "fticonfig.hh"
#include "fticonfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//


FtiConfigTableGetIPHelper::FtiConfigTableGetIPHelper(FtiConfig& ftic)
    : FtiConfigTableGet(ftic)
{
#ifdef HOST_OS_WINDOWS
    register_ftic_primary();
#endif
}

FtiConfigTableGetIPHelper::~FtiConfigTableGetIPHelper()
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
FtiConfigTableGetIPHelper::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);

    UNUSED(error_msg);
}

int
FtiConfigTableGetIPHelper::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);

    UNUSED(error_msg);
}

bool
FtiConfigTableGetIPHelper::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET, ftex_list) != true)
	return false;
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(ftex.get_fte4());
    }
    
    return true;
}

bool
FtiConfigTableGetIPHelper::get_table6(list<Fte6>& fte_list)
{
    UNUSED(fte_list);
    return false;
}

#ifndef HOST_OS_WINDOWS

bool
FtiConfigTableGetIPHelper::get_table(int , list<FteX>& )
{
    return false;
}

#else // HOST_OS_WINDOWS

bool
FtiConfigTableGetIPHelper::get_table(int family, list<FteX>& fte_list)
{
    // Check that the family is supported
    switch(family) {
    case AF_INET:
	if (! ftic().have_ipv4())
	    return false;
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

	if (pFwdTable != NULL)
	    free(pFwdTable);

	return false;
    }

    IPv4 dst_addr;
    IPv4 dst_mask;
    IPv4 nexthop_addr;
    bool xorp_route;

    for (unsigned int i = 0; i < pFwdTable->dwNumEntries; i++) {
	dst_addr.copy_in((uint8_t*)&(pFwdTable->table[i].dwForwardDest));
	dst_mask.copy_in((uint8_t*)&(pFwdTable->table[i].dwForwardMask));
	nexthop_addr.copy_in((uint8_t*)&(pFwdTable->table[i].dwForwardNextHop));

	// XXX: No easy way of telling XORP routes apart from manually
	// added statics in NT.
	if (pFwdTable->table[i].dwForwardProto == PROTO_IP_NETMGMT)
	    xorp_route = true;

	// TODO: define default routing metric and admin distance.
	char if_name[IFNAMSIZ];
	if (NULL == if_indextoname(pFwdTable->table[i].dwForwardIfIndex,
				   if_name)) {
	    XLOG_FATAL("if_indextoname() failed\n");
	}
	Fte4 fte4 = Fte4(IPv4Net(dst_addr, dst_mask.mask_len()), nexthop_addr,
		   if_name, if_name, 0xffff, 0xffff, xorp_route);

	// If the next hop is the final destination, then consider
	// the route to be a 'connected' route.
	if (pFwdTable->table[i].dwForwardType == MIB_IPROUTE_TYPE_DIRECT)
	    fte4.mark_connected_route();

	debug_msg("adding entry %s\n", fte4.str().c_str());

	fte_list.push_back(fte4);
    }

    free(pFwdTable);

    return true;
}

#endif // HOST_OS_WINDOWS
