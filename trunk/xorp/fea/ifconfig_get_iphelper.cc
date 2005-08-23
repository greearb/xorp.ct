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

#ident "$XORP: xorp/fea/ifconfig_get_iphelper.cc,v 1.2 2005/08/18 15:45:46 bms Exp $"

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

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//

// XXX: This will go away when we update our system headers.
#ifndef MIB_IPADDR_DELETED
#define MIB_IPADDR_DELETED	0x0040
#endif

IfConfigGetIPHelper::IfConfigGetIPHelper(IfConfig& ifc)
    : IfConfigGet(ifc)
{
#ifdef HOST_OS_WINDOWS
    register_ifc_primary();
#endif
}

IfConfigGetIPHelper::~IfConfigGetIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper API mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetIPHelper::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);

    UNUSED(error_msg);
}

int
IfConfigGetIPHelper::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);

    UNUSED(error_msg);
}

bool
IfConfigGetIPHelper::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HOST_OS_WINDOWS

bool
IfConfigGetIPHelper::read_config(IfTree& )
{
    return false;
}

#else // HOST_OS_WINDOWS
bool
IfConfigGetIPHelper::read_config(IfTree& it)
{
    PMIB_IFTABLE	pIfTable;
    PMIB_IPADDRTABLE	pAddrTable;
    DWORD		result, tries;
    ULONG		dwSize;

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(MIB_IFTABLE);

    do {
	pIfTable = (PMIB_IFTABLE) ((tries == 0) ? malloc(dwSize) :
				   realloc(pIfTable, dwSize));
	if (pIfTable == NULL)
	    break;
	result = GetIfTable(pIfTable, &dwSize, TRUE);
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
	XLOG_ERROR("GetIfTable(): %s\n", pMsg);
	LocalFree(pMsg);

	if (pIfTable != NULL)
	    free(pIfTable);

	return false;
    }

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(MIB_IPADDRTABLE);

    do {
	pAddrTable = (PMIB_IPADDRTABLE) ((tries == 0) ? malloc(dwSize) :
				     realloc(pAddrTable, dwSize));
	if (pAddrTable == NULL)
	    break;
	result = GetIpAddrTable(pAddrTable, &dwSize, TRUE);
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
	XLOG_ERROR("GetIfTable(): %s\n", pMsg);
	LocalFree(pMsg);

	if (pAddrTable != NULL)
	    free(pAddrTable);
	free(pIfTable);

	return false;
    }

    assert (pIfTable->dwNumEntries != 0);
    assert (pAddrTable->dwNumEntries != 0);

    bool	is_newlink;
    uint32_t	if_index;
    char	if_name[IFNAMSIZ];

    //
    // Parse MIB_IFROW.
    //
    for (unsigned int i = 0; i < pIfTable->dwNumEntries; i++) {

	if_index = pIfTable->table[i].dwIndex;
	debug_msg("interface index: %u\n", if_index);

	// No point; Windows doesn't fill out the wszName field with
	// anything useful.
	//wcstombs(if_name, pIfTable->table[i].wszName, sizeof(if_name));

	if (if_indextoname(if_index, if_name) == NULL)
	    XLOG_FATAL("if_indextoname() failed.\n");
	debug_msg("interface name: %s\n", if_name);

	ifc().map_ifindex(if_index, if_name);

	// Name
	if (it.get_if(if_name) == it.ifs().end()) {
	    it.add_if(if_name);
	    is_newlink = true;
	}
	IfTreeInterface& fi = it.get_if(if_name)->second;

	// Index
	if (is_newlink || (if_index != fi.pif_index()))
	    fi.set_pif_index(if_index);

	// MAC
	switch (pIfTable->table[i].dwType) {
	case MIB_IF_TYPE_LOOPBACK:
	    break;
	case MIB_IF_TYPE_ETHERNET:
	    if (pIfTable->table[i].dwPhysAddrLen == sizeof(struct ether_addr)) {
		struct ether_addr ea;
		memcpy(&ea, pIfTable->table[i].bPhysAddr, sizeof(ea));
		EtherMac ether_mac(ea);
		if (is_newlink || (ether_mac != EtherMac(fi.mac())))
		    fi.set_mac(ether_mac);
		break;
	    } else
		; /* FALLTHROUGH */
	default:
	    XLOG_ERROR("Address size %d uncatered for interface %s",
		       (int)pIfTable->table[i].dwPhysAddrLen, if_name);
	    break;
	}

	// MTU
	uint32_t mtu = pIfTable->table[i].dwMtu;
	if (is_newlink || (mtu != fi.mtu()))
	    fi.set_mtu(mtu);

	// XXX: Always set enclosing ifname as enabled.
	fi.set_enabled(true);

	// XXX: vifname == ifname on this platform
	if (is_newlink)
	    fi.add_vif(if_name);
	IfTreeVif& fv = fi.get_vif(if_name)->second;

	// Flags
	// XXX: We don't grok multicast yet.
	fv.set_enabled(fi.enabled() && (pIfTable->table[i].dwAdminStatus ==
		       MIB_IF_ADMIN_STATUS_UP));
	fv.set_broadcast(pIfTable->table[i].dwType ==
		       MIB_IF_TYPE_ETHERNET);
	fv.set_loopback(pIfTable->table[i].dwType ==
		       MIB_IF_TYPE_LOOPBACK);
	fv.set_point_to_point(pIfTable->table[i].dwType ==
		       MIB_IF_TYPE_PPP);
	fv.set_multicast(false);

	// IPv4 addresses

	// TODO: Implement GetAdaptersAddresses() instead of
	// performing a linear scan of the IP address MIB (as this
	// limits us to IPv4 unicast addresses only).

	// XXX: Currently there is no way of discovering the peer
	// address for a point-to-point interface other than
	// looking for connected routes in Windows' FIB.

	IPv4 lcl_addr;
	IPv4 subnet_mask;
	IPv4 broadcast_addr;

	// XXX: wType is unused2 in MinGW's headers.

	for (unsigned int j = 0; j < pAddrTable->dwNumEntries; j++) {
	    if (if_index == pAddrTable->table[j].dwIndex &&
	        !(pAddrTable->table[j].unused2 & MIB_IPADDR_DELETED)) {

		lcl_addr.copy_in((uint8_t *)&(pAddrTable->table[j].dwAddr));
		fv.add_addr(lcl_addr);
		IfTreeAddr4& fa = fv.get_addr(lcl_addr)->second;

		fa.set_enabled(fv.enabled());
		fa.set_broadcast(fv.broadcast());
		fa.set_loopback(fv.loopback());
		fa.set_point_to_point(fv.point_to_point());
		fa.set_multicast(false);

		subnet_mask.copy_in((uint8_t*)&(pAddrTable->table[j].dwMask));
		fa.set_prefix_len(subnet_mask.mask_len());

		if (fa.broadcast()) {
		    // Workaround for Windows bug: infer broadcast address
		    // from subnet mask and network address.
		    //
		    // WINE source says that dwBCastAddr specifies whether the
		    // broadcast address is all-1's or all-0's in host
		    // portion, *NOT* the address itself.
		    broadcast_addr = lcl_addr & subnet_mask;
		    if (pAddrTable->table[j].dwBCastAddr != 0x00000000)
			broadcast_addr = broadcast_addr | ~subnet_mask;
		    fa.set_bcast(broadcast_addr);
		}
	    }
	}
    }

    free(pIfTable);
    free(pAddrTable);

    return true;
}

#endif // HOST_OS_WINDOWS
