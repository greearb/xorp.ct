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

#ident "$XORP: xorp/fea/ifconfig_set.cc,v 1.17 2004/11/05 00:48:03 bms Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#include <net/if.h>

#include "ifconfig.hh"
#include "ifconfig_set.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//


IfConfigSet::IfConfigSet(IfConfig& ifc)
    : _is_running(false),
      _ifc(ifc),
      _is_primary(true)
{
    
}

IfConfigSet::~IfConfigSet()
{
    
}

void
IfConfigSet::register_ifc_primary()
{
    _ifc.register_ifc_set_primary(this);
}

void
IfConfigSet::register_ifc_secondary()
{
    _ifc.register_ifc_set_secondary(this);
}

bool
IfConfigSet::push_config(const IfTree& it)
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;

    // Clear errors associated with error reporter
    ifc().er().reset();

    //
    // Sanity check config - bail on bad interface and bad vif names
    //
    for (ii = it.ifs().begin(); ii != it.ifs().end(); ++ii) {
	const IfTreeInterface& i = ii->second;
	if (ifc().get_insert_ifindex(i.ifname()) == 0) {
	    ifc().er().interface_error(i.ifname(), "interface not recognized");
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return false;
	}
	for (vi = i.vifs().begin(); i.vifs().end() != vi; ++vi) {
	    const IfTreeVif& v= vi->second;
	    if (v.vifname() != i.ifname()) {
		ifc().er().vif_error(i.ifname(), v.vifname(), "bad vif name");
		XLOG_ERROR(ifc().er().last_error().c_str());
		return false;
	    }
	}
    }

    //
    // Walk config
    //
    push_iftree_begin();
    for (ii = it.ifs().begin(); ii != it.ifs().end(); ++ii) {
	const IfTreeInterface& i = ii->second;

	// Soft interfaces and their child nodes should never be pushed.
	if (i.is_soft())
	    continue;

	push_interface_begin(i);

	for (vi = i.vifs().begin(); vi != i.vifs().end(); ++vi) {
	    const IfTreeVif& v = vi->second;

	    push_vif_begin(i, v);

	    IfTreeVif::V4Map::const_iterator a4i;
	    for (a4i = v.v4addrs().begin(); a4i != v.v4addrs().end(); ++a4i) {
		const IfTreeAddr4& a = a4i->second;
		if (a.state() != IfTreeItem::NO_CHANGE)
		    push_vif_address(i, v, a);
	    }

#ifdef HAVE_IPV6
	    IfTreeVif::V6Map::const_iterator a6i;
	    for (a6i = v.v6addrs().begin(); a6i != v.v6addrs().end(); ++a6i) {
		const IfTreeAddr6& a = a6i->second;
		if (a.state() != IfTreeItem::NO_CHANGE)
		    push_vif_address(i, v, a);
	    }
#endif // HAVE_IPV6

	    push_vif_end(i, v);
	}

	push_interface_end(i);
    }
    push_iftree_end();

    if (ifc().er().error_count() != 0)
	return false;
    
    return true;
}

