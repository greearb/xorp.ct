// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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

//
// Copy some of the interfrace state from the pulled configuration
//
static void
copy_interface_state(const IfTreeInterface* pulled_ifp,
		     IfTreeInterface& config_iface)
{
    if (pulled_ifp == NULL)
	return;

    if (config_iface.pif_index() != pulled_ifp->pif_index())
	config_iface.set_pif_index(pulled_ifp->pif_index());
    if (config_iface.no_carrier() != pulled_ifp->no_carrier())
	config_iface.set_no_carrier(pulled_ifp->no_carrier());
    if (config_iface.baudrate() != pulled_ifp->baudrate())
	config_iface.set_baudrate(pulled_ifp->baudrate());
    if (config_iface.mtu() == 0) {
	if (config_iface.mtu() != pulled_ifp->mtu())
	    config_iface.set_mtu(pulled_ifp->mtu());
    }
    if (config_iface.mac().is_zero()) {
	if (config_iface.mac() != pulled_ifp->mac())
	    config_iface.set_mac(pulled_ifp->mac());
    }
    if (config_iface.interface_flags() != pulled_ifp->interface_flags())
	config_iface.set_interface_flags(pulled_ifp->interface_flags());
}

//
// Copy some of the vif state from the pulled configuration
//
static void
copy_vif_state(const IfTreeVif* pulled_vifp, IfTreeVif& config_vif)
{
    if (pulled_vifp == NULL)
	return;

    if (config_vif.pif_index() != pulled_vifp->pif_index())
	config_vif.set_pif_index(pulled_vifp->pif_index());
    if (config_vif.broadcast() != pulled_vifp->broadcast())
	config_vif.set_broadcast(pulled_vifp->broadcast());
    if (config_vif.loopback() != pulled_vifp->loopback())
	config_vif.set_loopback(pulled_vifp->loopback());
    if (config_vif.point_to_point() != pulled_vifp->point_to_point())
	config_vif.set_point_to_point(pulled_vifp->point_to_point());
    if (config_vif.multicast() != pulled_vifp->multicast())
	config_vif.set_multicast(pulled_vifp->multicast());
    if (config_vif.vif_flags() != pulled_vifp->vif_flags())
	config_vif.set_vif_flags(pulled_vifp->vif_flags());
}

