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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set.cc,v 1.13 2007/09/26 07:01:23 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

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
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    // Clear errors associated with error reporter
    error_reporter.reset();

    //
    // Sanity check config - bail on bad interface and bad vif names.
    //
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& i = ii->second;

	//
	// Set the "soft" flag for interfaces that are emulated
	//
	if (i.discard() && is_discard_emulated(i))
	    i.set_soft(true);
	if (i.unreachable() && is_unreachable_emulated(i))
	    i.set_soft(true);

	//
	// Skip the ifindex check if the interface has no mapping to
	// an existing interface in the system.
	//
	if (i.is_soft())
	    continue;

	//
	// Check that the interface is recognized by the system
	//
	uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
	if (if_index == 0) {
	    if (i.state() == IfTreeItem::DELETED) {
		// XXX: ignore deleted interfaces that are not recognized
		continue;
	    }
	    error_reporter.interface_error(i.ifname(),
					   "interface not recognized");
	    break;
	}

	//
	// Check the interface and vif name
	//
	for (vi = i.vifs().begin(); vi != i.vifs().end(); ++vi) {
	    IfTreeVif& v = vi->second;
	    if (v.is_vlan())
		continue;
	    if (v.vifname() != i.ifname()) {
		error_reporter.vif_error(i.ifname(), v.vifname(),
					 "bad vif name");
		break;
	    }
	}
	if (error_reporter.error_count() > 0)
	    break;
    }

    if (error_reporter.error_count() > 0) {
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return (XORP_ERROR);
    }

    //
    // Walk config
    //
    push_iftree_begin(iftree);
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& i = ii->second;

	//
	// Set the "soft" flag for interfaces that are emulated
	//
	// XXX: We need to do it again in case some of the push_*() methods
	// explicitly called pull_config() and destroyed the previously
	// set "soft" flag.
	//
	if (i.discard() && is_discard_emulated(i))
	    i.set_soft(true);
	if (i.unreachable() && is_unreachable_emulated(i))
	    i.set_soft(true);

	// Soft interfaces and their child nodes should never be pushed
	if (i.is_soft())
	    continue;

	uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
	if ((if_index == 0) && (i.state() == IfTreeItem::DELETED)) {
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
		push_vif_address(i, v, a);
	    }

#ifdef HAVE_IPV6
	    IfTreeVif::IPv6Map::iterator a6i;
	    for (a6i = v.ipv6addrs().begin(); a6i != v.ipv6addrs().end(); ++a6i) {
		IfTreeAddr6& a = a6i->second;
		// XXX: delete the address if the vif is deleted
		if (v.state() == IfTreeItem::DELETED)
		    a.mark(IfTreeItem::DELETED);
		push_vif_address(i, v, a);
	    }