void
IfConfigSet::push_iftree_begin()
{
    string errmsg;

    //
    // Begin the configuration
    //
    do {
	if (config_begin(errmsg) < 0) {
	    errmsg = c_format("Failed to begin configuration: %s",
			      errmsg.c_str());
	    ifc().er().config_error(errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_iftree_end()
{
    string errmsg;

    //
    // End the configuration
    //
    do {
	if (config_end(errmsg) < 0) {
	    errmsg = c_format("Failed to end configuration: %s",
			      errmsg.c_str());
	    ifc().er().config_error(errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_interface_begin(const IfTreeInterface& i)
{
    string errmsg;
    uint16_t if_index = ifc().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Add the interface
    //
    do {
	if (add_interface(i.ifname(), if_index, errmsg) < 0) {
	    errmsg = c_format("Failed to add interface: %s",
			      errmsg.c_str());
	    ifc().er().interface_error(i.ifname(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_interface_end(const IfTreeInterface& i)
{
    string errmsg;
    uint16_t if_index = ifc().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Set the flags
    //
    do {
	uint32_t pulled_flags = 0;
	uint32_t new_flags;
	bool pulled_up, new_up, deleted, enabled;
	IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());

	deleted = i.is_marked(IfTreeItem::DELETED);
	enabled = i.enabled();

	// Get the flags from the pulled config
	if (ii != ifc().pulled_config().ifs().end())
	    pulled_flags = ii->second.if_flags();
	new_flags = pulled_flags;

	pulled_up = pulled_flags & IFF_UP;
	if (pulled_up && (deleted || !enabled))
	    new_flags &= ~IFF_UP;
	if ( (!pulled_up) && enabled)
	    new_flags |= IFF_UP;

	if (is_primary() && (new_flags == pulled_flags))
	    break;		// XXX: nothing changed

	new_up = (new_flags & IFF_UP)? true : false;
	if (config_interface(i.ifname(), if_index, new_flags, new_up,
			     deleted, errmsg)
	    < 0) {
	    errmsg = c_format("Failed to configure interface: %s",
			      errmsg.c_str());
	    ifc().er().interface_error(i.ifname(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);

    //
    // Set the MTU
    //
    do {
	uint32_t new_mtu = i.mtu();
	uint32_t pulled_mtu = 0;
	IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());

	if (ii != ifc().pulled_config().ifs().end())
	    pulled_mtu = ii->second.mtu();

	if (is_secondary() && (new_mtu == 0)) {
	    // Get the MTU from the pulled config
	    new_mtu = pulled_mtu;
	}

	if (new_mtu == 0)
	    break;		// XXX: ignore the MTU setup

	if (is_primary() && (new_mtu == pulled_mtu))
	    break;		// Ignore: the MTU hasn't changed

	if (set_interface_mtu(i.ifname(), if_index, new_mtu, errmsg) < 0) {
	    errmsg = c_format("Failed to set MTU to %u bytes: %s",
			      new_mtu, errmsg.c_str());
	    ifc().er().interface_error(i.name(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);

    //
    // Set the MAC address
    //
    do {
	Mac new_mac = i.mac();
	Mac pulled_mac;
	IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());

	if (ii != ifc().pulled_config().ifs().end())
	    pulled_mac = ii->second.mac();

	if (is_secondary() && new_mac.empty()) {
	    // Get the MAC from the pulled config
	    new_mac = pulled_mac;
	}

	if (new_mac.empty())
	    break;		// XXX: ignore the MAC setup

	if (is_primary() && (new_mac == pulled_mac))
	    break;		// Ignore: the MTU hasn't changed
	
	if (is_primary()
	    && (ii != ifc().pulled_config().ifs().end())
	    && ii->second.enabled()) {
	    //
	    // XXX: we don't allow the set the MAC address if the interface
	    // is not DOWN.
	    //
	    errmsg = c_format("Cannot set Ethernet MAC address: "
			      "the interface is not DOWN");
	    ifc().er().interface_error(i.name(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}

	struct ether_addr ea;
	try {
	    EtherMac em;
	    em = EtherMac(new_mac);
	    if (em.get_ether_addr(ea) == false) {
		errmsg = c_format("Expected Ethernet MAC address, got \"%s\"",
				  new_mac.str().c_str());
		ifc().er().interface_error(i.name(), errmsg);
		XLOG_ERROR(ifc().er().last_error().c_str());
		return;
	    }
	} catch (const BadMac& bm) {
	    errmsg = c_format("Invalid MAC address \"%s\"",
			      new_mac.str().c_str());
	    ifc().er().interface_error(i.name(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}

	if (set_interface_mac_address(i.ifname(), if_index, ea, errmsg) < 0) {
	    errmsg = c_format("Failed to set the MAC address: %s",
			      errmsg.c_str());
	    ifc().er().interface_error(i.name(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}

	break;
    } while(false);
}

void
IfConfigSet::push_vif_begin(const IfTreeInterface&	i,
			  const IfTreeVif&		v)
{
    string errmsg;
    uint16_t if_index = ifc().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Add the vif
    //
    do {
	if (add_vif(i.ifname(), v.vifname(), if_index, errmsg) < 0) {
	    errmsg = c_format("Failed to add vif: %s", errmsg.c_str());
	    ifc().er().vif_error(i.ifname(), v.vifname(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_vif_end(const IfTreeInterface&	i,
			  const IfTreeVif&		v)
{
    string errmsg;
    uint16_t if_index = ifc().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Set the flags
    //
    do {
	uint32_t pulled_flags = 0;
	uint32_t new_flags;
	bool pulled_up, new_up, deleted, enabled;
	bool pulled_broadcast = false;
	bool pulled_loopback = false;
	bool pulled_point_to_point = false;
	bool pulled_multicast = false;
	bool new_broadcast = false;
	bool new_loopback = false;
	bool new_point_to_point = false;
	bool new_multicast = false;
	IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());

	deleted = (i.is_marked(IfTreeItem::DELETED) |
		   v.is_marked(IfTreeItem::DELETED));
	enabled = i.enabled() & v.enabled();

	// Get the flags from the pulled config
	if (ii != ifc().pulled_config().ifs().end())
	    pulled_flags = ii->second.if_flags();
	new_flags = pulled_flags;

	pulled_up = pulled_flags & IFF_UP;
	if (pulled_up && (deleted || !enabled))
	    new_flags &= ~IFF_UP;
	if ( (!pulled_up) && enabled)
	    new_flags |= IFF_UP;

	if (is_primary() && (new_flags == pulled_flags))
	    break;		// XXX: nothing changed

	// Set the vif flags
	if (ii != ifc().pulled_config().ifs().end()) {
	    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(v.vifname());
	    if (vi != ii->second.vifs().end()) {
		pulled_broadcast = vi->second.broadcast();
		pulled_loopback = vi->second.loopback();
		pulled_point_to_point = vi->second.point_to_point();
		pulled_multicast = vi->second.multicast();
	    }
	}
	if (is_primary()) {
	    new_broadcast = v.broadcast();
	    new_loopback = v.loopback();
	    new_point_to_point = v.point_to_point();
	    new_multicast = v.multicast();
	}
	if (is_secondary()) {
	    new_broadcast = pulled_broadcast;
	    new_loopback = pulled_loopback;
	    new_point_to_point = pulled_point_to_point;
	    new_multicast = pulled_multicast;
	}

	new_up = (new_flags & IFF_UP)? true : false;
	if (config_vif(i.ifname(), v.vifname(), if_index, new_flags, new_up,
		       deleted, new_broadcast, new_loopback,
		       new_point_to_point, new_multicast, errmsg)
	    < 0) {
	    errmsg = c_format("Failed to configure vif: %s", errmsg.c_str());
	    ifc().er().vif_error(i.ifname(), v.vifname(), errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_vif_address(const IfTreeInterface&	i,
			      const IfTreeVif&		v,
			      const IfTreeAddr4&	a)
{
    string errmsg;
    uint16_t if_index = ifc().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    debug_msg("Pushing %s\n", a.str().c_str());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// XXX:
	// A created but disabled address will not appear in the live
	// config that was read from the kernel.
	// 
	// This is a lot of work!
	//
	IfTree::IfMap::iterator ii = ifc().live_config().get_if(i.ifname());
	XLOG_ASSERT(ii != ifc().live_config().ifs().end());
	IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(v.vifname());
	XLOG_ASSERT(vi != ii->second.vifs().end());
	vi->second.add_addr(a.addr());
	IfTreeVif::V4Map::iterator ai = vi->second.get_addr(a.addr());
	ai->second = a;
	return;
    }

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED) |
		    a.is_marked(IfTreeItem::DELETED));

    if (deleted || !enabled) {
	if (delete_vif_address(i.ifname(), v.vifname(), if_index,
			       IPvX(a.addr()), a.prefix_len(), errmsg)
	    < 0) {
	    errmsg = c_format("Failed to delete address: %s", errmsg.c_str());
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	return;
    }

    //
    // Set the address
    //
    do {
	IPv4 oaddr(IPv4::ZERO());
	if (a.broadcast())
	    oaddr = a.bcast();
	else if (a.point_to_point())
	    oaddr = a.endpoint();
	
	uint32_t prefix_len = a.prefix_len();
	
	const IfTreeAddr4* ap = NULL;
	do {
	    IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());
	    if (ii == ifc().pulled_config().ifs().end())
		break;
	    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(v.vifname());
	    if (vi == ii->second.vifs().end())
		break;
	    IfTreeVif::V4Map::const_iterator ai = vi->second.get_addr(a.addr());
	    if (ai == vi->second.v4addrs().end())
		break;
	    ap = &ai->second;
	    break;
	} while (false);

	// Test if a new address
	bool new_address = true;
	do {
	    if (is_secondary())
		break;
	    if (ap == NULL)
		break;
	    if (ap->addr() != a.addr())
		break;
	    if (a.broadcast() && (ap->bcast() != a.bcast()))
		break;
	    if (a.point_to_point() && (ap->endpoint() != a.endpoint()))
		break;
	    if (ap->prefix_len() != prefix_len)
		break;
	    new_address = false;
	    break;
	} while (false);
	if (! new_address)
	    break;		// Ignore: the address hasn't changed

	if (add_vif_address(i.ifname(), v.vifname(), if_index, a.broadcast(),
			    a.point_to_point(), IPvX(a.addr()), IPvX(oaddr),
			    prefix_len, errmsg)
	    < 0) {
	    errmsg = c_format("Failed to configure address: %s",
			      errmsg.c_str());
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	
	break;
    } while (false);
}

#ifdef HAVE_IPV6
void
IfConfigSet::push_vif_address(const IfTreeInterface&	i,
			      const IfTreeVif&		v,
			      const IfTreeAddr6&	a)
{
    string errmsg;
    uint16_t if_index = ifc().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// A created but disabled address will not appear in the live
	// config rippled up from the kernel via the routing socket
	// 
	// This is a lot of work!
	//
	IfTree::IfMap::iterator ii = ifc().live_config().get_if(i.ifname());
	XLOG_ASSERT(ii != ifc().live_config().ifs().end());
	IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(v.vifname());
	XLOG_ASSERT(vi != ii->second.vifs().end());
	vi->second.add_addr(a.addr());
	IfTreeVif::V6Map::iterator ai = vi->second.get_addr(a.addr());
	ai->second = a;
	return;
    }

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED) |
		    a.is_marked(IfTreeItem::DELETED));

    if (deleted || !enabled) {
	if (delete_vif_address(i.ifname(), v.vifname(), if_index, a.addr(),
			       a.prefix_len(), errmsg)
	    < 0) {
	    errmsg = c_format("Failed to delete address: %s", errmsg.c_str());
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	return;
    }

    //
    // Set the address
    //
    do {
	IPv6 oaddr(IPv6::ZERO());
	if (a.point_to_point())
	    oaddr = a.endpoint();
	
	// XXX: for whatever reason a prefix length of zero does not cut it, so
	// initialize prefix to 64.  This is exactly as ifconfig does.
	uint32_t prefix_len = a.prefix_len();
	if (0 == prefix_len)
	    prefix_len = 64;

	const IfTreeAddr6* ap = NULL;
	do {
	    IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());
	    if (ii == ifc().pulled_config().ifs().end())
		break;
	    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(v.vifname());
	    if (vi == ii->second.vifs().end())
		break;
	    IfTreeVif::V6Map::const_iterator ai = vi->second.get_addr(a.addr());
	    if (ai == vi->second.v6addrs().end())
		break;
	    ap = &ai->second;
	    break;
	} while (false);

	// Test if a new address
	bool new_address = true;
	do {
	    if (is_secondary())
		break;
	    if (ap == NULL)
		break;
	    if (ap->addr() != a.addr())
		break;
	    if (a.point_to_point() && (ap->endpoint() != a.endpoint()))
		break;
	    if (ap->prefix_len() != prefix_len)
		break;
	    new_address = false;
	    break;
	} while (false);
	if (! new_address)
	    break;		// Ignore: the address hasn't changed

	if (add_vif_address(i.ifname(), v.vifname(), if_index, false,
			    a.point_to_point(), IPvX(a.addr()), IPvX(oaddr),
			    prefix_len, errmsg)
	    < 0) {
	    errmsg = c_format("Failed to configure address: %s",
			      errmsg.c_str());
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     errmsg);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	
	break;
    } while (false);
}
#endif // HAVE_IPV6