int
IfConfigSet::push_config(const IfTree& iftree)
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::iterator vi;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    const IfTree& system_iftree = ifconfig().system_config();

    // Clear errors associated with error reporter
    error_reporter.reset();

    //
    // Pre-configuration processing:
    // - Sanity check config - bail on bad interface and bad vif names.
    // - Set "soft" flag for interfaces.
    // - Propagate "DELETED" status from interfaces to vifs.
    // - Propagate "DELETED" status from vifs to addresses.
    //
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& config_iface = *(ii->second);

	//
	// Set the "soft" flag for interfaces that are emulated
	//
	if ((config_iface.discard() && is_discard_emulated(config_iface))
	    || (config_iface.unreachable()
		&& is_unreachable_emulated(config_iface))) {
	    config_iface.set_soft(true);
	}

	//
	// Skip the rest of processing if the interface has no mapping to
	// an existing interface in the system.
	//
	if (config_iface.is_soft())
	    continue;

	//
	// Skip configuration for default system config interfaces
	//
	if (config_iface.default_system_config())
	    continue;

	//
	// Check that the interface is recognized by the system
	//
	if (system_iftree.find_interface(config_iface.ifname()) == NULL) {
	    if (config_iface.state() == IfTreeItem::DELETED) {
		// XXX: ignore deleted interfaces that are not recognized
		continue;
	    }
	    // Maybe it was already deleted from xorp due to OS removal.  We should
	    // just ignore this one
	    //error_reporter.interface_error(config_iface.ifname(),
	    //			   "interface not recognized");
	    //break;
	}

	//
	// Check the interface and vif name
	//
	for (vi = config_iface.vifs().begin();
	     vi != config_iface.vifs().end();
	     ++vi) {
	    IfTreeVif& config_vif = *(vi->second);
	    if (config_vif.vifname() != config_iface.ifname()) {
		error_reporter.vif_error(config_iface.ifname(),
					 config_vif.vifname(),
					 "bad vif name, must match iface name");
		break;
	    }
	}
	if (error_reporter.error_count() > 0)
	    break;

	//
	// Propagate the "DELETED" status from interfaces to vifs and addresses
	//
	for (vi = config_iface.vifs().begin();
	     vi != config_iface.vifs().end();
	     ++vi) {
	    IfTreeVif& config_vif = *(vi->second);
	    if (config_iface.state() == IfTreeItem::DELETED)
		config_vif.mark(IfTreeItem::DELETED);

	    // Propagate the "DELETE" status to the IPv4 addresses
	    IfTreeVif::IPv4Map::iterator a4i;
	    for (a4i = config_vif.ipv4addrs().begin();
		 a4i != config_vif.ipv4addrs().end();
		 ++a4i) {
		IfTreeAddr4& config_addr = *(a4i->second);
		if (config_vif.state() == IfTreeItem::DELETED)
		    config_addr.mark(IfTreeItem::DELETED);
	    }

	    // Propagate the "DELETE" status to the IPv6 addresses
#ifdef HAVE_IPV6
	    IfTreeVif::IPv6Map::iterator a6i;
	    for (a6i = config_vif.ipv6addrs().begin();
		 a6i != config_vif.ipv6addrs().end();
		 ++a6i) {
		IfTreeAddr6& config_addr = *(a6i->second);
		if (config_vif.state() == IfTreeItem::DELETED)
		    config_addr.mark(IfTreeItem::DELETED);
	    }
#endif // HAVE_IPV6
	}
    }

    if (error_reporter.error_count() > 0) {
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return (XORP_ERROR);
    }

    //
    // Push the config:
    // 1. Push the vif creation/deletion (e.g., VLAN).
    // 2. Pull the config from the system (e.g., to obtain information
    //    such as interface indexes for newly created interfaces/vifs).
    // 3. Push the interface/vif/address information.
    //
    push_iftree_begin(iftree);

    //
    // 1. Push the vif creation/deletion (e.g., VLAN).
    //
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& config_iface = *(ii->second);
	const IfTreeInterface* system_ifp = NULL;

	system_ifp = system_iftree.find_interface(config_iface.ifname());

	//
	// Skip interfaces that should never be pushed:
	// - Soft interfaces
	// - Default system config interfaces
	//
	if (config_iface.is_soft())
	    continue;
	if (config_iface.default_system_config())
	    continue;

	push_if_creation(system_ifp, config_iface);
    }

    //
    // 2. Pull the config from the system (e.g., to obtain information
    //    such as interface indexes for newly created interfaces/vifs).
    //
    ifconfig().pull_config(NULL, -1);

    //
    // 3. Push the interface/vif/address configuration.
    //
    for (ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end();
	 ++ii) {
	IfTreeInterface& config_iface = *(ii->second);
	const IfTreeInterface* system_ifp = NULL;

	system_ifp = system_iftree.find_interface(config_iface.ifname());

	//
	// Skip interfaces that should never be pushed:
	// - Soft interfaces
	// - Default system config interfaces
	//
	if (config_iface.is_soft())
	    continue;
	if (config_iface.default_system_config())
	    continue;

	if ((system_ifp == NULL)
	    && (config_iface.state() == IfTreeItem::DELETED)) {
	    // XXX: ignore deleted interfaces that are not recognized
	    continue;
	}

	push_interface_begin(system_ifp, config_iface);

	for (vi = config_iface.vifs().begin();
	     vi != config_iface.vifs().end();
	     ++vi) {
	    IfTreeVif& config_vif = *(vi->second);
	    const IfTreeVif* system_vifp = NULL;

	    if (system_ifp != NULL)
		system_vifp = system_ifp->find_vif(config_vif.vifname());

	    push_vif_begin(system_ifp, system_vifp, config_iface, config_vif);

	    //
	    // Push the IPv4 addresses
	    //
	    IfTreeVif::IPv4Map::iterator a4i;
	    for (a4i = config_vif.ipv4addrs().begin();
		 a4i != config_vif.ipv4addrs().end();
		 ++a4i) {
		IfTreeAddr4& config_addr = *(a4i->second);
		const IfTreeAddr4* system_addrp = NULL;

		if (system_vifp != NULL)
		    system_addrp = system_vifp->find_addr(config_addr.addr());

		push_vif_address(system_ifp, system_vifp, system_addrp,
				 config_iface, config_vif, config_addr);
	    }

#ifdef HAVE_IPV6
	    //
	    // Push the IPv6 addresses
	    //
	    IfTreeVif::IPv6Map::iterator a6i;
	    for (a6i = config_vif.ipv6addrs().begin();
		 a6i != config_vif.ipv6addrs().end();
		 ++a6i) {
		IfTreeAddr6& config_addr = *(a6i->second);
		const IfTreeAddr6* system_addrp = NULL;

		if (system_vifp != NULL)
		    system_addrp = system_vifp->find_addr(config_addr.addr());

		push_vif_address(system_ifp, system_vifp, system_addrp,
				 config_iface, config_vif, config_addr);
	    }
#endif // HAVE_IPV6

	    push_vif_end(system_ifp, system_vifp, config_iface, config_vif);
	}

	push_interface_end(system_ifp, config_iface);
    }

    push_iftree_end(iftree);

    if (error_reporter.error_count() != 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

void
IfConfigSet::push_iftree_begin(const IfTree& iftree)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    UNUSED(iftree);

    //
    // Begin the configuration
    //
    if (config_begin(error_msg) != XORP_OK) {
	error_msg = c_format("Failed to begin configuration: %s",
			     error_msg.c_str());
    }

    if (! error_msg.empty()) {
	error_reporter.config_error(error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_iftree_end(const IfTree& iftree)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    UNUSED(iftree);

    //
    // End the configuration
    //
    if (config_end(error_msg) != XORP_OK) {
	error_msg = c_format("Failed to end configuration: %s",
			     error_msg.c_str());
    }

    if (! error_msg.empty()) {
	error_reporter.config_error(error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_interface_begin(const IfTreeInterface*	system_ifp,
				  IfTreeInterface&		config_iface)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    if ((system_ifp == NULL) && config_iface.is_marked(IfTreeItem::DELETED)) {
	// Nothing to do: the interface has been deleted from the system
	return;
    }

    // Copy some of the state from the system configuration
    copy_interface_state(system_ifp, config_iface);

    //
    // Begin the interface configuration
    //
    if (config_interface_begin(system_ifp, config_iface, error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to begin interface configuration: %s",
			     error_msg.c_str());
    }

    if (! error_msg.empty()) {
	error_reporter.interface_error(config_iface.ifname(), error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_interface_end(const IfTreeInterface*	system_ifp,
				IfTreeInterface&	config_iface)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    //
    // End the interface configuration
    //
    if (config_interface_end(system_ifp, config_iface, error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to end interface configuration: %s",
			     error_msg.c_str());
    }

    if (! error_msg.empty()) {
	error_reporter.interface_error(config_iface.ifname(), error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_if_creation(const IfTreeInterface* system_ifp,
			      IfTreeInterface& config_if)
{
    // Only try to create/delete VLAN interfaces.  Could update
    // this if/when we support Linux VETH, BSD epair, etc.
    if (!config_if.is_vlan()) {
	return;
    }

    string error_msg;

    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    IfConfigVlanSet* ifconfig_vlan_set;

    // Get the plugin for VLAN setup
    ifconfig_vlan_set = fea_data_plane_manager().ifconfig_vlan_set();
    if (ifconfig_vlan_set == NULL) {
	error_msg = c_format("Failed to apply VLAN setup to "
			     "interface %s : no plugin found",
			     config_if.ifname().c_str());
	goto done;
    }

    //
    // Push the VLAN configuration: either add/update or delete it.
    //
    if (config_if.is_marked(IfTreeItem::DELETED)) {
	// Delete the VLAN (only if xorp created it)
	if (config_if.cr_by_xorp()) {
	    if (ifconfig_vlan_set->config_delete_vlan(config_if, error_msg) != XORP_OK) {
		error_msg = c_format("Failed to delete VLAN: %s  reason: %s ",
				     config_if.ifname().c_str(), error_msg.c_str());
	    }
	}
    }
    else {
	// Add/update the VLAN
	bool created_if = false;
	if (ifconfig_vlan_set->config_add_vlan(system_ifp,
					       config_if,
					       created_if,
					       error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to add VLAN to "
				 "interface %s  reason: %s",
				 config_if.ifname().c_str(),
				 error_msg.c_str());
	}
	else {
	    if (created_if)
		config_if.set_cr_by_xorp(true);
	}
    }

done:
    if (! error_msg.empty()) {
	error_reporter.vif_error(config_if.ifname(), config_if.ifname(),
				 error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_vif_begin(const IfTreeInterface*	system_ifp,
			    const IfTreeVif*		system_vifp,
			    IfTreeInterface&		config_iface,
			    IfTreeVif&			config_vif)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    if ((system_vifp == NULL) && config_vif.is_marked(IfTreeItem::DELETED)) {
	// Nothing to do: the vif has been deleted from the system
	return;
    }

    // Copy some of the state from the system configuration
    copy_interface_state(system_ifp, config_iface);
    copy_vif_state(system_vifp, config_vif);

    //
    // Begin the vif configuration
    //
    if (config_vif_begin(system_ifp, system_vifp, config_iface, config_vif,
			 error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to begin vif configuration: %s",
			     error_msg.c_str());
    }

    if (! error_msg.empty()) {
	error_reporter.vif_error(config_iface.ifname(), config_vif.vifname(),
				 error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_vif_end(const IfTreeInterface*	system_ifp,
			  const IfTreeVif*		system_vifp,
			  IfTreeInterface&		config_iface,
			  IfTreeVif&			config_vif)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();

    //
    // End the vif configuration
    //
    if (config_vif_end(system_ifp, system_vifp, config_iface, config_vif,
		       error_msg) != XORP_OK) {
	error_msg = c_format("Failed to end vif configuration: %s",
			     error_msg.c_str());
    }

    if (! error_msg.empty()) {
	error_reporter.vif_error(config_iface.ifname(), config_vif.vifname(),
				 error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

void
IfConfigSet::push_vif_address(const IfTreeInterface*	system_ifp,
			      const IfTreeVif*		system_vifp,
			      const IfTreeAddr4*	system_addrp,
			      IfTreeInterface&		config_iface,
			      IfTreeVif&		config_vif,
			      IfTreeAddr4&		config_addr)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    bool is_add = true;

    if (! fea_data_plane_manager().have_ipv4()) {
	error_msg = "IPv4 is not supported";
	goto done;
    }

    if (config_addr.is_marked(IfTreeItem::DELETED)
	|| (! config_addr.enabled())) {
	// XXX: Disabling an address is same as deleting it
	is_add = false;
    }

    //
    // XXX: If the broadcast address was omitted, recompute and set it here.
    // Note that we recompute it only if the underlying vif is
    // broadcast-capable.
    //
    if ((system_vifp != NULL)
	&& system_vifp->broadcast()
	&& (config_addr.prefix_len() > 0)
	&& (! (config_addr.broadcast() || config_addr.point_to_point()))) {
	IPv4 mask = IPv4::make_prefix(config_addr.prefix_len());
	IPv4 broadcast_addr = config_addr.addr() | ~mask;
	config_addr.set_bcast(broadcast_addr);
	config_addr.set_broadcast(true);
    }

    //
    // Push the address configuration: either add/update or delete it.
    //
    if (is_add) {
	//
	// Add/update the address
	//
	if (config_add_address(system_ifp, system_vifp, system_addrp,
			       config_iface, config_vif, config_addr,
			       error_msg)
	    != XORP_OK) {
	    if (strstr(error_msg.c_str(), "No such device")) {
		XLOG_ERROR("Failed to configure address because of device not found: %s",
			   error_msg.c_str());
		// We can't pass this back as an error to the cli because the device is gone,
		// and if we fail this set, the whole commit will fail.  This commit could be
		// deleting *another* device, with a (very near) future set of xorpsh commands
		// coming in to delete *this* interface in question.
		// This is a hack...we really need a way to pass warnings back to the
		// cli but NOT fail the commit if the logical state is correct but the
		// (transient, unpredictable, asynchronously discovered) state of the actual
		// OS network devices is currently out of state.
		// --Ben
		error_msg = "";
	    }
	    else {
		error_msg = c_format("Failed to add address, not device-no-found error: %s",
				     error_msg.c_str());
	    }
	}
    } else {
	//
	// Delete the address
	//
	if (system_addrp == NULL)
	    return;		// XXX: nothing to delete
	if (config_delete_address(system_ifp, system_vifp, system_addrp,
				  config_iface, config_vif, config_addr,
				  error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to delete address: %s",
				 error_msg.c_str());
	}
    }

 done:
    if (! error_msg.empty()) {
	error_reporter.vifaddr_error(config_iface.ifname(),
				     config_vif.vifname(),
				     config_addr.addr(),
				     error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}

#ifdef HAVE_IPV6
void
IfConfigSet::push_vif_address(const IfTreeInterface*	system_ifp,
			      const IfTreeVif*		system_vifp,
			      const IfTreeAddr6*	system_addrp,
			      IfTreeInterface&		config_iface,
			      IfTreeVif&		config_vif,
			      IfTreeAddr6&		config_addr)
{
    string error_msg;
    IfConfigErrorReporterBase& error_reporter =
	ifconfig().ifconfig_error_reporter();
    bool is_add = true;

    if (! fea_data_plane_manager().have_ipv6()) {
	error_msg = "IPv6 is not supported";
	goto done;
    }

    if (config_addr.is_marked(IfTreeItem::DELETED)
	|| (! config_addr.enabled())) {
	// XXX: Disabling an address is same as deleting it
	is_add = false;
    }

    //
    // XXX: For whatever reason a prefix length of zero does not cut it, so
    // initialize prefix to 64. This is exactly what ifconfig(8) does.
    //
    if (config_addr.prefix_len() == 0)
	config_addr.set_prefix_len(64);

    //
    // Push the address configuration: either add/update or delete it.
    //
    if (is_add) {
	//
	// Add/update the address
	//
	if (config_add_address(system_ifp, system_vifp, system_addrp,
			       config_iface, config_vif, config_addr,
			       error_msg)
	    != XORP_OK) {
	    if (strstr(error_msg.c_str(), "No such device")) {
		XLOG_ERROR("Failed to configure address because of device not found: %s",
			   error_msg.c_str());
		// We can't pass this back as an error to the cli because the device is gone,
		// and if we fail this set, the whole commit will fail.  This commit could be
		// deleting *another* device, with a (very near) future set of xorpsh commands
		// coming in to delete *this* interface in question.
		// This is a hack...we really need a way to pass warnings back to the
		// cli but NOT fail the commit if the logical state is correct but the
		// (transient, unpredictable, asynchronously discovered) state of the actual
		// OS network devices is currently out of state.
		// --Ben
		error_msg = "";
	    }
	    else {
		error_msg = c_format("Failed to configure address, not device-no-found error: %s",
				     error_msg.c_str());
	    }
	}
    } else {
	//
	// Delete the address
	//
	if (system_addrp == NULL)
	    return;		// XXX: nothing to delete
	if (config_delete_address(system_ifp, system_vifp, system_addrp,
				  config_iface, config_vif, config_addr,
				  error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Failed to delete address: %s",
				 error_msg.c_str());
	}
    }

 done:
    if (! error_msg.empty()) {
	error_reporter.vifaddr_error(config_iface.ifname(),
				     config_vif.vifname(),
				     config_addr.addr(),
				     error_msg);
	XLOG_ERROR("%s", error_reporter.last_error().c_str());
	return;
    }
}
#endif // HAVE_IPV6
