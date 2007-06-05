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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_dummy.cc,v 1.5 2007/05/23 04:08:24 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#include "fea/ifconfig.hh"
#include "ifconfig_set_dummy.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is dummy (for testing purpose).
//

IfConfigSetDummy::IfConfigSetDummy(IfConfig& ifconfig)
    : IfConfigSet(ifconfig)
{
#if 0	// XXX: by default Dummy is never registering by itself
    ifconfig.register_ifconfig_set_primary(this);
#endif
}

IfConfigSetDummy::~IfConfigSetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the dummy mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigSetDummy::push_config(IfTree& it)
{
    ifconfig().report_updates(it, true);
    ifconfig().set_live_config(it);

    return true;
}

int
IfConfigSetDummy::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    UNUSED(error_msg);

    return (XORP_OK);
}

bool
IfConfigSetDummy::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);	// TODO: return appropriate value.
}

int
IfConfigSetDummy::add_interface(const string& ifname,
				uint32_t if_index,
				string& error_msg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::add_vif(const string& ifname,
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

    return (XORP_OK);
}

int
IfConfigSetDummy::config_interface(const string& ifname,
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

    return (XORP_OK);
}

int
IfConfigSetDummy::config_vif(const string& ifname,
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
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::set_interface_mac_address(const string& ifname,
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
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::set_interface_mtu(const string& ifname,
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
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::add_vif_address(const string& ifname,
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
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetDummy::delete_vif_address(const string& ifname,
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
    UNUSED(error_msg);

    return (XORP_OK);
}
