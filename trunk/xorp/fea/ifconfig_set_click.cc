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

#ident "$XORP: xorp/fea/ifconfig_set_click.cc,v 1.21 2005/03/19 23:30:12 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/run_command.hh"

#include "ifconfig.hh"
#include "ifconfig_set.hh"
#include "nexthop_port_mapper.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//

IfConfigSetClick::IfConfigSetClick(IfConfig& ifc)
    : IfConfigSet(ifc),
      ClickSocket(ifc.eventloop()),
      _cs_reader(*(ClickSocket *)this),
      _kernel_click_config_generator(NULL),
      _user_click_config_generator(NULL),
      _has_kernel_click_config(false),
      _has_user_click_config(false)
{
}

IfConfigSetClick::~IfConfigSetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    register_ifc_secondary();

    return (XORP_OK);
}

int
IfConfigSetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    terminate_click_config_generator();

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

int
IfConfigSetClick::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetClick::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    //
    // Trigger the generation of the configuration
    //
    if (execute_click_config_generator(error_msg) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

bool
IfConfigSetClick::is_discard_emulated(const IfTreeInterface& i) const
{
    return (false);	// TODO: return correct value
    UNUSED(i);
}

int
IfConfigSetClick::add_interface(const string& ifname,
				uint16_t if_index,
				string& error_msg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	//
	// Add the new interface
	//
	if (_iftree.add_if(ifname) != true) {
	    error_msg = c_format("Cannot add interface '%s'", ifname.c_str());
	    return (XORP_ERROR);
	}
	ii = _iftree.get_if(ifname);
	XLOG_ASSERT(ii != _iftree.ifs().end());
    }

    // Update the interface
    if (ii->second.pif_index() != if_index)
	ii->second.set_pif_index(if_index);

    return (XORP_OK);
}

