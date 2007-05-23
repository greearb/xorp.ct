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

#ident "$XORP: xorp/fea/forwarding_plane/ifconfig/ifconfig_set_iphelper.cc,v 1.4 2007/04/30 23:40:34 pavlin Exp $"

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

#include "fea/ifconfig.hh"
#include "fea/ifconfig_get.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//


IfConfigSetIPHelper::IfConfigSetIPHelper(IfConfig& ifconfig)
    : IfConfigSet(ifconfig)
{
#ifdef HOST_OS_WINDOWS
    ifconfig.register_ifconfig_set_primary(this);
#endif
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


#ifndef HOST_OS_WINDOWS
int
IfConfigSetIPHelper::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::add_interface(const string& ifname,
				   uint32_t if_index,
				   string& error_msg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(if_index);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::add_vif(const string& ifname,
			     const string& vifname,
			     uint32_t if_index,
			     string& error_msg)
{
    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::config_interface(const string& ifname,
				      uint32_t if_index,
				      uint32_t flags,
				      bool is_up,
				      bool is_deleted,
				      string& error_msg)
{
    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index,
	      XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted));
    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::config_vif(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				uint32_t flags,
				bool is_up,
				bool is_deleted,
				bool broadcast,
				bool loopback,
				bool point_to_point,
				bool multicast,
				string& error_msg)
{
    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted),
	      bool_c_str(broadcast),
	      bool_c_str(loopback),
	      bool_c_str(point_to_point),
	      bool_c_str(multicast));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);
    UNUSED(broadcast);
    UNUSED(loopback);
    UNUSED(point_to_point);
    UNUSED(multicast);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::set_interface_mac_address(const string& ifname,
					       uint32_t if_index,
					       const struct ether_addr& ether_addr,
					       string& error_msg)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(ether_addr);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::set_interface_mtu(const string& ifname,
				       uint32_t if_index,
				       uint32_t mtu,
				       string& error_msg)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, XORP_UINT_CAST(mtu));

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(mtu);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::add_vif_address(const string& ifname,
				     const string& vifname,
				     uint32_t if_index,
				     bool is_broadcast,
				     bool is_p2p,
				     const IPvX& addr,
				     const IPvX& dst_or_bcast,
				     uint32_t prefix_len,
				     string& error_msg)
{
    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast), bool_c_str(is_p2p),
	      addr.str().c_str(), dst_or_bcast.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_broadcast);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst_or_bcast);
    UNUSED(prefix_len);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::add_vif_address4(const string& ifname,
				      const string& vifname,
				      uint32_t if_index,
				      bool is_broadcast,
				      bool is_p2p,
				      const IPvX& addr,
				      const IPvX& dst_or_bcast,
				      uint32_t prefix_len,
				      string& error_msg)
{
    debug_msg("add_vif_address4 "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast), bool_c_str(is_p2p),
	      addr.str().c_str(), dst_or_bcast.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_broadcast);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst_or_bcast);
    UNUSED(prefix_len);
    UNUSED(error_msg);

    return (XORP_ERROR);
};

int
IfConfigSetIPHelper::add_vif_address6(const string& ifname,
				      const string& vifname,
				      uint32_t if_index,
				      bool is_p2p,
				      const IPvX& addr,
				      const IPvX& dst,
				      uint32_t prefix_len,
				      string& error_msg)
{
    debug_msg("add_vif_address6 "
	      "(ifname = %s vifname = %s if_index = %u is_p2p = %s "
	      "addr = %s dst = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_p2p), addr.str().c_str(),
	      dst.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst);
    UNUSED(prefix_len);
    UNUSED(error_msg);

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::delete_vif_address(const string& ifname,
					const string& vifname,
					uint32_t if_index,
					const IPvX& addr,
					uint32_t prefix_len,
					string& error_msg)
{
    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

#else // HOST_OS_WINDOWS

int
IfConfigSetIPHelper::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    UNUSED(error_msg);

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    UNUSED(error_msg);

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIPHelper::add_interface(const string& ifname,
				   uint32_t if_index,
				   string& error_msg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(error_msg);

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIPHelper::add_vif(const string& ifname,
			     const string& vifname,
			     uint32_t if_index,
			     string& error_msg)
{
    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(error_msg);

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_interface(const string& ifname,
				      uint32_t if_index,
				      uint32_t flags,
				      bool is_up,
				      bool is_deleted,
				      string& error_msg)
{
    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index,
	      XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted));

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);
    UNUSED(error_msg);

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIPHelper::config_vif(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				uint32_t flags,
				bool is_up,
				bool is_deleted,
				bool broadcast,
				bool loopback,
				bool point_to_point,
				bool multicast,
				string& error_msg)
{
    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted),
	      bool_c_str(broadcast),
	      bool_c_str(loopback),
	      bool_c_str(point_to_point),
	      bool_c_str(multicast));

#if 0
    MIB_IFROW ifrow;
    DWORD result;

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(flags);
    UNUSED(is_deleted);
    UNUSED(broadcast);
    UNUSED(loopback);
    UNUSED(point_to_point);
    UNUSED(multicast);

    memset(&ifrow, 0, sizeof(ifrow));
    ifrow.dwIndex = if_index;

    result = GetIfEntry(&ifrow);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot obtain existing MIB_IFROW for "
			     "interface %u: error %d\n", if_index,
			     (int)result);
	return (XORP_ERROR);
    }
    ifrow.dwAdminStatus = is_up ? MIB_IF_ADMIN_STATUS_UP :
				  MIB_IF_ADMIN_STATUS_DOWN;
    result = SetIfEntry(&ifrow);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot set administrative status of "
			     "interface %u: error %d\n", if_index,
			     (int)result);
	return (XORP_ERROR);
    }