#endif // HAVE_IPV6

	    push_vif_end(i, v);
	}

	push_interface_end(i);
    }
    push_iftree_end(iftree);

    if (error_reporter.error_count() != 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

void
IfConfigSet::push_iftree_begin(IfTree& iftree)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    UNUSED(iftree);

    //
    // Begin the configuration
    //
    do {
	if (config_begin(error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to begin configuration: %s",
				 error_msg.c_str());
	    break;
	}
	break;
    } while (false);

    if (! error_msg.empty()) {
	error_reporter.config_error(error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_iftree_end(IfTree& iftree)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    UNUSED(iftree);

    //
    // End the configuration
    //
    do {
	if (config_end(error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to end configuration: %s",
				 error_msg.c_str());
	    break;
	}
	break;
    } while (false);

    if (! error_msg.empty()) {
	error_reporter.config_error(error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_interface_begin(IfTreeInterface& i)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    uint32_t if_index = ifconfig().get_insert_ifindex(i.ifname());
    const IfTreeInterface* pulled_ifp = NULL;

    pulled_ifp = ifconfig().pulled_config().find_interface(i.ifname());

    XLOG_ASSERT(if_index > 0);
    if (i.pif_index() != if_index)
	i.set_pif_index(if_index);

    // Reset the flip flag for this interface
    set_interface_flipped(false);

    //
    // Begin the interface configuration
    //
    do {
	if (config_interface_begin(pulled_ifp, i, error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to begin interface configuration: %s",
				 error_msg.c_str());
	    break;
	}
	break;
    } while (false);

    if (! error_msg.empty()) {
	error_reporter.interface_error(i.ifname(), error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_interface_end(IfTreeInterface& i)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    const IfTreeInterface* pulled_ifp = NULL;

    pulled_ifp = ifconfig().pulled_config().find_interface(i.ifname());

    //
    // End the interface configuration
    //
    do {
	if (config_interface_end(pulled_ifp, i, error_msg) != XORP_OK) {
	    error_msg = c_format("Failed to end interface configuration: %s",
				 error_msg.c_str());
	    break;
	}
	break;
    } while (false);

    // Set the flip flag for this interface
    if (is_interface_flipped()) {
	i.set_flipped(true);
	set_interface_flipped(false);
    }

    if (! error_msg.empty()) {
	error_reporter.interface_error(i.ifname(), error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_vif_begin(IfTreeInterface&	i,
			    IfTreeVif&		v)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    uint32_t if_index = ifconfig().get_insert_ifindex(v.vifname());
    const IfTreeInterface* pulled_ifp = NULL;
    const IfTreeVif* pulled_vifp = NULL;
    bool is_vif_obsoleted = false;

    pulled_ifp = ifconfig().pulled_config().find_interface(i.ifname());
    if (pulled_ifp != NULL)
	pulled_vifp = pulled_ifp->find_vif(v.vifname());

    //
    // XXX: The vif might not exist yet, hence if_index might not be found
    //
    if (if_index > 0) {
	if (v.pif_index() != if_index)
	    v.set_pif_index(if_index);
    }

    //
    // Begin the vif configuration
    //
    do {
	if (config_vif_begin(pulled_ifp, pulled_vifp, i, v, error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to begin vif configuration: %s",
				 error_msg.c_str());
	    break;
	}

	// Configure VLAN vif
	if (v.is_vlan()) {
	    IfConfigVlanSet* ifconfig_vlan_set;

	    // Get the plugin for VLAN setup
	    ifconfig_vlan_set = fea_data_plane_manager().ifconfig_vlan_set();
	    if (ifconfig_vlan_set == NULL) {
		error_msg = c_format("Failed to apply VLAN setup to "
				     "interface %s vlan %s : no plugin found",
				     i.ifname().c_str(), v.vifname().c_str());
		break;
	    }

	    // Reset the obsoleted flag for this vif
	    ifconfig_vlan_set->set_vif_obsoleted(false);

	    if (ifconfig_vlan_set->config_vlan(pulled_ifp, pulled_vifp, i, v,
					       error_msg)
		!= XORP_OK) {
		error_msg = c_format("Failed to apply VLAN setup to "
				     "interface %s vlan %s: %s",
				     i.ifname().c_str(),
				     v.vifname().c_str(),
				     error_msg.c_str());
		break;
	    }

	    if (ifconfig_vlan_set->is_vif_obsoleted()) {
		is_vif_obsoleted = true;
		ifconfig_vlan_set->set_vif_obsoleted(false);
	    }
	}

	break;
    } while (false);

    //
    // Pull the new configuration if the vif was obsoleted (added or deleted)
    //
    // XXX: Calling pull_config() here is sub-optimal. Future refactoring
    // of the push_config() method should eliminate those excessive calls.
    //
    if (is_vif_obsoleted) {
	ifconfig().unmap_ifname(v.vifname());
	ifconfig().pull_config();
    }

    //
    // Regenerate the if_index in case the vif was added or deleted
    //
    if_index = ifconfig().get_insert_ifindex(v.vifname());
    if (if_index > 0) {
	if (v.pif_index() != if_index)
	    v.set_pif_index(if_index);
    }

    if (! error_msg.empty()) {
	error_reporter.vif_error(i.ifname(), v.vifname(), error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_vif_end(IfTreeInterface&	i,
			  IfTreeVif&		v)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    const IfTreeInterface* pulled_ifp = NULL;
    const IfTreeVif* pulled_vifp = NULL;

    pulled_ifp = ifconfig().pulled_config().find_interface(i.ifname());
    if (pulled_ifp != NULL)
	pulled_vifp = pulled_ifp->find_vif(v.vifname());

    //
    // End the vif configuration
    //
    do {
	if (config_vif_end(pulled_ifp, pulled_vifp, i, v, error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to end vif configuration: %s",
				 error_msg.c_str());
	    break;
	}
	break;
    } while (false);

    if (! error_msg.empty()) {
	error_reporter.vif_error(i.ifname(), v.vifname(), error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_vif_address(IfTreeInterface&	i,
			      IfTreeVif&	v,
			      IfTreeAddr4&	a)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    const IfTreeInterface* pulled_ifp = NULL;
    const IfTreeVif* pulled_vifp = NULL;
    const IfTreeAddr4* pulled_addrp = NULL;

    pulled_ifp = ifconfig().pulled_config().find_interface(i.ifname());
    if (pulled_ifp != NULL) {
	pulled_vifp = pulled_ifp->find_vif(v.vifname());
	if (pulled_vifp != NULL)
	    pulled_addrp = pulled_vifp->find_addr(a.addr());
    }

    //
    // XXX: If the broadcast address was omitted, recompute and set it here.
    // Note that we recompute it only if the underlying vif is
    // broadcast-capable.
    //
    if ((pulled_vifp != NULL)
	&& pulled_vifp->broadcast()
	&& (a.prefix_len() > 0)
	&& (! (a.broadcast() || a.point_to_point()))) {
	IPv4 mask = IPv4::make_prefix(a.prefix_len());
	IPv4 broadcast_addr = a.addr() | ~mask;
	a.set_bcast(broadcast_addr);
	a.set_broadcast(true);
    }

    //
    // Push the address configuration
    //
    do {
	if (config_addr(pulled_ifp, pulled_vifp, pulled_addrp, i, v, a,
			error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to configure address: %s",
				 error_msg.c_str());
	    break;
	}

	break;
    } while(false);

    if (! error_msg.empty()) {
	error_reporter.vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

#ifdef HAVE_IPV6
void
IfConfigSet::push_vif_address(IfTreeInterface&	i,
			      IfTreeVif&	v,
			      IfTreeAddr6&	a)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    const IfTreeInterface* pulled_ifp = NULL;
    const IfTreeVif* pulled_vifp = NULL;
    const IfTreeAddr6* pulled_addrp = NULL;

    pulled_ifp = ifconfig().pulled_config().find_interface(i.ifname());
    if (pulled_ifp != NULL) {
	pulled_vifp = pulled_ifp->find_vif(v.vifname());
	if (pulled_vifp != NULL)
	    pulled_addrp = pulled_vifp->find_addr(a.addr());
    }

    //
    // XXX: For whatever reason a prefix length of zero does not cut it, so
    // initialize prefix to 64.  This is exactly what ifconfig(8) does.
    if (a.prefix_len() == 0)
	a.set_prefix_len(64);

    //
    // Push the address configuration
    //
    do {
	if (config_addr(pulled_ifp, pulled_vifp, pulled_addrp, i, v, a,
			error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to configure address: %s",
				 error_msg.c_str());
	    break;
	}

	break;
    } while(false);

    if (! error_msg.empty()) {
	error_reporter.vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}
#endif // HAVE_IPV6