int
IfConfigSetClick::add_vif(const string& ifname,
			  const string& vifname,
			  uint16_t if_index,
			  string& error_msg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot add interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);
    if (vi == fi.vifs().end()) {
	//
	// Add the new vif
	//
	if (fi.add_vif(vifname) != true) {
	    error_msg = c_format("Cannot add interface '%s' vif '%s'",
				 ifname.c_str(), vifname.c_str());
	    return (XORP_ERROR);
	}
	vi = fi.get_vif(vifname);
	XLOG_ASSERT(vi != fi.vifs().end());
    }

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::config_interface(const string& ifname,
				   uint16_t if_index,
				   uint32_t flags,
				   bool is_up,
				   bool is_deleted,
				   string& error_msg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index, flags, (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false");

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot configure interface '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    //
    // Update the interface
    //
    if (fi.pif_index() != if_index)
	fi.set_pif_index(if_index);

    if (fi.if_flags() != flags)
	fi.set_if_flags(flags);

    if (fi.enabled() != is_up)
	fi.set_enabled(is_up);

    if (is_deleted)
	_iftree.remove_if(ifname);

    return (XORP_OK);
}

int
IfConfigSetClick::config_vif(const string& ifname,
			     const string& vifname,
			     uint16_t if_index,
			     uint32_t flags,
			     bool is_up,
			     bool is_deleted,
			     bool broadcast,
			     bool loopback,
			     bool point_to_point,
			     bool multicast,
			     string& error_msg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index, flags,
	      (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false",
	      (broadcast)? "true" : "false",
	      (loopback)? "true" : "false",
	      (point_to_point)? "true" : "false",
	      (multicast)? "true" : "false");

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot configure interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);
    if (vi == fi.vifs().end()) {
	error_msg = c_format("Cannot configure interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeVif& fv = vi->second;

    //
    // Update the vif
    //
    if (fv.enabled() != is_up)
	fv.set_enabled(is_up);

    if (fv.broadcast() != broadcast)
	fv.set_broadcast(broadcast);

    if (fv.loopback() != loopback)
	fv.set_loopback(loopback);

    if (fv.point_to_point() != point_to_point)
	fv.set_point_to_point(point_to_point);

    if (fv.multicast() != multicast)
	fv.set_multicast(multicast);

    if (is_deleted) {
	fi.remove_vif(vifname);
	ifc().nexthop_port_mapper().delete_interface(ifname, vifname);
    }

    return (XORP_OK);

    UNUSED(if_index);
    UNUSED(flags);
}

int
IfConfigSetClick::set_interface_mac_address(const string& ifname,
					    uint16_t if_index,
					    const struct ether_addr& ether_addr,
					    string& error_msg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot set MAC address on interface '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    //
    // Set the MAC address
    //
    try {
	EtherMac new_ether_mac(ether_addr);
	Mac new_mac = new_ether_mac;

	if (fi.mac() != new_mac)
	    fi.set_mac(new_mac);
    } catch (BadMac) {
	error_msg = c_format("Cannot set MAC address on interface '%s' "
			     "to '%s': "
			     "invalid MAC address",
			     ifname.c_str(),
			     ether_ntoa(const_cast<struct ether_addr *>(&ether_addr)));
	return (XORP_ERROR);
    }

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::set_interface_mtu(const string& ifname,
				    uint16_t if_index,
				    uint32_t mtu,
				    string& error_msg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot set MTU on interface '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    //
    // Set the MTU
    //
    if (fi.mtu() != mtu)
	fi.set_mtu(mtu);

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::add_vif_address(const string& ifname,
				  const string& vifname,
				  uint16_t if_index,
				  bool is_broadcast,
				  bool is_p2p,
				  const IPvX& addr,
				  const IPvX& dst_or_bcast,
				  uint32_t prefix_len,
				  string& error_msg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      (is_broadcast)? "true" : "false",
	      (is_p2p)? "true" : "false", addr.str().c_str(),
	      dst_or_bcast.str().c_str(), prefix_len);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot add address to interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);

    if (vi == fi.vifs().end()) {
	error_msg = c_format("Cannot add address to interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeVif& fv = vi->second;

    //
    // Add the address
    //
    if (addr.is_ipv4()) {
	IfTreeVif::V4Map::iterator ai;
	IPv4 addr4 = addr.get_ipv4();
	IPv4 dst_or_bcast4 = dst_or_bcast.get_ipv4();

	ai = fv.get_addr(addr4);
	if (ai == fv.v4addrs().end()) {
	    if (fv.add_addr(addr4) != true) {
		error_msg = c_format("Cannot add address '%s' "
				     "to interface '%s' vif '%s'",
				     addr4.str().c_str(),
				     ifname.c_str(),
				     vifname.c_str());
		return (XORP_ERROR);
	    }
	    ai = fv.get_addr(addr4);
	    XLOG_ASSERT(ai != fv.v4addrs().end());
	}
	IfTreeAddr4& fa = ai->second;

	//
	// Update the address
	//
	if (fa.broadcast() != is_broadcast)
	    fa.set_broadcast(is_broadcast);

	if (fa.point_to_point() != is_p2p)
	    fa.set_point_to_point(is_p2p);

	if (fa.broadcast()) {
	    if (fa.bcast() != dst_or_bcast4)
		fa.set_bcast(dst_or_bcast4);
	}

	if (fa.point_to_point()) {
	    if (fa.endpoint() != dst_or_bcast4)
		fa.set_endpoint(dst_or_bcast4);
	}

	if (fa.prefix_len() != prefix_len)
	    fa.set_prefix_len(prefix_len);

	if (! fa.enabled())
	    fa.set_enabled(true);
    }

    if (addr.is_ipv6()) {
	IfTreeVif::V6Map::iterator ai;
	IPv6 addr6 = addr.get_ipv6();
	IPv6 dst_or_bcast6 = dst_or_bcast.get_ipv6();

	ai = fv.get_addr(addr6);
	if (ai == fv.v6addrs().end()) {
	    if (fv.add_addr(addr6) != true) {
		error_msg = c_format("Cannot add address '%s' "
				     "to interface '%s' vif '%s'",
				     addr6.str().c_str(),
				     ifname.c_str(),
				     vifname.c_str());
		return (XORP_ERROR);
	    }
	    ai = fv.get_addr(addr6);
	    XLOG_ASSERT(ai != fv.v6addrs().end());
	}
	IfTreeAddr6& fa = ai->second;

	//
	// Update the address
	//
	if (fa.point_to_point() != is_p2p)
	    fa.set_point_to_point(is_p2p);

	if (fa.point_to_point()) {
	    if (fa.endpoint() != dst_or_bcast6)
		fa.set_endpoint(dst_or_bcast6);
	}

	if (fa.prefix_len() != prefix_len)
	    fa.set_prefix_len(prefix_len);

	if (! fa.enabled())
	    fa.set_enabled(true);
    }

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::delete_vif_address(const string& ifname,
				     const string& vifname,
				     uint16_t if_index,
				     const IPvX& addr,
				     uint32_t prefix_len,
				     string& error_msg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      prefix_len);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Cannot delete address "
			     "on interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);

    if (vi == fi.vifs().end()) {
	error_msg = c_format("Cannot delete address "
			     "on interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeVif& fv = vi->second;

    //
    // Delete the address
    //
    if (addr.is_ipv4()) {
	IfTreeVif::V4Map::iterator ai;
	IPv4 addr4 = addr.get_ipv4();

	ai = fv.get_addr(addr4);
	if (ai == fv.v4addrs().end()) {
	    error_msg = c_format("Cannot delete address '%s' "
				 "on interface '%s' vif '%s': "
				 "no such address",
				 addr4.str().c_str(),
				 ifname.c_str(),
				 vifname.c_str());
	    return (XORP_ERROR);
	}
	fv.remove_addr(addr4);
	ifc().nexthop_port_mapper().delete_ipv4(addr4);
    }

    if (addr.is_ipv6()) {
	IfTreeVif::V6Map::iterator ai;
	IPv6 addr6 = addr.get_ipv6();

	ai = fv.get_addr(addr6);
	if (ai == fv.v6addrs().end()) {
	    error_msg = c_format("Cannot delete address '%s' "
				 "on interface '%s' vif '%s': "
				 "no such address",
				 addr6.str().c_str(),
				 ifname.c_str(),
				 vifname.c_str());
	    return (XORP_ERROR);
	}
	fv.remove_addr(addr6);
	ifc().nexthop_port_mapper().delete_ipv6(addr6);
    }

    return (XORP_OK);

    UNUSED(if_index);
    UNUSED(prefix_len);
}

int
IfConfigSetClick::execute_click_config_generator(string& error_msg)
{
    string kernel_generator_file = ClickSocket::kernel_click_config_generator_file();
    string user_generator_file = ClickSocket::user_click_config_generator_file();
    string arguments;

    //
    // Test the Click configuration generator filenames are valid
    //
    if (ClickSocket::is_kernel_click()) {
	if (kernel_generator_file.empty()) {
	    error_msg = c_format(
		"Cannot execute the kernel-level Click configuration "
		"generator: "
		"empty generator file name");
	    return (XORP_ERROR);
	}
    }
    if (ClickSocket::is_user_click()) {
	if (user_generator_file.empty()) {
	    error_msg = c_format(
		"Cannot execute the user-level Click configuration "
		"generator: "
		"empty generator file name");
	    return (XORP_ERROR);
	}
    }

    //
    // Re-generate the XORP fea/click configuration and
    // pass it to the Click config generator.
    //
    // TODO: XXX: the internal re-generation is a temporary solution
    // that should go away after the xorpsh takes over the functionality
    // of calling the Click config generator.
    //
    string xorp_config = regenerate_xorp_iftree_config();
    xorp_config += regenerate_xorp_fea_click_config();

    //
    // XXX: kill any previously running instances
    //
    if (_kernel_click_config_generator != NULL) {
	delete _kernel_click_config_generator;
	_kernel_click_config_generator = NULL;
    }
    if (_user_click_config_generator != NULL) {
	delete _user_click_config_generator;
	_user_click_config_generator = NULL;
    }

    //
    // Cleanup
    //
    _has_kernel_click_config = false;
    _has_user_click_config = false;
    _generated_kernel_click_config.erase();
    _generated_user_click_config.erase();

    //
    // Execute the Click configuration generators
    //
    if (ClickSocket::is_kernel_click()) {
	_kernel_click_config_generator = new ClickConfigGenerator(
	    *this,
	    kernel_generator_file);
	if (_kernel_click_config_generator->execute(xorp_config, error_msg)
	    != XORP_OK) {
	    delete _kernel_click_config_generator;
	    _kernel_click_config_generator = NULL;
	    return (XORP_ERROR);
	}
    }
    if (ClickSocket::is_user_click()) {
	_user_click_config_generator = new ClickConfigGenerator(
	    *this,
	    user_generator_file);
	if (_user_click_config_generator->execute(xorp_config, error_msg)
	    != XORP_OK) {
	    delete _user_click_config_generator;
	    _user_click_config_generator = NULL;
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

void
IfConfigSetClick::terminate_click_config_generator()
{
    if (_kernel_click_config_generator != NULL) {
	delete _kernel_click_config_generator;
	_kernel_click_config_generator = NULL;
    }
    if (_user_click_config_generator != NULL) {
	delete _user_click_config_generator;
	_user_click_config_generator = NULL;
    }
    _has_kernel_click_config = false;
    _has_user_click_config = false;
    _generated_kernel_click_config.erase();
    _generated_user_click_config.erase();
}

void
IfConfigSetClick::click_config_generator_done(
    IfConfigSetClick::ClickConfigGenerator* click_config_generator,
    bool success,
    const string& error_msg)
{
    // Check for errors
    XLOG_ASSERT((click_config_generator == _kernel_click_config_generator)
		|| (click_config_generator == _user_click_config_generator));
    if (! success) {
	XLOG_ERROR("External Click configuration generator (%s) failed: %s",
		   click_config_generator->command_name().c_str(),
		   error_msg.c_str());
    }

    string command_stdout = click_config_generator->command_stdout();

    if (click_config_generator == _kernel_click_config_generator) {
	if (success) {
	    _has_kernel_click_config = true;
	    _generated_kernel_click_config = command_stdout;
	}
	_kernel_click_config_generator = NULL;
    }
    if (click_config_generator == _user_click_config_generator) {
	if (success) {
	    _generated_user_click_config = command_stdout;
	    _has_user_click_config = true;
	}
	_user_click_config_generator = NULL;
    }
    delete click_config_generator;

    if (! success)
	return;

    if ((_kernel_click_config_generator != NULL)
	|| (_user_click_config_generator != NULL)) {
	// XXX: we are still waiting for some output
	return;
    }

    string write_error_msg;
    if (write_generated_config(_has_kernel_click_config,
			       _generated_kernel_click_config,
			       _has_user_click_config,
			       _generated_user_click_config,
			       write_error_msg) != XORP_OK) {
	XLOG_ERROR("Failed to write the Click configuration: %s",
		   write_error_msg.c_str());
    }
}

int
IfConfigSetClick::write_generated_config(bool has_kernel_config,
					 const string& kernel_config,
					 bool has_user_config,
					 const string& user_config,
					 string& error_msg)
{
    string element = "";
    string handler = "hotconfig";

    if (ClickSocket::write_config(element, handler,
				  has_kernel_config, kernel_config,
				  has_user_config, user_config,
				  error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    //
    // Generate the new port mapping
    //
    generate_nexthop_to_port_mapping();

    //
    // Notify the NextHopPortMapper observers about any port mapping changes
    //
    ifc().nexthop_port_mapper().notify_observers();

    return (XORP_OK);
}

string
IfConfigSetClick::regenerate_xorp_iftree_config() const
{
    string config, preamble;
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::V4Map::const_iterator ai4;
    IfTreeVif::V6Map::const_iterator ai6;

    //
    // Configuration section: "interfaces{}"
    //
    preamble = "";
    config += preamble + c_format("interfaces {\n");
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	preamble = "    ";
	config += preamble + c_format("interface %s {\n",
				      fi.ifname().c_str());
	preamble = "\t";
	config += preamble + c_format("disable: %s\n",
				      (! fi.enabled()) ? "true" : "false");
	config += preamble + c_format("discard: %s\n",
				      fi.discard() ? "true" : "false");
	config += preamble + c_format("mac: %s\n", fi.mac().str().c_str());
	config += preamble + c_format("mtu: %u\n", fi.mtu());
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    preamble = "\t";
	    config += preamble + c_format("vif %s {\n", fv.vifname().c_str());
	    preamble = "\t    ";
	    config += preamble + c_format("disable: %s\n",
					  (! fv.enabled()) ? "true" : "false");
	    for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
		preamble = "\t    ";
		config += preamble + c_format("address %s {\n",
					      fa4.addr().str().c_str());
		preamble = "\t\t";
		config += preamble + c_format("prefix-length: %u\n",
					      fa4.prefix_len());
		if (fa4.broadcast()) {
		    config += preamble + c_format("broadcast: %s\n",
						  fa4.bcast().str().c_str());
		}
		if (fa4.point_to_point()) {
		    config += preamble + c_format("destination: %s\n",
						  fa4.endpoint().str().c_str());
		}
		config += preamble + c_format("multicast-capable: %s\n",
					      fa4.multicast() ? "true" : "false");
		config += preamble + c_format("point-to-point: %s\n",
					      fa4.point_to_point() ? "true" : "false");
		config += preamble + c_format("loopback: %s\n",
					      fa4.loopback() ? "true" : "false");
		config += preamble + c_format("disable: %s\n",
					      (! fa4.enabled()) ? "true" : "false");
		preamble = "\t    ";
		config += preamble + c_format("}\n");
	    }
	    for (ai6 = fv.v6addrs().begin(); ai6 != fv.v6addrs().end(); ++ai6) {
		const IfTreeAddr6& fa6 = ai6->second
;		preamble = "\t    ";
		config += preamble + c_format("address %s {\n",
					      fa6.addr().str().c_str());
		preamble = "\t\t";
		config += preamble + c_format("prefix-length: %u\n",
					      fa6.prefix_len());
		if (fa6.point_to_point()) {
		    config += preamble + c_format("destination: %s\n",
						  fa6.endpoint().str().c_str());
		}
		config += preamble + c_format("multicast-capable: %s\n",
					      fa6.multicast() ? "true" : "false");
		config += preamble + c_format("point-to-point: %s\n",
					      fa6.point_to_point() ? "true" : "false");
		config += preamble + c_format("loopback: %s\n",
					      fa6.loopback() ? "true" : "false");
		config += preamble + c_format("disable: %s\n",
					      (! fa6.enabled()) ? "true" : "false");
		preamble = "\t    ";
		config += preamble + c_format("}\n");
	    }
	    preamble = "\t";
	    config += preamble + c_format("}\n");
	}
	preamble = "    ";
	config += preamble + c_format("}\n");
    }
    preamble = "";
    config += preamble + c_format("}\n");

    return (config);
}

string
IfConfigSetClick::regenerate_xorp_fea_click_config() const
{
    string config, preamble;

    //
    // Configuration section: "fea{}"
    //
    // XXX: note that we write only a partial configuration: those
    // that is related to enabling/disabling kernel or user-level Click.
    //
    preamble = "";
    config += preamble + c_format("fea {\n");
    do {
	preamble = "    ";
	config += preamble + c_format("click {\n");
	do {
	    preamble = "\t";
	    config += preamble + c_format("disable: %s\n",
					  (! ClickSocket::is_enabled()) ?
					  "true" : "false");
	    do {
		config += preamble + c_format("kernel-click {\n");
		preamble = "\t    ";
		config += preamble + c_format("disable: %s\n",
					      (! ClickSocket::is_kernel_click()) ?
					      "true" : "false");
		preamble = "\t";
		config += preamble + c_format("}\n");
	    } while (false);
	    do {
		config += preamble + c_format("user-click {\n");
		preamble = "\t    ";
		config += preamble + c_format("disable: %s\n",
					      (! ClickSocket::is_user_click()) ?
					      "true" : "false");
		preamble = "\t";
		config += preamble + c_format("}\n");
	    } while (false);
	} while (false);
	preamble = "    ";
	config += preamble + c_format("}\n");
    } while (false);
    preamble = "";
    config += preamble + c_format("}\n");

    return (config);
}

/**
 * Generate the next-hop to port mapping.
 *
 * @return the number of generated ports.
 */
int
IfConfigSetClick::generate_nexthop_to_port_mapping()
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::V4Map::const_iterator ai4;
    IfTreeVif::V6Map::const_iterator ai6;
    int xorp_rt_port, local_xorp_rt_port;

    //
    // Calculate the port for local delivery.
    // XXX: The last port in xorp_rt is reserved for local delivery
    //
    local_xorp_rt_port = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    local_xorp_rt_port++;
	}
    }

    //
    // Generate the next-hop to port mapping.
    // Note that all local IP addresses are mapped to the port
    // designated for local delivery.
    //
    ifc().nexthop_port_mapper().clear();
    xorp_rt_port = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    ifc().nexthop_port_mapper().add_interface(fi.ifname(),
						      fv.vifname(),
						      xorp_rt_port);
	    for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
		ifc().nexthop_port_mapper().add_ipv4(fa4.addr(),
						     local_xorp_rt_port);
		IPv4Net ipv4net(fa4.addr(), fa4.prefix_len());
		ifc().nexthop_port_mapper().add_ipv4net(ipv4net, xorp_rt_port);
		if (fa4.point_to_point())
		    ifc().nexthop_port_mapper().add_ipv4(fa4.endpoint(),
							 xorp_rt_port);
	    }
	    for (ai6 = fv.v6addrs().begin(); ai6 != fv.v6addrs().end(); ++ai6) {
		const IfTreeAddr6& fa6 = ai6->second;
		ifc().nexthop_port_mapper().add_ipv6(fa6.addr(),
						     local_xorp_rt_port);
		IPv6Net ipv6net(fa6.addr(), fa6.prefix_len());
		ifc().nexthop_port_mapper().add_ipv6net(ipv6net, xorp_rt_port);
		if (fa6.point_to_point())
		    ifc().nexthop_port_mapper().add_ipv6(fa6.endpoint(),
							 xorp_rt_port);
	    }
	    xorp_rt_port++;
	}
    }

    return (xorp_rt_port);
}

IfConfigSetClick::ClickConfigGenerator::ClickConfigGenerator(
    IfConfigSetClick& ifc_set_click,
    const string& command_name)
    : _ifc_set_click(ifc_set_click),
      _eventloop(ifc_set_click.ifc().eventloop()),
      _command_name(command_name),
      _run_command(NULL),
      _tmp_socket(-1)
{
}

IfConfigSetClick::ClickConfigGenerator::~ClickConfigGenerator()
{
    if (_run_command != NULL)
	delete _run_command;
    if (_tmp_socket >= 0) {
	close(_tmp_socket);
	unlink(_tmp_filename.c_str());
    }
}

int
IfConfigSetClick::ClickConfigGenerator::execute(const string& xorp_config,
						string& error_msg)
{
    // Create a temporary file
    char tmp_filename[1024] = "/tmp/xorp_fea_click.XXXXXXXX";
    int s = mkstemp(tmp_filename);
    if (s < 0) {
	error_msg = c_format("Cannot create a temporary file: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (::write(s, xorp_config.c_str(), xorp_config.size())
	!= static_cast<ssize_t>(xorp_config.size())) {
	error_msg = c_format("Error writing to temporary file: %s",
			     strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    _command_arguments = tmp_filename;	// XXX: the filename is the argument
    _tmp_socket = s;
    _tmp_filename = tmp_filename;

    _run_command = new RunCommand(
	_eventloop,
	_command_name,
	_command_arguments,
	callback(this, &IfConfigSetClick::ClickConfigGenerator::stdout_cb),
	callback(this, &IfConfigSetClick::ClickConfigGenerator::stderr_cb),
	callback(this, &IfConfigSetClick::ClickConfigGenerator::done_cb));
    if (_run_command->execute() != XORP_OK) {
	delete _run_command;
	_run_command = NULL;
	close(_tmp_socket);
	_tmp_socket = -1;
	unlink(_tmp_filename.c_str());
	error_msg = c_format("Could not execute the Click "
			     "configuration generator");
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

void
IfConfigSetClick::ClickConfigGenerator::stdout_cb(RunCommand* run_command,
						  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stdout += output;
}

void
IfConfigSetClick::ClickConfigGenerator::stderr_cb(RunCommand* run_command,
						  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    XLOG_ERROR("External Click configuration generator (%s) stderr output: %s",
	       run_command->command().c_str(),
	       output.c_str());
}

void
IfConfigSetClick::ClickConfigGenerator::done_cb(RunCommand* run_command,
						bool success,
						const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);
    _ifc_set_click.click_config_generator_done(this, success, error_msg);
}