#else
    UNUSED(error_msg);
#endif

    return (XORP_OK);
}

int
IfConfigSetIPHelper::set_interface_mac_address(const string& ifname,
					       uint32_t if_index,
					       const struct ether_addr& ether_addr,
					       string& error_msg)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(ether_addr);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::set_interface_mtu(const string& ifname,
				       uint32_t if_index,
				       uint32_t mtu,
				       string& error_msg)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, XORP_UINT_CAST(mtu));

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(mtu);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::add_vif_address(const string& ifname,
				     const string& vifname,
				     uint32_t if_index,
				     bool is_broadcast,
				     bool is_p2p,
				     const IPvX& addr,
				     const IPvX& dst_or_bcast,
				     uint32_t prefix_len,
				     string& error_msg)
{
    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast), bool_c_str(is_p2p),
	      addr.str().c_str(), dst_or_bcast.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    switch (addr.af()) {
    case AF_INET:
	return add_vif_address4(ifname, vifname, if_index, is_broadcast,
	is_p2p, addr, dst_or_bcast, prefix_len, error_msg);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	return add_vif_address6(ifname, vifname, if_index, is_p2p,
	addr, dst_or_bcast, prefix_len, error_msg);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::add_vif_address4(const string& ifname,
				      const string& vifname,
				      uint32_t if_index,
				      bool is_broadcast,
				      bool is_p2p,
				      const IPvX& addr,
				      const IPvX& dst_or_bcast,
				      uint32_t prefix_len,
				      string& error_msg)
{
    debug_msg("add_vif_address4 "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast), bool_c_str(is_p2p),
	      addr.str().c_str(), dst_or_bcast.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    PMIB_IPADDRTABLE pAddrTable;
    DWORD       result, tries;
    ULONG       dwSize;
    IPAddr	ipaddr;
    IPMask	ipmask;

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(is_broadcast);
    UNUSED(is_p2p);
    UNUSED(dst_or_bcast);

    addr.copy_out((uint8_t*)&ipaddr);
    IPvX::make_prefix(addr.af(), prefix_len).get_ipv4().copy_out(
	(uint8_t*)&ipmask);

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
        XLOG_ERROR("GetIpAddrTable(): %s\n", win_strerror(result));
        if (pAddrTable != NULL)
            free(pAddrTable);
        return XORP_OK;
    }

    for (unsigned int i = 0; i < pAddrTable->dwNumEntries; i++) {
	if (pAddrTable->table[i].dwAddr == ipaddr) {
	    XLOG_ERROR("IP address already exists on interface with index %lu.",
			pAddrTable->table[i].dwIndex);
            return XORP_OK;
	}
    }

    ULONG	ntectx = 0, nteinst = 0;

    result = AddIPAddress(ipaddr, ipmask, if_index, &ntectx, &nteinst);
    if (result != NO_ERROR) {
	error_msg = c_format("Cannot add IP address to interface %u: "
			     "error %d\n", if_index, (int)result);
	return XORP_OK;
    }

    // We can't delete IP addresses using IP Helper unless we cache the
    // returned NTE context. This means we can only delete addresses
    // which were created during the lifetime of the FEA.
    //
    // Also, these entries have to be keyed by the address we added,
    // so we can figure out which ntectx value to use when deleting it.
    //
    _nte_map.insert(make_pair(make_pair(if_index,
					(IPAddr)addr.get_ipv4().addr()),
					ntectx));
    return XORP_OK;
};

int
IfConfigSetIPHelper::add_vif_address6(const string& ifname,
				      const string& vifname,
				      uint32_t if_index,
				      bool is_p2p,
				      const IPvX& addr,
				      const IPvX& dst,
				      uint32_t prefix_len,
				      string& error_msg)
{
    debug_msg("add_vif_address6 "
	      "(ifname = %s vifname = %s if_index = %u is_p2p = %s "
	      "addr = %s dst = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_p2p), addr.str().c_str(),
	      dst.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst);
    UNUSED(prefix_len);
    UNUSED(error_msg);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIPHelper::delete_vif_address(const string& ifname,
					const string& vifname,
					uint32_t if_index,
					const IPvX& addr,
					uint32_t prefix_len,
					string& error_msg)
{
    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(vifname);
    UNUSED(prefix_len);

    // Check that the family is supported
    switch (addr.af()) {
    case AF_INET:
	if (! ifconfig().have_ipv4()) {
	    error_msg = "IPv4 is not supported";
	    return (XORP_ERROR);
	}
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
	if (! ifconfig().have_ipv6()) {
	    error_msg = "IPv6 is not supported";
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }

    switch (addr.af()) {
    case AF_INET:
    {
	map<pair<uint32_t, IPAddr>, ULONG>::iterator ii;
	ii = _nte_map.find(make_pair(if_index, (IPAddr)addr.get_ipv4().addr()));
	if (ii == _nte_map.end()) {
	    error_msg = c_format("Cannot find address %s in table of "
				 "previously added addresses, therefore "
				 "cannot remove using IP Helper API.\n",
				 addr.str().c_str());
	    return (XORP_ERROR);
	}
	ULONG ntectx = ii->second;

	DWORD result = DeleteIPAddress(ntectx);
	if (result != NO_ERROR) {
	    error_msg = c_format("Cannot delete address %s via IP Helper "
				 "API, error: %d\n",
				 addr.str().c_str(), (int)result);
	    return (XORP_ERROR);
	}
    }

#ifdef HAVE_IPV6
    case AF_INET6:
	error_msg = "method not supported";
	return (XORP_ERROR);
	break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }

    return (XORP_OK);
}
#endif // !HOST_OS_WINDOWS
