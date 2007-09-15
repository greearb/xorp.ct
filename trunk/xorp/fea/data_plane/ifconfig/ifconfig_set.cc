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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set.cc,v 1.9 2007/09/15 01:22:36 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/ifconfig_set.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// This is (almost) platform independent mechanism that can be
// overwritten by each system-specific implementation by
// (re)implementing the IfConfigSet::push_config() virtual method
// in the class that inherits from IfConfigSet.
//

int
IfConfigSet::push_config(IfTree& iftree)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    // Clear errors associated with error reporter
    ifconfig().ifconfig_error_reporter().reset();

    //
    // Sanity check config - bail on bad interface and bad vif names
    //
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& i = ii->second;
	//
	// Skip the ifindex check if the interface has no mapping to
	// an existing interface in the system.
	//
	if (i.is_soft() || (i.discard() && is_discard_emulated(i)))
	    continue;

	//
	// Check that the interface is recognized by the system
	//
	if (ifconfig().get_insert_ifindex(i.ifname()) == 0) {
	    if (i.state() == IfTreeItem::DELETED) {
		// XXX: ignore deleted interfaces that are not recognized
		continue;
	    }
	    ifconfig().ifconfig_error_reporter().interface_error(
		i.ifname(),
		"interface not recognized");
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return (XORP_ERROR);
	}

	//
	// Check the interface and vif name
	//
	for (vi = i.vifs().begin(); vi != i.vifs().end(); ++vi) {
	    IfTreeVif& v= vi->second;
	    if (v.is_vlan())
		continue;
	    if (v.vifname() != i.ifname()) {
		ifconfig().ifconfig_error_reporter().vif_error(i.ifname(),
							       v.vifname(),
							       "bad vif name");
		XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
		return (XORP_ERROR);
	    }
	}
    }

    //
    // Walk config
    //
    push_iftree_begin();
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& i = ii->second;

	// Set the "discard_emulated" flag if the discard interface is emulated
	if (i.discard() && is_discard_emulated(i))
	    i.set_discard_emulated(true);

	// Soft interfaces and their child nodes should never be pushed.
	if (i.is_soft())
	    continue;

	if (i.discard() && is_discard_emulated(i))
	    continue;

	if ((ifconfig().get_insert_ifindex(i.ifname()) == 0)
	    && (i.state() == IfTreeItem::DELETED)) {
		// XXX: ignore deleted interfaces that are not recognized
		continue;
	}

	push_interface_begin(i);

	for (vi = i.vifs().begin(); vi != i.vifs().end(); ++vi) {
	    IfTreeVif& v = vi->second;

	    // XXX: delete the vif if the interface is deleted
	    if (i.state() == IfTreeItem::DELETED)
		v.mark(IfTreeItem::DELETED);

	    push_vif_begin(i, v);

	    IfTreeVif::IPv4Map::iterator a4i;
	    for (a4i = v.ipv4addrs().begin(); a4i != v.ipv4addrs().end(); ++a4i) {
		IfTreeAddr4& a = a4i->second;
		// XXX: delete the address if the vif is deleted
		if (v.state() == IfTreeItem::DELETED)
		    a.mark(IfTreeItem::DELETED);
		if (a.state() != IfTreeItem::NO_CHANGE)
		    push_vif_address(i, v, a);
	    }

#ifdef HAVE_IPV6
	    IfTreeVif::IPv6Map::iterator a6i;
	    for (a6i = v.ipv6addrs().begin(); a6i != v.ipv6addrs().end(); ++a6i) {
		IfTreeAddr6& a = a6i->second;
		// XXX: delete the address if the vif is deleted
		if (v.state() == IfTreeItem::DELETED)
		    a.mark(IfTreeItem::DELETED);
		if (a.state() != IfTreeItem::NO_CHANGE)
		    push_vif_address(i, v, a);
	    }
#endif // HAVE_IPV6

	    push_vif_end(i, v);
	}

	push_interface_end(i);
    }
    push_iftree_end();

    if (ifconfig().ifconfig_error_reporter().error_count() != 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

void
IfConfigSet::push_iftree_begin()
{
    string error_msg;

    //
    // Begin the configuration
    //
    do {
	if (config_begin(error_msg) < 0) {
	    error_msg = c_format("Failed to begin configuration: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().config_error(error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_iftree_end()
{
    string error_msg;

    //
    // End the configuration
    //
    do {
	if (config_end(error_msg) < 0) {
	    error_msg = c_format("Failed to end configuration: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().config_error(error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_interface_begin(const IfTreeInterface& i)
{
    string error_msg;
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Add the interface
    //
    do {
	if (add_interface(i.ifname(), if_index, error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to add interface: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().interface_error(i.ifname(),
								 error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_interface_end(IfTreeInterface& i)
{
    string error_msg;
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);
    uint32_t new_flags = 0;
    bool new_up = false;
    bool deleted = false;

    //
    // Set the flags
    //
    do {
	uint32_t pulled_flags = 0;
	bool pulled_up, enabled;
	const IfTreeInterface* ifp;

	ifp = ifconfig().pulled_config().find_interface(i.ifname());

	deleted = i.is_marked(IfTreeItem::DELETED);
	enabled = i.enabled();

	// Get the flags from the pulled config
	if (ifp != NULL)
	    pulled_flags = ifp->interface_flags();
	new_flags = pulled_flags;

	pulled_up = pulled_flags & IFF_UP;
	if (pulled_up && (deleted || !enabled))
	    new_flags &= ~IFF_UP;
	if ( (!pulled_up) && enabled)
	    new_flags |= IFF_UP;

	new_up = (new_flags & IFF_UP)? true : false;

	if ((! is_pulled_config_mirror()) && (new_flags == pulled_flags))
	    break;		// XXX: nothing changed

	if (config_interface(i.ifname(), if_index, new_flags, new_up,
			     deleted, error_msg)
	    < 0) {
	    error_msg = c_format("Failed to configure interface: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().interface_error(i.ifname(),
								 error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return;
	}
	break;
    } while (false);

    //
    // Set the MTU
    //
    do {
	bool was_disabled = false;
	uint32_t new_mtu = i.mtu();
	uint32_t pulled_mtu = 0;
	const IfTreeInterface* ifp;

	ifp = ifconfig().pulled_config().find_interface(i.ifname());

	if (ifp != NULL)
	    pulled_mtu = ifp->mtu();

	if (is_pulled_config_mirror() && (new_mtu == 0)) {
	    // Get the MTU from the pulled config
	    new_mtu = pulled_mtu;
	}

	if (new_mtu == 0)
	    break;		// XXX: ignore the MTU setup

	if ((! is_pulled_config_mirror()) && (new_mtu == pulled_mtu))
	    break;		// Ignore: the MTU hasn't changed

	if ((ifp != NULL) && new_up) {
	    //
	    // XXX: Set the interface DOWN otherwise we may not be able to
	    // set the MTU (limitation imposed by the Linux kernel).
	    //
	    config_interface(i.ifname(), if_index, new_flags & ~IFF_UP, false,
			     deleted, error_msg);
	    was_disabled = true;
	}

	if (set_interface_mtu(i.ifname(), if_index, new_mtu, error_msg) < 0) {
	    error_msg = c_format("Failed to set MTU to %u bytes: %s",
				 XORP_UINT_CAST(new_mtu),
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().interface_error(i.ifname(),
								 error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    if (was_disabled) {
		config_interface(i.ifname(), if_index, new_flags, true,
				 deleted, error_msg);
	    }
	    return;
	}

	if (was_disabled) {
	    config_interface(i.ifname(), if_index, new_flags, true,
			     deleted, error_msg);
	    i.set_flipped(true);	// XXX: the interface was flipped
	}

	break;
    } while (false);

    //
    // Set the MAC address
    //
    do {
	bool was_disabled = false;
	Mac new_mac = i.mac();
	Mac pulled_mac;
	const IfTreeInterface* ifp;

	ifp = ifconfig().pulled_config().find_interface(i.ifname());

	if (ifp != NULL)
	    pulled_mac = ifp->mac();

	if (is_pulled_config_mirror() && new_mac.empty()) {
	    // Get the MAC from the pulled config
	    new_mac = pulled_mac;
	}

	if (new_mac.empty())
	    break;		// XXX: ignore the MAC setup

	if ((! is_pulled_config_mirror()) && (new_mac == pulled_mac))
	    break;		// Ignore: the MAC hasn't changed

	struct ether_addr ea;
	try {
	    EtherMac em;
	    em = EtherMac(new_mac);
	    if (em.copy_out(ea) != EtherMac::ADDR_BYTELEN) {
		error_msg = c_format("Expected Ethernet MAC address, "
				     "got \"%s\"",
				     new_mac.str().c_str());
		ifconfig().ifconfig_error_reporter().interface_error(i.ifname(),
								     error_msg);
		XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
		return;
	    }
	} catch (const BadMac& bm) {
	    error_msg = c_format("Invalid MAC address \"%s\"",
				 new_mac.str().c_str());
	    ifconfig().ifconfig_error_reporter().interface_error(i.ifname(),
								 error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return;
	}

	if ((ifp != NULL) && new_up) {
	    //
	    // XXX: Set the interface DOWN otherwise we may not be able to
	    // set the MAC address (limitation imposed by the Linux kernel).
	    //
	    config_interface(i.ifname(), if_index, new_flags & ~IFF_UP, false,
			     deleted, error_msg);
	    was_disabled = true;
	}

	if (set_interface_mac_address(i.ifname(), if_index, ea, error_msg)
	    < 0) {
	    error_msg = c_format("Failed to set the MAC address: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().interface_error(i.ifname(),
								 error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    if (was_disabled) {
		config_interface(i.ifname(), if_index, new_flags, true,
				 deleted, error_msg);
	    }
	    return;
	}

	if (was_disabled) {
	    config_interface(i.ifname(), if_index, new_flags, true,
			     deleted, error_msg);
	    i.set_flipped(true);	// XXX: the interface was flipped
	}

	break;
    } while(false);
}

void
IfConfigSet::push_vif_begin(const IfTreeInterface&	i,
			  const IfTreeVif&		v)
{
    string error_msg;
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Add the vif
    //
    do {
	if (add_vif(i.ifname(), v.vifname(), if_index, error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to add vif: %s", error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().vif_error(i.ifname(),
							   v.vifname(),
							   error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	    return;
	}
	break;
    } while (false);
}

void
IfConfigSet::push_vif_end(const IfTreeInterface&	i,
			  const IfTreeVif&		v)
{
    string error_msg;
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
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
	const IfTreeInterface* ifp;

	ifp = ifconfig().pulled_config().find_interface(i.ifname());

	deleted = (i.is_marked(IfTreeItem::DELETED) |
		   v.is_marked(IfTreeItem::DELETED));
	enabled = i.enabled() & v.enabled();

	// Get the flags from the pulled config
	if (ifp != NULL)
	    pulled_flags = ifp->interface_flags();
	new_flags = pulled_flags;

	pulled_up = pulled_flags & IFF_UP;
	if (pulled_up && (deleted || !enabled))
	    new_flags &= ~IFF_UP;
	if ( (!pulled_up) && enabled)
	    new_flags |= IFF_UP;

	if ((! is_pulled_config_mirror()) && (new_flags == pulled_flags))
	    break;		// XXX: nothing changed

	// Set the vif flags
	if (ifp != NULL) {
	    const IfTreeVif* vifp = ifp->find_vif(v.vifname());
	    if (vifp != NULL) {
		pulled_broadcast = vifp->broadcast();
		pulled_loopback = vifp->loopback();
		pulled_point_to_point = vifp->point_to_point();
		pulled_multicast = vifp->multicast();
	    }
	}
	if (is_pulled_config_mirror()) {
	    new_broadcast = pulled_broadcast;
	    new_loopback = pulled_loopback;
	    new_point_to_point = pulled_point_to_point;
	    new_multicast = pulled_multicast;
	} else {
	    new_broadcast = v.broadcast();
	    new_loopback = v.loopback();
	    new_point_to_point = v.point_to_point();
	    new_multicast = v.multicast();
	}

	new_up = (new_flags & IFF_UP)? true : false;
	if (config_vif(i.ifname(), v.vifname(), if_index, new_flags, new_up,
		       deleted, new_broadcast, new_loopback,
		       new_point_to_point, new_multicast, error_msg)
	    < 0) {
	    error_msg = c_format("Failed to configure vif: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().vif_error(i.ifname(),
							   v.vifname(),
							   error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
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
    string error_msg;
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    debug_msg("Pushing %s\n", a.str().c_str());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// A created but disabled address will not appear in the live
	// config that was read from the kernel.
	//
	IfTreeVif* vifp;
	vifp = ifconfig().live_config().find_vif(i.ifname(), v.vifname());
	XLOG_ASSERT(vifp != NULL);
	vifp->add_addr(a.addr());
	IfTreeAddr4* ap = vifp->find_addr(a.addr());
	XLOG_ASSERT(ap != NULL);
	*ap = a;
	return;
    }

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED) |
		    a.is_marked(IfTreeItem::DELETED));

    if (deleted || !enabled) {
	if (delete_vif_address(i.ifname(), v.vifname(), if_index,
			       IPvX(a.addr()), a.prefix_len(), error_msg)
	    < 0) {
	    error_msg = c_format("Failed to delete address: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().vifaddr_error(i.ifname(),
							       v.vifname(),
							       a.addr(),
							       error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
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
	const IfTreeVif* vifp = ifconfig().pulled_config().find_vif(i.ifname(),
								    v.vifname());
	if (vifp != NULL)
	    ap = vifp->find_addr(a.addr());

	// Test if a new address
	bool new_address = true;
	do {
	    if (is_pulled_config_mirror())
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

	//
	// XXX: If the broadcast address was omitted, recompute it here.
	// Note that we recompute it only if the underlying vif is
	// broadcast-capable.
	//
	bool is_broadcast = a.broadcast();
	if ((vifp != NULL)
	    && (! (a.broadcast() || a.point_to_point()))
	    && (prefix_len > 0)
	    && vifp->broadcast()) {
	    IPv4 mask = IPv4::make_prefix(prefix_len);
	    oaddr = a.addr() | ~mask;
	    is_broadcast = true;
	}

	//
	// Try to add the address.
	// If the address exists already (e.g., with different prefix length),
	// then delete it first.
	//
	if (ap != NULL) {
	    delete_vif_address(i.ifname(), v.vifname(), if_index,
			       IPvX(a.addr()), ap->prefix_len(), error_msg);
	}
	if (add_vif_address(i.ifname(), v.vifname(), if_index, is_broadcast,
			    a.point_to_point(), IPvX(a.addr()), IPvX(oaddr),
			    prefix_len, error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to configure address: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().vifaddr_error(i.ifname(),
							       v.vifname(),
							       a.addr(),
							       error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
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
    string error_msg;
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    debug_msg("Pushing %s\n", a.str().c_str());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// A created but disabled address will not appear in the live
	// config rippled up from the kernel via the routing socket
	//
	IfTreeVif* vifp;
	vifp = ifconfig().live_config().find_vif(i.ifname(), v.vifname());
	XLOG_ASSERT(vifp != NULL);
	vifp->add_addr(a.addr());
	IfTreeAddr6* ap = vifp->find_addr(a.addr());
	XLOG_ASSERT(ap != NULL);
	*ap = a;
	return;
    }

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED) |
		    a.is_marked(IfTreeItem::DELETED));

    if (deleted || !enabled) {
	if (delete_vif_address(i.ifname(), v.vifname(), if_index,
			       IPvX(a.addr()), a.prefix_len(), error_msg)
	    < 0) {
	    error_msg = c_format("Failed to delete address: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().vifaddr_error(i.ifname(),
							       v.vifname(),
							       a.addr(),
							       error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
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
	// initialize prefix to 64.  This is exactly as ifconfig(8) does.
	uint32_t prefix_len = a.prefix_len();
	if (prefix_len == 0)
	    prefix_len = 64;

	const IfTreeAddr6* ap = ifconfig().pulled_config().find_addr(
	    i.ifname(), v.vifname(), a.addr());

	// Test if a new address
	bool new_address = true;
	do {
	    if (is_pulled_config_mirror())
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

	//
	// Try to add the address.
	// If the address exists already (e.g., with different prefix length),
	// then delete it first.
	//
	if (ap != NULL) {
	    delete_vif_address(i.ifname(), v.vifname(), if_index,
			       IPvX(a.addr()), ap->prefix_len(), error_msg);
	}
	if (add_vif_address(i.ifname(), v.vifname(), if_index, false,
			    a.point_to_point(), IPvX(a.addr()), IPvX(oaddr),
			    prefix_len, error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to configure address: %s",
				 error_msg.c_str());
	    ifconfig().ifconfig_error_reporter().vifaddr_error(i.ifname(),
							       v.vifname(),
							       a.addr(),
							       error_msg);
	    XLOG_ERROR("%s", ifconfig().ifconfig_error_reporter().last_error().c_str());
	}
	
	break;
    } while (false);
}
#endif // HAVE_IPV6
