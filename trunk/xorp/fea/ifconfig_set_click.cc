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

#ident "$XORP: xorp/fea/ifconfig_set_click.cc,v 1.13 2004/12/07 23:09:12 pavlin Exp $"


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
      _click_config_generator_run_command(NULL),
      _click_config_generator_tmp_socket(-1)
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
    if (_is_running)
	return (XORP_OK);

    if (! ClickSocket::is_enabled()) {
	error_msg = c_format("Click is not enabled");
	return (XORP_ERROR);	// XXX: Not enabled
    }

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

#if 0		// TODO: XXX: PAVPAVPAV: remove it!
    //
    // Generate the configuration and write it.
    //
    string config = generate_config();
    if (write_generated_config(config, error_msg) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
#endif

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
    string command = ClickSocket::click_config_generator_file();
    string arguments;

    if (command.empty()) {
	error_msg = c_format(
	    "Cannot execute the Click configuration generator: "
	    "empty generator file name");
	return (XORP_ERROR);
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
    arguments = tmp_filename;	// XXX: the filename is the argument

    // XXX: kill any previously running instance
    if (_click_config_generator_run_command != NULL) {
	delete _click_config_generator_run_command;
	_click_config_generator_run_command = NULL;
    }
    if (_click_config_generator_tmp_socket >= 0) {
	close(_click_config_generator_tmp_socket);
	_click_config_generator_tmp_socket = -1;
	unlink(_click_config_generator_tmp_filename.c_str());
    }
    _click_config_generator_tmp_socket = s;
    _click_config_generator_tmp_filename = tmp_filename;

    // Clear any previous stdout
    _click_config_generator_stdout.erase();

    _click_config_generator_run_command = new RunCommand(
	ifc().eventloop(),
	command,
	arguments,
	callback(this, &IfConfigSetClick::click_config_generator_stdout_cb),
	callback(this, &IfConfigSetClick::click_config_generator_stderr_cb),
	callback(this, &IfConfigSetClick::click_config_generator_done_cb));

    if (_click_config_generator_run_command->execute() != XORP_OK) {
	delete _click_config_generator_run_command;
	_click_config_generator_run_command = NULL;
	close(_click_config_generator_tmp_socket);
	_click_config_generator_tmp_socket = -1;
	unlink(_click_config_generator_tmp_filename.c_str());
	error_msg = c_format("Could not execute the Click configuration "
			     "generator");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
IfConfigSetClick::terminate_click_config_generator()
{
    if (_click_config_generator_run_command != NULL) {
	delete _click_config_generator_run_command;
	_click_config_generator_run_command = NULL;
    }
    if (_click_config_generator_tmp_socket >= 0) {
	close(_click_config_generator_tmp_socket);
	_click_config_generator_tmp_socket = -1;
	unlink(_click_config_generator_tmp_filename.c_str());
    }
}

void
IfConfigSetClick::click_config_generator_stdout_cb(RunCommand* run_command,
						   const string& output)
{
    XLOG_ASSERT(run_command == _click_config_generator_run_command);
    _click_config_generator_stdout += output;
}

void
IfConfigSetClick::click_config_generator_stderr_cb(RunCommand* run_command,
						   const string& output)
{
    XLOG_ASSERT(run_command == _click_config_generator_run_command);
    XLOG_ERROR("External Click configuration generator stderr output: %s",
	       output.c_str());
}

void
IfConfigSetClick::click_config_generator_done_cb(RunCommand* run_command,
						 bool success,
						 const string& error_msg)
{
    XLOG_ASSERT(run_command == _click_config_generator_run_command);
    if (! success) {
	XLOG_ERROR("External Click configuration generator (%s) failed: %s",
		   run_command->command().c_str(), error_msg.c_str());
    }
    delete _click_config_generator_run_command;
    _click_config_generator_run_command = NULL;
    if (_click_config_generator_tmp_socket >= 0) {
	close(_click_config_generator_tmp_socket);
	_click_config_generator_tmp_socket = -1;
	unlink(_click_config_generator_tmp_filename.c_str());
    }
    if (! success)
	return;

    string write_error_msg;
    if (write_generated_config(_click_config_generator_stdout, write_error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Failed to write the Click configuration: %s",
		   write_error_msg.c_str());
    }
}

int
IfConfigSetClick::write_generated_config(const string& config,
					 string& error_msg)
{
    string element = "";
    string handler = "hotconfig";

    if (ClickSocket::write_config(element, handler, config, error_msg)
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
	config += preamble + c_format("enabled: %s\n",
				      fi.enabled() ? "true" : "false");
	config += preamble + c_format("discard: %s\n",
				      fi.discard() ? "true" : "false");
	config += preamble + c_format("mac: %s\n", fi.mac().str().c_str());
	config += preamble + c_format("mtu: %u\n", fi.mtu());
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    preamble = "\t";
	    config += preamble + c_format("vif %s {\n", fv.vifname().c_str());
	    preamble = "\t    ";
	    config += preamble + c_format("enabled: %s\n",
					  fv.enabled() ? "true" : "false");
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
		config += preamble + c_format("enabled: %s\n",
					      fa4.enabled() ? "true" : "false");
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
		config += preamble + c_format("enabled: %s\n",
					      fa6.enabled() ? "true" : "false");
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
	    config += preamble + c_format("enabled: %s\n",
					  ClickSocket::is_enabled() ?
					  "true" : "false");
	    do {
		config += preamble + c_format("kernel-click {\n");
		preamble = "\t    ";
		config += preamble + c_format("enabled: %s\n",
					      ClickSocket::is_kernel_click() ?
					      "true" : "false");
		preamble = "\t";
		config += preamble + c_format("}\n");
	    } while (false);
	    do {
		config += preamble + c_format("user-click {\n");
		preamble = "\t    ";
		config += preamble + c_format("enabled: %s\n",
					      ClickSocket::is_user_click() ?
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
    int xorp_rt_port;

    //
    // Generate the next-hop to port mapping
    //
    // XXX: last port in xorp_rt is reserved for local delivery
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
		ifc().nexthop_port_mapper().add_ipv4(fa4.addr(), xorp_rt_port);
		IPv4Net ipv4net(fa4.addr(), fa4.prefix_len());
		ifc().nexthop_port_mapper().add_ipv4net(ipv4net, xorp_rt_port);
		if (fa4.point_to_point())
		    ifc().nexthop_port_mapper().add_ipv4(fa4.endpoint(),
							 xorp_rt_port);
	    }
	    for (ai6 = fv.v6addrs().begin(); ai6 != fv.v6addrs().end(); ++ai6) {
		const IfTreeAddr6& fa6 = ai6->second;
		ifc().nexthop_port_mapper().add_ipv6(fa6.addr(), xorp_rt_port);
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

// TODO: this method will go away in the future
string
IfConfigSetClick::generate_config()
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::V4Map::const_iterator ai4;
    // IfTreeVif::V6Map::const_iterator ai6;
    int xorp_rt_port, max_xorp_rt_port, max_vifs;

    string config;

    //
    // Finalize IfTree state
    //
    _iftree.finalize_state();

    //
    // Calculate the number of vifs
    //
    max_vifs = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    max_vifs++;
	}
    }

    max_xorp_rt_port = generate_nexthop_to_port_mapping();

    config += "//\n";
    config += "// Generated by XORP FEA\n";
    config += "//\n";
    config += "\n";
    config += "//\n";
    config += "// Configured interfaces:\n";
    config += "//\n";
    config += "/*\n";
    config += _iftree.str();
    config += "*/\n";

    config += "\n";
    config += "// Shared IP input path and routing table\n";
    string xorp_ip = "_xorp_ip";
    string xorp_rt = "_xorp_rt";
    string check_ip_header = "CheckIPHeader(INTERFACES";
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
		check_ip_header += c_format(" %s/%u",
					    fa4.addr().str().c_str(),
					    fa4.prefix_len());
	    }
	}
    }
    check_ip_header += ")";

    config += c_format("%s :: Strip(14)\n", xorp_ip.c_str());
    config += c_format("    -> %s\n", check_ip_header.c_str());
    config += c_format("    -> %s :: LinearIPLookup();\n", xorp_rt.c_str());

    config += "\n";
    config += "// ARP responses are copied to each ARPQuerier and the host.\n";
    string xorp_arpt = "_xorp_arpt";
    config += c_format("%s :: Tee(%u);\n", xorp_arpt.c_str(), max_vifs + 1);

    int port = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi, ++port) {
	    const IfTreeVif& fv = vi->second;
	    string xorp_c = c_format("_xorp_c%u", port);
	    string xorp_out = c_format("_xorp_out%u", port);
	    string xorp_to_device = c_format("_xorp_to_device%u", port);
	    string xorp_ar = c_format("_xorp_ar%u", port);
	    string xorp_arpq = c_format("_xorp_arpq%u", port);

	    string from_device, to_device, arp_responder, arp_querier;
	    string paint, print_non_ip;

	    if (! fv.enabled()) {
		from_device = "NullDevice";
	    } else {
		//
		// TODO: XXX: we should find a way to configure polling
		// devices. Such devices should use "PollDevice" instead
		// of "FromDevice".
		//
		from_device = c_format("FromDevice(%s)", fv.vifname().c_str());
	    }

	    if (! fv.enabled()) {
		to_device = "Discard";
	    } else {
		to_device = c_format("ToDevice(%s)", fv.vifname().c_str());
	    }

	    arp_responder = c_format("ARPResponder(");
	    for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
		if (ai4 != fv.v4addrs().begin())
		    arp_responder += " ";
		arp_responder += fa4.addr().str();
	    }
	    arp_responder += c_format(" %s)", fi.mac().str().c_str());

	    IPv4 first_addr4 = IPv4::ZERO();
	    ai4 = fv.v4addrs().begin();
	    if (ai4 != fv.v4addrs().end()) {
		first_addr4 = ai4->second.addr();
	    }
	    arp_querier = c_format("ARPQuerier(%s, %s)",
				   first_addr4.str().c_str(),
				   fi.mac().str().c_str());

	    paint = c_format("Paint(%u)", port + 1);
	    print_non_ip = c_format("Print(\"%s non-IP\")",
				    fv.vifname().c_str());

	    config += "\n";
	    config += c_format("// Input and output paths for %s\n",
			       fv.vifname().c_str());
	    config += c_format("%s :: Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);\n",
			       xorp_c.c_str());
	    config += c_format("%s -> %s;\n",
			       from_device.c_str(),
			       xorp_c.c_str());
	    config += c_format("%s :: Queue(200) -> %s :: %s;\n",
			       xorp_out.c_str(),
			       xorp_to_device.c_str(),
			       to_device.c_str());
	    config += c_format("%s[0] -> %s :: %s -> %s;\n",
			       xorp_c.c_str(),
			       xorp_ar.c_str(),
			       arp_responder.c_str(),
			       xorp_out.c_str());
	    config += c_format("%s :: %s -> %s;\n",
			       xorp_arpq.c_str(),
			       arp_querier.c_str(),
			       xorp_out.c_str());
	    config += c_format("%s[1] -> %s;\n",
			       xorp_c.c_str(),
			       xorp_arpt.c_str());
	    config += c_format("%s[%u] -> [1]%s;\n",
			       xorp_arpt.c_str(),
			       port,
			       xorp_arpq.c_str());
	    config += c_format("%s[2] -> %s -> %s;\n",
			       xorp_c.c_str(),
			       paint.c_str(),
			       xorp_ip.c_str());
	    config += c_format("%s[3] -> %s -> Discard;\n",
			       xorp_c.c_str(),
			       print_non_ip.c_str());
	}
    }

    //
    // Send packets to the host
    //
    config += "\n";
    config += "// Local delivery\n";
    string xorp_toh = "_xorp_toh";
    // TODO: XXX: PAVPAVPAV: fix ether_encap
    string ether_encap = c_format("EtherEncap(0x0800, 1:1:1:1:1:1, 2:2:2:2:2:2)");
    // TODO: XXX: PAVPAVPAV: Fix the Local delivery for *BSD and/or user-level Click
    if (ClickSocket::is_kernel_click()) {
	config += c_format("%s :: ToHost;\n", xorp_toh.c_str());
    } else {
	config += c_format("%s :: Discard;\n", xorp_toh.c_str());
    }
    config += c_format("%s[%u] -> %s;\n",
		       xorp_arpt.c_str(),
		       max_vifs,
		       xorp_toh.c_str());
    // XXX: last port in xorp_rt is reserved for local delivery
    config += c_format("%s[%u] -> %s -> %s;\n",
		       xorp_rt.c_str(),
		       max_xorp_rt_port,
		       ether_encap.c_str(),
		       xorp_toh.c_str());

    //
    // Forwarding paths for each interface
    //
    port = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi, ++port) {
	    const IfTreeVif& fv = vi->second;
	    string xorp_cp = c_format("_xorp_cp%u", port);
	    string paint_tee = c_format("PaintTee(%u)", port + 1);
	    string xorp_gio = c_format("_xorp_gio%u", port);
	    string xorp_dt = c_format("_xorp_dt%u", port);
	    string xorp_fr = c_format("_xorp_fr%u", port);
	    string xorp_arpq = c_format("_xorp_arpq%u", port);

	    string ipgw_options = c_format("IPGWOptions(");
	    for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
		//
		// TODO: XXX: PAVPAVPAV: check whether the syntax is to use
		// spaces or commas.
		//
		if (ai4 != fv.v4addrs().begin())
		    ipgw_options += " ";
		ipgw_options += c_format("%s", fa4.addr().str().c_str());
	    }
	    ipgw_options += ")";

	    IPv4 first_addr4 = IPv4::ZERO();
	    ai4 = fv.v4addrs().begin();
	    if (ai4 != fv.v4addrs().end()) {
		first_addr4 = ai4->second.addr();
	    }
	    string fix_ip_src = c_format("FixIPSrc(%s)",
					 first_addr4.str().c_str());
	    string ip_fragmenter = c_format("IPFragmenter(%u)",
					    fi.mtu());

	    config += "\n";
	    config += c_format("// Forwarding path for %s\n",
			       fv.vifname().c_str());
	    xorp_rt_port = ifc().nexthop_port_mapper().lookup_nexthop_interface(fi.ifname(), fv.vifname());
	    XLOG_ASSERT(xorp_rt_port >= 0);
	    config += c_format("%s[%u] -> DropBroadcasts\n",
			       xorp_rt.c_str(),
			       xorp_rt_port);
	    config += c_format("    -> %s :: %s\n",
			       xorp_cp.c_str(),
			       paint_tee.c_str());
	    config += c_format("    -> %s :: %s\n",
			       xorp_gio.c_str(),
			       ipgw_options.c_str());
	    config += c_format("    -> %s\n",
			       fix_ip_src.c_str());
	    config += c_format("    -> %s :: DecIPTTL\n",
			       xorp_dt.c_str());
	    config += c_format("    -> %s :: %s\n",
			       xorp_fr.c_str(),
			       ip_fragmenter.c_str());
	    config += c_format("    -> [0]%s;\n",
			       xorp_arpq.c_str());

	    config += c_format("%s[1] -> ICMPError(%s, timeexceeded) -> %s;\n",
			       xorp_dt.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	    config += c_format("%s[1] -> ICMPError(%s, unreachable, needfrag) -> %s;\n",
			       xorp_fr.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	    config += c_format("%s[1] -> ICMPError(%s, parameterproblem) -> %s;\n",
			       xorp_gio.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	    config += c_format("%s[1] -> ICMPError(%s, redirect, host) -> %s;\n",
			       xorp_cp.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	}
    }

    return (config);
}
